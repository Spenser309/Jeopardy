/*
 ============================================================================
 Name        : jeopady.c
 Author      : Spenser Gilliland <spenser@gillilanding.com>
 Version     :
 Copyright   : GPLv3
 Description : Hello World in C, Ansi-style
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

static short int portno = 22000;
static short int conns = 5;

int* jeo_master(char* name) {
	char answer;
	int sockfd, n;
	struct sockaddr_in addr, cli_addr;
	socklen_t clilen;
	int num_players = 0;
	int done = 0;
	char buffer[255];
	int *cli_sockfd = malloc(sizeof(int) * 5);

	printf("Starting Jeopardy master\n");

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(portno);
	if ( bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		perror("ERROR: Binding to socket");
		exit(EXIT_FAILURE);
	}

	do {
		if( listen(sockfd, conns) < 0) {
			perror("ERROR: Listen on socket");
			exit(EXIT_FAILURE);
		}
		printf("Listening for Connections\n");
		clilen = sizeof(cli_addr);
		cli_sockfd[num_players] = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		printf("Accepting connection from %x\n", cli_addr.sin_addr.s_addr);
		n = read(cli_sockfd[num_players], buffer, 255);
		if (n < 0) {
			perror("ERROR: Cannot read startup message");
			exit(EXIT_FAILURE);
		}

		fflush(stdin);
		printf("Would you like to play with %s?\n", buffer);
		answer = getchar();
		if(answer == 'y') {
			num_players++;
		} else {
			close(cli_sockfd[num_players]);
			cli_sockfd[num_players] = NULL;
		}

		if (num_players == 5) {
			printf("Maximum number of players added to game starting...\n");
			done = 1;
		} else {
			fflush(stdin);
			printf("Would you like to start the game?");
			answer = getchar();

			if(answer == 'y')
				done = 1;
		}
	} while(done == 0);
	close(sockfd);
	return cli_sockfd;
}

int jeo_client(char* name, struct hostent *master) {
	int sockfd, n;
	struct sockaddr_in serv_addr;
	char buffer[255];

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
		exit(EXIT_FAILURE);
	}

	n = write(sockfd, name, strlen(name)+1);
	if (n < 0 ) {
		perror("ERROR: Writing data");
		exit(EXIT_FAILURE);
	}
	printf("connected\n");



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

