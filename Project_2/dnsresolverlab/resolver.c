#include<arpa/inet.h>
#include<netinet/in.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<time.h>

#include <netdb.h>
#include <unistd.h>

typedef unsigned int dns_rr_ttl;
typedef unsigned short dns_rr_type;
typedef unsigned short dns_rr_class;
typedef unsigned short dns_rdata_len;
typedef unsigned short dns_rr_count;
typedef unsigned short dns_query_id;
typedef unsigned short dns_flags;

#define BUF_SIZE 500

int currentLength;

typedef struct {
	char *name;
	dns_rr_type type;
	dns_rr_class class;
	dns_rr_ttl ttl;
	dns_rdata_len rdata_len;
	unsigned char *rdata;
} dns_rr;

struct dns_answer_entry;
struct dns_answer_entry {
	char *value;
	struct dns_answer_entry *next;
};
typedef struct dns_answer_entry dns_answer_entry;

void print_bytes(unsigned char *bytes, int byteslen) {
	int i, j, byteslen_adjusted;
	unsigned char c;

	if (byteslen % 8) {
		byteslen_adjusted = ((byteslen / 8) + 1) * 8;
	} else {
		byteslen_adjusted = byteslen;
	}
	for (i = 0; i < byteslen_adjusted + 1; i++) {
		if (!(i % 8)) {
			if (i > 0) {
				for (j = i - 8; j < i; j++) {
					if (j >= byteslen_adjusted) {
						printf("  ");
					} else if (j >= byteslen) {
						printf("  ");
					} else if (bytes[j] >= '!' && bytes[j] <= '~') {
						printf(" %c", bytes[j]);
					} else {
						printf(" .");
					}
				}
			}
			if (i < byteslen_adjusted) {
				printf("\n%02X: ", i);
			}
		} else if (!(i % 4)) {
			printf(" ");
		}
		if (i >= byteslen_adjusted) {
			continue;
		} else if (i >= byteslen) {
			printf("   ");
		} else {
			printf("%02X ", bytes[i]);
		}
	}
	printf("\n");
}

void canonicalize_name(char *name) {
	/*
	 * Canonicalize name in place.  Change all upper-case characters to
	 * lower case and remove the trailing dot if there is any.  If the name
	 * passed is a single dot, "." (representing the root zone), then it
	 * should stay the same.
	 *
	 * INPUT:  name: the domain name that should be canonicalized in place
	 */

	int namelen, i;

	// leave the root zone alone
	if (strcmp(name, ".") == 0) {
		return;
	}

	namelen = strlen(name);
	// remove the trailing dot, if any
	if (name[namelen - 1] == '.') {
		name[namelen - 1] = '\0';
	}

	// make all upper-case letters lower case
	for (i = 0; i < namelen; i++) {
		if (name[i] >= 'A' && name[i] <= 'Z') {
			name[i] += 32;
		}
	}
}

int name_ascii_to_wire(char *name, unsigned char *wire) {	//wire length is name length + 2

	int length = strlen(name);
	int count = 0;
	int previousPeriodIndex = currentLength;
	currentLength++;

	int i = 0;
	for(i = 0; i < length; i++) {
		if(name[i] == '.') {
			//update previous period with length, update index
			wire[previousPeriodIndex] =(char) count;
			previousPeriodIndex = previousPeriodIndex + count + 1;
			count = 0;
		}
		else {
			count++;
			wire[currentLength] = name[i];
		}
		currentLength++;
	}
	wire[previousPeriodIndex] = count;
	wire[currentLength] = (char) 0;
	currentLength++;
	return currentLength;


	/*
	 * Convert a DNS name from string representation (dot-separated labels)
	 * to DNS wire format, using the provided byte array (wire).  Return
	 * the number of bytes used by the name in wire format.
	 *
	 * INPUT:  name: the string containing the domain name
	 * INPUT:  wire: a pointer to the array of bytes where the
	 *              wire-formatted name should be constructed
	 * OUTPUT: the length of the wire-formatted name.
	 */
}

char *name_ascii_from_wire2(unsigned char *wire, int location) {


	int currentBuilderSpot = 0;
	char* builder = (char *)malloc((500) * sizeof(char));

	int countdown = -1;
	while(wire[location] != (char) 0) {

		if(wire[location] >= 192) {
			char* nextValue = name_ascii_from_wire2(wire, wire[location + 1]);
			strcat(builder, nextValue);
			return builder;
		}

		if(countdown != -1) {
			builder[currentBuilderSpot] = '.';
			currentBuilderSpot++;
		}
		countdown = (int) wire[location];
		location++;
		for(;countdown > 0; countdown--) {
			builder[currentBuilderSpot] = wire[location];
			currentBuilderSpot++;
			location++;
		}
	}

	return builder;

}

int currentResponseLocation;

char *name_ascii_from_wire(unsigned char *wire) {


	int currentBuilderSpot = 0;
	char* builder = (char *)malloc((500) * sizeof(char));

	int countdown = -1;
	while(wire[currentResponseLocation] != (char) 0) {
		if(wire[currentResponseLocation] >= 192) {
			char* nextValue = name_ascii_from_wire2(wire, wire[currentResponseLocation + 1]);
			strcat(builder, nextValue);
			currentResponseLocation += 2;
			return builder;
		}

		if(countdown != -1) {
			builder[currentBuilderSpot] = '.';
			currentBuilderSpot++;
		}
		countdown = (int) wire[currentResponseLocation];
		currentResponseLocation++;
		for(;countdown > 0; countdown--) {
			builder[currentBuilderSpot] = wire[currentResponseLocation];
			currentBuilderSpot++;
			currentResponseLocation++;
		}
	}
	currentResponseLocation++;
	return builder;

}






