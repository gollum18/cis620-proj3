/**
 * Implements a net app that maps Internet services to an IP
 * address and port for a LAN.
 * Changelog:
 *	03/13/2020 - Created initial version.
 *	03/15/2020 - Refactored some of the cache methods.
 *	03/20/2020 - Started net code.
 *	03/20/2020 - Change size_t to socklen_t.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include "util.h"

#define MAX_ENTRIES 32
#define NOT_FOUND MAX_ENTRIES+1

//###,###,###,###,###,###
struct cache_entry_t {
	char service[24];
	char addr_info[24];
	unsigned short occupied;
	unsigned long age;
};

// The service cache
static struct cache_entry_t addr_cache[MAX_ENTRIES];

/**
 * Ages all of the entries in the service cache by one unit.
 */
void age_entries() {
	for (size_t i = 0; i < MAX_ENTRIES; i++) {
		if (addr_cache[i].occupied) {
			addr_cache[i].age++;
		}
	}
}

/**
 * Retrieves an entry from the service cache.
 * @param service A string containing the service to lookup.
 * @returns A pointer to the service address string on success, NULL on error.
 */
char * get_entry(char * service) {
	for (size_t i = 0; i < MAX_ENTRIES; i++) {
		if (addr_cache[i].occupied && strcmp(service, addr_cache[i].service) == 0) {
			return addr_cache[i].addr_info;
		}
	}

	return NULL;
}

/**
 * Initializes the service cache.
 */
void init_cache() {
	for (size_t i = 0; i < MAX_ENTRIES; i++) {
		memset(addr_cache[i].service, 0, sizeof(addr_cache[i].service));
		memset(addr_cache[i].addr_info, 0, sizeof(addr_cache[i].addr_info));
		addr_cache[i].age = 0;
		addr_cache[i].occupied = 0;
	}
}

/**
 * Pages an entry from the service cache. Called when the cache is full and a 
 * put request is made.
 * @returns A position in the cache to store the new entry.
 */
size_t page_entry() {
	size_t pos = 0;
	unsigned long age = addr_cache[0].age;

	for (size_t i = 1; i < MAX_ENTRIES; i++) {
		if (addr_cache[i].occupied && addr_cache[i].age > age) {
			age = addr_cache[i].age;
			pos = i;
		}
	}

	memset(addr_cache[pos].service, 0, sizeof(addr_cache[pos].service));
	memset(addr_cache[pos].addr_info, 0, sizeof(addr_cache[pos].addr_info));
	addr_cache[pos].occupied = 0;
	addr_cache[pos].age = 0;

	return pos;
}

void print_cache() {
	for (size_t i = 0; i < MAX_ENTRIES; i++) {
		if (addr_cache[i].occupied) {
			printf("Entry %zd: %s => %s\n", i, addr_cache[i].service, addr_cache[i].addr_info);
		}
	}
}

/**
 * Stores an entry in the service cache.
 * @param service The name of the service.
 * @param addr_info The address string that points to the service host.
 */
void put_entry(char * service, char * addr_info) {
	size_t pos = NOT_FOUND;

	for (size_t i = 0; i < MAX_ENTRIES; i++) {
		if (!addr_cache[i].occupied) {
			pos = i;
			break;
		}
	}

	if (pos == NOT_FOUND) {
		pos = page_entry();
	}

	strncpy(addr_cache[pos].service, service, sizeof(addr_cache[pos].service));
	strncpy(addr_cache[pos].addr_info, addr_info, sizeof(addr_cache[pos].addr_info));
	addr_cache[pos].age = 0;
	addr_cache[pos].occupied = 1;
}

int test_addr_cache() {
	init_cache();

	put_entry("database", "192,168,1,2,233,37");
	put_entry("mapper", "192,168,1,1,15,7");
	put_entry("webserver", "192,168,1,3,32,55");

	char * addr_info = get_entry("database");

	if (strcmp(addr_info, "192,168,1,2,233,37") != 0) {
		return -1;
	}

	return 0;
}

/**
 * Starts the service mapper.
 */
int main(int argc, char * argv[]) {
	struct sockaddr_in local, remote;
	socklen_t len=sizeof(local), rlen=sizeof(remote);
	int sk=0;
	char sendbuf[BUFMAX], recvbuf[BUFMAX];

	if ((sk = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket error");
		exit(1);
	}

	local.sin_family = AF_INET;
	local.sin_port = htons(MAPPER_PORT);
	local.sin_addr.s_addr = INADDR_ANY;

	if (bind(sk, (struct sockaddr *)&local, len) < 0) {
		perror("bind error");
		close(sk);
		exit(1);
	}

	while (1) {
		memset(recvbuf, 0, BUFMAX);

		// receive over the UDP socket, dest host is remote
		recvfrom(sk, recvbuf, BUFMAX, 0, (struct sockaddr *)&remote, (socklen_t *)&rlen);

		char * tokens[3]; // max token amt is 3
		parse_string(recvbuf, tokens, 3, " ");

		if (strcmp(tokens[0], "PUT") == 0) { // PUT command
			// locally store the address
			printf("Received from %s: PUT %s %s\n", inet_ntoa(remote.sin_addr), tokens[1], tokens[2]);
			put_entry(tokens[1], tokens[2]);
			strncpy(sendbuf, "OK", sizeof(sendbuf));
		} else if (strcmp(tokens[0], "GET") == 0) { // GET command
			printf("Received from %s: GET %s\n", inet_ntoa(remote.sin_addr), tokens[1]);
			// retrieve the entry
			char * addr_info = get_entry(tokens[1]);
			if (addr_info == NULL) { // error
				strncpy(sendbuf, "FAIL", sizeof(sendbuf));
			} else {
				strncpy(sendbuf, addr_info, sizeof(sendbuf));
			}
		} else { // error
			strncpy(sendbuf, "FAIL", sizeof(sendbuf));
		}

		// send the message to the dest host
		sendto(sk, sendbuf, sizeof(sendbuf), 0, (struct sockaddr *)&remote, rlen);
	}

	close(sk);
	return 0;
}
