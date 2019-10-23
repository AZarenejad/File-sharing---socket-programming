#include "server.h"

int beat = 0;

static char *itoa_simple_helper(char *dest, int i) {
  if (i <= -10) {
    dest = itoa_simple_helper(dest, i/10);
  }
  *dest++ = '0' - i%10;
  return dest;
}
char *itoa_simple(char *dest, int i) {
  char *s = dest;
  if (i < 0) {
    *s++ = '-';
  } else {
    i = -i;
  }
  *itoa_simple_helper(s, i) = '\0';
  return dest;
}

int initial_tcp_server(){
    int tcp_port = atoi(LISTENING_PORT);
	server_fd = socket(PF_INET, SOCK_STREAM, 0);
	socket_address.sin_family = PF_INET;
	socket_address.sin_addr.s_addr = INADDR_ANY;
	socket_address.sin_port = htons(tcp_port);
	address_length = sizeof(socket_address);
	if (bind(server_fd, (struct sockaddr*)&socket_address, address_length) < 0){
		perror("bind 2\n");
		return 1;
	}
	if (listen(server_fd, 100) < 0){
		perror("listen\n");
		return 1;
	}
    char* msg="server now listenning on port 9090\n";
    write(1,msg,strlen(msg));
    for (int i=0;i<MAX_CLIENT;i++){
        client_sockets[i]=0;
    }
    client_sockets[0] = server_fd;
}
void signal_handler(int sig){
	beat = 1;
}

