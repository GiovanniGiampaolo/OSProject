#pragma once
#include "vehicle.h"
#include "world.h"

typedef struct Gamer{
	int user_id;
	int game;
} Gamer;

void GAMER_INIT(Gamer** Gamers);
Gamer* NEW_GAMER(int user_id);
void ADD_GAMER(Gamer** buffer, int user_id);
Gamer* GET_GAMER(Gamer** buffer, int i);
int* GET_IDS(Gamer** buffer);
int GET_ID(Gamer** buffer, int i);
void REMOVE_GAMER(Gamer** buffer, int user_id);
void REMOVE(Gamer** buffer, int i);
void SET_GAME(Gamer** buffer, int i, int game);
void RESET_GAMERS(Gamer** buffer);
void ADD_ALL(World* world, Image* img, int my_id, Gamer** buffer);
void SORT(Gamer** buffer);
int NUM_PLAYERS(Gamer** buffer);
int FIND_GAMER(Gamer** buffer, int id);
int SERIALIZE_GAMERS(Gamer** buffer, char* buf);
int DESERIALIZE_GAMERS(char** buffer, char* buf);
int* UPDATE_GAMERS(Gamer** buffer);
void PRINT_GAMERS(Gamer** buffer);