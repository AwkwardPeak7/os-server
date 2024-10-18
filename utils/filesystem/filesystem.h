#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdbool.h>

bool fileExists(const char *dataDir, const char *filename);
unsigned int getFileSize(const char *dataDir, const char *filename);
bool directoryExists(const char *path);
unsigned int getDirectorySize(const char *path);

#endif