unsigned char msg[] = {
0x01, 0x00,
0x00, 0x01, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00
};

unsigned char msg2[] = {
	0x00, 0x01, 0x00, 0x01,
};

unsigned short create_dns_query(char *qname, dns_rr_type qtype, unsigned char *wire) {

	srand(time(0));

	currentLength = 0;
	sprintf(wire + currentLength, "%x", rand() & 0xff);
	currentLength++;
	sprintf(wire + currentLength, "%x", rand() & 0xff);
	currentLength++;



	int i = 0;
	for(i = 0; i < 10; i++) {
		wire[currentLength] = (char)msg[i];
		currentLength++;
	}

	name_ascii_to_wire(qname, wire);

	for(i = 0; i < 4; i++) {
		wire[currentLength] = (char) msg2[i];
		currentLength++;
	}

}




dns_answer_entry *resolve(char *qname, char *server) {

	char* wire = (char *)malloc((strlen(qname) + 16) * sizeof(char));

	create_dns_query(qname, 1, wire);

	//print_bytes(wire, currentLength);

	struct addrinfo hints;
	struct addrinfo *result, *rp;

	int sfd;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
	hints.ai_flags = 0;
	hints.ai_protocol = 0;          /* Any protocol */

	char* port = "53";

	getaddrinfo(server, port, &hints, &result);

	char* response = (char *) malloc(500 * sizeof(char));
	int size;


	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype,
				rp->ai_protocol);
		if (sfd == -1)
			continue;

		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
			send(sfd, wire, currentLength * sizeof(char), 0);                /* Success */
			 size = recv(sfd, response, BUF_SIZE, 0);
		}

		close(sfd);
	}

	currentResponseLocation = 12;

	char* returnName = name_ascii_from_wire(response);
	//print_bytes(response, size);
	//fflush(stdout);
	char* owner;

	currentResponseLocation += 4;


	struct dns_answer_entry *returnStructure = malloc (sizeof(struct dns_answer_entry));
	struct dns_answer_entry *currentReturn = returnStructure;
	struct dns_answer_entry *previous = NULL;




	unsigned int answers = (response[6] << 8) | response[7];

	//printf("%d\n", answers);




	//the for loop starts here :TODO

	for(; answers > 0; answers--) {

	//do the compression stuff
	if((unsigned int)response[currentResponseLocation] >= 192) {
		//it has to do with something, I am not sure
		currentResponseLocation++;
		int location = response[currentResponseLocation];
		owner = name_ascii_from_wire2(response, location);
		printf("hi\n");

	} else {
		//printf("%d\n", response[currentResponseLocation]);
		owner = name_ascii_from_wire(response);
		printf("bye\n");
	}
	currentResponseLocation += 1;

	int tags = (response[currentResponseLocation] << 8) | response[currentResponseLocation + 1];
	currentResponseLocation += 2;

	currentResponseLocation += 2;

	currentResponseLocation += 4;

	int rDataLength = (response[currentResponseLocation] << 8) | response[currentResponseLocation + 1];
	currentResponseLocation += 2;
	//printf("%d\n", currentResponseLocation);
	printf("%s\n", owner);
	printf("%s\n", returnName);

	if(strcmp(returnName, owner) == 0) {
	//if(1) {
		if(tags == 1) {

			char* destination = malloc(sizeof(char) * 100);
			inet_ntop(AF_INET, &response[currentResponseLocation], destination, 100);

			/*
			printf("%s\n", destination);
			fflush(stdout);

			char value[1000];
			int x;
			for(x = 0; x < rDataLength*2-1; x++) {
				char temp[3];
				sprintf(temp, "%u", (int)response[currentResponseLocation]);
				if(x != 0) {
					strcat(value, ".");
					x++;
				}

				printf("%s\n", temp);
				fflush(stdout);
				strcat(value, temp);
				currentResponseLocation++;

			}
			*/
			currentReturn->value = destination;
			currentReturn->next = malloc(sizeof(struct dns_answer_entry));
			previous = currentReturn;
			currentReturn = currentReturn->next;

		} else if(tags == 5) {

			returnName = name_ascii_from_wire(response);
			//printf("%d\n", currentResponseLocation);
			canonicalize_name(returnName);
			currentReturn->value = malloc(sizeof(char) * 100);
			//printf("%s\n", qname);
			strcpy(currentReturn->value, returnName);
			currentReturn->next = malloc(sizeof(struct dns_answer_entry));
			previous = currentReturn;
			currentReturn = currentReturn->next;
		}
	}
}

	previous->next = NULL;

	return returnStructure;

}

int main(int argc, char *argv[]) {
	dns_answer_entry *ans;
	if (argc < 3) {
		fprintf(stderr, "Usage: %s <domain name> <server>\n", argv[0]);
		exit(1);
	}
	ans = resolve(argv[1], argv[2]);
	while (ans != NULL) {
		printf("%s\n", ans->value);
		ans = ans->next;
	}
}
