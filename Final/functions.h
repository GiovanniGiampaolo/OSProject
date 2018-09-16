#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdio.h>
#include <netinet/in.h>

ssize_t SEND(int socket_desc, char *buf, size_t msg_len);
ssize_t RECV(int socket_desc, char *buf, size_t buf_len);
void ERROR_HELPER(int ret, char* msg);
void PTHREAD_ERROR_HELPER(int ret, char* msg);  
int IS_ADDRESS(char *address);
void DEBUG_PRINT(char *text);
ssize_t SENDTO(int socket_desc, char* msg, int size, struct sockaddr_in* server_addr);
ssize_t RECVFROM(int socket_desc, char* buffer, int size);
int RANDOM_INTEGER(int size);

#endif
