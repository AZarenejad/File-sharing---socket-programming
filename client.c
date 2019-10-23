#include "client.h"


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

void send_broadcast(int udp_port, char* message){
	int broadcast_socket;
	if ((broadcast_socket = socket(PF_INET, SOCK_DGRAM, 0)) < 0 ){
		char* error = "Broadcast socket creation failed!\n";
		write(2,error,strlen(error));
		exit(EXIT_FAILURE);
	}
	

	int broadcast = 1;
	setsockopt(broadcast_socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

	struct sockaddr_in broadcast_address;
	broadcast_address.sin_family = PF_INET;
	broadcast_address.sin_addr.s_addr = htonl(INADDR_BROADCAST); 
	broadcast_address.sin_port = htons(udp_port);

	sendto(broadcast_socket, message, strlen(message), MSG_DONTWAIT, (const struct sockaddr *)&broadcast_address,
			sizeof broadcast_address);
	close(broadcast_socket);
}

int upload_file(int server_fd, char* command){
	int file_fd;
	char* file_name = command + 7;
	char dest[100]={0};
	strcat(dest,path);
	strcat(dest,"/");
	strcat(dest,file_name);
	// printf("%s\n",dest);
	if ((file_fd = open(dest, O_RDONLY)) < 0){
		char* msg = "File does not exist!\n";
		write(2,msg,strlen(msg));
		return 0;
	}

	char buffer[MAX_LINE];
	send(server_fd, command, strlen(command), 0);
	int message_length;

	if((message_length = read(server_fd, buffer, sizeof(buffer))) <= 0){
		char* msg = "no answer from server!\n";
		write(2,msg,strlen(msg));
		return -1;
	}
	else if (message_length == 1 && buffer[0] == '0'){
		char* msg = "server didnt accept the file!\n";
		write(2,msg,strlen(msg));
		return 0;
	}

	int read_length;
	char* msg = "Uploading...\n";
	write(2,msg,strlen(msg));


	while((read_length = read(file_fd, buffer, sizeof(buffer))) > 0){
		if (send(server_fd, buffer, read_length, 0) != read_length){
			char* msg = "Sending intrupted!\n";
			write(2,msg,strlen(msg));
			return -1;
		}
	}

	if(read_length < 0){
		char* msg = "could not read whole file!\n";
		write(2,msg,strlen(msg));
    	return -1;
	}

	msg = "File is uploaded to server suessfully!\n";
	write(2,msg,strlen(msg));
	return 1;
}
int download_file(int server_fd, char* command){
	char buffer[MAX_LINE] = {0};
	send(server_fd, command, strlen(command), 0);
	int message_length;

	if((message_length = read(server_fd, buffer, sizeof(buffer))) <= 0){
		char * error = "no answer from sender!\n";
		write(2,error,strlen(error));
		return 0;
	}
	else if (message_length == 1 && buffer[0] == '0'){
		char * error = "server dont have file\n";
		write(2,error,strlen(error));
		return 0;
	}

	int file_fd;
	char* file_name = command + 9;
	char dest[100]={0};
	strcat(dest,path);
	strcat(dest,"/");
	strcat(dest,file_name);
	if ((file_fd = open(dest, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU)) < 0){
		char * error = "create new file error!\n";
		write(2,error,strlen(error));
		return -1;
	}

	int read_length = 0;
	fd_set read_fd;
	struct timeval timeout = {1, 0};
	setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof timeout);

	char * msg = "Downloading...\n";
	write(2,msg,strlen(msg));;

	while((read_length = read(server_fd, buffer, sizeof(buffer))) > 0){
		if (write(file_fd, buffer, read_length) != read_length){
			char * error = "could not write to file!\n";
			write(2,error,strlen(error));
			return -1;
		}

		if (read_length < MAX_LINE)
			break;
	}

	if(read_length < 0){
		char * error = "file receiving error!\n";
		write(2,error,strlen(error));
		return 0;
	}

	msg = "File is downloaded successfully!\n";
	write(2,msg,strlen(msg));
	return 1;
}
void initial_broadcast_socket(const char* broadcast_port){
	broadcast_fd = socket(PF_INET, SOCK_DGRAM, 0);
	memset(&broadcast_address, 0, sizeof(broadcast_address)); 
	broadcast_address.sin_family = PF_INET;
	broadcast_address.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	broadcast_address.sin_port = htons(atoi(broadcast_port));
	broadcast_address_length = sizeof(broadcast_address);
	int broadcast = 1;

	setsockopt(broadcast_fd, SOL_SOCKET, SO_REUSEADDR, &broadcast, sizeof(broadcast));

	if (bind(broadcast_fd, (struct sockaddr*)&broadcast_address, broadcast_address_length) < 0){
		char* error = "could not bind to broadcast port!\n";
		write(2,error,strlen(error));
		exit(EXIT_FAILURE);
	}
}

