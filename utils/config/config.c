#include "config.h"
#include <stdio.h>

cJSON *parseConfig() {
	FILE *file = fopen("config.json", "r");
	if (file == NULL) {
		perror("Couldn't open config file");
		return NULL;
	}

	fseek(file, 0, SEEK_END);
	const long length = ftell(file);
	fseek(file, 0, SEEK_SET);

	char *buffer = malloc(length + 1);
	fread(buffer, 1, length, file);
	buffer[length] = '\0';

	fclose(file);

	cJSON *json = cJSON_Parse(buffer);
	free(buffer);

	return json;
}