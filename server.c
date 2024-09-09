#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cjson/cJSON.h>
#include <sys/stat.h>
#include <dirent.h>

#define CHUNK_SIZE 128 

bool fileExists(const char* filename) {
	char path[1024] = "files/";
	strcat(path, filename);

	return access(path, F_OK) == 0;
}

int getFileSize(const char* filename) {
	char path[1024] = "files/";
	strcat(path, filename);
    FILE *file = fopen(path, "rb");

    fseek(file, 0, SEEK_END);
    int file_size = ftell(file);
    fseek(file, 0, SEEK_SET);  

	fclose(file);

	return file_size;
}

void sendFileSize(const char *filename, int socketfd) {
	int size = getFileSize(filename);

	cJSON* json = cJSON_CreateObject();
	cJSON_AddNumberToObject(json, "filesize", size);
	const char* resp = cJSON_Print(json);

	cJSON_Delete(json);

	write(socketfd, resp, strlen(resp));

	free(resp);
}

void sendFile(const char *filename, int socketfd) {
	char path[1024] = "files/";
	strcat(path, filename);

    FILE *file = fopen(path, "rb");

    char buffer[CHUNK_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, CHUNK_SIZE, file)) > 0) {
        if (send(socketfd, buffer, bytes_read, 0) == -1) {
            perror("Error sending file data");
            fclose(file);
        }
    }

    fclose(file);
}

void receiveFile(const char* filename, int filesize, int socketfd) {
	char path[1024] = "files/";
	mkdir(path, S_IRWXU | S_IRWXG);
	strcat(path, filename);

	FILE *file = fopen(path, "wb");

	int sofar = 0;
	char buffer[CHUNK_SIZE];

	while (sofar < filesize) {
		int n = recv(socketfd, buffer, CHUNK_SIZE, 0);
		sofar+=n;

		fwrite(buffer, 1, n, file);
	}
	fclose(file);
}

void sendFileList(int socketfd) {
	DIR* dir = opendir("files");
	if (dir == NULL) {
		perror("couldn't open directory: files");
		char resp[] = "[]";
		write(socketfd, resp, sizeof(resp));
		return;
	}

	cJSON* json = cJSON_CreateArray();

	struct dirent *entry;
	struct stat st;
	while ((entry = readdir(dir)) != NULL) {
        // Skip the current and parent directories
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Use stat to get file type information
        if (stat(entry->d_name, &st) == 0 && S_ISDIR(st.st_mode)) {
			continue;
		}
		cJSON_AddItemToArray(json, cJSON_CreateString(entry->d_name));
    }
	closedir(dir);

	const char* resp = cJSON_Print(json);
	write(socketfd, resp, strlen(resp));
	cJSON_Delete(json);
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
			// TODO: file size check for failure
			char success[] = "{\"success\": true}";
			write(client_sock, success, sizeof(success));

			receiveFile(filename, filesize, client_sock);

		} else if (strcmp(command, "DOWNLOAD") == 0) {
			const char* filename = cJSON_GetObjectItem(json, "filename")->valuestring;
			if (fileExists(filename)) {
				char success[] = "{\"success\": true}";
				write(client_sock, success, sizeof(success));

				sendFileSize(filename, client_sock);

				sendFile(filename, client_sock);
			} else {
				char success[] = "{\"success\": false}";
				write(client_sock, success, sizeof(success));
			}
		} else if (strcmp(command, "VIEW") == 0) {
			sendFileList(client_sock);
		} else if (strcmp(command, "EXIT") == 0) {
			close(client_sock);
			//TODO: don't exit server here
			return 0;
		}

        // buffer clear
        memset(client_message, '\0', sizeof(client_message));
        read_size = 0;
		cJSON_Delete(json);
	}
	
	if(read_size == 0) {
		puts("Client disconnected");
		fflush(stdout);
	} else if(read_size == -1) {
		perror("recv failed");
	}
	
	return 0;
}