#include "filesystem.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>

bool fileExists(const char *dataDir, const char *filename) {
	char path[2048] = {};
	snprintf(path, 2048, "%s/%s", dataDir, filename);

	return access(path, F_OK) == 0;
}

unsigned int getFileSize(const char *dataDir, const char *filename) {
	char path[2048] = {};
	snprintf(path, 2048, "%s/%s", dataDir, filename);

	struct stat st;
	stat(path, &st);

	return st.st_size;
}

bool directoryExists(const char *path) {
	struct stat st;
	return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

long getDirectorySize(const char *path) {
	long size = 0;
	DIR *dir = opendir(path);
	struct dirent *entry;
	struct stat st;
	while ((entry = readdir(dir)) != NULL) {
		// Skip the current and parent directories
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
			continue;
		}

		// Use stat to get file type information
		char filePath[2048] = {};
		snprintf(filePath, 2048, "%s/%s", filePath, entry->d_name);
		if (stat(filePath, &st) == 0 && S_ISDIR(st.st_mode)) {
			continue;
		}

		size += st.st_size;
	}
	closedir(dir);

	return size;
}

void makePath(const char *path) {
	char temp[2048];
	snprintf(temp, sizeof(temp), "%s", path);

	for (char *p = temp + 1; *p; p++) {
		if (*p == '/') {
			*p = '\0';
			mkdir(temp, S_IRWXU | S_IRWXG);
			*p = '/';
		}
	}
	mkdir(temp, S_IRWXU | S_IRWXG);
}