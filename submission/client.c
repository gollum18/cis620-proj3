#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>

// These should also be placed in a header file

#define CLIENT_PORT 7777
#define SERVER_PORT 7777
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
void parse_input(char * src, char * dest[], int n, char * delim) {
	int i = 0;
	char * token = strtok(src, delim);
	do {
		dest[i++] = token;
	} while (i < n && (token = strtok(NULL, delim)) != NULL);
}

void print_help() {
	printf("\nValid commands are:\n");
	printf("\thelp\n");
	printf("\tquery <acctnum>\n");
	printf("\tupdate <acctnum> <value>\n");
	printf("\texit\n\n");
}

int main(int argc, char * argv[]) {
	// TODO: Setup all the networking stuff here

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
