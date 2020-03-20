/**
 * Contains utility functions and useful definitions.
 * Changelog:
 *	03/13/2020 - Initial Version
 *  03/17/2020 - Add in parsing function.
 *  03/18/2020 - Separate header and implementation.
 */

#ifndef __UTIL__
#define __UTIL__

#define CLIENT_PORT 7777
#define SERVER_PORT 7777
#define MAPPER_PORT 21896

#define BUFMAX 1024

#define QUERY_CODE 1000
#define UPDATE_CODE 1001

// used to translate between port string and numeric port
#define Q(p) (p/256) // Quotient, p = port
#define R(p) (p%256) // Remainder, p = port
#define P(q, r) ((q*256) + r) // Port, q = quotient, r = remainder

struct query_t {
	int code;
	int acctnum;
};

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
void parse_string(char *, char * [], size_t, char *);

/**
 * Converts an address and port to an address string.
 * @param addr The socket address to translate.
 * @param dest The destination buffer.
 * @param n The size of the destination buffer.
 */
void to_addr_string(struct sockaddr_in, char *, size_t); 

/**
 * Translates an address string back to an inet address and port. 
 * The address will be in network-byte order while the port will 
 * be in host-byte order.
 * @param src The buffer containing the address string.
 * @param n The size of the source buffer.
 * @param dest The destination socket address.
 */
void from_addr_string(char *, size_t, struct sockaddr_in *); 

#endif