void initial_heartbeat_socket(const char* heartbeat_port){
	heartbeat_fd = socket(PF_INET, SOCK_DGRAM, 0);
	memset(&heartbeat_address, 0, sizeof(heartbeat_address)); 
	heartbeat_address.sin_family = PF_INET;
	heartbeat_address.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	heartbeat_address.sin_port = htons(atoi(heartbeat_port));
	heartbeat_address_length = sizeof(heartbeat_address);
	int broadcast = 1;

	setsockopt(heartbeat_fd, SOL_SOCKET, SO_REUSEADDR, &broadcast, sizeof(broadcast));

	if (bind(heartbeat_fd, (struct sockaddr*)&heartbeat_address, heartbeat_address_length) < 0){
		char* error = "could not bind to heartbeat port!\n";
		write(2,error,strlen(error));
		exit(EXIT_FAILURE);
	}
	char* msg = "waiting for server...\n";
	write(2,msg,strlen(msg));
}

void signal_handler(int sig){
	possible_to_send_broadcast = 1;
}

void initial_client_socket_for_peer_connection(const char* client_port){
	client_connection = socket(PF_INET, SOCK_STREAM, 0);
	client_address.sin_family = PF_INET;
	client_address.sin_addr.s_addr = INADDR_ANY;
	client_address.sin_port = htons(atoi(client_port));
	client_address_length = sizeof(client_address);
	int reuse = 1;
	setsockopt(client_connection, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));

	if (bind(client_connection, (struct sockaddr*)&client_address, client_address_length) < 0){
		char* error = "could not bind to tcp peer to peer port!\n";
		write(2,error,strlen(error));
		exit(EXIT_FAILURE);
	}
		
	if (listen(client_connection, MAX_PENDING) < 0){
		char* error = "could not listen!\n";
		write(2,error,strlen(error));
		exit(EXIT_FAILURE);
	}
	write(2,"now client is listening on port ",33);
	write(2,client_port,strlen(client_port));
	write(2,"\n",1);
}

// int is_server_up(char* port_x){
//     char recv_string[MAXRECVSTRING+1];
//     int recv_string_len;
//     struct timeval tout;

//     if( (heartbeat_fd = socket( PF_INET , SOCK_DGRAM, IPPROTO_UDP)) == -1 ){
//         char* error = "fail to open socket\n";
//         write(2, error, strlen(error));
//         return 0 ;
//     }

//     int broadcast = 1;
//     if (setsockopt(heartbeat_fd, SOL_SOCKET, SO_REUSEADDR, &broadcast,sizeof (broadcast)) == -1) {
//       char* error = "fail to set broadcast\n";
//         write(2, error, strlen(error));
//         return 0 ;
//     }

//     heartbeat_addr.sin_family = PF_INET;
//     heartbeat_addr.sin_port = htons(atoi(port_x));
//     heartbeat_addr.sin_addr.s_addr = INADDR_BROADCAST;

