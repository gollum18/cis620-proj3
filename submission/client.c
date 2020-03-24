/**
 * Implements a client that interacts with the database service.
 * Changelog:
 *	03/13/2020 - Created initial version.
 *	03/15/2020 - Started net code.
 *	03/18/2020 - Continued net code.
 *	03/20/2020 - Change size_t to socklen_t.
 *	03/21/2020 - Refactor get_service_address.
 *  03/24/2020 - Implement changes to broadcasting.
			     Introduce addr_info_t type to fix recv issue.
 * Bugs:
 *	03/25/2020 - Client connects successfully to server by
 *				 immediately closes when calling query.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>

#define CLIENT_PORT 7776
#define MAPPER_PORT 21896

#define BUFMAX 1024

#define QUERY_CODE 1000
#define UPDATE_CODE 1001

#define DOT0H_BC_ADDR "192.168.0.255" // Try this 1 first
#define DOT1H_BC_ADDR "192.168.1.255" // If the above doesn't work, try this one
#define LLAB_BC_ADDR "137.148.254.255" // Use this for the submission
#define BROADCAST_ADDR DOT1H_BC_ADDR // change this to change the broadcast address

#define P(q, r) (q*256) + r // Port, q = quotient, r = remainder

struct addr_info_t {
	char addr_info[24];
};

struct query_t {
	int code;
	int acctnum;
};

struct update_t {
	int code;
	int acctnum;
	float value;
};

void parse_string(char * src, 
				  char * dest[], 
				  size_t n, 
				  char * delim) {
	int i = 0;
	char * token = strtok(src, delim);
	do {
		dest[i++] = token;
	} while (i < n && (token = strtok(NULL, delim)) != NULL);
}

void from_addr_string(char * src, 
					  size_t n, 
					  struct sockaddr_in * dest) {
	char * tokens[6];
	parse_string(src, tokens, 6, ",");

	char buf[24];
	snprintf(buf, 24, "%s.%s.%s.%s", tokens[0], tokens[1], tokens[2], tokens[3]);

	inet_aton(buf, &(dest->sin_addr));
	// port is already in network byte order
	dest->sin_port = P(atoi(tokens[4]), atoi(tokens[5]));
}

void print_help() 
{
	printf("\nValid commands are:\n");
	printf("\thelp\n");
	printf("\tquery <acctnum>\n");
	printf("\tupdate <acctnum> <value>\n");
	printf("\tquit\n\n");
}

/**
 * Initializes a database lookup query.
 * @param query A pointer to a query_t struct to initialize.
 * @param acctnum The account number to query.
 */
void init_query(struct query_t * query, int code, int acctnum) {
    query->code = htonl(code);
	query->acctnum = htonl(acctnum);
}

/**
 * Initializes a database update query.
 * @param update A pointer to an update_t struct to initialize.
 * @param acctnum The account number to update.
 * @param value The value to update the account with.
 */
void init_update(struct update_t * update, int code, int acctnum, float value) {
    update->code = htonl(code);
	update->acctnum = htonl(acctnum);
    int *ip = (int*)&value;
    *ip = htonl(*ip); //can be ntohl or htonl
}

/**
 * Attempts to retrieve a service address via UDP broadcasting.
 * @param service The service to try to get address string of.
 * @param broadcast_addr The address to broadcast to.
 * @param recvbuf The buffer to write the address string back to.
 * @param n The size of recvbuf in bytes.
 * @param service The service to query.
 */
