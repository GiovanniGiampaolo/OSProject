#include "gamer.h"
#include "help.h"
#include "errno.h"
#include <stdlib.h>
#include <stdio.h>

struct _Gamer {
	char* user_id;
};

Gamer* NEW_GAMER(char* user_id){
	Gamer *gamer = malloc(sizeof(Gamer));
	gamer->user_id = user_id;
	return gamer;
}

void ADD_GAMER(Gamer** buffer, char* user_id){
	Gamer* gamer_i = NEW_GAMER(user_id);
	
	int i;
	for(i = 0; i < N_GAMER; i++){
		if (buffer[i] == NULL){
			buffer[i] = gamer_i;
			break;
		}
	}
	SORT(buffer);
}

void REMOVE_GAMER(Gamer** buffer, char* user_id){
	int i;
	size_t user_len = strlen(user_id);
	
	for(i = 0; i < N_GAMER; i++){
		if (buffer[i] != NULL && !memcmp(buffer[i]->user_id, user_id, user_len))
		{		
			buffer[i] = NULL;
			break;
		}
	}
	SORT(buffer);
}

void SORT(Gamer** buffer){
	Gamer* ret[N_GAMER];
	int i, j = 0;
	for(i = 0; i < N_GAMER; i++){
		if (buffer[i] != NULL){
			ret[j] = buffer[i];
			free(buffer[i]);
			j++;
		}
	}
	free(buffer);
	buffer = ret;
}

void PRINT_GAMERS(Gamer** buffer){
	int i;
	for (i = 0; i < N_GAMER; i++){
		if (buffer[i] != NULL) printf("[%d]%s-> ", i,buffer[i]->user_id);
		else printf("[%d]NULL-> ",i);
	}
	printf("\n");
}
