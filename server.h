#define _GNU_SOURCE

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#define LISTENING_PORT "9090"
#define MAX_PENDING 10
#define MAX_CLIENT 100
#define MAX_LINE 512

int server_fd;
struct sockaddr_in socket_address;
int address_length;
fd_set read_fd;


int client_sockets[MAX_CLIENT];
char buffer[MAX_LINE] = {0};

int send_file(int destination_fd, char* file_name);
int receive_file(int source_fd, char* file_name);
void signal_handler(int sig);
void send_heart_beat(int udp_port);
int initial_tcp_server();
void send_heartbeat(const char* heartbeat_port);
int send_file(int destination_fd, char* file_name);
int receive_file(int source_fd, char* file_name);

static char *itoa_simple_helper(char *dest, int i);
char *itoa_simple(char *dest, int i) ;