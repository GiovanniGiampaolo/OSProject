typedef struct _Gamer Gamer;

Gamer *NEW_GAMER(char *user_id);
void ADD_GAMER(Gamer** buffer,char *user_id);
void REMOVE_GAMER(Gamer** buffer, char *user_id);
void PRINT_GAMERS();

   
