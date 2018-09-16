CC = gcc
CFLAGS = -w -g -std=gnu99
LIBS = -lglut -lGLU -lGL -lm -lpthread 
#Mac OSX --> LIBS = -framework GLUT -framework OpenGL -lm -lpthread

DEPS =  image.h\
	linked_list.h\
	so_game_protocol.h\
	surface.h\
	vec3.h\
	vehicle.h\
	world.h\
	world_viewer.h\
	help.h\
	functions.h\
	gamer.h

OBJ =   image.c\
	linked_list.c\
	so_game_protocol.c\
	surface.c\
	vec3.c\
	vehicle.c\
	world.c\
	world_viewer.c\
	functions.c\
	gamer.c\

BINS = so_game_server\
	so_game_client\

all: $(BINS)

so_game_server: so_game_server.c $(DEPS) 
	$(CC) $(CFLAGS) -Ofast -o so_game_server so_game_server.c $(OBJ) $(LIBS)

so_game_client: so_game_client.c $(DEPS)
	$(CC) $(CFLAGS) -Ofast -o so_game_client so_game_client.c $(OBJ) $(LIBS)

.PHONY: clean
clean:
	rm -f $(BINS)
