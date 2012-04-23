/*
 * jeopardy.h
 *
 *  Created on: Apr 22, 2012
 *      Author: spenser
 */

#ifndef JEOPARDY_H_
#define JEOPARDY_H_

/* Error codes */
#define OK 0
#define ENOCONN 1

struct jeo_entry {
	int value;
	char* category;
	char* question;
	char* answer;
};

struct jeo_question {
	int value;
	char* question;
};

struct jeo_cat {
	struct jeo_question questions[5];
};

struct jeo_board {
	struct jeo_cat categories[6];
};


int jeo_load_db(struct jeo_entry** db, char* filename);
struct jeo_board jeo_build_board(struct jeo_entry** db);
void jeo_free_db(struct jeo_entry** db, int size);

#endif /* JEOPARDY_H_ */
