/**
 * Implements a net app that maps Internet services to an IP
 * address and port for a LAN.
 * Changelog:
 *	03/13/2020 - Created initial version.
 *	03/15/2020 - Refactored some of the cache methods.
 *	03/20/2020 - Started net code.
 *	03/20/2020 - Change size_t to socklen_t.
 *  03/24/2020 - Implement changes to broadcasting.
 *			   - Add in addr_info_str to fix recv issue at client
 *	03/27/2020 - Add in packet wrapper for INET messages.
 *			   - No longer use addr_info_str type.
 *	03/28/2020 - Get servicemap working 100%.
 *	03/30/2020 - Complete testing of servicemap.c
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// service map defines
#define BUFMAX 1024
#define NENTRIES 32
#define NOT_FOUND NENTRIES + 1
#define PORT 21896

// packet code defines
#define PTYPE_REGISTER 0
#define PTYPE_LOOKUP 10
#define PTYPE_QUERY 20
#define PTYPE_UPDATE 30
#define PTYPE_RECORD 40
#define PTYPE_ERROR 50

//
// packet stuff
//

// database query type
struct query_t {
	int code;
	int acctnum;
};

// database update type
struct update_t {
	int code;
	int acctnum;
	float value;
};

// database record type
struct record_t {
	int acctnum;
	char name[20];
	float value;
	int age;
};

// allows for sending/receiving fixed sized chunks to/from clients
// all of these refer to the same region in memory
union body_t {
	char message[BUFMAX/4]; // can be commands, errors, etc...
	struct query_t query;
	struct update_t update;
	struct record_t record;
};

// send this type to/from clients - best to define in header file
struct pkt_t {
	unsigned short ptype; // what data type is stored in the packet
	union body_t body; // the data stored in the packet
};

//
// service cache stuff
//

struct entry_t {
	char service[20];
	char addrstr[24];
	unsigned short occupied;
	unsigned long age;
};

// locally caches services and their addresses for use by 
//	clients of those services
static struct entry_t scache[NENTRIES];

//
// PROTOTYPES
//

void age_cache();
char * get_cache(char *);
unsigned int page_cache();
void parse_string(char *, char * [], int, char *);
void put_cache(char *, char *);

//
// METHODS
//

/**
 * Ages the entries in the service cache by one unit.
 */
void age_cache() {
	for (unsigned int i = 0; i < NENTRIES; i++) {
		if (scache[i].occupied) {
			scache[i].age++;
		}
	}
}

/**
 * Attempts to retrieve an entry from the service cache.
 * @param service A pointer to a buffer containing the service
 * to lookup.
 * @return The service address on success, NULL on error.
 */
char * get_cache(char * service) {
	unsigned int pos = NOT_FOUND;
	
	for (unsigned int i = 0; i < NENTRIES; i++) {
		if (scache[i].occupied) {
			if (strcmp(scache[i].service, service) == 0) {
				pos = i;
				break;
			}
		}
	}

	if (pos == NOT_FOUND) {
		return NULL;
	} else {
		scache[pos].age = 0;
		return scache[pos].addrstr;	
	}
}

/**
 * Pages an entry from the service cache.
 */
unsigned int page_cache() {
	unsigned int pos = 0, age = 0;

	for (unsigned int i = 0; i < NENTRIES; i++) {
		if (scache[i].age > age) {
			pos = i;
			age = scache[i].age;
		}
	}

	scache[pos].occupied = 0;
	scache[pos].age = 0;

	return pos;
}

/**
 * Parses a string and stores it in the destination buffer. This
 * function internally mutates the src string. If you need it 
 * after calling this function, pass a copy of it to this function.
 * @param src A pointer to a buffer containing the src string.
 * @param dest A pointer to a buffer array to write tokens back to.
 * @param n The number of tokens to store in dest.
 * @param delim The tokenization delimiter.
 */
void parse_string(char * src, char * dest[], int n, char * delim) {
	int tokens = 0;
	char * token = strtok(src, delim);
	do {
		dest[tokens++] = token;
	} while (tokens < n && (token = strtok(NULL, delim)) != NULL);
}

/**
 * Stores a service/addrstr pair in the service cache.
 * @param service The LAN unique name of the service.
 * @param addrstr The LAN unique address string of the server 
 * providing the service.
 */
