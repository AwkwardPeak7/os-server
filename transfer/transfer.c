#include "transfer.h"

#include <cjson/cJSON.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#include "../utils/filesystem/filesystem.h"

#define CHUNK_SIZE 128

void sendFileSize(const char *dataDir, const char *filename, int socketfd) {
	const unsigned int size = getFileSize(dataDir, filename);

	cJSON *json = cJSON_CreateObject();
	cJSON_AddNumberToObject(json, "filesize", size);
	const char *resp = cJSON_Print(json);

	cJSON_Delete(json);

	send(socketfd, resp, strlen(resp), 0);
}

void sendFile(const char *dataDir, const char *filename, int socketfd) {
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

void receiveFile(const char *dataDir, const char *filename, int filesize, int socket) {
	char path[2048] = {};
	snprintf(path, 2048, "%s/%s", dataDir, filename);

	FILE *file = fopen(path, "wb");

	unsigned int soFar = 0;

	while (soFar < filesize) {
		char buffer[CHUNK_SIZE];
		const unsigned int n = recv(socket, buffer, CHUNK_SIZE, 0);
		soFar += n;

		fwrite(buffer, 1, n, file);
	}
	fclose(file);
}

// TODO: too chonky function... refactor
void sendFileList(const char *dataDir, int socketfd) {
	DIR *dir = opendir(dataDir);
	if (dir == NULL) {
		perror("couldn't open directory: files");
		const char resp[] = "[]";
		send(socketfd, resp, sizeof(resp), 0);
		return;
	}

	cJSON *json = cJSON_CreateArray();

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

		cJSON *item = cJSON_CreateObject();
		cJSON_AddStringToObject(item, "filename", entry->d_name);
		cJSON_AddNumberToObject(item, "size", st.st_size);
		unsigned int time = st.st_mtime;
		if (time == 0) {
			time = st.st_ctime;
		}
		cJSON_AddNumberToObject(item, "timestamp", time);

		cJSON_AddItemToArray(json, item);
	}
	closedir(dir);

	const char *resp = cJSON_Print(json);
	send(socketfd, resp, strlen(resp), 0);
	cJSON_Delete(json);
}
