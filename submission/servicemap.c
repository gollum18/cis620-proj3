#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include "shared.h"

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
		if (addr_cache[i].occupied && strcmp(name, addr_cache[i].service) == 0) {
			return addr_cache[i].addr_info;
		}
	}

	return NULL;
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

	addr_cache[pos].occupied = 0;

	return pos;
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

	strcpy(addr_info[pos].service, service);
	strcpy(addr_info[pos].addr_info, addr_info);
	addr_info[pos].age = 0;
	addr_info[pos].occupied = 1;
}

/**
 * Starts the service mapper 
 */
int main(int argc, char * argv[]) {

}