/**
 * Implements a client that interacts with the database service.
 * Changelog:
 *	03/13/2020 - Created initial version.
 *	03/15/2020 - Started net code.
 *	03/18/2020 - Continued net code.
 *	03/20/2020 - Change size_t to socklen_t.
 *	03/21/2020 - Refactor get_service_address.
 *  03/24/2020 - Implement changes to broadcasting.
 *				 Introduce addr_info_t type to fix recv issue.
 *	03/27/2020 - Add in packet wrapper for INET messages.
 *	03/28/2020 - Get client 100% working.
 *	03/30/2020 - Complete testing of client.
 * Bugs:
 *	03/25/2020 - --Client connects successfully to server by
 *				 immediately closes when calling query-- FIXED
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

// client defines
#define BUFMAX 1024
#define CLIENT_PORT 7776
#define MAPPER_PORT 21896

// various broadcast addresses
#define DOT0_BC_ADDR "192.168.0.255" // home
#define DOT1_BC_ADDR "192.168.1.255" // home
#define LLAB_BC_ADDR "137.148.205.255" // use when submitting

// the broadcast address (set to one of above)
#define BROADCAST_ADDR LLAB_BC_ADDR

// packet code defines
#define PTYPE_REGISTER 0
#define PTYPE_LOOKUP 10
#define PTYPE_QUERY 20
#define PTYPE_UPDATE 30
#define PTYPE_RECORD 40
#define PTYPE_ERROR 50

// database command codes
#define DB_QUERY_CODE 1000
#define DB_UPDATE_CODE 1001

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
	// used for commands, messages, random data, etc...
	//	behavior is undefined if this is not NULL terminated
	char message[BUFMAX/4]; // MUST BE NULL TERMINATED
	struct query_t query;
	struct update_t update;
	struct record_t record;
};

// send this type to/from clients - best to define in header file
struct pkt_t {
	unsigned short ptype;
	union body_t body;
};

//
// PROTOTYPES
//

void decode_addrstr(char * addrstr, char * ip, unsigned short * port);
int main(int, char * []);
void parse_string(char *, char * [], int, char *);
void print_help();
int request_service(char *, struct sockaddr_in *);

//
// METHODS
//

/**
 * Prints command line help.
 */
void print_help() {
	printf("-- Help --\n");
	printf("\tquery <acctnum:int>\n");
	printf("\tupdate <acctnum:int> <value:decimal>\n");
	printf("\thelp\n");
	printf("\tquit\n");
	printf("\n");
}

/**
 * Decodes an address string storing the IP address in \'ip\' and
 * the port in \'port\'.
 * @param addrstr A pointer to a buffer containing the address
 * string received from the server.
 * @param ip A pointer to stored the IP address in.
 * @param port A pointer to stored the port number in.
 */
void decode_addrstr(char * addrstr, char *ip, unsigned short * port) {
	char * tokens[6];
	parse_string(addrstr, tokens, 6, ",");

	// BUG REPORT - snprintf does not work here
	//	will print 127.0.1 into ip not 127.0.1.1
	for (int i = 0; i < 4; i++) {
		strcat(ip, tokens[i]);
		if (i < 3) {
			strcat(ip, ".");
		}
	}
	*port = (atoi(tokens[4]) * 256) + atoi(tokens[5]);
}

/**
 *
 * @param src The buffer containing the string to parse.
 * @param dest The destination buffer for src tokens.
 * @param n The number of tokens to store in dest.
 * @param delim The tokenization delimeter.
 */
void parse_string(char * src, char * dest[], int n, char * delim) {
	int tokens = 0;
	char * token = strtok(src, delim);
	do {
		dest[tokens++] = token;
	} while (tokens < n && (token = strtok(NULL, delim)) != NULL);
}

/**
 * Requests a service from the service mapper.
 * @param service The service to request.
 * @param dest The destination socket address to intialize.
 * @returns 0 on success, -1 in error.
 */
