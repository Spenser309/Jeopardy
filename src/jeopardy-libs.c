/*
 * jeopardy-libs.c
 *
 *  Created on: Apr 22, 2012
 *      Author: spenser
 */

#include <stdio.h>
#include <stdio_ext.h> /* For fpurge */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/select.h>
#include "jeopardy.h"


static int max_cats = 100;

void jeo_print_board(struct jeo_board board) {

	int i, j;

	printf("\n                                        ==  Jeopardy  ==\n   ");

	for(j = 0; j < 6; j++) {
		printf("%-18i",j);
	}
	printf("\n   ");
	for(j = 0; j < 6; j++) {
		printf("%-18s",board.categories[j].category);
	}
	printf("\n");

	for(i = 0; i < 5; i++) {
		printf("%i: ", i);
		for(j=0; j < 6; j++) {
			if (CHECK_BIT(board.avail,i*6+j) == 0) {
				printf("%-18i",(i+1)*100);
			} else {
				printf("---               ");
			}
		}
		printf("\n");
	}

}

void jeo_send_board(struct jeo_board board, FILE** clients, int num_clients) {
	int i, j;

	for(i = 0; i < num_clients; i++) {
		for(j = 0; j < 6; j++) {
			fprintf(clients[i],"%s;", board.categories[j].category);
		}
		fprintf(clients[i],"\n");
		fflush(clients[i]);
	}
}

void jeo_select_question(struct jeo_board* board, int player, int* x, int* y, FILE** clients) {
	char buf[10];
	int ret;

	if ( player == 0) { /* Local player */
		do {
			printf("Please Select a Question?\n");
			do {
				ngetline(buf, 10, stdin);
				ret = sscanf(buf,"%1i%1i", x, y);
			} while(ret != 2);
		} while(ret != 2 || *x < 0 || *x > 5 || *y < 0 || *y > 4 || CHECK_BIT(board->avail, ((*y*6)+(*x))));
		printf("Selected the Category \"%s\" for %i Points\n",board->categories[*x].category,(*y+1)*100);
	} else {
		printf("Waiting on player %i\n", player);
		ngetline(buf, 10, clients[player-1]);
		ret = sscanf(buf, "%i;%i", x,y);
		if(ret != 2 || *x < 0 || *x > 5 || *y < 0 || *y > 4 || CHECK_BIT(board->avail, ((*y*6)+(*x)))) {
			printf("ERROR: selected unavailable location r = %i, c = %i\n", *x, *y);
			exit(EXIT_FAILURE);
		}
	}
	SET_BIT(board->avail, ((*y*6)+(*x)));
}

int jeo_check_answer(struct jeo_board board, int player, int x, int y, FILE** clients) {
	char buf[256];

	memset(buf,'-',256);

	if( player == 0) {
		printf(">");
		ngetline(buf, 256, stdin);
		if (strcmp(buf, board.categories[x].questions[y].answer) == 0) {
			printf("Correct\n");
			return 0;
		} else {
			printf("Incorrect\n");
			return 1;
		}
	} else {
		ngetline(buf, 256, clients[player]);
	}

	return strcmp(buf, board.categories[x].questions[y].answer);
}


void jeo_print_question(struct jeo_board board, int x, int y, FILE** clients, int num_clients) {
	int i;

	printf("The Question is : \"%s\"\n", board.categories[x].questions[y].question);

#ifdef DEBUG
		printf("The Answer is : \"%s\"\n", board.categories[x].questions[y].answer);
#endif

	for(i = 0; i < num_clients; i++) {
		fprintf(clients[i],"%1i;%1i;%s\n", x, y, board.categories[x].questions[y].question);
		fflush(clients[i]);
	}
}

void jeo_print_status(FILE** clients, int player, int win, int* points, int num_players) {
	int i, j;

	printf("Player %i: Did provided an %s answer\n", player, win ? "correct":"incorrect");
	printf("Player:Points = ");
	for(i = 0; i < num_players; i++) {
		printf("%i:%i  ", i, points[i]);
	}

	for(i = 0; i < num_players-1; i++) {
		fprintf(clients[i],"%i;%i;", player, win);
		for(j = 0; j < num_players; j++) {
			fprintf(clients[i],"%i;", points[i]);
		}
		fprintf(clients[i],"\n");
		fflush(clients[i]);
	}
}

int jeo_get_beep(FILE** clients, int num_clients, int played) {
	int i;
	int rannum, ranwin;
	int winner = 0;
	int* winners = calloc(sizeof(int),num_clients+1);
	fd_set rfds;
	struct timeval tv;
	int retval;
	int num_players = 0;

	char key[] = "beep";
	char beepbuf[5] = "----";

	tv.tv_sec = 1; /* Wait 7 seconds */
	tv.tv_usec = 0;

	FD_ZERO(&rfds);
	if(CHECK_BIT(0, played) == 0) {
		FD_SET(0, &rfds); /* Add stdin */
		num_players++;
	}

	for(i = 0; i < num_clients; i++) {
		if(CHECK_BIT(i+1, played) == 0) {
			FD_SET(fileno(clients[i]), &rfds);
			num_players++;
		}
	}

	do {
		retval = select(num_players+1, &rfds, NULL, NULL, &tv);
		if(retval < 0) {
			perror("ERROR: Something bad happened during select call");
			exit(EXIT_FAILURE);
		} else if (retval) {
			if(FD_ISSET(0, &rfds)) {
				ngetline(beepbuf, 5, stdin);
				if (strcmp(beepbuf, key) == 0) {
					printf("0 ");
					winners[0] = 1;
					winner++;
				}
				memset(beepbuf, '-', 5);
			}
			for(i = 0; i < num_clients; i++) {
				if(FD_ISSET(fileno(clients[i]), &rfds)) {
						ngetline(beepbuf, 5, clients[i]);
						if (strcmp(beepbuf, key) == 0) {
							printf("%i ",i);
							winners[i+1] = 1;
							winner++;
						}
				}
				memset(beepbuf, '-', 5);
			}
			printf("\n");
		} else {
			printf("No one has taken this question\n");
			free(winners);
			return -1;
		}
	} while(winner == 0);

	srand((unsigned int) time(NULL));

	rannum = rand() - 1;
	ranwin = rannum / (RAND_MAX/winner);

	for(i = 0; i < num_clients+1; i++) {
		if(winners[i] == 1) {
			ranwin--;
		}
		if (ranwin == -1) {
			winner = i;
			break;
		}
	}

	free(winners);

	printf("Player %i: what is your answer?\n", winner);
	return winner;
}

