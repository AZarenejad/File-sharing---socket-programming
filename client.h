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
int possible_to_send_broadcast = 0;
int client_connection;
struct sockaddr_in client_address;
int client_address_length;

struct sockaddr_in socket_address, server_address;

int server_address_length;

int server_connection;
int address_length ;
fd_set heartbeat_fd_set;


char path[100];
char heartbeat_message[128];


int upload_file(int server_fd, char* command);
int download_file(int server_fd, char* command);
void signal_handler(int sig);
void initial_broadcast_socket(const char* broadcast_port);
void initial_heartbeat_socket(const char* heartbeat_port);
void send_broadcast(int udp_port, char* message);
void connect_to_server();
void receive_broadcast(int broadcast_fd, const char* my_port);
void initial_client_socket_for_peer_connection(const char* client_port);
int send_file(int destination_fd, char* file_name);
int receive_file(int source_fd, char* file_name);
static char *itoa_simple_helper(char *dest, int i);

char *itoa_simple(char *dest, int i);

char *itoa_simple(char *dest, int i);








