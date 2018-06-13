#include <math.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>  // htons()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
#include <errno.h>

int main(){
  
  struct sockaddr_in server_addr;
  char pack[32];
  size_t pack_len, bytes_sent;

  // create a socket
  int socket_desc = socket(AF_INET, SOCK_DGRAM, 0);

  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  server_addr.sin_family      = AF_INET;
  server_addr.sin_port        = htons(5000);

  strcpy(pack, "Sono arrivati tot bytes");

  while(1){    
    //send UDP packet
	if (bytes_sent = sendto(socket_desc, pack, strlen(pack)+1, 0, (struct sockaddr*) &server_addr,
		 sizeof(server_addr)) < 0) {
		printf("errore\n");
	}

    fprintf(stderr,"pack = %s bytes_sent = %zu\n", pack, bytes_sent);

    usleep(3000000); 
  }

  close(socket_desc); 
  return 0;
}