int jeo_build_board(struct jeo_board* board, struct jeo_cat* cats, int size) {
	int i;
	unsigned int rancol, rannum;
	unsigned int seed = (unsigned int) time(NULL);
	srand(seed);

	for(i = 0; i < 6; i++) {
		rannum = rand() - 1;
		rancol = rannum / (RAND_MAX/size);

		if(cats[rancol].avail == 0) {
			board->categories[i] = cats[rancol];
			cats[rancol].avail = 1;
		} else {
			i--; /* Retry */
		}
	}

	board->avail = 0;

	return 0;
}

int jeo_load_cats(struct jeo_cat** cats, struct jeo_entry* db, int size) {
	int i,j,q;
	int num_cats = 0;

	*cats = calloc(sizeof(struct jeo_cat), max_cats);

	for (i = 0; i < size; i++) {
		q = -1;
		for(j = 0; j < num_cats; j++) {
			if(strcmp(db[i].category, (*cats)[j].category) == 0) {
				if(!(db[i].value <= 500 && (db[i].value >= 0))) {
					printf("ERROR: value in db out of range\n");
					exit(EXIT_FAILURE);
				}

				q = (db[i].value / 100) - 1;

				(*cats)[j].questions[q].question = db[i].question;
				(*cats)[j].questions[q].answer = db[i].answer;
			}
		}

		/* Not found so create a new category */
		if(q == -1) {
			(*cats)[num_cats].category = db[i].category;
			num_cats++;
			i--; /* Reparse with the new category added */
		}
	}

	/* NOTE: THIS DOES NOT TAKE INTO ACCOUNT INSTANCES WHERE CATEGORIES ARE NOT
	 * COMPLETELY FILLED */
	/* IE. when there is a 100,200,300,500 questions but not a 400 question. */

	return num_cats;
}

void jeo_free_cats(struct jeo_cat** cats, int size) {
	free(*cats);
}

int jeo_load_db(struct jeo_entry** db, char* filename) {
	int def_entries = 100;
	int i = 0;
	char line[600]; /* Max line size is 600 */
	char* category;
	char* value;
	char* question;
	char* answer;

	FILE* dbfile = fopen(filename, "r");

	if(dbfile == NULL) {
		perror("ERROR: Could not open file");
		exit(EXIT_FAILURE);
	}

	*db = malloc(sizeof(struct jeo_entry)*def_entries);

	if(*db == NULL) {
		printf("ERROR: Could not allocate memory for DB");
		exit(EXIT_FAILURE);
	}

	while(fgets(line, 600, dbfile) != NULL) {

		line[strlen(line)-2] = '\0'; /* Chomp off the trailing \n */

		category = strtok(line,";");
		value = strtok(NULL,";");
		question = strtok(NULL,";");
		answer = strtok(NULL,";");

		if(category == NULL || value == NULL || question == NULL || answer == NULL) {
			printf("ERROR: Could not parse db\n");
			exit(EXIT_FAILURE);
		}

		(*db)[i].value = strtol(value, NULL, 10);
		(*db)[i].category = malloc(sizeof(char)*(strlen(category)+1));
		(*db)[i].question = malloc(sizeof(char)*(strlen(question)+1));
		(*db)[i].answer = malloc(sizeof(char)*(strlen(answer)+1));

		if((*db)[i].category == NULL || (*db)[i].question == NULL || (*db)[i].answer == NULL) {
			printf("ERROR: Could not allocate memory for db entry\n");
			exit(EXIT_FAILURE);
		}

		strcpy((*db)[i].category, category);
		strcpy((*db)[i].question, question);
		strcpy((*db)[i].answer, answer);

		i++;
	}

	fclose(dbfile);

	return i;
}

void jeo_free_db(struct jeo_entry** db, int size) {
	int i;

	for(i = 0; i < size; i++) {
		free((*db)[i].answer);
		free((*db)[i].question);
		free((*db)[i].category);
	}

	free(*db);
}

int jeo_round_winner(int points[], int num_players) {
	int max = 0;
	int i;

	for(i = 1; i < num_players; i++) {
		if(points[i] > points[max]) {
			max = i;
		}
	}

	return max;
}

int ngetline (char *buf, size_t size, FILE* file) {
    int ch, extra;

    if (fgets (buf, size, file) == NULL)
        return NO_INPUT;

    if (buf[strlen(buf)-1] != '\n') {
        extra = 0;
        while (((ch = getchar()) != '\n') && (ch != EOF))
            extra = 1;
        return (extra == 1) ? TOO_LONG : OK;
    }

    buf[strlen(buf)-1] = '\0';
    return OK;
}

