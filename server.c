#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cjson/cJSON.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <pthread.h>

#define CHUNK_SIZE 128 

bool fileExists(const char* dataDir, const char* filename) {
	char path[2048] = {};
	snprintf(path, 2048, "%s/%s", dataDir, filename);

	return access(path, F_OK) == 0;
}

long int getFileSize(const char* dataDir, const char* filename) {
	char path[2048] = {};
	snprintf(path, 2048, "%s/%s", dataDir, filename);

	struct stat st;
	stat(path, &st);

	return st.st_size;
}

void sendFileSize(const char* dataDir, const char *filename, int socketfd) {
	int size = getFileSize(dataDir, filename);

	cJSON* json = cJSON_CreateObject();
	cJSON_AddNumberToObject(json, "filesize", size);
	const char* resp = cJSON_Print(json);

	cJSON_Delete(json);

	send(socketfd, resp, strlen(resp), 0);
}

void sendFile(const char* dataDir, const char *filename, int socketfd) {
	char path[2048] = {};
	snprintf(path, 2048, "%s/%s", dataDir, filename);

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

void receiveFile(const char* dataDir, const char* filename, int filesize, int socketfd) {
	char path[2048] = {};
	snprintf(path, 2048, "%s/%s", dataDir, filename);

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

void sendFileList(const char* dataDir, int socketfd) {
	DIR* dir = opendir(dataDir);
	if (dir == NULL) {
		perror("couldn't open directory: files");
		char resp[] = "[]";
		send(socketfd, resp, sizeof(resp), 0);
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
		char path[2048] = {}; 
		snprintf(path, 2048, "%s/%s", dataDir, entry->d_name);
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
			continue;
		}

		cJSON* item = cJSON_CreateObject();
		cJSON_AddStringToObject(item, "filename", entry->d_name);
		cJSON_AddNumberToObject(item, "size", st.st_size);
		time_t time = st.st_mtime;
		if (time == 0) {
			time = st.st_ctime;
		}
		cJSON_AddNumberToObject(item, "timestamp", time);

		cJSON_AddItemToArray(json, item);
    }
	closedir(dir);

	const char* resp = cJSON_Print(json);
	send(socketfd, resp, strlen(resp), 0);
	cJSON_Delete(json);
}

struct server_args {
	int client_socket;
};

bool directoryExists(const char* path) {
	struct stat st;
	return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

const char* Authenticate(int socketfd, const char* dataDir) {
	char client_message[2000];

	recv(socketfd , client_message , 2000 , 0);
	puts(client_message);
	cJSON* json = cJSON_Parse(client_message);

	if (json == NULL) {
		char success[] = "{\"success\": false}";
		send(socketfd, success, sizeof(success), 0);
		return NULL;
	}

	const char* command = cJSON_GetObjectItem(json, "command")->valuestring;
	
	bool signup = strcmp(command, "SIGNUP") == 0;
	bool login = strcmp(command, "LOGIN") == 0;

	const char* hash = cJSON_GetObjectItem(json, "hash")->valuestring;

	char* path = (char*)malloc(1024);
	snprintf(path, 1024, "%s/%s", dataDir, hash);

	bool dirExists = directoryExists(path);

	if (signup && !dirExists) {
		mkdir(path, S_IRWXU | S_IRWXG);
		char success[] = "{\"success\": true}";
		send(socketfd, success, sizeof(success), 0);
		return path;
	} else if (login && dirExists) {
		char success[] = "{\"success\": true}";
		send(socketfd, success, sizeof(success), 0);
		return path;
	} else {
		char success[] = "{\"success\": false}";
		send(socketfd, success, sizeof(success), 0);
	}

	free(path);
	return NULL;
}

void* serveClient(void* args) {
	struct server_args* arg = (struct server_args*) args;

	printf("Connection accepted from %d\n", arg->client_socket);

	// TODO: data dir configuarable
	const char* userDir = Authenticate(arg->client_socket, "data");	
	if (userDir == NULL) {
		close(arg->client_socket);
		return NULL;
	}

	char client_message[2000];
	while(1) {
		recv(arg->client_socket , client_message , 2000 , 0);
		puts(client_message);
		cJSON* json = cJSON_Parse(client_message);

		if (json == NULL) {
			char success[] = "{\"success\": false}";
			send(arg->client_socket, success, sizeof(success), 0);
			continue;
		}

		const char* command = cJSON_GetObjectItem(json, "command")->valuestring;

		if (strcmp(command, "UPLOAD") == 0) {
			const char* filename = cJSON_GetObjectItem(json, "filename")->valuestring;
			int filesize = cJSON_GetObjectItem(json, "filesize")->valueint;
			// TODO: file size check for failure
			char success[] = "{\"success\": true}";
			send(arg->client_socket, success, sizeof(success), 0);

			receiveFile(userDir, filename, filesize, arg->client_socket);

		} else if (strcmp(command, "DOWNLOAD") == 0) {
			const char* filename = cJSON_GetObjectItem(json, "filename")->valuestring;
			if (fileExists(userDir, filename)) {
				cJSON* response = cJSON_CreateObject();
				cJSON_AddBoolToObject(response, "success", true);
				cJSON_AddNumberToObject(response, "filesize", getFileSize(userDir, filename));
				const char* success = cJSON_Print(response);
				cJSON_Delete(response);

				send(arg->client_socket, success, strlen(success), 0);

				// buffer clear
				memset(client_message, '\0', sizeof(client_message));

				recv(arg->client_socket , client_message , 2000 , 0);
				puts(client_message);
				cJSON* json_response = cJSON_Parse(client_message);
				if (json_response != NULL) {
					cJSON* success_item = cJSON_GetObjectItem(json_response, "success");
					if (cJSON_IsBool(success_item) && cJSON_IsTrue(success_item)) {
						sendFile(userDir, filename, arg->client_socket);
					}
					cJSON_Delete(json_response);
				}

			} else {
				char success[] = "{\"success\": false}";
				send(arg->client_socket, success, sizeof(success), 0);
			}
		} else if (strcmp(command, "VIEW") == 0) {
			sendFileList(userDir, arg->client_socket);
		} else if (strcmp(command, "EXIT") == 0) {
			//close(arg->client_socket);
			printf("EXIT called: %d",arg->client_socket);
			close(arg->client_socket);

			return NULL; // exit
		} else {
			char success[] = "{\"success\": false}";
			send(arg->client_socket, success, sizeof(success), 0);
		}

        // buffer clear
        memset(client_message, '\0', sizeof(client_message));
		cJSON_Delete(json);
	}

	return NULL;
}

int main() {
	printf("pid: %d\n", getpid());
	
	int server_socket = socket(AF_INET , SOCK_STREAM , 0);
	if (server_socket == -1) {
		printf("Could not create server socket\n");
        return 1;
	}
	printf("Server Socket created\n");
	
    struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(14000);
	
	if(bind(server_socket,(struct sockaddr *)&server , sizeof(server)) < 0) {
		perror("bind failed. Error");
		return 1;
	}
	puts("bind done");
	
	listen(server_socket , 3);
	
	int c = sizeof(struct sockaddr_in);
	
    struct sockaddr_in client;

	pthread_t thread_ids[3] = {-1, -1, -1};
	int client_sockets[3] = {-1, -1, -1};
    struct server_args args[3] = {};

	while (1) {
		puts("main -- first for");
		for (int i = 0; i < 3; i++) {
			if (client_sockets[i] != -1) continue;

			int client_sock = accept(server_socket, (struct sockaddr *)&client, (socklen_t*)&c);
			if (client_sock < 0) {
				perror("accept failed");
				continue;
			}

			client_sockets[i] = client_sock;
			args[i].client_socket = client_sock;

			pthread_create(&thread_ids[i], NULL, serveClient, (void*)&args[i]);
		}

		puts("main -- second for");
		for (int i = 0; i < 3; i++) {
            if (thread_ids[i] != -1) {
                int res = pthread_tryjoin_np(thread_ids[i], NULL);
                if (res == 0) {
					//printf("thread %d terminated", thread_ids[i]);
                    client_sockets[i] = -1;
                    thread_ids[i] = -1;
                }
            }
        }

		usleep(100000); // 100ms
		
	}
	
	return 0;
}