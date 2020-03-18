#ifndef __SERVICEMAP__
#define __SERVICEMAP__

#define NAME_LEN 16
#define ADDRINFO_LEN 24

struct smap_entry_t {
	char name[NAME_LEN];
	char addr_info[ADDRINFO_LEN];
	unsigned int age;
	unsigned short occupied;
};

static void add_entry(char *, char *);
static void age();
static void init_map();
static char * lookup_entry(char *);
static int page_entry();
static void remove_entry(char *);

#endif
