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
	int size = 0;
	int i;

	if (argc < 1) {
		printf("usage: %s <filename>", argv[0]);
		exit(EXIT_FAILURE);
	}

	printf("Testing file: %s\n", argv[1]);


	size = jeo_load_db(&db, argv[1]);
	if(size < 0) {
		printf("ERROR: Could not load Jeopardy db\n");
		exit(EXIT_FAILURE);
	}

	for(i = 0; i < size; i++) {
		printf("%i: c = %s, v = %i, q = %s, a = %s\n",
				i, db[i].category, db[i].value,
				   db[i].question, db[i].answer);
	}

	jeo_free_db(&db, size);

	return 0;
}
