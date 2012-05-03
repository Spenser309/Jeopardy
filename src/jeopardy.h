/*
 * jeopardy.h
 *
 *  Created on: Apr 22, 2012
 *      Author: spenser
 */

#ifndef JEOPARDY_H_
#define JEOPARDY_H_

/* ngetline error codes */
#define OK       0
#define NO_INPUT 1
#define TOO_LONG 2

/* Error codes */
#define OK 0
#define NOK 1
#define ENOCONN 2

/* Bitmap Editing */
#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))
#define SET_BIT(var,pos) ((var) = (1<<pos) | (var))

struct jeo_entry {
	int value;
	char* category;
	char* question;
	char* answer;
};

struct jeo_question {
	char* question;
	char* answer;
};

struct jeo_cat {
	struct jeo_question questions[5];
	char* category;
	int avail;
};

struct jeo_board {
	struct jeo_cat categories[6];
	int avail;
};

struct jeo_player {
	char* name;
	int fn;
	FILE* fp;
	int pts;
};

int jeo_get_beep(int* clients, int num_clients, int played);
void jeo_notify_clients(int player, int* clients, int num_clients);
void jeo_print_question(struct jeo_board board, int x, int y, int* clients, int num_clients);
void jeo_select_question(struct jeo_board* board, int player, int* x, int* y, int* clients);
int jeo_check_answer(struct jeo_board board, int player, int x, int y, int* clients);
void jeo_print_status(int* clients, int player, int win, int* points, int num_players);
void jeo_print_board(struct jeo_board board);
void jeo_send_board(struct jeo_board board, int* clients, int num_clients);
int jeo_load_db(struct jeo_entry** db, char* filename);
int jeo_load_cats(struct jeo_cat** cats, struct jeo_entry* db, int size);
int jeo_build_board(struct jeo_board* board, struct jeo_cat* cats, int size);
void jeo_free_db(struct jeo_entry** db, int size);
void jeo_free_cats(struct jeo_cat** cats, int size);
int jeo_round_winner(int points[], int num_players);
int ngetline (char *buf, size_t size, FILE* file);
int fngetline (char *buf, size_t size, int file);

#endif /* JEOPARDY_H_ */
