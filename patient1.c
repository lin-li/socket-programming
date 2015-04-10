/*
 *	code for patient 1
 *	author: Lin Li
*/

#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>	
#include <unistd.h> 	
#include <string.h>	 
#include <sys/wait.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/errno.h>
#include <sys/socket.h> 
#include <netinet/in.h>

int main() {
	struct sockaddr_in myaddr;	// client address
	struct sockaddr_in servaddr; // server address
	
	int fd;	// socket
	
	char *myname = "localhost"; // client name
	char *servname = "nunki.usc.edu"; // server name
	
	int servport = 21399; // port number of the server
	int docport; // port number of the doctor
	
	char username[80], password[80]; // username and password

	FILE *fp; // file

	int bufsize = 1024; // buffer size
	char buf[bufsize]; // buffer for communidation between sockets

	char *tmp; // temporary string variable
	char *token; // used in parsing message

	char doc_name[80];

	size_t alen;

	/**
	 * read username and password from file
	 */
	fp = fopen("patient1.txt", "r");
	if (!fp) {
		perror("fopen failed");
		return -1;
	}
	if (!fscanf(fp, "%s %s", username, password)) {
		perror("reading username and password failed");
		return -1;
	}
	fclose(fp);

	/**
	 * create a tcp/ip socket
	 * request the Internet address protocol
	 * and a reliable 2-way byte stream (TCP/IP)
	 */
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("cannot create tcp socket");
		return -1;
	}

	/**
	 * bind to an arbitrary return address
	 * in this case, since it's the client side, so we won't
	 * care about the port number as no application will connect here
	 * INADDR_ANY is the IP address and 0 is the socket
	 * htonl converts a long integer (e.g. address) to a network
	 * representation (IP-standard byte ordering)
	 */
	memset((void *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_port = htons(0);
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	/**
	 * bind the address
	 */
	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror("bind failed");
		close(fd);
		return -1;
	}

	/** 
	 * retrieve the locally-bound name of the specified socket 
	 * store it in the sockaddr structure
	 */
	alen = sizeof(myaddr); 
	if (getsockname(fd, (struct sockaddr *)&myaddr, (socklen_t *)&alen) < 0) {
		perror("getsockname failed");
		close(fd);
		return -1;
	}

	/** 
	 * retrieve the local ip address
	 * store it in the sockaddr structure
	 */
	struct hostent *mh = gethostbyname(myname);
	if (!mh) {
		perror("gethostbyname failed");
		close(fd);
		return -1;
	}
	memcpy((void *)&myaddr.sin_addr, mh->h_addr_list[0], mh->h_length);

	printf("Phase 1: Patient 1 has TCP port number %u and IP address %s.\n", ntohs(myaddr.sin_port), inet_ntoa(myaddr.sin_addr));

	/**
	 * put address family and port number into server's address structure
	 */
	memset((void *)&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(servport);

	/**
	 * look up the ip address of the server
	 * put the ip address into server's address structure
	 */
	struct hostent *sh = gethostbyname(servname);
	if (!sh) {
		perror("gethostbyname failed");
		close(fd);
		return -1;
	}
	memcpy((void *)&servaddr.sin_addr, sh->h_addr_list[0], sh->h_length);

	/**
	 * connect to the server 
	 */
	if (connect(fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
		perror("connect failed");
		return -1;
	}

	/**
	 * send authentication information to the server
	 */
	asprintf(&tmp, "authenticate %s %s", username, password);
	strcpy(buf, tmp);
	if (write(fd, buf, bufsize) < 0) {
		perror("write failed");
		return -1;
	} 

	printf("Phase 1: Authentication request from Patient 1 with username %s and password %s has been sent to the Health Center Server.\n", username, password);

	/**
	 * read authentication result from the server
	 * check if authentication is successful
	 */
	if (read(fd, buf, bufsize) < 0) {
		perror("read failed");
		return -1;
	}
	
	printf("Phase 1: Patient 1 authentication result: %s.\n", buf);
	printf("Phase 1: End of Phase 1 for Patient 1.\n");

	/**
	 * if authentication fails, quit the program
	 */ 
	if (strcmp(buf, "success") != 0) {
		return 0;
	}

	/**
	 * ask the server for availabilities
	 */
	scanf("%s", buf);
	if (write(fd, buf, bufsize) < 0) {
		perror("write failed");
		return -1;
	}

	/**
	 * read availabilities from the server
	 */
	if (read(fd, buf, bufsize) < 0) {
		perror("read failed");
		return -1;
	}

	printf("Phase 2: The following appointments are available for Patient 1:\n");
	printf("%s", buf);

	/**
	 * enter the preferred appointment index and sent it to the server
	 */
	printf("Please enter the preferred appointment index and press enter:\n");

	strcpy(buf, "");
	scanf("%s", tmp);
	strcat(buf, tmp);
	strcat(buf, " ");
	scanf("%s", tmp);
	strcat(buf, tmp);
	
	if (write(fd, buf, bufsize) < 0) {
		perror("write failed");
		return -1;
	} 

	/**
	 * read the reply from the server
	 */
	if (read(fd, buf, bufsize) < 0) {
		perror("read failed");
		return -1;
	} 

	if (strcmp(buf, "notavailable") == 0) {
		printf("Phase 2: The requested appointment from Patient 1 is not available.\n");
		close(fd);
		return 0;
	} else {
		token = strtok(buf, " ");
		strcpy(doc_name, buf);
		token = strtok(NULL, " ");
		docport = atoi(token);
		printf("Phase 2: The requested appointment is available and reserved to Patient 1. The assigned doctor port number is: %d.\n", docport);
		close(fd);
	}

	/*
	 * create a udp/ip socket
	 * request the Internet address protocol
	 * and a datagram interface (UDP/IP)
	 */
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create udp socket");
		return -1;
	}

	/**
	 * bind to an arbitrary return address
	 * in this case, since it's the client side, so we won't
	 * care about the port number as no application will connect here
	 * INADDR_ANY is the IP address and 0 is the socket
	 * htonl converts a long integer (e.g. address) to a network
	 * representation (IP-standard byte ordering)
	 */
	memset((void *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_port = htons(0);
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror("bind failed");
		close(fd);
		return -1;
	}

	/** 
	 * put socket id and ip address in the sockaddr structure
	 */
	alen = sizeof(myaddr); 
	if (getsockname(fd, (struct sockaddr *)&myaddr, (socklen_t *)&alen) < 0) {
		perror("getsockname failed");
		close(fd);
		return -1;
	}
	memcpy((void *)&myaddr.sin_addr, mh->h_addr_list[0], mh->h_length);

	printf("Phase 3: Patient 1 has a dynamic UDP port number %u and IP address %s.\n", ntohs(myaddr.sin_port), inet_ntoa(myaddr.sin_addr));
	
	/**
	 * put address family, port number, ip address into server's address structure
	 */
	memset((void *)&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(docport);
	memcpy((void *)&servaddr.sin_addr, sh->h_addr_list[0], sh->h_length);

	/**
	 * send insurance plan to doctor
	 */
	fp = fopen("patient1insurance.txt", "r");
	if (!fp) {
		perror("fopen failed");
		return -1;
	}
	if (!fscanf(fp, "%s\n", buf)) {
		perror("reading insurance failed");
		return -1;
	}
	fclose(fp);
	if (sendto(fd, buf, bufsize, 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		perror("sendto failed");
		return -1;
	}

	printf("Phase 3: The cost estimation request from Patient 1 with insurance plan %s has been sent to the doctor with port number %u and IP address %s.\n", buf, ntohs(servaddr.sin_port), inet_ntoa(servaddr.sin_addr));

	/**
	 * receive cost estimation from doctor
	 */ 
	socklen_t slen = sizeof(servaddr);
	if (recvfrom(fd, buf, bufsize, 0, (struct sockaddr *)&servaddr, &slen) < 0) {
		perror("recvfrom failed");
		return -1;
	}

	printf("Phase 3: Patient 1 receives %s$ estimation cost from doctor with port number %d and name %s.\n", buf, docport, doc_name);
	printf("Phase 3: End of Phase 3 for Patient 1.\n");

	return 0;
}