// 	  if(inet_pton(AF_INET, IP, & heartbeat_addr.sin_addr) <=0 ) { 
//       char* error = "Invalid address/ Address not supported \n";
// 		  write(2, error, strlen(error)); 
// 	  }

//     if( bind(heartbeat_fd, (struct sockaddr *) &heartbeat_addr, sizeof(heartbeat_addr)) == -1) {
//         close(heartbeat_fd);
//         char* error = "fail to bind to get hearbeat\n";
//         write(2, error, strlen(error));
//         return 0 ;
//     }
     
//     tout.tv_sec = 2;
//     tout.tv_usec = 0;

//     if (setsockopt(heartbeat_fd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
//         write(2,"setsockopt(SO_REUSEADDR) failed\n",35);

//     if (setsockopt(heartbeat_fd, SOL_SOCKET, SO_RCVTIMEO, &tout, sizeof(tout)) < 0){
//         close(heartbeat_fd);

//         return 0;
//     }

//     write(2,"client: waiting to recive hearbeat...\n",39);
//     if( ( recv_string_len = recvfrom(heartbeat_fd, recv_string, MAXRECVSTRING, 0, NULL , 0)) < 0){
//         close(heartbeat_fd);
//         write(2, "Server is not  alive!\n", 23);
//         return 0;
//     }
//     write(1, "Server is alive!\n", 18);
//     recv_string[recv_string_len] = '\0';
//     // write(1, "server port: ",14);
//     // write(1, recv_string, strlen(recv_string));
//     // write(1, "\n", 1);
//     fill_server_information(recv_string);
//     close(heartbeat_fd);
//     return 1;
// }
void connect_to_server(){
	int server_port = atoi(heartbeat_message);
	// printf("%d\n",server_port);
	server_address.sin_family = PF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(server_port);
	server_address_length = sizeof server_address;
	int reuse = 1;
	setsockopt(server_connection, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
	if (bind(server_connection, (struct sockaddr*)&socket_address, address_length) < 0){
		char* error = "could not bind to port for connectiong to server!\n";
		write(2,error,strlen(error));
		exit(EXIT_FAILURE);
	}
	if (connect(server_connection, (struct sockaddr*)&server_address, server_address_length) < 0){
		char* error = "could not connect to the server!\n";
		write(2,error,strlen(error));
		exit(EXIT_FAILURE);
	}
}

void receive_broadcast(int broadcast_fd, const char* my_port){
	int received_data;
	char response[128];
	char* file_name = response;

	if ((received_data = recvfrom(broadcast_fd, response, 128, 0, NULL, NULL)) < 0){
		char* error = "cant receive clients' broadcast!\n";
		write(2,error,strlen(error));
	}
	else if (received_data > 0){
		response[received_data] = '\0';
		if (memmem(response, strlen(response), my_port, strlen(my_port)) == response)
			return;

		char* port = response;
		int length = 0;

		while (file_name[0] != ' '){
			++length;
			++file_name;
		}

		port[length] = '\0';
		++file_name;

		int file_fd;
		char dest[100]={0};
		strcat(dest,path);
		strcat(dest,"/");
		strcat(dest,file_name);
		if ((file_fd = open(dest, O_RDONLY)) > 0){
			close(file_fd);
			write(2,"file is found in my directory.\n",32);

			int new_connection = socket(PF_INET, SOCK_STREAM, 0);
			struct sockaddr_in my_address, other_address;
			my_address.sin_family = PF_INET;
			my_address.sin_addr.s_addr = INADDR_ANY;
			my_address.sin_port = htons(atoi(my_port));
			int address_length = sizeof(my_address);

			int other_port = atoi(port);
			other_address.sin_family = PF_INET;
			other_address.sin_addr.s_addr = INADDR_ANY;
			other_address.sin_port = htons(other_port);
			int other_address_length = sizeof( other_address);
			int reuse = 1;
			setsockopt(new_connection, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));

			if (bind(new_connection, (struct sockaddr*)&my_address, address_length) < 0){
				char* error = "Could not bind a socket\n";
				write(2,error,strlen(error));
			}

			if (connect(new_connection, (struct sockaddr*)&other_address, other_address_length) < 0){
				char* error = "Could not connect to client!\n";
				write(2,error,strlen(error));
			}
				

			char buffer[10];
			struct timeval timeout = {1, 0};
			setsockopt(new_connection, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof timeout);
			int nbytes = read(new_connection, buffer, 10);
			buffer[nbytes] = '\0';

			if (strlen(buffer) != 1 || buffer[0] != '1'){
				char* msg = "Someone else sent the file!\n";
				write(2,msg,strlen(msg));
				close(new_connection);
				return;
			}
			send_file(new_connection, file_name);
			close(new_connection);
		}
	}
}

