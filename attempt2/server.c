#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

// server defines
#define BACKLOG 5
#define BUFMAX 1024
#define SERVER_PORT 7777
#define MAPPER_PORT 21896
#define DBFILE "db20"

// various broadcast addresses
#define DOT0_BC_ADDR "192.168.0.255" // home
#define DOT1_BC_ADDR "192.168.1.255" // home
#define LLAB_BC_ADDR "137.148.254.255" // use when submitting

// the broadcast address (set to one of the above)
#define BROADCAST_ADDR DOT1_BC_ADDR

// packet code defines
#define PTYPE_REGISTER 0 // packet contains service register msg
#define PTYPE_LOOKUP 10 // packet contains service lookup msg
#define PTYPE_QUERY 20 // packet contains query msg
#define PTYPE_UPDATE 30 // packet contains update msg
#define PTYPE_RECORD 40 // packet contains record msg
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
	char message[BUFMAX/4];
	struct query_t query;
	struct update_t update;
	struct record_t record;
};

// send this type to.from clients - best to define in header file
struct pkt_t {
	unsigned short ptype;
	union body_t body;
};

//
// PROTOTYPES
//

int advertise_service(char *);
int get_service_addr(char *, size_t);
void get_service_port(unsigned short, unsigned short *, unsigned short *);
int main(int, char * []);
void parse_string(char *, char * [], int, char *);
int query_record(struct query_t, struct record_t *);
void signal_handler(int);
int update_record(struct update_t);

//
// METHODS
//

void signal_handler(int sig) {
	if (sig == SIGCHLD) {
		wait(0);
	}
}	

/**
 * Queries a record in the database.
 * @param query The structure containing query information.
 * @param record The structure to write the record back to.
 * @returns 0 on success, -1 on error.
 */
int query_record(struct query_t query, struct record_t * record) {
	int acctnum = query.acctnum;

	int fd = open(DBFILE, O_RDONLY);
	if (fd < 0) {
		perror("open error");
		return -1;
	}

	ssize_t bytes_read = 0;
	do {
		bytes_read = read(fd, record, sizeof(struct record_t));
	} while (bytes_read > 0 && record->acctnum != acctnum);
	
	close(fd);

	if (record->acctnum != acctnum) {
		return -1;
	}

	return 0;
}

/**
 * Updates a record in the database.
 * @param update The structure containing update information.
 * @returns 0 on success, -1 on error.
 */