int request_service(char * service, struct sockaddr_in * dest) {
	struct sockaddr_in local, remote;
	socklen_t len=sizeof(local), rlen=sizeof(remote);
	int sk;
	char sendbuf[BUFMAX], recvbuf[BUFMAX];

	if ((sk = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket error");
		return -1;
	}

	local.sin_family = AF_INET;
	local.sin_port = htons(CLIENT_PORT);
	local.sin_addr.s_addr = INADDR_ANY;

	if (bind(sk, (struct sockaddr *)&local, len) < 0) {
		perror("bind error");
		close(sk);
		return -1;
	}

	remote.sin_family = AF_INET;
	remote.sin_port = htons(MAPPER_PORT);
	remote.sin_addr.s_addr = inet_addr(BROADCAST_ADDR);

	// enable broadcasting on the socket
	int broadcast = 1;
	if (setsockopt(sk, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
		perror("setsockopt error");
		close(sk);
		return -1;
	}

	// construct a packet to send
	struct pkt_t pkt;

	pkt.ptype = htons(PTYPE_LOOKUP);
	memset(pkt.body.message, 0, sizeof(pkt.body.message));
	snprintf(pkt.body.message, sizeof(pkt.body.message), "GET %s%c", service, '\0');
	memset(sendbuf, 0, sizeof(sendbuf));
	memcpy(sendbuf, &pkt, sizeof(struct pkt_t));

	// attempt to send a packet
	ssize_t net_bytes = 0;
	if ((net_bytes = sendto(sk, sendbuf, sizeof(struct pkt_t), 0, (struct sockaddr *)&remote, rlen)) < 0) {
		perror("sendto error");
		close(sk);
		return -1;
	}

	if (net_bytes != sizeof(struct pkt_t)) {
		perror("sendto error");
		close(sk);
		return -1;
	}

	// attempt to receive a packet
	if ((net_bytes = recvfrom(sk, recvbuf, sizeof(struct pkt_t), 0, (struct sockaddr *)&remote, &rlen)) < 0) {
		perror("recvfrom error");
		close(sk);
		return -1;
	}

	if (net_bytes != sizeof(struct pkt_t)) {
		perror("recvfrom error");
		close(sk);
		return -1;
	}

	// receive over the same packet
	memcpy(&pkt, recvbuf, sizeof(struct pkt_t));
	pkt.ptype = ntohs(pkt.ptype);

	// check packet format
	if (pkt.ptype != PTYPE_LOOKUP) {
		perror("packet error");
		close(sk);
		return -1;
	}

	close(sk);

	// decode the pkt contents
	char raddr[24];
	unsigned short rport;
	memset(raddr, 0, sizeof(raddr));
	decode_addrstr(pkt.body.message, raddr, &rport);
	printf("Service provided by %s at port %d\n", raddr, rport);

	// set the fields in the dest socket address
	dest->sin_family = AF_INET;
	dest->sin_port = rport; // port's already in big endian
	dest->sin_addr.s_addr = inet_addr(raddr);

	return 0;
}

/**
 * Entry point of the client program.
 * @param argc Number of arguments passed via command line.
 * @param argv Arguments passed via command line.
 */
int main(int argc, char * argv[]) {
	struct sockaddr_in remote;
	socklen_t rlen=sizeof(remote);
	char sendbuf[BUFMAX], recvbuf[BUFMAX];

	// attempt to initialize the remote socket
	if (request_service("CISBANK", &remote) < 0) {
		perror("request_service error");
		return 1;
	}

	int sendmsg = 0;
	char inbuf[BUFMAX];
	struct pkt_t pkt;
	while (1) {
		printf(">: ");
		fgets(inbuf, BUFMAX, stdin);
		if (strlen(inbuf) == 0) {
			continue;
		}
		inbuf[strlen(inbuf)-1] = '\0';

		char * tokens[3];
		parse_string(inbuf, tokens, 3, " ");

		if (strcmp(tokens[0], "query") == 0) {
			// construct a query packet, send to server
			pkt.ptype = htons(PTYPE_QUERY);
			pkt.body.query.code = htonl(DB_QUERY_CODE);
			pkt.body.query.acctnum = htonl(atoi(tokens[1]));
			sendmsg = 1;
		} else if (strcmp(tokens[0], "update") == 0) {
			// construct an update packet, send to server
			pkt.ptype = htons(PTYPE_UPDATE);
			pkt.body.update.code = htonl(DB_UPDATE_CODE);
			pkt.body.update.acctnum = htonl(atoi(tokens[1]));
			float value = strtof(tokens[2], NULL);
			int * ip = (int *)&value;
			*ip = htonl(*ip);
			pkt.body.update.value = value;
			sendmsg = 1;
		} else if (strcmp(tokens[0], "help") == 0) {
			print_help();
		} else if (strcmp(tokens[0], "quit") == 0) {
			break;
		} else {
			printf("Invalid command entered! Try <help> to see a list of valid commands.\n");
			continue;
		}

		if (sendmsg) {
			struct sockaddr_in local;
			socklen_t len=sizeof(local);
			int sk;

			if ((sk = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
				perror("socket error");
				continue;
			}

			local.sin_family = AF_INET;
			local.sin_port = htons(CLIENT_PORT);
			local.sin_addr.s_addr = INADDR_ANY;

			if (bind(sk, (struct sockaddr *)&local, len) < 0) {
				perror("bind error");
				close(sk);
				continue;
			}

			if (connect(sk, (struct sockaddr *)&remote, rlen) < 0) {
				perror("connect error");
				continue;
			}

			memset(sendbuf, 0, sizeof(sendbuf));
			memcpy(sendbuf, &pkt, sizeof(struct pkt_t));

			ssize_t net_bytes = 0;
			if ((net_bytes = send(sk, sendbuf, sizeof(struct pkt_t), 0)) != sizeof(struct pkt_t)) {
				perror("send error");
				continue;
			}

			if ((net_bytes = recv(sk, recvbuf, sizeof(struct pkt_t), 0)) != sizeof(struct pkt_t)) {
				perror("recv error");
				continue;
			}

			memcpy(&pkt, recvbuf, sizeof(struct pkt_t));

			pkt.ptype = ntohs(pkt.ptype);
			if (pkt.ptype == PTYPE_RECORD) {
				// convert the fields to local byte order
				pkt.body.record.acctnum = ntohl(pkt.body.record.acctnum);
				pkt.body.record.age = ntohl(pkt.body.record.age);
				int * ip = (int *)&pkt.body.record.value;
				*ip = ntohl(*ip);

				printf("%s %d %f\n", pkt.body.record.name, pkt.body.record.acctnum, pkt.body.record.value);
			} else if (pkt.ptype == PTYPE_UPDATE) {
				if (strcmp(pkt.body.message, "OK") == 0) {
					// TODO: Notify the update succeeded
				} else {
					printf("Packet Error: %s\n\n", pkt.body.message);
				}
			} else if (pkt.ptype == PTYPE_ERROR) {
				printf("Packet Error: %s\n\n", pkt.body.message);
			} else {
				printf("An unexpected error occurred!\n\n");
			}

			close(sk);
			sendmsg = 0;
		}
	}

	return 0;
}
