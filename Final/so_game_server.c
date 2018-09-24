#include <math.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include <arpa/inet.h>  // htons()
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h>
#include <semaphore.h>
#include <time.h>
#include "image.h"
#include "surface.h"
#include "world.h"
#include "vehicle.h"
#include "world_viewer.h"
#include "functions.h"
#include "help.h"
#include "gamer.h"
#include "so_game_protocol.h"

Image* surface_texture;
Image* surface_elevation;
Image* vehicle_texture;
int gamer_counter;
Gamer* Gamers[N_GAMER];
sem_t lock, synchro;
char action;
int action_id;

typedef struct {
    volatile int run;
    World* world;
} updaterArgs; 

typedef struct {
    volatile int run;
    World* world;
} multicastArgs;

typedef struct {
    volatile int run;
} notificationArgs;

typedef struct {
    int socket_desc;
    struct sockaddr_in *client_addr;
    World* world;
} handler_args;


void* updater_thread(void* args_){
    updaterArgs* args=(updaterArgs*)args_;
  
    char vehicle_buf[100];
    VehicleUpdatePacket* vehicle_pack;
    Vehicle* vehicle = (Vehicle*) malloc(sizeof(Vehicle));
    int vehicle_pack_len, ret, current_gamers;

    struct sockaddr_in udp_server_addr = {0};
    int sockaddr_len = sizeof(struct sockaddr_in);

    udp_server_addr.sin_addr.s_addr = INADDR_ANY;
    udp_server_addr.sin_family      = AF_INET;
    udp_server_addr.sin_port        = htons(UDP_SERVER_PORT);

    //create a socket
    int udp_socket_desc = socket(AF_INET, SOCK_DGRAM, 0);
    ERROR_HELPER(udp_socket_desc, "Could not create socket");

    //restarst the server
    //quickly restart the server after a crash 
    int reuseaddr_opt = 1;
    ret = setsockopt(udp_socket_desc, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_opt, sizeof(reuseaddr_opt));
    ERROR_HELPER(ret, "Cannot set SO_REUSEADDR option");

    ret = bind(udp_socket_desc, (struct sockaddr*) &udp_server_addr, sockaddr_len);
    ERROR_HELPER(ret, "cannot bind socket");

    while(args->run){

    	current_gamers = NUM_PLAYERS(Gamers);
    	if (!current_gamers) continue;

        //recv UDP packets
        //Sends packets about actual position
        //This thread takes it, reads it and updates it
        ret = RECVFROM(udp_socket_desc, vehicle_buf, sizeof(vehicle_buf)-1);
        vehicle_pack_len = strlen(vehicle_buf);
        vehicle_pack = (VehicleUpdatePacket*)Packet_deserialize(vehicle_buf, vehicle_pack_len);

        //update vehicle
        //it updates the veichle positions
        vehicle = World_getVehicle(args->world, vehicle_pack->id);
        vehicle->translational_force_update = vehicle_pack->translational_force;
        vehicle->rotational_force_update = vehicle_pack->rotational_force;

        World_update(args->world);
    }

    Packet_free(&vehicle_pack->header);
    Vehicle_destroy(vehicle);
    close(udp_socket_desc);
    return 0;
}

