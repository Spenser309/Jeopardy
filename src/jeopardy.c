/*
 ============================================================================
 Name        : jeopardy.c
 Author      : Spenser Gilliland <spenser@gillilanding.com>
 Version     : v0.1
 Copyright   : N/A
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
	char line[2];
	char answer;
	ngetline(line, 2, stdin);
	sscanf(line,"%c", &answer);
	while(answer != 'y' && answer != 'n') {
		printf("Please type y or n\n");
		ngetline(line, 2, stdin);
		sscanf(line,"%c", &answer);
	}

	return answer;
}

int jeo_master_setup(FILE*** cli_sockfps, int** cli_sockfds) {
	int sockfd;
	struct sockaddr_in addr, cli_addr;
	socklen_t cli_addr_size = sizeof(cli_addr);
	int num_players = 0;
	int done = 0;
	char buffer[255];
	*cli_sockfds = malloc(sizeof(int) * max_conns);
	*cli_sockfps = malloc(sizeof(FILE*) * max_conns);

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
		(*cli_sockfds)[num_players] = accept(sockfd, (struct sockaddr *) &cli_addr, &cli_addr_size);

		if((*cli_sockfds)[num_players] < 0) {
			perror("WARNING: Could not accept connection lets try the next one");
			usleep(100000); /* Probably Something bad happened wait a bit see if it fixes itself */
			continue;
		}
		printf("Accepting connection from %s\n", inet_ntoa(cli_addr.sin_addr));

		if(((*cli_sockfps)[num_players] = fdopen((*cli_sockfds)[num_players], "rw")) == NULL) {
			perror("ERROR: Could not turn socket into fp");
			exit(EXIT_FAILURE);
		}

		/* Get Player Name */
		ngetline(buffer, 255,(*cli_sockfps)[num_players]);

		printf("Would you like to play with %s? (y/n) ", buffer);
		if(yesorno() == 'y') {
			num_players++; /* Add player */
		} else {
			fclose((*cli_sockfps)[num_players]);
			close((*cli_sockfds)[num_players]); /* Close Connection */
		}

		if (num_players == max_conns) {
			printf("Maximum number of players added to game starting...\n");
			done = 1;
		} else {
			printf("Would you like to start the game? (y/n) ");
			if(yesorno() == 'y')
				done = 1;
		}
	} while(done == 0);
	close(sockfd);

	printf("Game Starting\n");
	return num_players;
}



void jeo_master(char* name, char* db_filename) {
	int i;

	/* From the DB */
	struct jeo_entry* db = NULL; /* Each line in the db is represented by an jeo_entry */
	struct jeo_cat* cats = NULL; /* Each category from the db is represented by a jeo_cat */
	struct jeo_board board; /* A game board */
	int dsize = 0; /* Number of entries */
	int csize = 0; /* Number of categories */

	/* From the Network */
	int* clientfns;
	FILE** clientfps;
	int num_clients;

    /* Game Play */
	int x,y; /* Coordinate of Question to answer */
	int leader, player; /* current leader and current player */
	int round, question;
	int played;
	int *points;
	int done = 0;

	printf("Loading Question DB");
	dsize = jeo_load_db(&db, db_filename);
	if(dsize < 0) {
		printf("ERROR: Could not load Question DB\n");
		exit(EXIT_FAILURE);
	}
	printf("...");
	csize = jeo_load_cats(&cats, db, dsize);
	if(csize < 0) {
		printf("ERROR: Could not determine categories\n");
		exit(EXIT_FAILURE);
	}
	printf("...done\n");

	printf("Starting Network Server\n");

	num_clients = jeo_master_setup(&clientfps, &clientfns);

	points = calloc(sizeof(int), num_clients+1);

	printf("Let's Play Jeopardy!!!\n");

	for(round = 0; round < 3; round++) {
		printf("Start of Round %i\n", round);
		jeo_build_board(&board, cats, csize);

		jeo_send_board(board, clientfps, num_clients);

		leader = 0;
		for(question = 0; question < 30; question++) {
			played = 0;

			jeo_print_board(board);
			jeo_select_question(&board, leader, &x, &y, clientfps);
			jeo_print_question(board, x, y, clientfps, num_clients);

			do {
				player = jeo_get_beep(clientfps, num_clients, played);

				if(player == -1) { /* Skip Question */
					printf("The answer was %s\n", board.categories[x].questions[y].answer);
					done = 1;
				} else {

					SET_BIT(played,player);

					if (jeo_check_answer(board, player, x, y, clientfps) == 0) {
						leader = player;
						points[player] += (y+1)*100;
						printf("Player %i: gave a correct answer his score has been increased to %i\n", player, points[player]);
						done = 1;
					} else {
						points[player] -= (y+1)*100;
						printf("Player %i: gave an incorrect answer his score has been lowered to %i\n", player, points[player]);
					}
				}
				jeo_print_status(clientfps, player,done, points, num_clients+1);
			} while(done == 0);
		}
		leader = jeo_round_winner(points,num_clients+1);
		if (round < 2) {
			printf("\n\nThe leader of round %i was player %i; Player %i start us off for round %i.\n", round, leader, leader, round+1);
		}
	}

	printf("\n\nThe winner is Player %i\n",jeo_round_winner(points,num_clients+1));

	printf("GAME OVER\n");

	for(i = 0; i < num_clients; i++) {
		fclose(clientfps[i]);
		close(clientfns[i]);
	}
	free(clientfps);
	free(clientfns);
	jeo_free_cats(&cats, csize);
	jeo_free_db(&db, dsize);

	free(points);
}

int jeo_client_ident(char* name, int sockfd) {
	FILE* w_conn = fdopen(sockfd, "w");

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

	printf("Starting jeopardy client %s\n", master->h_name);
	sockfd = jeo_client_connect(name, master);

	if(!jeo_client_ident(name,sockfd) != 0) {
		printf("He doesn't want to play with you.\n");
		exit(EXIT_FAILURE);
	}

	close(sockfd);
	return 0;
}

int main(int argc, char *argv[]) {

	struct hostent *master;

	if(argc < 3) {
		printf("usage: %s [-c] <name> [<address>]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if(argc < 4 ) {
		jeo_master(argv[1], argv[2]);
	}
	else {
		sleep(1);
		master = gethostbyname(argv[3]);
		if (master == NULL) {
			printf("ERROR: Invalid hostname\n");
			exit(EXIT_FAILURE);
		}
		jeo_client(argv[2], master);
	}
	return 0;
}

