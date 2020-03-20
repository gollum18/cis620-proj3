/**
 * Implementation of the functions declared in util.h
 * Changelog:
 *	03/20/2020 - Initial version.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include "util.h"

/**
 * Parses a string into individual words/components.
 * @param src The string to be split.
 * @param dest The destination array to write tokens back to.
 * @param n The size of dest.
 * @param delim The delimiter to split src on.
 */
void parse_string(char * src, 
				  char * dest[], 
				  size_t n, 
				  char * delim) 
{
	int i = 0;
	char * token = strtok(src, delim);
	do {
		dest[i++] = token;
	} while (i < n && (token = strtok(NULL, delim)) != NULL);
}

/**
 * Converts an address and port to an address string.
 * @param addr The socket address to translate.
 * @param dest The destination buffer.
 * @param n The size of the destination buffer.
 */
void to_addr_string(struct sockaddr_in addr,
                    char * dest,
                    size_t n)
{
    memset(dest, 0, n);

    // the port should already be in network-byte order
    unsigned short quotient = Q(addr.sin_port);
    unsigned short remainder = R(addr.sin_port);

    char * addr_str = inet_ntoa(*(struct in_addr *)&addr);
    char * tokens[4];
    parse_string(addr_str, tokens, 4, ".");

    snprintf(dest, n, "%s,%s,%s,%s,%d,%d%c", tokens[0], tokens[1], tokens[2], tokens[3], quotient, remainder, '\0');
}

/**
 * Translates an address string back to an inet address and port.
 * The address will be in network-byte order while the port will
 * be in host-byte order.
 * @param src The buffer containing the address string.
 * @param n The size of the source buffer.
 * @param dest The destination socket address.
 */
void from_addr_string(char * src,
                      size_t n,
                      struct sockaddr_in * dest)
{
    char * tokens[6];
    parse_string(src, tokens, 6, ",");

    char buf[24];
    snprintf(buf, 24, "%s.%s.%s.%s%c", tokens[0], tokens[1], tokens[2], tokens[3], '\0');

    inet_aton(buf, (struct in_addr *)dest);
    dest->sin_port = ntohs(P(atoi(tokens[4]), atoi(tokens[5])));
}

