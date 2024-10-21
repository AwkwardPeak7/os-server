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
#include <pthread.h>

#include "utils/filesystem/filesystem.h"
#include "utils/config/config.h"
#include "utils/queue/queue.h"
#include "utils/map/map.h"
#include "transfer/transfer.h"

pthread_mutex_t users_lock = PTHREAD_MUTEX_INITIALIZER;
map* logged_in_users = NULL;


struct server_args {
	int client_socket;
	char *dataDir;
	unsigned int dataLimit;
};

char *Authenticate(int socket, const char *dataDir) {
	char client_message[2000];

	recv(socket, client_message, 2000, 0);
	puts(client_message);
	cJSON *json = cJSON_Parse(client_message);

	if (json == NULL) {
		const char success[] = "{\"success\": false}";
		send(socket, success, sizeof(success), 0);
		return NULL;
	}

	const char *command = cJSON_GetObjectItem(json, "command")->valuestring;

	const bool signup = strcmp(command, "SIGNUP") == 0;
	const bool login = strcmp(command, "LOGIN") == 0;

	const char *hash = cJSON_GetObjectItem(json, "hash")->valuestring;

	char *path = malloc(1024);
	snprintf(path, 1024, "%s/%s", dataDir, hash);

	const bool dirExists = directoryExists(path);

	if ((signup && !dirExists) || (login && dirExists)) {
		mkdir(path, S_IRWXU | S_IRWXG);
		const char success[] = "{\"success\": true}";
		send(socket, success, sizeof(success), 0);
	}
	else {
		const char success[] = "{\"success\": false}";
		send(socket, success, sizeof(success), 0);
		free(path);
		path = NULL;
	}

	if (path != NULL) {
		pthread_mutex_lock(&users_lock);
		increment(logged_in_users, (unsigned char*)hash);
		pthread_mutex_unlock(&users_lock);
		printf("map incremented with user: %s -> %d\n", hash, get(logged_in_users, (unsigned char*)hash));
	}

	cJSON_Delete(json);
	return path;
}

void *serveClient(void *args) {
	const struct server_args *arg = (struct server_args *) args;

	printf("Connection accepted from %d\n", arg->client_socket);

	char *userDir = Authenticate(arg->client_socket, arg->dataDir);
	if (userDir == NULL) {
		close(arg->client_socket);
		return NULL;
	}

	while (1) {
		char client_message[2000];
		recv(arg->client_socket, client_message, 2000, 0);
		puts(client_message);
		cJSON *json = cJSON_Parse(client_message);

		if (json == NULL) {
			const char success[] = "{\"success\": false}";
			send(arg->client_socket, success, sizeof(success), 0);
			continue;
		}

		const char *command = cJSON_GetObjectItem(json, "command")->valuestring;

		if (strcmp(command, "UPLOAD") == 0) {
			const char *filename = cJSON_GetObjectItem(json, "filename")->valuestring;
			const int filesize = cJSON_GetObjectItem(json, "filesize")->valueint;
			const unsigned int userDirSize = getDirectorySize(userDir);
			if (userDirSize + filesize > arg->dataLimit) {
				const char success[] = "{\"success\": false}";
				send(arg->client_socket, success, sizeof(success), 0);
				continue;
			}
			const char success[] = "{\"success\": true}";
			send(arg->client_socket, success, sizeof(success), 0);

			receiveFile(userDir, filename, filesize, arg->client_socket);
		} else if (strcmp(command, "DOWNLOAD") == 0) {
			const char *filename = cJSON_GetObjectItem(json, "filename")->valuestring;
			if (fileExists(userDir, filename)) {
				cJSON *response = cJSON_CreateObject();
				cJSON_AddBoolToObject(response, "success", true);
				cJSON_AddNumberToObject(response, "filesize", getFileSize(userDir, filename));
				const char *success = cJSON_Print(response);
				cJSON_Delete(response);

				send(arg->client_socket, success, strlen(success), 0);

				// buffer clear
				memset(client_message, '\0', sizeof(client_message));

				recv(arg->client_socket, client_message, 2000, 0);
				puts(client_message);
				cJSON *json_response = cJSON_Parse(client_message);
				if (json_response != NULL) {
					const cJSON *success_item = cJSON_GetObjectItem(json_response, "success");
					if (cJSON_IsBool(success_item) && cJSON_IsTrue(success_item)) {
						sendFile(userDir, filename, arg->client_socket);
					}
					cJSON_Delete(json_response);
				}
			} else {
				const char success[] = "{\"success\": false}";
				send(arg->client_socket, success, sizeof(success), 0);
			}
		} else if (strcmp(command, "VIEW") == 0) {
			sendFileList(userDir, arg->client_socket);
		} else if (strcmp(command, "EXIT") == 0) {
			printf("EXIT called: %d", arg->client_socket);
			close(arg->client_socket);
			free(userDir);

			return NULL; // exit
		} else {
			const char success[] = "{\"success\": false}";
			send(arg->client_socket, success, sizeof(success), 0);
		}

		// buffer clear
		memset(client_message, '\0', sizeof(client_message));
		cJSON_Delete(json);
	}
}

int main() {
	printf("pid: %d\n", getpid());

	cJSON *config = parseConfig();
	if (config == NULL) {
		perror("Couldn't parse config file");
		return 1;
	}

	char *dataDir = cJSON_GetObjectItem(config, "dataDir")->valuestring;
	if (!directoryExists(dataDir)) {
		makePath(dataDir);
		if (!directoryExists(dataDir)) {
			perror("Couldn't create data directory");
			return 1;
		}
	}

	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == -1) {
		printf("Could not create server socket\n");
		return 1;
	}
	printf("Server Socket created\n");

	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(cJSON_GetObjectItem(config, "port")->valueint);

	if (bind(server_socket, (struct sockaddr *) &server, sizeof(server)) < 0) {
		perror("bind failed. Error");
		return 1;
	}
	puts("bind done");

	const int threads = cJSON_GetObjectItem(config, "threads")->valueint;
	// FIXME: this shit only accepting 3 connections/ only making 3 threads
	listen(server_socket, threads);

	int c = sizeof(struct sockaddr_in);

	struct sockaddr_in client;

	pthread_t *thread_ids = malloc(threads * sizeof(pthread_t));
	int *client_sockets = malloc(threads * sizeof(int));
	struct server_args *args = malloc(threads * sizeof(struct server_args));

	for (int i = 0; i < threads; i++) {
		thread_ids[i] = -1;
		client_sockets[i] = -1;

		args[i].dataDir = (char *) malloc(strlen(dataDir));
		strcpy(args[i].dataDir, dataDir);
		args[i].dataLimit = cJSON_GetObjectItem(config, "dataLimit")->valueint;
	}

	logged_in_users = createMap(threads);

	cJSON_Delete(config);

	while (1) {
		for (int i = 0; i < threads; i++) {
			if (client_sockets[i] != -1) continue;

			const int client_sock = accept(server_socket, (struct sockaddr *) &client, (socklen_t *) &c);

			if (client_sock < 0) {
				perror("accept failed");
				continue;
			}

			client_sockets[i] = client_sock;
			args[i].client_socket = client_sock;

			pthread_create(&thread_ids[i], NULL, serveClient, &args[i]);
		}

		for (int i = 0; i < threads; i++) {
			if (thread_ids[i] != -1) {
				const int res = pthread_tryjoin_np(thread_ids[i], NULL);
				if (res == 0) {
					client_sockets[i] = -1;
					thread_ids[i] = -1;
				}
			}
		}

		usleep(100000); // 100ms
	}
}
