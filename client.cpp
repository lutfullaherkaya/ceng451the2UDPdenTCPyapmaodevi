/*
** talker.c -- a datagram "client" demo
* default execution: ./client 127.0.0.1 5201 5200
*/

#include "ldp.h"
#include "client.h"

int bindMyPort(int sockfd, char* port) {
	
	// code taken from beej guide: http://beej.us/guide/bgnet/html/#bind
	struct addrinfo hints, *res;

	// first, load up address structs with getaddrinfo():

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;  // use IPv4 or IPv6, whichever
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

	getaddrinfo(NULL, port, &hints, &res);

	// bind it to the port we passed in to getaddrinfo():
	// TODO: For dongusuyle butun iplere bak bagli listedeki
	bind(sockfd, res->ai_addr, res->ai_addrlen);
}

int main(int argc, char *argv[])
{
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	// default c-line arguments
	char* serverIP = "127.0.0.1", *myPort = "5201", *serverPort = "5200";
	

	if (argc != 4) {
		fprintf(stderr,"usage: ./client <server-ip> <client-port-number> <server-port-number>.\n");
		exit(1);
	}
	serverIP = argv[1];
	myPort = argv[2];
	serverPort = argv[3];


	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // set to AF_INET to use IPv4
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(argv[1], serverPort, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}

		break;
	}
	bindMyPort(sockfd, myPort);

	if (p == NULL) {
		fprintf(stderr, "talker: failed to create socket\n");
		return 2;
	}

	if ((numbytes = sendto(sockfd, argv[2], strlen(argv[2]), 0,
			 p->ai_addr, p->ai_addrlen)) == -1) {
		perror("talker: sendto");
		exit(1);
	}

	freeaddrinfo(servinfo);

	printf("talker: sent %d bytes to %s\n", numbytes, argv[1]);
	close(sockfd);

	return 0;
}
