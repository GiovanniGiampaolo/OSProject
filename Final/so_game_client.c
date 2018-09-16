#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#else
#include <GL/gl.h>
#include <GL/glut.h>
#endif

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>  
#include <netinet/in.h> 
#include <sys/socket.h>
#include <errno.h>
#include <semaphore.h>

#include "image.h"
#include "surface.h"
#include "world.h"
#include "vehicle.h"
#include "world_viewer.h"
#include "functions.h"
#include "gamer.h"
#include "help.h"
#include "so_game_protocol.h"

Gamer* Gamers[N_GAMER];
Image* my_texture_for_server;
int gamer_counter = 0;
int user_id;

typedef struct {
    volatile int run;
    Vehicle* vehicle;
    World* world;
} updaterArgs;

typedef struct {
    volatile int run;
    World* world;
} listenArgs;

typedef struct {
    volatile int run;
    World* world;
} notificationArgs; 


void* updater_thread(void* args_){
  updaterArgs* args=(updaterArgs*)args_;
  
  char vehicle_buf[100];
  VehicleUpdatePacket* vehicle_pack = (VehicleUpdatePacket*)malloc(sizeof(VehicleUpdatePacket));
  PacketHeader v_head;
  int vehicle_pack_len, ret;

  struct sockaddr_in udp_server_addr = {0};
    
  udp_server_addr.sin_addr.s_addr = INADDR_ANY;
  udp_server_addr.sin_family      = AF_INET;
  udp_server_addr.sin_port        = htons(UDP_SERVER_PORT);

  //create a socket
  int udp_socket_desc = socket(AF_INET, SOCK_DGRAM, 0);
  ERROR_HELPER(udp_socket_desc, "Could not create socket");

  v_head.type = VehicleUpdate;
  
  while(args->run){
    
    //vehicle parameters
    vehicle_pack = calloc(1,sizeof(VehicleUpdatePacket));
    vehicle_pack->header = v_head;
    vehicle_pack->id = args->vehicle->id;
    vehicle_pack->rotational_force = args->vehicle->rotational_force_update;
    vehicle_pack->translational_force = args->vehicle->translational_force_update;
    
    /*printf("%sid    = \t%d%s\n", CYAN, vehicle_pack->id, RESET);
    printf("%strans = \t%f%s\n", CYAN, vehicle_pack->translational_force, RESET);
    printf("%srot   = \t%f%s\n", CYAN, vehicle_pack->rotational_force, RESET);*/
    //fprintf(stderr,"%supdater   =  %d%s\n", CYAN, vehicle_pack_len, RESET);
    
    //send UDP packet
    vehicle_pack_len = Packet_serialize(vehicle_buf, &vehicle_pack->header);
    ret = SENDTO(udp_socket_desc, vehicle_buf, vehicle_pack_len, &udp_server_addr);
    
    usleep(30000); 
  }

  Packet_free(&vehicle_pack->header);
  close(udp_socket_desc); 
  return 0;
}