void* multi_thread(void* args_){
    multicastArgs* args = (multicastArgs*) args_;
   
    char world_buf[1000];
    int world_pack_len, ret, i, current_gamers;
    
    ClientUpdate* updates;
    WorldUpdatePacket* world_packet = (WorldUpdatePacket*) malloc(sizeof(WorldUpdatePacket));
    PacketHeader w_head;
    w_head.type = WorldUpdate;
    world_packet->header = w_head;
    Vehicle* vehicle_i; 

    struct sockaddr_in udp_server_addr = {0};
    
    udp_server_addr.sin_addr.s_addr = inet_addr(GROUP_ADDRESS); 
    udp_server_addr.sin_family      = AF_INET;  
    udp_server_addr.sin_port        = htons(4000);

    //create a socket
    int udp_socket_desc = socket(AF_INET, SOCK_DGRAM, 0);
    ERROR_HELPER(udp_socket_desc, "Could not create socket");

    while(args->run){
    	
    	current_gamers = NUM_PLAYERS(Gamers);
    	if (!current_gamers) continue;

    	vehicle_i = (Vehicle*) calloc(1, sizeof(Vehicle));
    	updates = calloc(current_gamers, sizeof(ClientUpdate));

    	for (i = 0; i < current_gamers; i++){
    		
    		vehicle_i = World_getVehicle(args->world, GET_ID(Gamers, i));
		  			
    		//fill the fields
  			updates[i].id = vehicle_i->id;
  			updates[i].x = vehicle_i->x;
  			updates[i].y = vehicle_i->y;
  			updates[i].theta = vehicle_i->theta;
  			
  		}

  		world_packet->num_vehicles = current_gamers;
  		world_packet->updates = updates;

  		world_pack_len = Packet_serialize(world_buf, &world_packet->header);
  		ret = SENDTO(udp_socket_desc, world_buf, world_pack_len, &udp_server_addr);
 
        usleep(30000);
    }

    Packet_free(&world_packet->header);
    Vehicle_destroy(vehicle_i);
    close(udp_socket_desc);
    return 0;
}

void* notify(void* args_){
	notificationArgs* args = (notificationArgs*) args_;

    int ret;
    char buf[100];
    int size;

    struct sockaddr_in udp_server_addr = {0};
    
    udp_server_addr.sin_addr.s_addr = inet_addr(GROUP_ADDRESS); 
    udp_server_addr.sin_family      = AF_INET;  
    udp_server_addr.sin_port        = htons(3000);

    //create a socket
    int udp_socket_desc = socket(AF_INET, SOCK_DGRAM, 0);
    ERROR_HELPER(udp_socket_desc, "Could not create socket");

    while(args->run){
    	//notify
    	//add/remove gamer
    	sem_wait(&lock);
    	sleep(1);
    	if (action == ADD) size = SERIALIZE_GAMERS(Gamers, buf);
    	
    	else if (action == RMV) size = sprintf(buf, "%c%d", RMV, action_id);
    	
    	ret = SENDTO(udp_socket_desc, buf, size, &udp_server_addr);
    }

    close(udp_socket_desc);
    return 0;
}


