#include "function.h"

#include "help.h"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "help.h"

ssize_t SEND(int socket_desc, char *buf, size_t msg_len){
	int ret_val;
	while ((ret_val = send(socket_desc, buf, msg_len, 0)) < 0) {
		if (errno == EINTR) continue;
		ERROR_HELPER(-1, "Cannot write to the socket");
    	} return ret_val;
}

ssize_t RECEIVE(int socket_desc, char *buf, size_t buf_len){
	int recv_val;
	while ((recv_val = recv(socket_desc, buf, buf_len, 0)) < 0) {
	        if (errno == EINTR) continue;
        	ERROR_HELPER(-1, "Cannot read from socket");
	}
	buf[recv_val] = '\0';
	return recv_val;
}

void GENERIC_ERROR_HELPER(int cond, char *errCode, char *msg){ 
	do {                \
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



