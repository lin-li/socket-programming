/*
 *	code for health center server
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
 * data structure for user information 
 */
struct user {
	char username[80];
	char password[80];
};

/**
 * data structure for appointment session information 
 */
struct session {
	char time_index[80];
	char day[80];
	char hour[80];
	char doc_id[80];
	char doc_port[80];
	int reserved;
};

/**
 * data structure for socket information 
 * used as variable passed to worker thread
 */
struct socketinfo {
	int rqst;
	struct sockaddr_in addr;
};

/**
 * global variables shared by different threads
 */
struct session *sessions; // array of all appointment sessions
struct user *users; // array of all users
int num_session = 0; // number of appointment sessions
int num_user = 0; // number of users

/**
 * thread to perform service
 */
void *handle_connection(void *info_pointer) {
	struct socketinfo info = *(struct socketinfo*)info_pointer;

	int rqst = info.rqst; // socket descriptor for the request

	int bufsize = 1024; // buffer size 
	char buf[bufsize]; // buffer for communication between sockets
	
	char username[80], password[80]; // username and password of patient
	char time_index[80]; // time index of the appointment
	
	char *token; // used in parsing message

	int i;
	
	/**
	 * read username and password of the client
	 */
	if (read(rqst, buf, bufsize) < 0) {
		perror("read failed");
		return 0;
	}

	/**
	 * parse the message to get username and password
	 */ 
	token = strtok(buf, " ");
	token = strtok(NULL, " ");
	strcpy(username, token);
	token = strtok(NULL, " ");
	strcpy(password, token);
		
	printf("Phase 1: The Health Center Server has received request from a patient with username %s and password %s.\n", username, password);

	/** 
	 * check if there is a match
	 */ 
	int found = 0;
	for (i = 0; i < num_user; ++i) {
		if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) == 0) {
			found = 1;
			break;
		}
	}

	/** 
	 * send the authentication response
	 */ 
	if (found == 0) {
		strcpy(buf, "failure");
	} else {
		strcpy(buf, "success");
	}
	if (write(rqst, buf, bufsize) < 0) {
		perror("write failed");
		return 0;
	}

	printf("Phase 1: The Health Center Server sends the response %s to patient with username %s.\n", buf, username);

	if (found == 0) {
		close(rqst);
		return 0;
	}

	/**
	 * receive the request for available time slots
	 */
	if (read(rqst, buf, bufsize) < 0) {
		perror("read failed");
		return 0;
	}

	printf("Phase 2: The Health Center Server, receives a request for available time slots from patient with port number %u and IP address %s.\n", ntohs(info.addr.sin_port), inet_ntoa(info.addr.sin_addr));

	/**
	 * send available time slots to the patient
	 */
	strcpy(buf, "");
	for (i = 0; i < num_session; ++i) {
		if (sessions[i].reserved == 0) {
			char *tmp;
			asprintf(&tmp, "%s %s %s\n", sessions[i].time_index, sessions[i].day, sessions[i].hour);
			strcat(buf, tmp);
		}
	}
	if (write(rqst, buf, bufsize) < 0) {
		perror("write failed");
		return 0;
	}

	printf("Phase 2: The Health Center Server sends available time slots to patient with username %s.\n", username);

	/**
	 * receive appointment preference from the patient
	 */ 
	if (read(rqst, buf, bufsize) < 0) {
		perror("read failed");
		return 0;
	}
	token = strtok(buf, " ");
	token = strtok(NULL, " ");
	strcpy(time_index, token);
	
	printf("Phase 2: The Health Center receives a request for appointment %s from patient with port number %u and username %s.\n", time_index, ntohs(info.addr.sin_port), username);

	/**
	 * check the availability of the selected time slot
	 */
	for (i = 0; i < num_session; ++i) {
		if (strcmp(sessions[i].time_index, time_index) == 0 && sessions[i].reserved == 0) {
			sessions[i].reserved = 1;
			char *tmp;
			asprintf(&tmp, "%s %s", sessions[i].doc_id, sessions[i].doc_port);
			strcpy(buf, tmp);
			write(rqst, buf, bufsize);
			printf("Phase 2: The Health Center Server confirms the following appointment %s to patient with username %s.\n", time_index, username);
			close(rqst);
			return 0;
		}
	}

	strcpy(buf, "notavailable");
	if (write(rqst, buf, bufsize) < 0) {
		perror("write failed");
		return 0;
	}

	printf("Phase 2: The Health Center Server rejects the following appointment %s to patient with username %s.\n", time_index, username);

	close(rqst);
	return 0;
}