void put_cache(char * service, char * addrstr) {
	unsigned int pos = NOT_FOUND;

	age_cache();

	for (unsigned int i = 0; i < NENTRIES; i++) {
		if (!scache[i].occupied) {
			pos = i;
			break;
		}
	}

	if (pos == NOT_FOUND) {
		pos = page_cache();
	}

	memset(scache[pos].service, 0, sizeof(scache[pos].service));
	memset(scache[pos].addrstr, 0, sizeof(scache[pos].addrstr));
	strncpy(scache[pos].service, service, sizeof(scache[pos].service));
	strncpy(scache[pos].addrstr, addrstr, sizeof(scache[pos].addrstr));
	scache[pos].occupied = 1;
	scache[pos].age = 0;
}

/**
 * Entry point of the service map.
 * @param argc Number of args passed via the command line.
 * @param argv Args passed from the command line.
 */
int main(int argc, char * argv[]) {
	struct sockaddr_in local, remote;
	socklen_t len=sizeof(local), rlen=sizeof(remote);
	int sk;
	char sendbuf[BUFMAX], recvbuf[BUFMAX];

	if ((sk = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket error");
		return 1;
	}

	local.sin_family = AF_INET;
	local.sin_port = htons(PORT);
	local.sin_addr.s_addr = INADDR_ANY;

	if (bind(sk, (struct sockaddr *)&local, len) < 0) {
		perror("bind error");
		close(sk);
		return 1;
	}

	while (1) {
		// these get written to/read from sendbuf/recvbuf
		struct pkt_t pkt;
		
		memset(pkt.body.message, 0, sizeof(pkt.body.message));
		memset(sendbuf, 0, sizeof(sendbuf));
		memset(recvbuf, 0, sizeof(recvbuf));

		ssize_t net_bytes = 0;
		if ((net_bytes = recvfrom(sk, recvbuf, sizeof(struct pkt_t), 0, (struct sockaddr *)&remote, &rlen)) < 0) {
			perror("recvfrom error");
			continue;
		}

		if (net_bytes != sizeof(struct pkt_t)) {
			perror("recvfrom error");
			continue;
		}

		memcpy(&pkt, recvbuf, sizeof(struct pkt_t));
		pkt.ptype = ntohs(pkt.ptype);
		printf("Received from %s: %s\n", inet_ntoa(remote.sin_addr), pkt.body.message);

		if (pkt.ptype == PTYPE_REGISTER) {
			char * tokens[3];
			parse_string(pkt.body.message, tokens, 3, " ");

			if (strcmp(tokens[0], "PUT") == 0) {
				put_cache(tokens[1], tokens[2]);

				pkt.ptype = htons(PTYPE_REGISTER);
				memset(pkt.body.message, 0, sizeof(pkt.body.message));
				strcpy(pkt.body.message, "OK");
			} else {
				pkt.ptype = PTYPE_ERROR;
			}
		} else if (pkt.ptype == PTYPE_LOOKUP) {
			char * tokens[2];
			parse_string(pkt.body.message, tokens, 2, " ");

			if (strcmp(tokens[0], "GET") == 0) {
				char * addrstr;
				if ((addrstr = get_cache(tokens[1])) != NULL) {
					pkt.ptype = htons(PTYPE_LOOKUP);
					memset(pkt.body.message, 0, sizeof(pkt.body.message));
					strcpy(pkt.body.message, addrstr);
				} else {
					pkt.ptype = PTYPE_ERROR;
				}
			} else {
				pkt.ptype = PTYPE_ERROR;
			}
		} else {
			pkt.ptype = PTYPE_ERROR;
		}

		// check if there was an error - overwrite code
		if (pkt.ptype == PTYPE_ERROR) {
			pkt.ptype = htons(pkt.ptype);
			strcpy(pkt.body.message, "FAIL");
		}

		// write back the response packet		
		memcpy(sendbuf, &pkt, sizeof(struct pkt_t));

		if (sendto(sk, sendbuf, sizeof(struct pkt_t), 0, (struct sockaddr *)&remote, rlen) < 0) {
			perror("sendto error");
			continue;
		}
	}

	return 0;
}