int send_file(int destination_fd, char* file_name){
	char buffer[MAX_LINE];
	int file_fd;

	char dest[100]={0};
	strcat(dest,path);
	strcat(dest,"/");
	strcat(dest,file_name);
	if ((file_fd = open(dest, O_RDONLY)) < 0){
		char* error = "Could not open file!\n";
		write(2,error,strlen(error));
		return 0;
	}

	int read_length;
	while((read_length = read(file_fd, buffer, sizeof(buffer))) > 0){
		if (send(destination_fd, buffer, read_length, 0) != read_length){
			char* error = "could not write to file properly!\n";
			write(2,error,strlen(error));
			return 0;
		}
	}

	if(read_length < 0){
    	char* error = "file not recieved properly\n";
		write(2,error,strlen(error));
		return 0;
	}
	char* msg ="File is sent with great success!\n";
	write(2,msg,strlen(msg));
	return 1;
}
int receive_file(int source_fd, char* file_name){
	char buffer[MAX_LINE];
	int file_fd;
	char dest[100]={0};
	strcat(dest,path);
	strcat(dest,"/");
	strcat(dest,file_name);
	if ((file_fd = open(dest, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU)) < 0){
		char* error = "create file error!\n";
		write(2,error,strlen(error));
		return 0;
	}

	int read_length = 0;
	fd_set read_fd;
	struct timeval timeout = {1, 0};
	setsockopt(source_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof timeout);

	while((read_length = read(source_fd, buffer, sizeof(buffer))) > 0){
		if (write(file_fd, buffer, read_length) != read_length){
			char* err = "write error";
			write(2,err,strlen(err));
			return 0;
		}
		if (read_length < MAX_LINE)
			break;
	}

	if(read_length < 0){
		char* err = "file not recieve properly!\n";
		write(2,err,strlen(err));
		return 0;
	}
	char* msg = "New file is received with success!\n";
	write(2,msg,strlen(msg));
	return 1;
}


