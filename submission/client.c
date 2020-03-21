/**
 * Implements a client that interacts with the database service.
 * Changelog:
 *	03/13/2020 - Created initial version.
 *	03/15/2020 - Started net code.
 *	03/18/2020 - Continued net code.
 *	03/20/2020 - Change size_t to socklen_t.
 *	03/21/2020 - Refactor get_service_address.
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
#include "util.h"

void print_help() 
{
	printf("\nValid commands are:\n");
	printf("\thelp\n");
	printf("\tquery <acctnum>\n");
	printf("\tupdate <acctnum> <value>\n");
	printf("\tquit\n\n");
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

	// BUG: This recv call corrupts the string sent from the
	//	servicemap and I am not sure why - It may be the way that
	//	the server is encoding the string and sending it to the 
	//	servicemap but as of rn, I'm not sure what's going on
	// It's not the servicemap as I printed out what was being
	//	sent and it's what I expect at home (127,0,1,1,97,31)
	recvfrom(sk, recvbuf, sizeof(recvbuf), 0, (struct sockaddr *)&remote, &rlen);

bail:
	close(sk);
	return rval;
}

int main(int argc, char * argv[]) 
{
	// networking variables
	struct sockaddr_in local, remote;
	int local_sk=0, remote_sk=0;
	socklen_t len=sizeof(local), rlen=sizeof(remote);
	char sendbuf[BUFMAX], recvbuf[BUFMAX];

	// Broadcast a request to the mapper for the addr string
	if (get_service_address("CISBANK", DOT1H_BC_ADDR, recvbuf, sizeof(recvbuf)) < 0) {
		perror("broadcast error");
		exit(1);
	}

	// Convert the address string back to ip address/port
	from_addr_string(recvbuf, sizeof(recvbuf), &remote);

	printf("Service provided by %s at port %d\n", inet_ntoa(remote.sin_addr), ntohs(remote.sin_port));

	// setup the local TCP socket
	if ((local_sk = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket error");
		exit(1);
	}

	local.sin_family = AF_INET;
	local.sin_port = htons(CLIENT_PORT);
	local.sin_addr.s_addr = INADDR_ANY;

	if (bind(local_sk, (struct sockaddr *)&local, len) < 0) {
		perror("bind error");
		exit(1);
	}

	// local cmd prompt variables
	char cmdbuf[BUFMAX];
	char * tokens[3]; // longest cmd is three tokens
	while (1) {
		printf(">: ");
		fgets(cmdbuf, BUFMAX, stdin);
		cmdbuf[strlen(cmdbuf)-1] = '\0'; // get rid of \n
		parse_string(cmdbuf, tokens, 3, " ");

		if (strcmp(tokens[0], "query") == 0) {
			// TODO: Build a query request and send to database
		} else if (strcmp(tokens[0], "update") == 0) {
			// TODO: Build an update request and sent to database
		} else if (strcmp(tokens[0], "help") == 0) {
			print_help();
		} else if (strcmp(tokens[0], "quit") == 0) {
			break;
		} else {
			printf("<<<Invalid command entered, try \'help\' to see a list of valid commands.>>>\n");
		}
	}

	return 0;
}
