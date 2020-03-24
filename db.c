#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>

#define BUFMAX 256
#define DBFILE "db20"

struct record_t {
	int acctnum;
	char name[20];
	float value;
	int age;
};

void print_help();
void parse_string(char *, char * [], int, char *);
void print_record(struct record_t);
int seek_record(int);
void view_db(int);
int update_record(int, float);
int write_record(int, char *, float, int);

void print_help() {
	printf("help\n");
	printf("print <acctnum:int>\n");
	printf("quit\n");
	printf("viewdb <rperpage:int>\n");
	printf("update <acctnum:int> <value:float>\n");
	printf("write <acctnum:int> <name:str> <value:float> <age:int>\n");
}

void parse_string(char * src, 
				  char * dest[], 
				  int n, 
				  char * delim) {
	char * token = strtok(src, delim);
	int tokens = 0;
	do {
		dest[tokens++] = token;
	} while (tokens < n && (token = strtok(NULL, delim)) != NULL);
}

void print_record(struct record_t record) {
	printf("-- Begin Record --\n");
	printf("\tAcct. Number: %d\n", record.acctnum);
	printf("\tName: %s\n", record.name);
	printf("\tValue: %f\n", record.value);
	printf("\tAge: %d\n", record.age);
	printf("-- End Record --\n\n");
}

int seek_record(int acctnum) {
	int fd = open(DBFILE, O_RDONLY);
	if (fd < 0) {
		perror("open error");
		return -1;
	}

	struct record_t record;

	ssize_t bytes_read = 0;
	do {
		bytes_read = read(fd, &record, sizeof(struct record_t));
	} while (bytes_read > 0 && record.acctnum != acctnum);

	if (record.acctnum == acctnum) {
		print_record(record);
	} else {
		printf("No record with acct. number \'%d\' found!\n", acctnum);
	}

	close(fd);
}

int update_record(int acctnum, float value) {
	int fd = open(DBFILE, O_RDWR), rval=0;
	if (fd < 0) {
		perror("open error");
		return -1;
	}

	struct record_t record;
	
	ssize_t bytes_read = 0;
	do {
		bytes_read = read(fd, &record, sizeof(struct record_t));
	} while (bytes_read > 0 && record.acctnum != acctnum);

	if (lockf(fd, F_LOCK, -sizeof(struct record_t)) == 0){
		lseek(fd, -sizeof(struct record_t), SEEK_CUR);

		read(fd, &record, sizeof(struct record_t));

		record.value += value;

		lseek(fd, -sizeof(struct record_t), SEEK_CUR);

		write(fd, &record, sizeof(struct record_t));

		lockf(fd, F_ULOCK, -sizeof(struct record_t));
	} else {
		rval = -1;
	}

	close(fd);
	return rval;
}

void view_db(int rperpage) {
	int fd = open(DBFILE, O_RDONLY);
	if (fd < 0) {
		perror("open error");
		return;
	}

	char input[1];
	char page[rperpage*sizeof(struct record_t)];
	struct record_t record;

	ssize_t bytes_read = 0;
	int pnum = 0;
	do {
		bytes_read = read(fd, page, rperpage*sizeof(struct record_t));
		
		if (bytes_read > 0) {
			printf("Page Number: %d\n", pnum+1);
			for (ssize_t bmark = 0; bmark < bytes_read; bmark += sizeof(struct record_t)) {
				memcpy(&record, &page[bmark], sizeof(struct record_t));
				print_record(record);
			}
			printf("press any key to continue...\n");
			getc(stdin);
		}
		pnum++;
	} while (bytes_read > 0);

	close(fd);
}

int write_record(int acctnum, char * name, float value, int age) {
	int fd = open(DBFILE, O_WRONLY);
	if (fd < 0) {
		perror("open error");
		return -1;
	}

	struct record_t record;
	record.acctnum = acctnum;
	strncpy(record.name, name, sizeof(record.name));
	record.name[sizeof(record.name)] = '\0';
	record.value = value;
	record.age = age;

	lseek(fd, 0, SEEK_END);

	write(fd, &record, sizeof(struct record_t));

	close(fd);
}

int main(int argc, char * argv) {
	char input[BUFMAX];
	char * tokens[5];
	
	while (1) {
		memset(input, 0, sizeof(input));
		
		printf("> ");
		fgets(input, BUFMAX, stdin);
		input[strlen(input)-1] = '\0';
		if (strlen(input) == 0) {
			continue;
		}
		parse_string(input, tokens, 5, " ");

		// seek, view, update, write, help, quit
		if (strcmp(tokens[0], "print") == 0) {
			seek_record(atoi(tokens[1]));
		} else if (strcmp(tokens[0], "viewdb") == 0) { 
			view_db(atoi(tokens[1]));
		} else if (strcmp(tokens[0], "update") == 0) {
			update_record(atoi(tokens[1]), strtof(tokens[2], NULL));
		} else if (strcmp(tokens[0], "write") == 0) {
			write_record(atoi(tokens[1]), tokens[2], strtof(tokens[3], NULL), atoi(tokens[4]));
		} else if (strcmp(tokens[0], "help") == 0) {
			print_help();
		} else if (strcmp(tokens[0], "quit") == 0) {
			break;
		} else {
			printf("Error: Invalid command entered! Try <help> for a list of valid commands.\n");
		}

		printf("\n");
	}
}
