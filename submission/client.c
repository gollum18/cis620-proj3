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

// These should also be placed in a header file

#define CLIENT_PORT 7777
#define MAPPER_PORT 21896

#define BUFMAX 1024

// These two structs should really be in a header file
//	as they are replicated between 2 files

// client->database query message
struct query_t {
	int code;
	int acctnum;
};

// client->database update message
struct update_t {
	int code;
	int acctnum;
	float value;
};

/**
 * Parses a string into individual words/components.
 * @param src The string to be split.
 * @param dest The destination array to write tokens back to.
 * @param n The size of dest.
 * @param delim The delimiter to split src on.
 */
void parse_input(char * src, char * dest[], int n, char * delim) 
{
	int i = 0;
	char * token = strtok(src, delim);
	do {
		dest[i++] = token;
	} while (i < n && (token = strtok(NULL, delim)) != NULL);
}

void print_help() 
{
	printf("\nValid commands are:\n");
	printf("\thelp\n");
	printf("\tquery <acctnum>\n");
	printf("\tupdate <acctnum> <value>\n");
	printf("\texit\n\n");
}

/**
 * Attempts to retrieve a service address via UDP broadcasting.
 * @param local The local initialized socket address.
 * @param len The size of local in bytes.
 * @param recvbuf The buffer to write the address string back to.
 * @param n The size of recvbuf in bytes.
 * @param service The service to query.
 */
int get_service_address(struct sockaddr_in local, 
						 size_t len, 
						 char * recvbuf, 
						 size_t n,
						 char * service)
{
	struct sockaddr_in remote;
	size_t rlen=sizeof(remote);
	char sendbuf[BUFMAX];
	int sk, rval=0;

	remote.sin_family = AF_INET;
	remote.sin_port = htons(MAPPER_PORT);
	remote.sin_addr.s_addr = inet_addr("255.255.255.255");

	snprintf(sendbuf, BUFMAX, "GET %s", service);

	if ((sk = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		return -1;
	}

	if (bind(sk, (struct sockaddr *)&local, len) < 0) {
		rval=-1;
		goto bail;
	}

	sendto(sk, sendbuf, BUFMAX, 0, (struct sockaddr *)&remote, rlen);

	// this call blocks
	recv(sk, recvbuf, BUFMAX, 0);

bail:
	close(sk);
	return rval;
}

int main(int argc, char * argv[]) 
{
	// networking variables
	struct sockaddr_in local, remote;
	int local_sk=0, remote_sk=0;
	size_t len=sizeof(local), rlen=sizeof(remote);
	char sendbuf[BUFMAX], recvbuf[BUFMAX];

	// Broadcast a request to the mapper for the addr string
	if (get_service_address(local, len, recvbuf, BUFMAX, "CISBANK") < 0) {
		perror("broadcast error");
		exit(1);
	}

	// TODO: Convert the address string back to ip address/port

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
		printf("client: ");
		fgets(cmdbuf, BUFMAX, stdin);
		cmdbuf[strlen(cmdbuf)-1] = '\0'; // get rid of \n
		parse_input(cmdbuf, tokens, 3, " ");

		if (strcmp(tokens[0], "query") == 0) {
			// TODO: Build a query request and send to database
		} else if (strcmp(tokens[0], "update") == 0) {
			// TODO: Build an update request and sent to database
		} else if (strcmp(tokens[0], "help") == 0) {
			print_help();
		} else if (strcmp(tokens[0], "exit") == 0) {
			break;
		} else {
			printf("<<<Invalid command entered, try \'help\' to see a list of valid commands.>>>\n");
		}
	}

	return 0;
}