int main(int argc, char const *argv[]){
	if (argc != 4){
		char* error = "Invalid inputs for ports.\n";
		write(2,error,strlen(error));
		exit(EXIT_FAILURE);
	}
	write(2,"Enter your directory:",21);
	int path_size = read(0,path,100);
	path[path_size-1]='\0';


	
	initial_broadcast_socket(argv[2]);
	initial_heartbeat_socket(argv[1]);

	int received_data = 0;
	int server_is_alive = 0;
	
	struct timeval past, now;
	gettimeofday(&now, NULL);
	past = now;

	

	while (1){
		FD_ZERO(&heartbeat_fd_set);
		FD_SET(heartbeat_fd, &heartbeat_fd_set);
		struct timeval timeout = {0, 0};
		select(heartbeat_fd + 1, &heartbeat_fd_set, NULL, NULL, &timeout);

		if (FD_ISSET(heartbeat_fd, &heartbeat_fd_set) && 
		(received_data = recvfrom(heartbeat_fd, heartbeat_message, 128, 0, NULL, NULL)) < 0){
			server_is_alive = 0;
			char* msg = "error on receiving server's heartbeat!\n";
			write(2,msg,strlen(msg));
			break;
		}
		else if (received_data > 0){
			server_is_alive = 1;
			heartbeat_message[received_data] = '\0';
			char* msg = "Server is up with port: ";
			write(2,msg,strlen(msg));
			write(2,heartbeat_message,strlen(heartbeat_message));
			write(2,"\n", 1);
			break;
		}

		gettimeofday(&now, NULL);
		if (now.tv_sec > past.tv_sec + 1){
			server_is_alive = 0;
			char* msg = "server is down!\n";
			write(2,msg,strlen(msg));
			break;
		}
	}

	initial_client_socket_for_peer_connection(argv[3]);

	// Server
	server_connection = socket(PF_INET, SOCK_STREAM, 0);
	socket_address.sin_family = PF_INET;
	socket_address.sin_addr.s_addr = INADDR_ANY;
	socket_address.sin_port = htons(atoi(argv[3]));
	address_length = sizeof( socket_address);

	if (server_is_alive){
		connect_to_server();
	}

	fd_set read_fd;
	int find_with_broadcast = 0;

	signal(SIGALRM, signal_handler);
	alarm(1);

	while (1){
		char command[100], broadcast_message[100];
		int read_size;

		struct timeval timeout = {0, 1000};
		FD_ZERO(&read_fd);
		FD_SET(0, &read_fd);
		FD_SET(broadcast_fd, &read_fd);
		FD_SET(client_connection, &read_fd);
		int maxfd = broadcast_fd > client_connection ? broadcast_fd : client_connection;
		int new_event = select(maxfd + 1, &read_fd, NULL, NULL, &timeout);

		if (possible_to_send_broadcast && find_with_broadcast){
			possible_to_send_broadcast = 0;
			char* msg = "Sending broadcast request\n";
			write(2,msg,strlen(msg));
			send_broadcast(atoi(argv[2]), broadcast_message);
			alarm(1);
		}

		if (new_event < 0 && errno == EINTR)
			continue;

		int data_is_ready = FD_ISSET(0, &read_fd);

		if (data_is_ready && (read_size = read(0, command, 100)) < 0){
			char* error = "get command error\n";
			write(2,error,strlen(error));
		}
		else if (data_is_ready){
			command[read_size - 1] = '\0';
			if (memmem(command, read_size, "download", 8) == command){
				int download_result = 0;
				if (server_is_alive && (download_result = download_file(server_connection, command)) < 0){
					char* error="download failed\n";
					write(2,error,strlen(error));
				}
				else if (download_result == 0){
					find_with_broadcast = 1;
					for (int i = 0; i < strlen(argv[3]); i++){
						broadcast_message[i] = argv[3][i];
					}
					for (int i = strlen(argv[3]); i < strlen(command) + 1; i++){
						broadcast_message[i] = command[i - strlen(argv[3]) + 8];
					}
				}
			}
			else if (memmem(command, read_size, "upload", 6) == command){
				if (server_is_alive)
					upload_file(server_connection, command);
				else{
					char* error= "server is down for uploading\n";
					write(2,error,strlen(error));
				}
			}
			else{
				char* error = "invalid command\n";
				write(2,error,strlen(error));
			}
				
		}

		if (FD_ISSET(broadcast_fd, &read_fd)){
			receive_broadcast(broadcast_fd, argv[3]);
		}

		if (FD_ISSET(client_connection, &read_fd)){
			int new_socket;
			if ((new_socket = accept(client_connection, (struct sockaddr*)&client_address, (socklen_t*)&client_address_length)) < 0){
				char* error = "could not accept sender connection!\n";
				write(2,error,strlen(error));
			}
			else if (find_with_broadcast){
				find_with_broadcast = 0;
				send(new_socket, "1", 1, 0);
				receive_file(new_socket, broadcast_message + strlen(argv[3]) + 1);
			}
			else
				send(new_socket, "0", 1, 0);
			close(new_socket);
		}
	}
}







