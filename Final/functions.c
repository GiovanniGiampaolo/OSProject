#include "functions.h"
#include "help.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>

ssize_t SEND(int socket_desc, char *buf, size_t msg_len){
	size_t ret_val = 0;
	while ((ret_val = send(socket_desc, buf, msg_len, 0)) < 0) {
		if (errno == EINTR) continue;
		ERROR_HELPER(-1, "Cannot write to the socket");
    	} 
	return ret_val;
}

ssize_t RECV(int socket_desc, char *buf, size_t buf_len){
	size_t recv_val;
	while ((recv_val = recv(socket_desc, buf, buf_len, 0)) < 0) {
	        if (errno == EINTR) continue;
        	ERROR_HELPER(-1, "Cannot read from socket");
	} 
	buf[recv_val] = '\0';	
	return recv_val;
}

ssize_t SENDTO(int socket_desc, char* msg, int size, struct sockaddr_in* server_addr){
	size_t ret;
	if (ret = sendto(socket_desc, msg, size, 0, server_addr, sizeof(struct sockaddr_in)) < 0) {
		ERROR_HELPER(-1, "failed to send UDP packet");
	}
	return ret;
}

ssize_t RECVFROM(int socket_desc, char* buffer, int size){
	size_t ret;
	if (ret = recvfrom(socket_desc, buffer, size, MSG_WAITALL, NULL, 0) < 0) {
		if (errno == EAGAIN) return -1;
		ERROR_HELPER(-1, "failed to receive UDP packet");
	}
	return ret;
}

void GENERIC_ERROR_HELPER(int cond, char *errCode, char *msg){ 
	do {                
		if (cond) {                                                  
			fprintf(stderr, "%s: %s\n", msg, strerror(errCode)); 
			exit(EXIT_FAILURE);                                  
		}                                                            
	} while(0);
}

void ERROR_HELPER(int ret, char *msg){
    GENERIC_ERROR_HELPER((ret <0), errno, msg);
}

void PTHREAD_ERROR_HELPER(int ret, char* msg){
    GENERIC_ERROR_HELPER((ret != 0), ret, msg);
}

int IS_ADDRESS(char *address){
	while (*address != '\0'){
		if (*address < '0' || *address > '9'){
		    if (*address == '.') *address++;
		    else return 0;
		} else *address++;
	} return 1; 
}

int RANDOM_INTEGER(int size){
	char num[size];
	int i, ret;
	srand(time(NULL));
	for (i = 0; i < size; i++){
		num[i] = (rand()%10)+'0';
		if (i == 0 && num[i] == 0) num[i] = 1;
	};
	ret = atoi(num);
	if (ret < 10) return ret*100;
	if (ret < 100) return ret*10;
	return ret;
}

void DEBUG_PRINT(char *text){
	fprintf(stderr, "%s%s%s\n", RED, text, RESET);
}

