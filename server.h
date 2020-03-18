#ifndef __SERVER__
#define __SERVER__

#define DBFILE "db20"

struct record_t {
	int acctnum;
	char name[20];
	float value;
	int age;
};

int query_record(int, struct record_t *);
int update_record(int, float);

#endif
