#define _GNU_SOURCE

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>

#define MAX_LINE 512
#define MAX_PENDING 10

struct sockaddr_in broadcast_address;
int broadcast_fd ;
int heartbeat_fd;
struct sockaddr_in heartbeat_address;
int heartbeat_address_length;
int broadcast_address_length;



char path[100];
int upload_file(int server_fd, char* command);
int download_file(int server_fd, char* command);
void signal_handler(int sig);
void initial_broadcast(int client_broadcast_port);
void initial_heartbeat(int heartbeat_port);
void send_broadcast(int udp_port, char* message);
int send_file(int destination_fd, char* file_name);















int download_file(int server_fd, char* file_name);
int upload_file(int server_fd, char* file_name);
void send_broadcast(int udp_port, char* message);
void receive_broadcast(int broadcast_fd, const char* my_port);
void signal_handler(int sig);

int send_file(int destination_fd, char* file_name);
int receive_file(int source_fd, char* file_name);