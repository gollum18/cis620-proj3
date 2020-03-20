#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/fcntl.h>

#define CLIENT_PORT 7777
#define SERVER_PORT 7777
#define MAPPER_PORT 21896

#define BUFMAX 1024

#define DBFILE "db20"

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

struct record_t {
	int acctnum;
	char name[20];
	float value;
	int age;
};

/**
 * Attempts to query a record in the database.
 * @param acctnum The account number of the record to query.
 * @param dest A pointer to a record_t to write the record back to.
 * @returns 0 on success, -1 on error.
 */
int query_db(int acctnum, struct record_t * dest) {
	int rval = 0;
	
	int fd = open(DBFILE, O_RDONLY);
	if (fd < 0) {
		return -1;
	}

	ssize_t bytes_read = -1;
	while (dest->acctnum != acctnum && bytes_read != 0) {
		bytes_read = read(fd, dest, sizeof(struct record_t));
	}

	if (dest->acctnum != acctnum) {
		rval = -1;
	} else {
		rval = 0;
	}

	close(fd);

	return rval;
}

/**
 * Attempts to update a record in the database.
 * @param acctnum The account number of the record to update.
 * @param value The value to update the record with.
 * @returns 0 on success, -1 on error.
 */
int update_db(int acctnum, float value) {
	int rval = 0;
	
	int fd = open(DBFILE, O_RDONLY);
	if (fd < 0) {
		return -1;
	}

	// scan for the record
	ssize_t bytes_read = -1;
	struct record_t record;
	while (record.acctnum != acctnum && bytes_read != 0) {
		bytes_read = read(fd, &record, sizeof(struct record_t));
	}

	// only update if the record was actually found
	if (record.acctnum == acctnum) {
		if (lseek(fd, -sizeof(struct record_t), SEEK_CUR) < 0) {
			rval = -1;
			goto bail;
		}

		lockf(fd, F_LOCK, sizeof(struct record_t));

		if (read(fd, &record, sizeof(struct record_t)) < 0) {
			rval = -1;
			lockf(fd, F_ULOCK, -sizeof(struct record_t));
			goto bail;
		}

		record.value += value;

		if (lseek(fd, -sizeof(struct record_t), SEEK_CUR) < 0) {
			rval = -1;
			lockf(fd, F_ULOCK, -sizeof(struct record_t));
			goto bail;
		}

		write(fd, &record, sizeof(struct record_t));

		lockf(fd, F_ULOCK, -sizeof(struct record_t));
	}

bail:
	close(fd);

	return rval;
}

/**
 * Utility function that pretty prints the database.
 */
void print_db() {
	struct record_t record;

	int fd = open(DBFILE, O_RDONLY);

	ssize_t bytes_read = -1;
	do {
		bytes_read = read(fd, &record, sizeof(struct record_t));
		if (bytes_read == 0) {
			printf("No records in database!\n");
			break;
		}

		printf("-- Begin Record --\n");
		printf("\tAccount Number: %d\n", record.acctnum);
		printf("\tName: %s\n", record.name);
		printf("\tValue: %f\n", record.value);
		printf("\tAge: %d\n", record.age);
		printf("-- End Record --\n\n");
	} while (bytes_read != 0);

	close(fd);
}

int main(int argc, char * argv[]) {
	
	return 0;
}