void* connection_handler(void *arg) {
	
    handler_args* args = (handler_args*)arg;
    
    int socket_desc = args->socket_desc;
    struct sockaddr_in* client_addr = args->client_addr;

    int ret = 0, msg_len = 0, recv_bytes = 0, pack_len = 0, user_id, i;
    char buf[1024], img_pack_buf[1000000];
    size_t buf_len = sizeof(buf), uid_len = sizeof(user_id),
        img_pack_buf_len = sizeof(img_pack_buf);
    size_t command_len = strlen(MAP_COMMAND);

    //parse client IP address and port
    //for obtain the IP port where communicate
    //port number is an unsigned short
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr->sin_addr), client_ip, INET_ADDRSTRLEN);
    uint16_t client_port = ntohs(client_addr->sin_port); 
    

    //assign user_id using random integer
    user_id = RANDOM_INTEGER(USER_SIZE);

    //welcome message
    msg_len = sprintf(buf, "%s---@gamer%s Ciaooooo, your user id is: %s%d%s", RED, RESET, BLUE, user_id, RESET);
    ret = SEND(socket_desc, buf, msg_len);

    printf("%s---@server%s the gamer's user_id is %s%d%s\n", RED, RESET, BLUE, user_id, RESET);
   
    //create a vehicle
    Vehicle* vehicle = (Vehicle*) malloc(sizeof(Vehicle));
    Vehicle_init(vehicle, args->world, user_id, NULL);

    //add vehicle to the world
    World_addVehicle(args->world, vehicle);

    //wait for commands
    recv_bytes = RECV(socket_desc, buf, buf_len-1);
	
    if (recv_bytes == command_len && !memcmp(buf, MAP_COMMAND, command_len)) 
    printf("%s---@server%s the gamer %s%d%s requested the map\n", RED, RESET, BLUE, user_id, RESET);
    
    //send map packets
    PacketHeader el_header;
    el_header.type = PostTexture;
    ImagePacket* el_pack = malloc(sizeof(ImagePacket));
    el_pack->header = el_header;
    el_pack->image = surface_elevation;
    el_pack->id = 0;
    
    pack_len = Packet_serialize(img_pack_buf, &el_pack->header);
    ret = SEND(socket_desc, img_pack_buf, pack_len);
    //fprintf(stderr,"%s---@server%s map elevation sent. byte = %s%u%s\n", RED, RESET, RED, pack_len, RESET);

    sleep(0.8); 

    PacketHeader text_header;
    text_header.type = PostElevation;
    ImagePacket *text_pack = malloc(sizeof(ImagePacket));
    text_pack->header = text_header;
    text_pack->image = surface_texture;
    text_pack->id = 0;
    
    pack_len = Packet_serialize(img_pack_buf, &text_pack->header);
    ret = SEND(socket_desc, img_pack_buf, pack_len);
    //fprintf(stderr,"%s---@server%s map texture sent. byte = %s%u%s\n", RED, RESET, RED, pack_len, RESET);

    ADD_GAMER(Gamers, user_id);

    //start critical section
    sem_wait(&synchro);
    gamer_counter = NUM_PLAYERS(Gamers);
    
    //unlock notify thread --> ADD
    action = ADD;
    action_id = user_id;
    sem_post(&lock);
    sem_post(&synchro);
    //end critical section

    printf("%s---@server%s there are %s%d%s gamers in the room\n", RED, RESET, RED, gamer_counter, RESET);

    //wait for end
    recv_bytes = RECV(socket_desc, buf, buf_len-1);
    if (recv_bytes == command_len && !memcmp(buf, END_COMMAND, command_len))
	   printf("%s---@server%s the gamer %s%d%s ended the game\n", RED,  RESET, BLUE, user_id, RESET);

	REMOVE_GAMER(Gamers, user_id); 

    ret = close(socket_desc);
    ERROR_HELPER(ret, "Cannot close socket for incoming connection");

    //start critical section
    sem_wait(&synchro);
    
    //fase di detach
    World_detachVehicle(args->world, vehicle);
    
    //unlock notifier
    action_id = user_id;
    action = RMV;
    sem_post(&lock);

    gamer_counter = NUM_PLAYERS(Gamers);
    sem_post(&synchro);
    //end critical section
	
	if(gamer_counter != 1){
    printf("%s---@server%s there are %s%d%s gamers in the room\n", RED, RESET, RED, NUM_PLAYERS(Gamers), RESET);
}
	else{
    printf("%s---@server%s there are %s%d%s gamer in the room\n", RED, RESET, RED, NUM_PLAYERS(Gamers), RESET);
}

    //free buffer and exit
    free(args->client_addr);
        
    free(args);
    Vehicle_destroy(vehicle);
    pthread_exit(NULL);
} 


