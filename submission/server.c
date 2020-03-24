/**
 * Implements a database service that allows clients to query for
 * and update records.
 * Changelog:
 *	03/13/2020 - Created initial version.
 *	03/15/2020 - Started net code.
 *	03/18/2020 - Continued net code.
 *	03/20/2020 - Add in signal handler for child process.
 *			   - Change size_t to socklen_t.
 *			   - Implement service broadcasting.
 *  03/24/2020 - Implement changes to broadcasting.
 *			   - Add in addr_info_t type to fix recv at client
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <signal.h>
#include <sys/wait.h>

#define SERVER_PORT 7777
#define MAPPER_PORT 21896

#define BUFMAX 1024

#define QUERY_CODE 1000
#define UPDATE_CODE 1001

#define DOT0H_BC_ADDR "192.168.0.255" // Try this 1st
#define DOT1H_BC_ADDR "192.168.1.255" // If the above doesnt work, use this
#define LLAB_BC_ADDR "137.148.254.255" // This should be what we use for the submission
#define BROADCAST_ADDR DOT1H_BC_ADDR // change this to one of the above

#define Q(p) (p/256) // Quotient, p = port
#define R(p) (p%256) // Remainder, p = port

#define DBFILE "db20"

struct record_t {
	int acctnum;
	char name[20];
	float value;
	int age;
};

struct addr_info_t {
	char addr_info[24];
};

struct query_t {
	int code;
	int acctnum;
};

struct update_t {
	int code;
	int acctnum;
	float value;
};

void parse_string(char * src,
				  char * dest[],
				  size_t n,
				  char * delim) {
	int i = 0;
	char * token = strtok(src, delim);
	do {
		dest[i++] = token;
	} while (i < n && (token = strtok(NULL, delim)) != NULL);
}

void to_addr_string(char * addr,
					unsigned short port,
					char * dest,
					size_t n) {
	memset(dest, 0, n);

	char * tokens[4];
	parse_string(addr, tokens, 4, ".");

	snprintf(dest, n, "%s,%s,%s,%s,%d,%d%c", tokens[0], tokens[1], tokens[2], tokens[3], Q(port), R(port), '\0');
}

/**
 * Parses a query into dest from src.
 * @param dest The destination struct to write to.
 * @param src The source buffer to read from.
 */
void parse_query(struct query_t * dest, char * src) {
	memcpy(dest, src, sizeof(struct query_t));
	dest->code = ntohl(dest->code);
	dest->acctnum = ntohl(dest->acctnum);
}

/**
 * Parses an update into dest from src.
 * @param dest The destination struct to write to.
 * @param src The source buffer to read from.
 */
void parse_update(struct update_t * dest, char * src) {
	memcpy(dest, src, sizeof(struct update_t));
	dest->code = ntohl(dest->code);
	dest->acctnum = ntohl(dest->acctnum);
	int *ip = (int *)&dest->value;
	*ip = ntohl(*ip);
}

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

/**
 * Broadcasts the service to the service mapper.
 * @param service The name of the service to advertise.
 * @param broadcast_addr The IPV4 broadcast address advertise to.
 * @returns 0 on success, -1 on error.
 */
