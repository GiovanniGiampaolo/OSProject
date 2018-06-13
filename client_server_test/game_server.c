#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>  // htons()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
#include <time.h>
#include "help.h"
#include "function.h"
#include "gamer.h"

typedef struct handler_args{
    int socket_desc;
    struct sockaddr_in *client_addr;
} handler_args;

//global variables
Gamer *gamer_buffer[];

void* connection_handler(void *arg) {

handler_args* args = (handler_args*)arg;

    int socket_desc = args->socket_desc;
    struct sockaddr_in* client_addr = args->client_addr;

    int ret = 0, recv_bytes, user_id;
    char buf[1000000];
    size_t buf_len = sizeof(buf), map_len = strlen(MAP_COMMAND);
    size_t msg_len;

    // parse client IP address and port
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr->sin_addr), client_ip, INET_ADDRSTRLEN);
    uint16_t client_port = ntohs(client_addr->sin_port); // port number is an unsigned short

    //funzione che sceglie casualmente user id
    

/*
    srand(time(NULL));
       for (int i = 0; i < 3; i++){
    	user_id[i] = rand()+'0';
        } user_id[3]= '\0';
        ret = SEND(socket_desc, buf, uid_len); */
    
    //welcome message
    user_id = RANDOM_INTEGER(3);

    sprintf(buf, "---@gamer Ciaoooooooooo, il tuo user_id è -> %d\n", user_id);
    msg_len = strlen(buf);
    SEND(socket_desc, buf, msg_len);
    memset(buf, 0, msg_len);
    //receive user id and create a gamer object
    
    //ADD_GAMER(gamer_buffer, user_id);
    printf("---@server The gamer's user_id is %d\n", user_id);
       
    //commands message
    sprintf(buf, "---@%d Request me the map using the command %s.\n", user_id, MAP_COMMAND);
    msg_len = strlen(buf);
    SEND(socket_desc, buf, msg_len);
    memset(buf, 0, msg_len);
    //if (DEBUG) PRINT_GAMERS(gamer_buffer);

    //wait for commands
    while(1){
	recv_bytes = RECEIVE(socket_desc, buf, buf_len);
        buf[recv_bytes] = '\0';
        //if (DEBUG) fprintf(stderr,"---@server Command --> %s and %d bytes\n", buf, recv_bytes);
	
	if (recv_bytes == 0) break;

	
	if (recv_bytes == map_len && !memcmp(buf, MAP_COMMAND, map_len)){
		printf("---@server The gamer %d requested the map\n",user_id);
		break;
        }
    
}
    ret = close(socket_desc);
    ERROR_HELPER(ret, "Cannot close socket for incoming connection");

    //free buffer and exit
    free(args->client_addr);
    free(args);
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    
    // we use network byte order
    uint16_t port_number = htons(SERVER_PORT);
    
    int ret;

    int socket_desc, client_desc;

    struct sockaddr_in server_addr = {0};
    struct sockaddr_in* client_addr = calloc(1, sizeof(struct sockaddr_in));
    int sockaddr_len = sizeof(struct sockaddr_in);

    //initialize socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    ERROR_HELPER(socket_desc, "Could not create socket");

    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = port_number;

    //quickly restart our server after a crash:
    int reuseaddr_opt = 1;
    ret = setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_opt, sizeof(reuseaddr_opt));
    ERROR_HELPER(ret, "Cannot set SO_REUSEADDR option");

    // bind address to socket
    ret = bind(socket_desc, (struct sockaddr*) &server_addr, sockaddr_len);
    ERROR_HELPER(ret, "Cannot bind address to socket");

    // start listening
    ret = listen(socket_desc, N_GAMER);
    ERROR_HELPER(ret, "Cannot listen on socket");
    if (DEBUG) fprintf(stderr, "---@server is listening..\n");

    //accept connection
    while(1){
	   client_desc = accept(socket_desc, (struct sockaddr*) client_addr, (socklen_t*) &sockaddr_len);
	   ERROR_HELPER(client_desc, "cannot accept connection");
	
	   //prepare thread creation
	   pthread_t thread;
	   handler_args* thread_args = malloc(sizeof(handler_args));
	   thread_args->socket_desc = client_desc;
	   thread_args->client_addr = client_addr;

	   //invoke connection handler
       ret = pthread_create(&thread, NULL, connection_handler, (void*)thread_args);
        PTHREAD_ERROR_HELPER(ret, "Could not create a new thread");
	   if (DEBUG) fprintf(stderr, "---@server gamer accepted...\n");

	   ret = pthread_detach(thread); //I don't need to join thread
        PTHREAD_ERROR_HELPER(ret, "Could not detach the thread");            
        
        //need a new buffer for client_addr
        client_addr = calloc(1, sizeof(struct sockaddr_in));
    }
    
    return 0;
}
