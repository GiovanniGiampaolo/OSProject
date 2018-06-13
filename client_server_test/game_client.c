#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>  // htons() and inet_addr()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
#include "help.h"

int main(int argc, char* argv[]) {

    in_addr_t ip_addr = inet_addr(SERVER_ADDRESS);

    if (argc == 2){
	
	   if (IS_ADDRESS(argv[1])){ 
            ip_addr = inet_addr(argv[1]);
	       fprintf(stderr, "---@gamer Starting REMOTE CONNECTION\n");
        } 
	   else {
	       fprintf(stderr, "---@gamer USAGE --> ./game_client IP_ADDRESS\n");
	       exit(EXIT_FAILURE);
	       }
    }

    int ret, user_id;
    size_t command_len = strlen(MAP_COMMAND);

    // variables for handling a socket
    int socket_desc;
    struct sockaddr_in server_addr = {0};

    // create a socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_HELPER(socket_desc, "Could not create socket");

    // set up parameters for the connection
    server_addr.sin_addr.s_addr = ip_addr;
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(SERVER_PORT);

    // initiate a connection on the socket
    ret = connect(socket_desc, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in));
    ERROR_HELPER(ret, "Could not create connection");

    char buf[1000000];
    size_t buf_len = sizeof(buf), msg_len;

    //display welcome message
    msg_len = RECEIVE(socket_desc, buf, buf_len-1);
    buf[msg_len] = '\0';
    printf("%s", buf);
    memset(buf, 0, msg_len);
    user_id = atoi(buf+msg_len-3);

    // display commands form server
    msg_len = RECEIVE(socket_desc, buf, buf_len-1);
    buf[msg_len] = '\0';
    printf("%s", buf);
    memset(buf, 0, msg_len);

    //send command
    while(1){
	   printf("---@%d ", user_id);
	   scanf("%s", buf);
	   msg_len = strlen(buf);
	   ret = SEND(socket_desc, buf, msg_len);
	   if (msg_len == command_len && !memcmp(buf, MAP_COMMAND, command_len)) printf("---@%d map requested\n", user_id);
	   memset(buf, 0, msg_len);
    }

    msg_len = RECEIVE(socket_desc, buf, buf_len-1);
    printf("bytes recv = %d\n", msg_len);
    printf("%s\n", buf);

    // close the socket
    ret = close(socket_desc);
    ERROR_HELPER(ret, "Cannot close socket");
    printf("---@%d exiting\n", user_id);
    
    return 0;
}
