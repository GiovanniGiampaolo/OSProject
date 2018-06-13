#ifndef FUNCTION_H
#define FUNCTION_H

#include <stdio.h>

ssize_t SEND(int socket_desc, char *buf, size_t msg_len);
ssize_t RECEIVE(int socket_desc, char *buf, size_t buf_len);
void ERROR_HELPER(int ret, char* msg);
void PTHREAD_ERROR_HELPER(int ret, char* msg);  
int IS_ADDRESS(char *address);
int RANDOM_INTEGER(int size);
#endif