int get_service_address(char * service, char * broadcast_addr, char * recvbuf, size_t n) {
	struct sockaddr_in local, remote;
	socklen_t len=sizeof(local), rlen=sizeof(remote);
	int sk, rval=0;
	char sendbuf[BUFMAX];

	memset(sendbuf, 0, sizeof(sendbuf));
	memset(recvbuf, 0, n);
	
	if ((sk = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		return -1;
	}

	// enable broadcasting on socket
	int option=1;
	if (setsockopt(sk, SOL_SOCKET, SO_BROADCAST, &option, sizeof(option))) {
		perror("broadcast setsocket error");
		rval = -1;
		goto bail;
	}

	local.sin_family = AF_INET;
	local.sin_port = htons(CLIENT_PORT);
	local.sin_addr.s_addr = INADDR_ANY;

	remote.sin_family = AF_INET;
	remote.sin_port = htons(MAPPER_PORT);
	remote.sin_addr.s_addr = inet_addr(broadcast_addr);

	if (bind(sk, (struct sockaddr *)&local, len) < 0) {
		rval=-1;
		goto bail;
	}

	snprintf(sendbuf, BUFMAX, "GET %s", service);

	sendto(sk, sendbuf, BUFMAX, 0, (struct sockaddr *)&remote, rlen);

	recvfrom(sk, recvbuf, sizeof(struct addr_info_t), 0, (struct sockaddr *)&remote, &rlen);

	struct addr_info_t s_addr_info;
	memcpy(&s_addr_info, recvbuf, sizeof(struct addr_info_t));
	memset(recvbuf, 0, sizeof(recvbuf));
	strcpy(recvbuf, s_addr_info.addr_info);

bail:
	close(sk);
	return rval;
}

/**
 * Attempts to connect to peer from socket sk and send a message.
 * @param local_sk The local socket to send from.
 * @param peer The peer to send a message to.
 * @param plen The size of peer.
 * @param sendbuf The buffer containing the message to send.
 * @param sendlen The length of sendbuf.
 * @param recvbuf The buffer to write response back to.
 * @param recvlen The length of recvbuf.
 * @returns 0 on success, -1 on error.
 */
int connect_and_send(int local_sk, 
					 struct sockaddr_in * peer, 
					 socklen_t plen, 
					 char * sendbuf, 
					 size_t sendlen, 
					 char * recvbuf, 
					 size_t recvlen) {
	// duplicate the socket: never connect w/original client socket
	int remote_sk = dup(local_sk);
	
	// connect on the duplicate
	if (connect(remote_sk, (struct sockaddr *)peer, plen) < 0) {
		perror("connect error");
		return -1;
	}

	// attempt to send the message
	send(local_sk, sendbuf, sendlen, 0);

	// attempt to receive
	recv(local_sk, recvbuf, recvlen, 0);

	// close the duplicate socket
	close(remote_sk);

	return 0;
}

int main(int argc, char * argv[]) 
{
	// networking variables
	struct query_t query;
	struct update_t update;
	struct sockaddr_in local, remote;
	int local_sk=0, rval=0;
	socklen_t len=sizeof(local), rlen=sizeof(remote);
	char sendbuf[BUFMAX], recvbuf[BUFMAX];

	// Broadcast a request to the mapper for the addr string
	if (get_service_address("CISBANK", BROADCAST_ADDR, recvbuf, sizeof(recvbuf)) < 0) {
		perror("broadcast error");
		return 1;
	}

	// Convert the address string back to ip address/port
	from_addr_string(recvbuf, sizeof(recvbuf), &remote);

	printf("Service provided by %s at port %d\n", inet_ntoa(remote.sin_addr), remote.sin_port);

	// setup the local TCP socket
	if ((local_sk = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket error");
		return 1;
	}

	local.sin_family = AF_INET;
	local.sin_port = htons(CLIENT_PORT);
	local.sin_addr.s_addr = INADDR_ANY;

	if (bind(local_sk, (struct sockaddr *)&local, len) < 0) {
		perror("bind error");
		rval = 1;
		goto bail;
	}

	// local cmd prompt variables
	char cmdbuf[BUFMAX];
	char * tokens[3]; // longest cmd is three tokens
	while (1) {
		printf(">: ");
		fgets(cmdbuf, BUFMAX, stdin); // fgets leaves \n at eos
		cmdbuf[strlen(cmdbuf)-1] = '\0'; // get rid of \n
		// BUG REPORT: This prevents a segfault when no input
		// is entered. When no input is entered, the above line
		// makes cmdbuf contain the string "\0" from "\n".
		if (strlen(cmdbuf) == 0) {
			continue;
		}
		parse_string(cmdbuf, tokens, 3, " ");

		if (strcmp(tokens[0], "query") == 0) {
			printf("QUERY RECEIVED");
			init_query(&query, QUERY_CODE, atoi(tokens[1]));
			memcpy(sendbuf, &query, sizeof(struct query_t));
			
			if (connect_and_send(local_sk, 
								 &remote, 
								 rlen, 
								 sendbuf, 
								 sizeof(struct query_t), 
								 recvbuf, 
								 sizeof(recvbuf)) < 0) { // error
				// TODO: Maybe print error here?
			} else { // no error

			}
		} else if (strcmp(tokens[0], "update") == 0) {
			// TODO: Build an update request and sent to database
			init_update(&update, UPDATE_CODE, atoi(tokens[1]), strtof(tokens[2], NULL));
			memcpy(sendbuf, &update, sizeof(struct update_t));
			
			if (connect_and_send(local_sk, 
								 &remote, 
								 rlen, 
								 sendbuf, 
								 sizeof(struct update_t), 
								 recvbuf, 
								 sizeof(recvbuf)) < 0) { // error
				// TODO: Maybe print error here?
			} else { // no error

			}
		} else if (strcmp(tokens[0], "help") == 0) {
			print_help();
		} else if (strcmp(tokens[0], "quit") == 0) {
			break;
		} else {
			printf("<<<Invalid command entered, try \'help\' to see a list of valid commands.>>>\n");
		}
	}

bail:
	close(local_sk);
	return rval;
}