int main(int argc, char **argv) {

    if (argc<3) {
	   printf("%susage: %s <elevation_image> <texture_image>%s\n", RED, argv[0], RESET);
	   exit(-1);
    }

    char* elevation_filename=argv[1];
    char* texture_filename=argv[2];

    World world; 
    gamer_counter = 0;
    sem_init(&lock, 0, 0);
    sem_init(&synchro, 0, 1);

    //load the images
    printf("%s---@server%s loading elevation image from %s .. ", RED, RESET, elevation_filename);
    surface_elevation = Image_load(elevation_filename);
    if (surface_elevation) {
	   printf("Done! \n"); 
    } else {
	   printf("Fail! \n");
    }

    printf("%s---@server%s loading texture image from %s .. ", RED, RESET, texture_filename);
    surface_texture = Image_load(texture_filename);
    if (surface_texture) {
	   printf("Done! \n");
    } else {
	   printf("Fail! \n");
    }

    //network byte order
    uint16_t port_number = htons(SERVER_PORT);

    int ret, socket_desc, client_desc;

    struct sockaddr_in server_addr = {0};
    struct sockaddr_in* client_addr = calloc(1, sizeof(struct sockaddr_in));
    int sockaddr_len = sizeof(struct sockaddr_in);

    //initialize socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    ERROR_HELPER(socket_desc, "Could not create socket");

    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = port_number;

    //quickly restart server after a crash:
    int reuseaddr_opt = 1;
    ret = setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_opt, sizeof(reuseaddr_opt));
    ERROR_HELPER(ret, "Cannot set SO_REUSEADDR option");

    //bind address to socket (for accept connection
    ret = bind(socket_desc, (struct sockaddr*) &server_addr, sockaddr_len);
    ERROR_HELPER(ret, "Cannot bind address to socket");

    //server start listening
    ret = listen(socket_desc, N_GAMER);
    ERROR_HELPER(ret, "Cannot listen on socket");
    printf("%s---@server%s is now waiting for gamers..\n", RED, RESET);

    //construct the world
    World_init(&world, surface_elevation, surface_texture,  2.5, 2.5, 2.5);

    pthread_t runner_thread;
    updaterArgs runner_args={
        .run=1,
        .world=&world
    };
    
    ret = pthread_create(&runner_thread, NULL, (void*) updater_thread, &runner_args);
    PTHREAD_ERROR_HELPER(ret, "Could not create thread");   

    pthread_t multicast_thread;
    multicastArgs multicast_args={
        .run=1,
        .world=&world
    };
    
    ret = pthread_create(&multicast_thread, NULL, (void*) multi_thread, &multicast_args);
    PTHREAD_ERROR_HELPER(ret, "Could not create thread");

    pthread_t notification_thread;
    notificationArgs notification_args={
        .run=1,
    };
    ret = pthread_create(&notification_thread, NULL, (void*) notify, &notification_args);
    PTHREAD_ERROR_HELPER(ret, "Could not create thread");

    //accept connection
    while(1){
	    client_desc = accept(socket_desc, (struct sockaddr*) client_addr, (socklen_t*) &sockaddr_len);
	    ERROR_HELPER(client_desc, "Cannot accept connection");
	
	    //prepare thread creation
	    pthread_t thread;
	    handler_args* thread_args = malloc(sizeof(handler_args));
	    thread_args->socket_desc = client_desc;
	    thread_args->client_addr = client_addr;
	    thread_args->world = &world;

	    //invoke connection_handler
        ret = pthread_create(&thread, NULL, connection_handler, (void*)thread_args);
        PTHREAD_ERROR_HELPER(ret, "Could not create a new thread! \n");
	    printf("%s---@server%s A new gamer is in the room..\n", RED, RESET);

	    ret = pthread_detach(thread);
        ERROR_HELPER(ret, "Could not detach the thread");       
        
        //need a new buffer for client_addr
        client_addr = calloc(1, sizeof(struct sockaddr_in));
    }

    runner_args.run = 0;
    void* retval; 
    pthread_join(runner_thread, &retval);
    
    multicast_args.run = 0;
    void* retval2;
    pthread_join(multicast_thread, &retval2);

    notification_args.run = 0;
    void* retval3;
    pthread_join(notify, &retval3);

    //free blocks not needed anymore
    World_destroy(&world);
    Image_free(surface_texture);
    Image_free(surface_elevation);

    return 0; 
}
