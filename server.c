#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cjson/cJSON.h>

#define CHUNK_SIZE 128 

void receiveFile(const char* filename, int filesize, int socketfd) {
	FILE *file = fopen(filename, "wb");

	int sofar = 0;
	char buffer[CHUNK_SIZE];

	while (sofar < filesize) {
		int n = recv(socketfd, buffer, CHUNK_SIZE, 0);
		sofar+=n;

		fwrite(buffer, 1, n, file);
	}
	fclose(file);
}

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
		puts(client_message);
		cJSON* json = cJSON_Parse(client_message);

		const char* command = cJSON_GetObjectItem(json, "command")->valuestring;

		if (strcmp(command, "UPLOAD") == 0) {
			const char* filename = cJSON_GetObjectItem(json, "filename")->valuestring;
			int filesize = cJSON_GetObjectItem(json, "filesize")->valueint;

			char success[] = "{\"success\": true}";
			write(client_sock, success, sizeof(success));

			receiveFile(filename, filesize, client_sock);

		} else if (strcmp(command, "download") == 0) {

		} else if (strcmp(command, "view") == 0) {

		} else if (strcmp(command, "exit") == 0) {
			close(client_sock);
			//TODO: don't exit server here
			return 0;
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