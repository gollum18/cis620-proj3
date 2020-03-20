#ifndef __UTIL__
#define __UTIL__

#define CLIENT_PORT 7777
#define SERVER_PORT 7777
#define MAPPER_PORT 21896

#define BUFMAX 1024

struct query_t {
	int code;
	int acctnum;
};

struct update_t {
	int code;
	int acctnum;
	float value;
};

#include <string.h>

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

#endif