int main() {
	int svc;        // listening socket
	int rqst;       // socket to peform service
	
	char *myname = "nunki.usc.edu"; // server name

	struct sockaddr_in myaddr;    // server address
	struct sockaddr_in clientaddr;  // client address
	
	int port = 21399; // port number

	char username[80], password[80]; // username and password of user
	char time_index[80], day[80], hour[80], doc_id[80], doc_port[80]; // time_index, day, hour, doctor id and doctor port number of appointment
	
	FILE *fp; // file

	pthread_t thread_id; // thread

	int i;

	/**
	 * read user authentication information from the file
	 */
	fp = fopen("users.txt", "r");
	if (!fp) {
		perror("fopen failed");
		return -1;
	}
	while(!feof(fp)) {
		if (!fscanf(fp, "%s %s\n", username, password)) {
			perror("reading user database failed");
			return -1;
		}
		num_user += 1;
	}
	fclose(fp);

	users = (struct user *)malloc(num_user * sizeof(struct user));;

	fp = fopen("users.txt", "r");
	if (!fp) {
		perror("fopen failed");
		return -1;
	}
	for (i = 0; i < num_user; ++i) {
		if (!fscanf(fp, "%s %s\n", username, password)) {
			perror("reading user database failed");
			return -1;
		}
		strcpy(users[i].username, username);
		strcpy(users[i].password, password);
	}
	fclose(fp);

	/**
	 * read available appointments from the file
	 */
	fp = fopen("availabilities.txt", "r");
	if (!fp) {
		perror("fopen failed");
		return -1;
	}
	while(!feof(fp)) {
		if (!fscanf(fp, "%s %s %s %s %s\n", time_index, day, hour, doc_id, doc_port)) {
			perror("reading availability database failed");
			return -1;
		}
		num_session += 1;
	}
	fclose(fp);

	sessions = (struct session *)malloc(num_session * sizeof(struct session));

	fp = fopen("availabilities.txt", "r");
	if (!fp) {
		perror("fopen failed");
		return -1;
	}
	for (i = 0; i < num_session; ++i) {
		if (!fscanf(fp, "%s %s %s %s %s\n", time_index, day, hour, doc_id, doc_port)) {
			perror("reading availability database failed");
			return -1;
		}
		strcpy(sessions[i].time_index, time_index);
		strcpy(sessions[i].day, day);
		strcpy(sessions[i].hour, hour);
		strcpy(sessions[i].doc_id, doc_id);
		strcpy(sessions[i].doc_port, doc_port);
		sessions[i].reserved = 0;
	}
	fclose(fp);

	/**
	 * create a tcp/ip socket
	 * request the Internet address protocol
	 * and a reliable 2-way byte stream (TCP/IP)
	 */
	if ((svc = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("cannot create tcp socket");
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

	printf("Phase 1: The Health Center Server has port number %u and IP address %s.\n", ntohs(myaddr.sin_port), inet_ntoa(myaddr.sin_addr));

	/** 
	 * set the socket for listening (queue backlog of 5)
	 */ 
	if (listen(svc, 5) < 0) {
		perror("listen failed");
		close(svc);
		return -1;
	}

	/**
	 * loop forever - wait for connection requests and perform the service
	 */
	while (1) {
		/**
		 * loop until accept a request
		 */
		socklen_t addrlen = sizeof(struct sockaddr_in);
		while ((rqst = accept(svc, (struct sockaddr *)&clientaddr, &addrlen)) < 0) {
			continue;
		}

		/**
         * create a worker thread to perform service
         */
        struct socketinfo info;
        info.rqst = rqst;
        memcpy((void *)&info.addr, (void *)&clientaddr, sizeof(clientaddr));
        
		if (pthread_create(&thread_id, NULL, handle_connection, (void*)&info) < 0) {
        	perror("pthread_create failed");
        	close(svc);
        	return -1;
        }
	}

	return 0;
}
