#include <math.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>  // htons()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>

int main(){

	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr = {0};
	struct hostent* host;
    char pack[32];
    size_t pack_len, recv_bytes;
    int ret;

    //create a socket
    int socket_desc = socket(AF_INET, SOCK_DGRAM, 0);

    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(5000);

    //quickly restart our server after a crash 
    int reuseaddr_opt = 1;
    ret = setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_opt, sizeof(reuseaddr_opt));
    
    int sockaddr_len = sizeof(struct sockaddr_in);

    ret = bind(socket_desc, (struct sockaddr*) &server_addr, sockaddr_len);
    int clientlen = sizeof(client_addr);
    while(1){
        //recv UDP packets
        if (ret = recvfrom(socket_desc, pack, sizeof(pack)-1, 0,
        	 (struct sockaddr*) &client_addr, &clientlen) < 0) {
			printf("error\n");
		}
		//pack[ret] = '\0';
        fprintf(stderr,"pack = %s recv_bytes = %zu\n", pack, strlen(pack));
        host = gethostbyaddr((const char*)&client_addr.sin_addr.s_addr, 
        		sizeof(client_addr.sin_addr.s_addr), AF_INET);
        usleep(30000);
    }
}
