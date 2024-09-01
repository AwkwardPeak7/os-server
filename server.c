
#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>

int main() {
	char client_message[2000];
	
	int socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1) {
		printf("Could not create socket\n");
        return 1;
	}
	printf("Socket created\n");
	
    struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(14000);
	
	if(bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0) {
		perror("bind failed. Error");
		return 1;
	}
	puts("bind done");
	
	listen(socket_desc , 1);
	
	puts("Waiting for incoming connections...");
	int c = sizeof(struct sockaddr_in);
	
    struct sockaddr_in client;

	int client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
	if (client_sock < 0) {
		perror("accept failed");
		return 1;
	}
	puts("Connection accepted");
	
    int read_size;
	while( (read_size = recv(client_sock , client_message , 2000 , 0)) > 0 ) {
        int res = strcmp(client_message, "hi");
        if (res == 0) {
            char str[] = "hello";
            write(client_sock, str, sizeof(str));
        } else {
            char str[] = "Mazak Na kar mere naal";
            write(client_sock, str, sizeof(str));
        }

        // buffer clear
        memset(client_message, '\0', sizeof(client_message));
        read_size = 0;
	}
	
	if(read_size == 0) {
		puts("Client disconnected");
		fflush(stdout);
	} else if(read_size == -1) {
		perror("recv failed");
	}
	
	return 0;
}