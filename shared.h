#ifndef __SHARED__
#define __SHARED__

#include <string.h>
#include <stdio.h>

#define NAME_LEN 16
#define ADDRINFO_LEN 24

#define BUFMAX 256

#define QUERY_CODE 1001
#define UPDATE_CODE 1002

#define CLIENT_PORT 7777
#define SERVER_PORT 7777
#define MAPPER_PORT 21896

#define DOT "."
#define COMMA ","
#define SPACE " "

#define LOCAL_BROADCAST "192.168.1.255"
#define CSU_BROADCAST "137.148.205.255"

#define SERVICE_DB "CISBANK"

#define Q(p) (p / 256)
#define R(p) (p % 256)
#define P(q, r) ((q * 256) + r)

// query message sent from the client to the db server
struct msg_query_t {
	int code;
	int acctnum;
};

// update message sent from the client to the db server
struct msg_update_t {
	int code;
	int acctnum;
	float value;
};

// temporarily holds address information for a service
struct addr_info_t {
	char addr[ADDR_LEN];
	unsigned short port;
};

/**
 * Creates an address string from address information.
 * @param addr_info A pointer to an addr_info_t struct holding 
 * address information.
 * @param dest The destination string.
 * @param n The length of 'dest'.
 */
void to_addr_str(
		struct addr_info_t addr_info, 
		char * dest, 
		int n
	) {
    memset(dest, 0, n);

    unsigned short q = Q(addr_info.port);
    unsigned short r = R(addr_info.port);
    char buf[n+1];

    memset(buf, 0, n);

    char * token = strtok(addr_info.addr, DOT);
    do {
        strcat(buf, token);
        strcat(buf, COMMA);
    } while ((token = strtok(NULL, DOT)) != NULL);

    snprintf(dest, n, "%s%d,%d", buf, q, r);
    dest[n] = '\0';
}

/**
 * Gets the address information from an address string.
 * @param addr_str The address string to extract address
 * information from.
 * @param dest A pointer to a add_info_t struct to store address
 * information in.
 */
void from_addr_str(
		char * addr_str, 
		struct addr_info_t * dest
	) {
    memset(dest->addr, 0, 15);

    char * parts[6];

    char * token = strtok(addr_str, COMMA);
    int i = 0;
    do {
        parts[i] = token;
        i++;
    } while ((token = strtok(NULL, COMMA)) != NULL);

    snprintf(dest->addr, 16, "%s.%s.%s.%s", parts[0],
        parts[1], parts[2], parts[3]);
    dest->port = P(atoi(parts[4]), atoi(parts[5]));
}

#endif
//test commit
