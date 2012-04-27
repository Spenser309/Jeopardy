/*
 * dbtest.c
 *
 *  Created on: Apr 22, 2012
 *      Author: spenser
 */


#include <stdlib.h>
#include <stdio.h>
#include "jeopardy.h"

int main (int argc, char *argv[]) {

	struct jeo_entry* db = NULL;
	struct jeo_cat* cats = NULL;
	struct jeo_board board;
	int csize = 0;
	int dsize = 0;

	int x,y; /* Coordinate of Question to answer */
	int leader, player; /* current leader and current player */
	int round, question;
	int played;
	int num_players = 1;
	int points[2] = {0, 0};
	int done;

	if (argc < 1) {
		printf("usage: %s <filename>", argv[0]);
		exit(EXIT_FAILURE);
	}

	printf("Testing file: %s\n", argv[1]);

	dsize = jeo_load_db(&db, argv[1]);
	if(dsize < 0) {
		printf("ERROR: Could not load Jeopardy db\n");
		exit(EXIT_FAILURE);
	}

	csize = jeo_load_cats(&cats, db, dsize);
	if(csize < 0) {
		printf("ERROR: Could not determine categories\n");
		exit(EXIT_FAILURE);
	}

	for(round = 0; round < 3; round++) {
		jeo_build_board(&board, cats, csize);
		leader = 0;
		for(question = 0; question < 30; question++) {
			played = 0;

			jeo_print_board(board);
			jeo_select_question(&board, leader, &x, &y, NULL);
			jeo_print_question(board, x, y, NULL, 0);

			do {
				player = jeo_get_beep(NULL, 0, played);

				if(player == -1) { /* Skip Question */
					printf("The answer was %s\n", board.categories[x].questions[y].answer);
					break;
				}

				SET_BIT(played,player);

				if (jeo_check_answer(board, player, x, y, NULL) == 0) {
					leader = player;
					points[player] += (y+1)*100;
					printf("Player %i: gave a correct answer his score has been increased to %i\n", player, points[player]);
					done = 1;
				} else {
					points[player] -= (y+1)*100;
					printf("Player %i: gave an incorrect answer his score has been lowered to %i\n", player, points[player]);
				}
			} while(done == 0);
		}
		leader = jeo_round_winner(points,num_players);
		printf("\n\nThe leader of round %i was player %i; Player %i start us off.\n", round, leader, leader);
	}

	printf("\n\nThe winner is %i\n",jeo_round_winner(points,num_players));

	jeo_free_db(&db, dsize);

	return 0;
}