int broadcast_service(char * service, char * broadcast_addr) {
	struct addr_info_t s_addr_info;
	struct sockaddr_in local, remote;
	socklen_t len=sizeof(local), rlen=sizeof(remote);
	int sk, rval=0;
	char sendbuf[BUFMAX], recvbuf[BUFMAX];

	if ((sk = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("broadcast socket error");
		return -1;
	}
	
	// enable broadcasting on socket
	int option=1;
	if (setsockopt(sk, SOL_SOCKET, SO_BROADCAST, &option, sizeof(option)) < 0) {
		perror("broadcast setsockopt error");
		rval = -1;
		goto bail;
	}

	local.sin_family = AF_INET;
	local.sin_port = htons(SERVER_PORT);
	local.sin_addr.s_addr = INADDR_ANY;

	if (bind(sk, (struct sockaddr *)&local, len) < 0) {
		perror("broadcast bind error");
		rval = -1;
		goto bail;
	}

	remote.sin_family = AF_INET;
	remote.sin_port = htons(MAPPER_PORT);
	// Linux lab broadcast address is 137.148.254.255
	// Local (In-Home) broadcast address 192.168.0/1.255
	// Generic broadcast for Class D addresses is 255.255.255.255
	remote.sin_addr.s_addr = inet_addr(broadcast_addr);

	// Get the hostname from the local machin
	char hostname[BUFMAX/4]; // hostname shouldn't be > 256 bytes
	gethostname(hostname, BUFMAX/4);

	// Get host addr via local dns, if not cached, this fails
	struct hostent * hostentry = gethostbyname(hostname);
    
	// BUG REPORT: I had to search high and low to figure out that
	//	host entries in h_addr_list are really in_addr pointers
	// Finally found it after stumbling upon it as a note on IBMs
	//	developer documentation of all things
	// This caveat is not listed in standard C documentation on 
	//	this function
	struct in_addr * inaddr = (struct in_addr *)hostentry->h_addr_list[0];

	to_addr_string(inet_ntoa(*inaddr), local.sin_port, s_addr_info.addr_info, sizeof(s_addr_info.addr_info));

	snprintf(sendbuf, sizeof(sendbuf), "PUT %s %s", service, s_addr_info.addr_info);

	// send to the mapper - does not block
	sendto(sk, sendbuf, sizeof(sendbuf), 0, (struct sockaddr *)&remote, rlen);

	// wait for the mappers response - this call blocks
	recvfrom(sk, recvbuf, sizeof(recvbuf), 0, (struct sockaddr *)&remote, &rlen);

	printf("Registration %s from %s\n", recvbuf, inet_ntoa(remote.sin_addr));

bail:
	close(sk);
	return rval;
}

/**
 * Handles signals raised by the application.
 * @param sig The signal to handle.
 */
void signal_handler(int sig) {
	if (sig == SIGCHLD) { // cleanup child process
		wait(0);
	}
}

int main(int argc, char * argv[]) {
	struct query_t query_msg;
	struct update_t update_msg;
	struct sockaddr_in local, remote;
	socklen_t len=sizeof(local), rlen=sizeof(remote);
	int local_sk=0, remote_sk=0, rval=0;
	char sendbuf[BUFMAX], recvbuf[BUFMAX];

	// broadcast the service to the mapper
	if (broadcast_service("CISBANK", BROADCAST_ADDR) < 0) {
		perror("broadcast error");
		return 1;
	}

	// register the child signal handler to cleanup child procs
	if (signal(SIGCHLD, signal_handler) == SIG_ERR) {
		perror("SIGCHLD");
		return 1;
	}

	if ((local_sk = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket error");
		return 1;
	}

	local.sin_family = AF_INET;
	local.sin_port = htons(SERVER_PORT);
	local.sin_addr.s_addr = INADDR_ANY;

	if (bind(local_sk, (struct sockaddr *)&local, len) < 0) {
		perror("bind error");
		rval = 1;
		goto bail;
	}

	if (listen(local_sk, 5) < 0) {
		perror("listen error");
		rval = 1;
		goto bail;
	}

	while (1) {
		// accept blocks
		if ((remote_sk = 
				accept(local_sk, (struct sockaddr *)&remote, (socklen_t *)&rlen)) < 0) {
			perror("accept error");
			rval = 1;
			goto bail;
		}

		printf("Service requested from %s\n", inet_ntoa(remote.sin_addr));

		pid_t cpid = fork();
		if (cpid == 0) { // child
			close(local_sk);
			if (read(remote_sk, recvbuf, BUFMAX) < 0) {
				close(remote_sk);
				perror("child read error");
				exit(-1);
			}

			if (sizeof(recvbuf) == sizeof(struct query_t)) {
				parse_query(&query_msg, recvbuf);
				
				if (query_msg.code == QUERY_CODE) {
					struct record_t record;

					// error
					if (query_db(query_msg.acctnum, &record) < 0) {
						// TODO: Write error to sendbuf
					} else { // no error
						record.acctnum = htonl(record.acctnum);
						int * ip = (int *)&(record.value);
						*ip = htonl(*ip);
						memcpy(sendbuf, &record, sizeof(struct record_t));
					}
				}
			} else if (sizeof(recvbuf) == sizeof(struct update_t)) {
				parse_update(&update_msg, recvbuf);

				if (update_msg.code == UPDATE_CODE) {
					// error
					if (update_db(update_msg.acctnum, 
								  update_msg.value)) {
						// TODO: Write error to sendbuf
					} else { // no error
						// Write success to sendbuf
					}
				}
			} else {
				perror("child command error");
				close(remote_sk);
				exit(1);
			}

			// TODO: Send the response to the client
			send(remote_sk, sendbuf, BUFMAX, 0);

			close(remote_sk);
			exit(0);
		} else if (cpid > 0) { // parent
			close(remote_sk);
		} else { // error
			perror("fork error");
			rval = 1;
			goto bail;
		}
	}

bail:
	close(local_sk);
	return rval;
}