int update_record(struct update_t update) {
	int acctnum = update.acctnum;
	float value = update.value;
	
	int fd = open(DBFILE, O_RDWR);
	if (fd < 0) {
		perror("open error");
		return -1;
	}

	// check for the record
	struct record_t record;
	ssize_t bytes_read = 0;
	do {
		bytes_read = read(fd, &record, sizeof(struct record_t));
	} while (bytes_read > 0 && record.acctnum != acctnum);

	// make sure the record was found
	if (record.acctnum != acctnum) {
		perror("record error");
		close(fd);
		return -1;
	}

	// attempt to lock the record
	if (lockf(fd, F_LOCK, -sizeof(struct record_t)) < 0) {
		perror("lockf error");
		close(fd);
		return -1;
	}

	// seek to the read position
	if (lseek(fd, -sizeof(struct record_t), SEEK_CUR) < 0) {
		perror("lseek error");
		lockf(fd, F_ULOCK, -sizeof(struct record_t));
		close(fd);
		return -1;
	}

	// read the record again, may have changed
	if ((bytes_read = read(fd, &record, sizeof(struct record_t))) < 0) {
		perror("read error");
		lockf(fd, F_ULOCK, sizeof(struct record_t));
		close(fd);
		return -1;
	}

	// update the record locally
	record.value += value;
	
	// seek to the write position
	if (lseek(fd, -sizeof(struct record_t), SEEK_CUR) < 0) {
		perror("lseek error");
		lockf(fd, F_ULOCK, -sizeof(struct record_t));
		close(fd);
		return -1;
	}

	// write the record back
	if (write(fd, &record, sizeof(struct record_t)) < 0) {
		perror("write error");
		lockf(fd, F_ULOCK, sizeof(struct record_t));
		close(fd);
		return -1;
	}

	// attempt to unlock the record
	if (lockf(fd, F_ULOCK, -sizeof(struct record_t)) < 0) {
		perror("lockf error");
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

/**
 * Gets the address of a service.
 * @param dest The destination to write the server address to.
 * @param len The length of the destination buffer.
 * @returns 0 on success, -1 on error.
 */
int get_service_addr(char * dest, size_t len) {
	char hostname[BUFMAX/4];
	if (gethostname(hostname, BUFMAX/4) < 0) { 
		return -1;
	}

	struct hostent * hentry;
	if ((hentry = gethostbyname(hostname)) == NULL) {
		return -1;
	}

	struct in_addr * inaddr = (struct in_addr *)hentry->h_addr_list[0];

	snprintf(dest, len, "%s%c", inet_ntoa(*inaddr), '\0');

	return 0;
}

void parse_string(char * src, char * dest[], int n, char * delim) {
	int tokens = 0;
	char * token = strtok(src, delim);
	do {
		dest[tokens++] = token;
	} while (tokens < n && (token = strtok(NULL, delim)) != NULL);
}

/**
 * Gets a service port number as a quotient/remainder pair.
 * @param port The port number of the server. 
 * @param quotient An integer pointer to write quotient back to.
 * @param remainder An integer pointer to write remainder back to.
 */
void get_service_port(unsigned short port, unsigned short * quotient, unsigned short * remainder) {
	*quotient = port / 256;
	*remainder = port % 256;
}

/**
 * Advertises a service to the service mapper.
 * @param service The name of the service to advertise.
 * @returns 0 on success, -1 on error.
 */
int advertise_service(char * service) {
	struct sockaddr_in local, remote;
	socklen_t len=sizeof(local), rlen=sizeof(remote);
	int sk;
	char sendbuf[BUFMAX], recvbuf[BUFMAX];

	memset(sendbuf, 0, sizeof(sendbuf));
	memset(recvbuf, 0, sizeof(recvbuf));

	if ((sk = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket error");
		return -1;
	}

	// configure the local socket address
	local.sin_family = AF_INET;
	local.sin_port = htons(SERVER_PORT);
	local.sin_family = INADDR_ANY;

	if (bind(sk, (struct sockaddr *)&local, len) < 0) {
		perror("bind error");
		close(sk);
		return -1;
	}

	// configure the remote socket address
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

	// create the sending pkt
	struct pkt_t pkt;
	pkt.ptype = htons(PTYPE_REGISTER);

	// get the service address
	char servaddr[24];
	if (get_service_addr(servaddr, sizeof(servaddr)) < 0) {
		perror("get_server_addr error");
		return -1;
	}

	// get the service port
	unsigned short quotient, remainder;
	get_service_port(htons(SERVER_PORT), &quotient, &remainder);

	// build the service address string
	char * tokens[4];
	char tempaddr[24];
	parse_string(servaddr, tokens, 4, ".");
	snprintf(tempaddr, sizeof(tempaddr), "%s,%s,%s,%s,%d,%d%c", 
			tokens[0], tokens[1], tokens[2], tokens[3], 
			quotient, remainder, '\0');

	// finish building the packet
	memset(pkt.body.message, 0, sizeof(pkt.body.message));
	snprintf(pkt.body.message, sizeof(pkt.body.message), 
		"PUT %s %s", service, tempaddr);
	memcpy(sendbuf, &pkt, sizeof(struct pkt_t));

	// attempt to send the register packet
	ssize_t net_bytes = 0;
	if ((net_bytes = sendto(sk, sendbuf, sizeof(struct pkt_t), 
			0, (struct sockaddr *)&remote, rlen)) < 0) {
		perror("sendto error");
		close(sk);
		return -1;
	}

	if (net_bytes != sizeof(struct pkt_t)) {
		perror("sendto error");
		close(sk);
		return -1;
	}

	// aawait the register response
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
	if (pkt.ptype != PTYPE_REGISTER) {
		perror("packet error");
		close(sk);
		return -1;
	}

	// make sure the service was registered successfully
	if (strcmp(pkt.body.message, "OK") != 0) {
		perror("advertise error");
		close(sk);
		return -1;
	} else { // registration ok
		printf("Registration OK from %s\n", inet_ntoa(remote.sin_addr));
	}

	close(sk);
	return 0;
}

int main(int argc, char * argv[]) {
	// register the signal handler
	if (signal(SIGCHLD, signal_handler) < 0) {
		perror("signal error");
		return 1;
	}
	
	// advertise the service to the service mapper
	if (advertise_service("CISBANK") < 0) {
		perror("advertise error");
		return 1;
	}

	struct sockaddr_in local, remote;
	socklen_t len=sizeof(local), rlen=sizeof(remote);
	int old_sk, new_sk; // old_sk=parent, new_sk=child
	char sendbuf[BUFMAX], recvbuf[BUFMAX];

	if ((old_sk = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket error");
		return 1;
	}

	local.sin_family = AF_INET;
	local.sin_port = htons(SERVER_PORT);
	local.sin_addr.s_addr = INADDR_ANY;

	if (bind(old_sk, (struct sockaddr *)&local, len) < 0) {
		perror("bind error");
		close(old_sk);
		return 1;
	}

	if (listen(old_sk, BACKLOG) < 0) {
		perror("listen error");
		close(old_sk);
		return 1;
	}

	while (1) {
		if ((new_sk = accept(old_sk, (struct sockaddr *)&remote, &rlen)) < 0) {
			perror("accept error");
			continue;
		}

		pid_t cpid = fork();
		if (cpid > 0) { // parent
			close(new_sk);
		} else if (cpid == 0) { // child
			close(old_sk);

			ssize_t net_bytes = 0;
			if ((net_bytes = recv(new_sk, recvbuf, sizeof(struct pkt_t), 0)) != sizeof(struct pkt_t)) {
				printf("recv error");
				close(new_sk);
				exit(1);
			}
			
			printf("Service Requested from %s\n", inet_ntoa(remote.sin_addr));

			struct pkt_t pkt;
			memcpy(&pkt, recvbuf, sizeof(struct pkt_t));
			pkt.ptype = ntohs(pkt.ptype);

			if (pkt.ptype == PTYPE_QUERY) {
				pkt.body.query.code = ntohl(pkt.body.query.code);
				if (pkt.body.query.code == DB_QUERY_CODE) {
					pkt.body.query.acctnum = ntohl(pkt.body.query.acctnum);
					if (query_record(pkt.body.query, &pkt.body.record) == 0) {
						pkt.ptype = htons(PTYPE_RECORD);
						pkt.body.record.acctnum = htonl(pkt.body.record.acctnum);
						pkt.body.record.age = htonl(pkt.body.record.age);
						int * ip = (int *)&pkt.body.record.value;
						*ip = htonl(*ip);
					} else { // error
						pkt.ptype = PTYPE_ERROR;
						strcpy(pkt.body.message, "Record not found!");
					}
				} else { // error
					pkt.ptype = PTYPE_ERROR;
					strcpy(pkt.body.message, "DB code does not match QUERY code!");
				}
			} else if (pkt.ptype == PTYPE_UPDATE) {
				pkt.body.update.code = ntohl(pkt.body.update.code);
				if (pkt.body.update.code == DB_UPDATE_CODE) {
					pkt.body.update.acctnum = ntohl(pkt.body.update.acctnum);
					int * ip = (int*)&pkt.body.update.value;
					*ip = ntohl(*ip);

					if (update_record(pkt.body.update) == 0) {
						printf("UPDATE: Record found!\n");
						pkt.ptype = htons(PTYPE_UPDATE);
						memset(pkt.body.message, 0, sizeof(pkt.body.message));
						strcpy(pkt.body.message, "OK");
					} else { // error
						pkt.ptype = PTYPE_ERROR;
						strcpy(pkt.body.message, "Record not found!");
					}
				} else { // error
					pkt.ptype = PTYPE_ERROR;
					strcpy(pkt.body.message, "DB code does not match UPDATE code!");
				}
			} else {
				pkt.ptype = PTYPE_ERROR;
				strcpy(pkt.body.message, "Invalid COMMAND code received!");
			}

			if (pkt.ptype == PTYPE_ERROR) {
				pkt.ptype = htons(pkt.ptype);
			}

			memcpy(sendbuf, &pkt, sizeof(struct pkt_t));
			if (send(new_sk, sendbuf, sizeof(struct pkt_t), 0) != sizeof(struct pkt_t)) {
				perror("send error");
				close(new_sk);
				exit(1);
			}

			close(new_sk);
			exit(0);
		} else {
			perror("fork error");
			close(new_sk);
			continue;
		}
	}

	close(old_sk);
	return 0;
}
