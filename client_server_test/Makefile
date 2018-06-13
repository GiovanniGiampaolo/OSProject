CC = gcc
CFLAGS = -w -g
AR = ar

DEPS = help.h\
       function.h\
	gamer.h\

LIBS = -lpthread

OBJ = function.c\
	gamer.c\

BIN = game_server\
	game_client\
	udp_server\
	udp_client

all: $(BIN)

game_server: game_server.c $(DEPS) 
	$(CC) $(CFLAGS) -o game_server game_server.c $(OBJ) $(LIBS)

game_client: game_client.c $(DEPS)
	$(CC) $(CFLAGS) -o game_client game_client.c $(OBJ) $(LIBS)

udp_server: game_server.c $(DEPS) 
	$(CC) $(CFLAGS) -o udp_server udp_server.c $(OBJ) $(LIBS)

udp_client: game_client.c $(DEPS)
	$(CC) $(CFLAGS) -o udp_client udp_client.c $(OBJ) $(LIBS)

.PHONY: clean
clean:
	rm -f $(BIN)
