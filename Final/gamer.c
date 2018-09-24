#include "gamer.h"
#include "help.h"
#include "errno.h"
#include "world.h"
#include "image.h"
#include "vehicle.h"
#include <stdlib.h>
#include <stdio.h>

void GAMER_INIT(Gamer** Gamers){
	Gamers = (Gamer**) malloc(N_GAMER*sizeof(Gamer));
}

int NUM_PLAYERS(Gamer** buffer){
	int i, n = 0;
	for(i = 0; i < N_GAMER; i++){
		if (buffer[i] != NULL) n++;
	}
	return n; 
}

Gamer* NEW_GAMER(int user_id){
	Gamer *gamer = malloc(sizeof(Gamer));
	gamer->user_id = user_id;
	gamer->game = 1;
	return gamer;
}

void ADD_ALL(World* world, Image* img, int my_id, Gamer** buffer){
	int i;
	Vehicle* vehicle_i;
	Gamer* gamer_i;

	for (i = 0; i < N_GAMER; i++){
		if ((gamer_i = GET_GAMER(buffer,i)) != NULL){
			printf("BELLA ZI %d\n", i);
			vehicle_i = calloc(1, sizeof(Vehicle));
		}
	}
}

void ADD_GAMER(Gamer** buffer, int user_id){
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

void REMOVE_GAMER(Gamer** buffer, int user_id){
	int i;
	for(i = 0; i < N_GAMER; i++){
		if (buffer[i] != NULL && buffer[i]->user_id == user_id)
		{		
			buffer[i] = NULL;
			break;
		}
	}
	SORT(buffer);
}

void REMOVE(Gamer** buffer, int i){
	buffer[i] == NULL;
}

Gamer* GET_GAMER(Gamer** buffer, int i){
	return buffer[i];
}

int GET_ID(Gamer** buffer, int i){
	return (GET_GAMER(buffer, i))->user_id;
}

void SET_GAME(Gamer** buffer, int i, int game){
	(GET_GAMER(buffer, i))->game = game;
}

void RESET_GAMERS(Gamer** buffer){
	int i;
	Gamer* gamer_i;
	for (i = 0; i < N_GAMER; i++){
		if ((gamer_i = GET_GAMER(buffer,i)) != NULL)
			gamer_i->game = 0;
	}
}

int* UPDATE_GAMERS(Gamer** buffer){
	int i, n = 0, ret[N_GAMER];
	Gamer* gamer_i;
	for (i = 0; i < N_GAMER; i++){
		if ((gamer_i = GET_GAMER(buffer,i)) != NULL && !(gamer_i->game)){
			ret[n++] = gamer_i->user_id;
			REMOVE(buffer, i);
		}
	}
	SORT(buffer);
	return ret;
}

int* GET_IDS(Gamer** buffer){
	int* ids = (int*) malloc(N_GAMER*sizeof(int));
	int i;
	Gamer* gamer_i;
	for (i = 0; i < N_GAMER; i++){
		if ((gamer_i = GET_GAMER(buffer, i)) != NULL)
			ids[i] = gamer_i->user_id;
	}
	return ids;
}

void SORT(Gamer** buffer){
	int i, j;
	for(i = 0; i < N_GAMER; i++){
		for(j = 0; j < N_GAMER; j++){
			if (buffer[j] == NULL && buffer[i]!= NULL && i != j && j < i){
				buffer[j] = buffer[i];
				buffer[i] = NULL;
			} 
		}
	}
}

int SERIALIZE_GAMERS(Gamer** buffer, char* dest){
	int i, n = 0;
	memset(dest, 0, sizeof(dest));
	for (i = 0; i < N_GAMER; i++){
		if (buffer[i] != NULL)
			n+=sprintf(dest+(i*USER_SIZE), "%d", buffer[i]->user_id);
	} return n;
}

int DESERIALIZE_GAMERS(char** buffer, char* buf){
	int i, n = 0, user_id;
	char dest[USER_SIZE];

	for (i = 0; i < N_GAMER; i+=3){
		memcpy(dest, buf+i,USER_SIZE);
		user_id = atoi(dest);
		n+=1;
	} return n;
}

int FIND_GAMER(Gamer** buffer, int id){
	int i;
	for (i = 0; i < N_GAMER; i++){
		if (buffer[i] != NULL && buffer[i]->user_id == id) {
			return i;
		} 
	}
	return -1;
}

void PRINT_GAMERS(Gamer** buffer){
	int i;
	for (i = 0; i < N_GAMER; i++){
		if (buffer[i] != NULL) fprintf(stderr,"%s[%d] = %d%s ", RED, i, buffer[i]->user_id, RESET);
		else fprintf(stderr,"%s[%d] = NULL %s", RED, i, RESET);
	}
	printf("\n");
}
