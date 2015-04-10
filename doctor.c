/*
 *	file: doctor.c
 *	author: Lin Li
*/

#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>	
#include <unistd.h> 	
#include <string.h>	 
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/errno.h>
#include <sys/socket.h> 
#include <netinet/in.h>

/**
 * data structure for insurances
 */
struct insurance {
	char name[80];
	char money[80];
};

int main() {
	if (fork() == 0) {

		int port = 41399; // port number

		int svc;        // our socket

		char *myname = "nunki.usc.edu"; // server name
	
		struct sockaddr_in myaddr;    // our address
		struct sockaddr_in clientaddr;  // client address

		char name[80], money[80]; // insurance name and insurance money

		int bufsize = 1024; // buffer size
		char buf[bufsize]; // buffer for read and write
	
		FILE *fp; // file

		pthread_t thread_id; // thread

		int i;

		struct insurance *insurances; // all insurances
		int num_insurance = 0; // insurance number

		/**
	 	 * read insurance information from the file
	 	 */
		fp = fopen("doc1.txt", "r");
		if (!fp) {
			perror("fopen failed");
			return -1;
		}
		while(!feof(fp)) {
			if (!fscanf(fp, "%s %s\n", name, money)) {
				perror("reading file failed");
				return -1;
			}
			num_insurance += 1;
		}
		fclose(fp);

		insurances = (struct insurance *)malloc(num_insurance * sizeof(struct insurance));;

		fp = fopen("doc1.txt", "r");
		if (!fp) {
			perror("fopen failed");
			return -1;
		}
		for (i = 0; i < num_insurance; ++i) {
			if (!fscanf(fp, "%s %s\n", name, money)) {
				perror("reading file failed");
				return -1;
			}
			strcpy(insurances[i].name, name);
			strcpy(insurances[i].money, money);
		}
		fclose(fp);

		/*
	 	 * create a udp/ip socket
	 	 * request the Internet address protocol
	 	 * and a datagram interface (UDP/IP)
	 	 */
		if ((svc = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			perror("cannot create udp socket");
			return -1;
		}

		/**
	 	 * set up our address
	 	 * htons converts a short integer into the network representation
	 	 * htonl converts a long integer into the network representation
	 	 * INADDR_ANY is the special IP address 0.0.0.0 which binds the
	 	 * transport endpoint to all IP addresses on the machine.
	 	 */
		memset((void *)&myaddr, 0, sizeof(myaddr));
		myaddr.sin_family = AF_INET;
		myaddr.sin_port = htons(port);
		myaddr.sin_addr.s_addr = htonl(INADDR_ANY);

		/**
	 	 * bind to the address
	 	 */ 
		if (bind(svc, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
			perror("bind failed");
			close(svc);
			return -1;
		}

		/** 
	 	 * retrieve the locally-bound name of the specified socket 
	 	 * store it in the sockaddr structure
	 	 */
		size_t alen = sizeof(myaddr); 
		if (getsockname(svc, (struct sockaddr *)&myaddr, (socklen_t *)&alen) < 0) {
			perror("getsockname failed");
			close(svc);
			return -1;
		}

		/** 
	 	 * retrieve the local ip address
	 	 * store it in the sockaddr structure
	 	 */
		struct hostent *mh = gethostbyname(myname);
		if (!mh) {
			perror("gethostbyname failed");
			close(svc);
			return -1;
		}
		memcpy((void *)&myaddr.sin_addr, mh->h_addr_list[0], mh->h_length);

		printf("Phase 3: Doctor 1 has a static port %u and IP address %s.\n", ntohs(myaddr.sin_port), inet_ntoa(myaddr.sin_addr));

		/**
	 	 * loop forever - wait for connection requests and perform the service
	 	 */
		while (1) {
			/**
		 	 * receive request from patient
		 	 */
			socklen_t alen = sizeof(clientaddr);
			if (recvfrom(svc, buf, bufsize, 0, (struct sockaddr *)&clientaddr, &alen) < 0) {
				perror("recvfrom failed");
				close(svc);
				return -1;
			}

			printf("Doctor 1 receives the request from the patient with port number %u and the insurance plan %s.\n", ntohs(clientaddr.sin_port), buf);

			/**
		 	 * transfer the estimated price to the patient
		 	 */
			for (i = 0; i < num_insurance; ++i) {
				if (strcmp(buf, insurances[i].name) == 0) {
					strcpy(buf, insurances[i].money);
					if (sendto(svc, buf, bufsize, 0, (struct sockaddr *)&clientaddr, sizeof(clientaddr)) < 0) {
						perror("sendto failed");
						return -1;
					}
					break;
				}
			}

			printf("Doctor 1 has sent estimated price %s$ to patient with port number %u.\n", buf, ntohs(clientaddr.sin_port));
		}
		return 0;
	} else {
		int port = 42399; // port number

		int svc;        // our socket

		char *myname = "nunki.usc.edu"; // server name
	
		struct sockaddr_in myaddr;    // our address
		struct sockaddr_in clientaddr;  // client address

		char name[80], money[80]; // insurance name and insurance money

		int bufsize = 1024; // buffer size
		char buf[bufsize]; // buffer for read and write
	
		FILE *fp; // file

		pthread_t thread_id; // thread

		int i;

		struct insurance *insurances; // all insurances
		int num_insurance = 0; // insurance number

		/**
	 	 * read insurance information from the file
	 	 */
		fp = fopen("doc2.txt", "r");
		if (!fp) {
			perror("fopen failed");
			return -1;
		}
		while(!feof(fp)) {
			if (!fscanf(fp, "%s %s\n", name, money)) {
				perror("reading file failed");
				return -1;
			}
			num_insurance += 1;
		}
		fclose(fp);

		insurances = (struct insurance *)malloc(num_insurance * sizeof(struct insurance));;

		fp = fopen("doc2.txt", "r");
		if (!fp) {
			perror("fopen failed");
			return -1;
		}
		for (i = 0; i < num_insurance; ++i) {
			if (!fscanf(fp, "%s %s\n", name, money)) {
				perror("reading file failed");
				return -1;
			}
			strcpy(insurances[i].name, name);
			strcpy(insurances[i].money, money);
		}
		fclose(fp);

		/*
	 	 * create a udp/ip socket
	 	 * request the Internet address protocol
	 	 * and a datagram interface (UDP/IP)
	 	 */
		if ((svc = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			perror("cannot create udp socket");
			return -1;
		}

		/**
	 	 * set up our address
	 	 * htons converts a short integer into the network representation
	 	 * htonl converts a long integer into the network representation
	 	 * INADDR_ANY is the special IP address 0.0.0.0 which binds the
	 	 * transport endpoint to all IP addresses on the machine.
	 	 */
		memset((void *)&myaddr, 0, sizeof(myaddr));
		myaddr.sin_family = AF_INET;
		myaddr.sin_port = htons(port);
		myaddr.sin_addr.s_addr = htonl(INADDR_ANY);

		/**
	 	 * bind to the address
	 	 */ 
		if (bind(svc, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
			perror("bind failed");
			close(svc);
			return -1;
		}

		/** 
	 	 * retrieve the locally-bound name of the specified socket 
	 	 * store it in the sockaddr structure
	 	 */
		size_t alen = sizeof(myaddr); 
		if (getsockname(svc, (struct sockaddr *)&myaddr, (socklen_t *)&alen) < 0) {
			perror("getsockname failed");
			close(svc);
			return -1;
		}

		/** 
	 	 * retrieve the local ip address
	 	 * store it in the sockaddr structure
	 	 */
		struct hostent *mh = gethostbyname(myname);
		if (!mh) {
			perror("gethostbyname failed");
			close(svc);
			return -1;
		}
		memcpy((void *)&myaddr.sin_addr, mh->h_addr_list[0], mh->h_length);

		printf("Phase 3: Doctor 2 has a static port %u and IP address %s.\n", ntohs(myaddr.sin_port), inet_ntoa(myaddr.sin_addr));

		/**
	 	 * loop forever - wait for connection requests and perform the service
	 	 */
		while (1) {
			/**
		 	 * receive request from patient
		 	 */
			socklen_t alen = sizeof(clientaddr);
			if (recvfrom(svc, buf, bufsize, 0, (struct sockaddr *)&clientaddr, &alen) < 0) {
				perror("recvfrom failed");
				close(svc);
				return -1;
			}

			printf("Doctor 2 receives the request from the patient with port number %u and the insurance plan %s.\n", ntohs(clientaddr.sin_port), buf);

			/**
		 	 * transfer the estimated price to the patient
		 	 */
			for (i = 0; i < num_insurance; ++i) {
				if (strcmp(buf, insurances[i].name) == 0) {
					strcpy(buf, insurances[i].money);
					if (sendto(svc, buf, bufsize, 0, (struct sockaddr *)&clientaddr, sizeof(clientaddr)) < 0) {
						perror("sendto failed");
						return -1;
					}
					break;
				}
			}

			printf("Doctor 2 has sent estimated price %s$ to patient with port number %u.\n", buf, ntohs(clientaddr.sin_port));
		}
		return 0;
	}
}
