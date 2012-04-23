/*
 ============================================================================
 Name        : jeopardy.c
 Author      : Spenser Gilliland <spenser@gillilanding.com>
 Version     :
 Copyright   : GPLv3
 Description : Jeopardy over TCP/IP
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "jeopardy.h"

static short int portno = 22000;
static short int max_conns = 5;

char yesorno() {
	char answer;

	answer = getchar();
	getchar(); /* Remove the trailing \n */
	while(answer != 'y' && answer != 'n') {
		printf("Please type y or n\n");
		answer = getchar();
		getchar(); /* Remove the trailing \n */
	}

	return answer;
}

int jeo_get_player_name(char** name, FILE* read, FILE* write) {

	return 0;
}

int* jeo_master_setup() {
	char answer;
	int sockfd, n;
	struct sockaddr_in addr, cli_addr;
	socklen_t cli_addr_size = sizeof(cli_addr);
	int num_players = 0;
	int done = 0;
	char buffer[255];
	int *cli_sockfd = malloc(sizeof(int) * max_conns);

	/* Build TCP server socket accessible on all IPs of localhost */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(portno);

	/* Bind to the socket */
	if ( bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		perror("ERROR: Binding to socket");
		exit(EXIT_FAILURE);
	}

	/* Start listening for connection attempts */
	if( listen(sockfd, max_conns) < 0) {
		perror("ERROR: Listen on socket");
		exit(EXIT_FAILURE);
	}

	printf("Listening for Players\n");

	do {
		cli_sockfd[num_players] = accept(sockfd, (struct sockaddr *) &cli_addr, &cli_addr_size);
		if(cli_sockfd[num_players] < 0) {
			perror("WARNING: Could not accept connection lets try the next one");
			usleep(100000); /* Probably Something bad happened wait a bit see if it fixes itself */
			continue;
		}
		printf("Accepting connection from %s\n", inet_ntoa(cli_addr.sin_addr));

		/* Get Player Name */
		n = read(cli_sockfd[num_players], buffer, 255);
		if (n < 0) {
			perror("ERROR: Cannot read startup message");
			exit(EXIT_FAILURE);
		}

		printf("Would you like to play with %s? (y/n) ", buffer);
		if(yesorno() == 'y') {
			num_players++; /* Add player */
		} else {
			close(cli_sockfd[num_players]); /* Close Connection */
		}

		if (num_players == max_conns) {
			printf("Maximum number of players added to game starting...\n");
			done = 1;
		} else {
			printf("Would you like to start the game? (y/n) ");
			answer = yesorno();

			if(answer == 'y')
				done = 1;
		}
	} while(done == 0);
	close(sockfd);

	return cli_sockfd;
}


int* jeo_master(char* name) {
	int* clients;

	printf("Starting Jeopardy master\n");

	clients = jeo_master_setup();

	return clients;
}

int jeo_client_ident(char* name, int sockfd) {
	FILE* w_conn = fdopen(sockfd, "w");
	/*FILE* r_conn = fdopen(sockfd, "r"); */

	if(fprintf(w_conn, "%s", name) < 0) {
		perror("ERROR: Could not write to socket\n");
		exit(EXIT_FAILURE);
	}



	return 0;
}

int jeo_client_connect(char* name, struct hostent *master) {
	int sockfd;
	struct sockaddr_in serv_addr;

	printf("Starting jeopardy client %s\n", master->h_name);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("ERROR: Opening socket");
		exit(EXIT_FAILURE);
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *) master->h_addr, (char *)&serv_addr.sin_addr.s_addr, master->h_length);
	serv_addr.sin_port = htons(portno);
	if ( connect(sockfd, (struct sockaddr *) & serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR: Connection refused");
		return -ENOCONN;
	}
	return sockfd;
}


int jeo_client(char* name, struct hostent *master) {
	int sockfd;
	int i;

	printf("Starting jeopardy client %s\n", master->h_name);
	sockfd = jeo_client_connect(name, master);

	if(!jeo_client_ident(name,sockfd) != 0) {
		printf("He doesn't want to play with you.\n");
		exit(EXIT_FAILURE);
	}

	/* Rounds */
	for(i = 0;i < 3; i++) {

		/* Receive Board*/
		/* jeo_client_get_board(int sockfd) != 0) {
			printf("ERROR: Could not receive board");
			exit(EXIT_FAILURE);
		} */

		/*if (jeo_client_get_board(int sockfd) != 0) {
			jeo_client_choose_question(int sockfd);
		}*/

		/* Need to change to non-blocking, poll the file descriptor */
		/* Check if a response is received */
		/*jeo_client_bell(int sockfd); */
	}

	close(sockfd);
	return 0;
}

int main(int argc, char *argv[]) {

	struct hostent *master;

	if(argc < 2) {
		printf("usage: %s <name> [<address>]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if(argc < 3 ) {
		jeo_master(argv[1]);
	}
	else {
		sleep(1);
		master = gethostbyname(argv[2]);
		if (master == NULL) {
			printf("ERROR: Invalid hostname\n");
			exit(EXIT_FAILURE);
		}
		jeo_client(argv[1], master);
	}
	return 0;
}