void* world_listener(void* args_){
    listenArgs* args = (listenArgs*) args_;
    
    char world_buf[1000];
    int world_pack_len, ret, i, *left;
    WorldUpdatePacket* world_packet;
    ClientUpdate* updates;
    Vehicle* vehicle_i;

    struct sockaddr_in mc_udp_addr = {0};
    int sockaddr_len = sizeof(struct sockaddr_in);

    mc_udp_addr.sin_addr.s_addr = INADDR_ANY;
    mc_udp_addr.sin_family      = AF_INET;
    mc_udp_addr.sin_port        = htons(4000);

    //create a socket 
    int udp_socket_desc = socket(AF_INET, SOCK_DGRAM, 0);
    ERROR_HELPER(udp_socket_desc, "Could not create socket");

    //quickly restart our server after a crash 
    int flag = 1;
    ret = setsockopt(udp_socket_desc, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    ERROR_HELPER(ret, "Cannot set SO_REUSEADDR option");

    ret = bind(udp_socket_desc, (struct sockaddr*) &mc_udp_addr, sockaddr_len);
    ERROR_HELPER(ret, "cannot bind socket");

    struct ip_mreq mrec;
    mrec.imr_multiaddr.s_addr = inet_addr(GROUP_ADDRESS);
    mrec.imr_interface.s_addr = htonl(INADDR_ANY);
    ret = setsockopt(udp_socket_desc, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mrec, sizeof(mrec));
    ERROR_HELPER(ret, "cannot add membership");

    while(args->run){

        ret = RECVFROM(udp_socket_desc, world_buf, sizeof(world_buf)-1);
        world_pack_len = strlen(world_buf);
        world_packet = (WorldUpdatePacket* )Packet_deserialize(world_buf, world_pack_len);
        updates = (ClientUpdate*) calloc(world_packet->num_vehicles, sizeof(ClientUpdate));
        gamer_counter = NUM_PLAYERS(Gamers);
        
        for (i = 0; i < world_packet->num_vehicles; i++){
            
            vehicle_i = (Vehicle*) calloc(1,sizeof(Vehicle));
            updates = (ClientUpdate*) world_packet->updates;
            ret = FIND_GAMER(Gamers, updates[i].id);
            
            if (updates[i].id == user_id || ret == -1) continue;

            /*printf("%sid      = \t%d%s\n", YELLOW,  updates[i].id, RESET);
            printf("%sx y     = \t%f %f%s\n", YELLOW,  updates[i].x, updates[i].y, RESET);
            printf("%stheta   = \t%f%s\n", YELLOW,  updates[i].theta, RESET);*/
               
            vehicle_i = World_getVehicle(args->world, updates[i].id);
            vehicle_i->x = updates[i].x;
            vehicle_i->y = updates[i].y;
            vehicle_i->theta = updates[i].theta;     
        }

        World_update(args->world);
        usleep(30000);
    }

    close(udp_socket_desc); 
    return 0;
}


void* notification_listener(void* args_){
    notificationArgs* args = (notificationArgs*) args_;

    int ret, i, id, *ids;
    char buf[100], dest[3], action;
    Vehicle* vehicle_i;

    struct sockaddr_in udp_server_addr = {0};
    int sockaddr_len = sizeof(struct sockaddr_in);

    udp_server_addr.sin_addr.s_addr = INADDR_ANY;
    udp_server_addr.sin_family      = AF_INET;
    udp_server_addr.sin_port        = htons(3000);

    //create a socket
    int udp_socket_desc = socket(AF_INET, SOCK_DGRAM, 0);
    ERROR_HELPER(udp_socket_desc, "Could not create socket");

    //quickly restart our server after a crash 
    int flag = 1;
    ret = setsockopt(udp_socket_desc, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    ERROR_HELPER(ret, "Cannot set SO_REUSEADDR option");

    ret = bind(udp_socket_desc, (struct sockaddr*) &udp_server_addr, sockaddr_len);
    ERROR_HELPER(ret, "cannot bind socket");

    struct ip_mreq mrec;
    mrec.imr_multiaddr.s_addr = inet_addr(GROUP_ADDRESS);
    mrec.imr_interface.s_addr = htonl(INADDR_ANY);
    ret = setsockopt(udp_socket_desc, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mrec, sizeof(mrec));
    ERROR_HELPER(ret, "cannot add membership");

    while(args->run){

        //add/remove player to the poll
        memset(buf, 0, sizeof(buf));
        vehicle_i = calloc(1, sizeof(Vehicle));
        ret = RECVFROM(udp_socket_desc, buf, sizeof(buf)-1);
        //fprintf(stderr,"%snotification received %s%s\n", BLUE, buf, RESET);
        action = buf[0];

        if(action == RMV){

            id = atoi(buf+1);
            vehicle_i = World_getVehicle(args->world, id);
            World_detachVehicle(args->world, vehicle_i);
            REMOVE_GAMER(Gamers, id);
            printf("%s---@%d%s player %s%d%s left the game!\n", GREEN, user_id, RESET, 
                            BLUE, id, RESET);
        }

        else {

            for (i = 0; i < strlen(buf); i+=3){
                memcpy(dest, buf+i,USER_SIZE);
                id = atoi(dest);
                ret = FIND_GAMER(Gamers, id);
                
                if (ret == -1 && id != user_id){
                    Vehicle_init(vehicle_i, args->world, id, my_texture_for_server);
                    World_addVehicle(args->world, vehicle_i);
                    ADD_GAMER(Gamers, id);
                    printf("%s---@%d%s player %s%d%s joined the game!\n", GREEN, user_id, RESET, 
                            BLUE, id, RESET);
                    vehicle_i = calloc(1,sizeof(Vehicle));
                }
            }
        } 
    }

    close(udp_socket_desc); 
    return 0;
}

int main(int argc, char **argv) {

    if (argc<2){
        printf("%susage: %s <player texture> <server address> (optional)%s\n", GREEN, argv[0], RESET);
        exit(-1);
    }

    uint16_t* ip_addr;
    if (argc > 3){
        ip_addr = inet_addr(argv[2]);
    }

    ip_addr = inet_addr(SERVER_ADDRESS);
    //port_number = atoi(argv[2]); 
    World world;

    int ret = 0, msg_len = 0, recv_bytes = 0, pack_len = 0;
    char buf[1024], img_pack_buf[1000000];
    size_t buf_len = sizeof(buf), img_pack_buf_len = sizeof(img_pack_buf); 
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
    printf("%s---@gamer%s connected to server!..\n", GREEN, RESET);

    //display welcome message
    recv_bytes = RECV(socket_desc, buf, buf_len-1);
    printf("%s\n", buf);

    user_id = atoi(buf+recv_bytes-strlen(RESET)-USER_SIZE);
    //fprintf(stderr,"%s%d%s\n", GREEN, user_id, RESET);
    
    printf("%s---@%d%s loading texture image from %s .. ", GREEN, user_id, RESET, argv[1]);
    my_texture_for_server = Image_load(argv[1]);
    if (my_texture_for_server) {
        printf("Done!\n");
    } else {
        printf("Fail!\n"); 
    } 
    
    //start the game asking map
    msg_len = sprintf(buf, MAP_COMMAND);
    ret = SEND(socket_desc, buf, msg_len);  
    
    //receive elevation from server
    pack_len = RECV(socket_desc, img_pack_buf, img_pack_buf_len-1);    
    fprintf(stderr,"%s---@%d%s map elevation received. byte = %s%u%s\n",
         GREEN, user_id, RESET, RED, pack_len, RESET);  
    ImagePacket* el_packet = (ImagePacket*) Packet_deserialize(img_pack_buf, pack_len);    
    memset(img_pack_buf, 0, pack_len);


    //receive texture from server
    pack_len = RECV(socket_desc, img_pack_buf, img_pack_buf_len-1);
    fprintf(stderr,"%s---@%d%s map texture received. byte = %s%u%s\n",
         GREEN, user_id, RESET, RED, pack_len, RESET); 
    ImagePacket* text_packet = (ImagePacket*) Packet_deserialize(img_pack_buf, pack_len);
    memset(img_pack_buf, 0, pack_len);
    
    //comment (provvisory for non-crash)
    Image* el_packet_image = Image_load("./images/maze.pgm");
    Image* text_packet_image = Image_load("./images/maze.ppm");

    // construct the world
    printf("%s---@%d%s launching the world\n", GREEN, user_id, RESET);
    World_init(&world, el_packet_image, text_packet_image, 0.5, 0.5, 0.5);

    //add this vehicle
    Vehicle* vehicle = (Vehicle*) malloc(sizeof(Vehicle));
    Vehicle_init(vehicle, &world, user_id, my_texture_for_server);
    World_addVehicle(&world, vehicle);

    printf("%s---@%d%s press %sESC%s for end the game\n", GREEN, user_id, RESET, BLUE, RESET);
	
    pthread_t runner_thread;
    updaterArgs runner_args={
       	 .run=1,
         .vehicle=vehicle,
       	 .world=&world
    };
    ret = pthread_create(&runner_thread, NULL, updater_thread, &runner_args); 
    PTHREAD_ERROR_HELPER(ret, "could not create thread");

    pthread_t listen_thread;
    listenArgs listen_args={
        .run=1,
        .world=&world
    };
    ret = pthread_create(&listen_thread, NULL, world_listener, &listen_args);
    PTHREAD_ERROR_HELPER(ret, "could not create thread");

    pthread_t notification_thread;
    notificationArgs notification_args={
        .run=1,
        .world=&world
    };
    ret = pthread_create(&notification_thread, NULL, notification_listener, &notification_args);
    PTHREAD_ERROR_HELPER(ret, "could not create thread");

    WorldViewer_runGlobal(&world, vehicle, &argc, argv);
    
    runner_args.run=0;
    void* retval;
    pthread_join(runner_thread, &retval);

    listen_args.run = 0;
    void* retval2;
    pthread_join(listen_thread, &retval2);

    notification_args.run = 0;
    void* retval3;
    pthread_join(notification_thread, &retval3);

    printf("%s---@%s%s exiting..\n", GREEN, user_id, RESET);
    
    //end message
    msg_len = sprintf(buf, END_COMMAND);
    ret = SEND(socket_desc, buf, msg_len);
 
    //free up space
    Packet_free(&el_packet->header);
    Packet_free(&text_packet->header);
    Vehicle_destroy(vehicle);
    
    ret = close(socket_desc);
    ERROR_HELPER(ret, "Cannot close socket");

    return 0;             
}
