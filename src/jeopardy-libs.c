/*
 * jeopardy-libs.c
 *
 *  Created on: Apr 22, 2012
 *      Author: spenser
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct jeo_entry {
	int value;
	char* category;
	char* question;
	char* answer;
};

int jeo_load_db(struct jeo_entry** db, char* filename) {
	int def_entries = 100;
	int i = 0;
	int temp;
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
		free(db[i]->answer);
		free(db[i]->question);
		free(db[i]->category);
	}

	free(*db);
}