void send_heartbeat(const char* heartbeat_port){
	int server_socket;
	if ((server_socket = socket(PF_INET, SOCK_DGRAM, 0)) < 0 ){ 
		perror("Socket creation failed!"); 
		exit(EXIT_FAILURE);
	}

	int broadcast = 1;
	setsockopt(server_socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

	struct sockaddr_in broadcast_address;

	broadcast_address.sin_family = PF_INET;
	broadcast_address.sin_addr.s_addr = htonl(INADDR_BROADCAST); 
	broadcast_address.sin_port = htons(atoi(heartbeat_port));
	char* msg = "server now heartbeating!\n";
	write(1, msg, strlen(msg));
	sendto(server_socket, LISTENING_PORT, strlen(LISTENING_PORT), MSG_DONTWAIT, (const struct sockaddr *)&broadcast_address,
			sizeof broadcast_address);
	close(server_socket);
}

int send_file(int destination_fd, char* file_name){
	char buffer[MAX_LINE];
	int file_fd;
    char dest[100]="Server/";
    strcat(dest,file_name);

	if ((file_fd = open(dest, O_RDONLY)) < 0){
		char* error_message = "open error!\n";
		write(1, error_message, strlen(error_message));
		return 0;
	}

	int read_length;
	while((read_length = read(file_fd, buffer, sizeof(buffer))) > 0){
		if (send(destination_fd, buffer, read_length, 0) != read_length){
			char* error_message = "Write error!\n";
			write(1, error_message, strlen(error_message));
			return 0;
		}
	}

	if(read_length < 0){
    	char* error_message = "Read error!\n";
		write(1, error_message, strlen(error_message));
		return 0;
	}

	char* message = "File is sent successfully!\n";
	write(1, message, strlen(message));
	return 1;
}

int receive_file(int source_fd, char* file_name){
	char buffer[128];
	int file_fd;
    char dest[100]="Server/";
    strcat(dest,file_name);
	if ((file_fd = open(dest, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU)) < 0){
		char* error_message = "Create error!\n";
		write(2, error_message, strlen(error_message));
		return 0;
	}

	int read_length;
	fd_set read_fd;
	struct timeval timeout = {0, 100};
	FD_ZERO(&read_fd);
	FD_SET(source_fd, &read_fd);
	select(source_fd + 1, &read_fd, NULL, NULL, &timeout);

	while(FD_ISSET(source_fd, &read_fd) && (read_length = read(source_fd, buffer, sizeof(buffer))) > 0){
		
		struct timeval timeout = {0, 100};
		FD_ZERO(&read_fd);
		FD_SET(source_fd, &read_fd);
		select(source_fd + 1, &read_fd, NULL, NULL, &timeout);

		if (write(file_fd, buffer, read_length) != read_length){
			char* error_message = "Write error!\n";
			write(2, error_message, strlen(error_message));
			return 0;
		}
	}

	if(read_length < 0){
    	char* error_message = "Read error!\n";
		write(2, error_message, strlen(error_message));
		return 0;
	}

	char* message = "New file is received suuccessfully!\n";
	write(2, message, strlen(message));
	return 1;
}


int main(int argc, char const *argv[]){
    char* direction="Server";
    mkdir(direction,0700);
	if (argc != 2){
		char* error_message = "Invalid Input. heartbeat port only needed!\n";
		write(1, error_message, strlen(error_message));
		return 1;
	}
	int udp_fd, udp_port;
	udp_port = atoi(argv[1]);

	// TCP Server
	initial_tcp_server();
	signal(SIGALRM, signal_handler);
	alarm(1);

	// Forever
	while (1){
		struct timeval timeout = {0, 0};
		int new_socket = 0;
		int last_node = server_fd;
		FD_ZERO(&read_fd);
		for (int i = 0; i < MAX_CLIENT; i++){
			if(client_sockets[i] > 0)
				FD_SET(client_sockets[i], &read_fd);

			if (client_sockets[i] > last_node)
				last_node = client_sockets[i];
		}
		if (beat){
			send_heartbeat(argv[1]);
			beat = 0;
			alarm(1);
		}
		int activity = select(last_node + 1, &read_fd, NULL, NULL, &timeout);

		if (activity < 0 && errno == EINTR)
			continue;

		if (FD_ISSET(server_fd, &read_fd)){
			if ((new_socket = accept(server_fd, (struct sockaddr*)&socket_address, (socklen_t*)&address_length)) < 0)
				perror("Accept Failed!\n");
			else{
				for (int i = 1; i < MAX_CLIENT; i++){
                    if (client_sockets[i] == 0){
						client_sockets[i] = new_socket;
                        write(1, "Adding to list of sockets as ", 30);
                		char msg[250]; 
                		itoa_simple(msg, i);
                		write(1, msg, strlen(msg));
                		write(1, "\n" , 1);
						break;
					}
				}
			}
		}
		else{
			for (int i = 1; i < MAX_CLIENT; i++){
				int curent_client = client_sockets[i];
				if (FD_ISSET(curent_client, &read_fd)){
					int read_size = 0;
					if ((read_size = read(curent_client, buffer, MAX_LINE)) > 0){
						buffer[read_size] = '\0';
						if (memmem(buffer, read_size, "download", 8) == buffer){
                            char* msg = "sending file to client...\n";
                            write(1,msg,strlen(msg));
							char* file_name = buffer + 9;
							int file_fd;
                            char dest[100]="Server/";
                            strcat(dest,file_name);
							if ((file_fd = open(dest, O_RDONLY)) > 0){
								close(file_fd);
								send(curent_client, "1", 1, 0);
								send_file(curent_client, file_name);
							}
							else
								send(curent_client, "0", 1, 0);
						}
						else if (memmem(buffer, MAX_LINE, "upload", 6) == buffer){
                            char* msg="recieving file from client...\n";
                            write(1,msg,strlen(msg));
							send(curent_client, "1", 1, 0);
							receive_file(curent_client, buffer + 7);
						}
						else{
							printf("%s\n", buffer);
							send(curent_client, "0", 1, 0);
						}
					}
					else{
						close(client_sockets[i]);
						client_sockets[i] = 0;
						char* message = "Socket disconnected\n";
						write(1, message, strlen(message));
					}
				}
			}
		}
	}
}






// void initialize_heartbeat_socket(char* heartbeat_port){
//     if( (heartbeat_socket_fd = socket(PF_INET , SOCK_DGRAM, IPPROTO_UDP)) < 0 ){
//         char* error = "fail to open socket\n";
//         write(1, error, strlen(error));
//         return ;
//     }
//     hearbeat_addr.sin_family = PF_INET;
//     hearbeat_addr.sin_port = htons(atoi(heartbeat_port));
//     hearbeat_addr.sin_addr.s_addr = INADDR_ANY;

//     int broadcast = 1;
// 	setsockopt(heartbeat_socket_fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
//     // if( bind(heartbeat_socket_fd, (struct sockaddr *) &hearbeat_addr , sizeof(hearbeat_addr)) < 0) {
//     //     char* error = "fail to bind\n";
//     //     write(1, error, strlen(error));
//     //     return ;
//     // }

// }

// void initialize_server_socket(){
//     for(int i = 0 ; i < MAX_CLIENTS ; i++){
//         client_sockets[i] = 0;
//     }

//     if( (server_socket_fd = socket( AF_INET , SOCK_STREAM, 0)) == 0 ){
//         char* error_make_socket = "fail to open socket\n";
//         write(1, error_make_socket, strlen(error_make_socket));
//         return ;
//     }

//     server_addr.sin_family = PF_INET;
//     server_addr.sin_port = htons(LISTEN_PORT);
//     server_addr.sin_addr.s_addr = inet_addr(LOCAL_IP_ADDRESS);

//    if (bind(server_socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr))<0){  
//        char* error ="bind error\n"; 
//         write(1,error,strlen(error));   
//         exit(EXIT_FAILURE);   
//     }   
//     write(1,"listener on port LISTEN_PORT: 9090\n",36);
//     if (listen(server_socket_fd, MAX_PENDING) < 0)   {
//         char* error = "listen error\n";
//         write(1, error, strlen(error));   
//         exit(EXIT_FAILURE);   
//     }
//     write(1,"Waiting for connection ...\n",28);
// }

// void sending_heartbeat(){   
//     const char* msg = HEARTBEAT_MSG;
//     if( sendto(heartbeat_socket_fd, msg , strlen(msg), 0, (struct sockaddr*)&hearbeat_addr, sizeof(hearbeat_addr)) == -1){
//         char* error = "sendto error\n";
//         write(1, error, strlen(error));
//         exit(EXIT_FAILURE);
//     }
//     write(1,"Server is now heartbeating.\n", 29);
//     signal(SIGALRM, sending_heartbeat);
//     alarm(1);
// }

// void handle_incoming_connections(){
//     int newSocket;
//     int addrlen = sizeof(server_addr);
//     if (FD_ISSET(server_socket_fd, &readfds)){   
//         if ((newSocket = accept(server_socket_fd,(struct sockaddr *)& server_addr, (socklen_t*)&addrlen))<0) {   
//             write(1, "accept\n", 8);   
//             exit(EXIT_FAILURE);   
//         }   
//         char* addr = inet_ntoa(server_addr.sin_addr);
//         char port[256];
//         itoa_simple(port, ntohs(server_addr.sin_port));
//         write(1, "New Connection, address: ", 26);
//         write(1, addr, strlen(addr));
//         write(1, " port: ", 8);   
//         write(1, port, strlen(port));
//         write(1, "\n", 1);   
                
//         char* message = "You connected to Server...\n";
 
//         if( send(newSocket, message, strlen(message), 0) != strlen(message) ) {   
//             write(1, "inform client send failed", 26);   
//         }       
//         write(1, "Connected message sent to client. \n", 36);

//         for (int i = 0; i < MAX_CLIENTS; i++){    
//             if( client_sockets[i] == 0 ){   
//                 client_sockets[i] = newSocket;   
//                 write(1, "Adding to list of sockets as ", 30);
//                 char msg[250]; 
//                 itoa_simple(msg, i);
//                 write(1, msg, strlen(msg));
//                 write(1, "\n" , 1);   
//                 break;   
//             }   
//         } 
//     } 
//     else {
//         int addrlen = sizeof(server_addr);
//         for (int i = 0; i < MAX_CLIENTS; i++){
//             int curent_client = client_sockets[i];
//             if ( FD_ISSET(curent_client, &readfds)){
//                 int read_size = 0;
//                 if ((read_size = read(curent_client, buffer, 1024)) > 0){
//                     buffer[read_size] = '\0';

//                     if (memmem(buffer, read_size, "download", 8) == buffer){
//                         char* msg = "\nserver is sending file to client...\n";
//                         write(1,msg,strlen(msg));
//                         int num_file = -1;
//                         for (int j = 0; j < files_num; j++){
//                             if (!strcmp(files[j], buffer + 9)){
//                                 num_file = j;
// 							    break;
// 							}
//                         }
//                         if (num_file == -1 ){
//                             send(curent_client,"0",1,0);
//                         }
//                         else{
//                             send(curent_client,"1",1,0);
//                             send_file(curent_client , files[num_file]);
//                         }
//                     }


//                    	else if (memmem(buffer, 1024, "upload", 6) == buffer){
//                         char* msg = "server is recieving file from client...\n";
//                         write(1,msg,strlen(msg));
//                         int duplicate = 0;
//                         for (int j = 0; j < files_num; j++){
//                             if (!strcmp(files[j], buffer + 7)){
//                                 send(curent_client, "0", 1, 0);
// 								duplicate = 1;
// 								break;
// 							}
// 						}
// 						if (duplicate)
// 							break;
                        
// 						files[files_num] = (char*)malloc((read_size - 7 + 1) * sizeof(char));
// 						strcpy(files[files_num], buffer + 7);
// 						send(curent_client, "1", 1, 0);
// 						if (FD_ISSET(curent_client, &readfds))
// 							receive_file(curent_client, files[files_num++]);
// 						else{
// 							char* error_message = "Upload receive error!\n";
// 							write(2, error_message, strlen(error_message));
// 							break;
// 						}
// 					}
// 					else
// 						send(curent_client, "-1", 1, 0);
// 				}
// 				else{
// 					close(client_sockets[i]);
// 					client_sockets[i] = 0;
// 					char* message = "Socket disconnected.\n";
// 					write(1, message, strlen(message));
// 				}
//             }
//         }
//     }
// }




