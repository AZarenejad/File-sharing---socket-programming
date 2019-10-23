all: server client

server :  server.h
	gcc  server.c -o server

client : client.h
	gcc  client.c -o client

clean:
	rm server 
	rm client
