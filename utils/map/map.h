#ifndef MAP_H
#define MAP_H

#include <stdbool.h>

typedef struct map
{
    int maxSize;
    char* *keys;
    mapEntry* *values;
} map;

typedef struct mapEntry
{
    unsigned int userCount;
    unsigned char* *fileNames;
    unsigned int *readingCount;
    bool *writing;
} mapEntry;

map* createMap(int maxSize);
void freeMap(map* mp);

void addUser(map* mp, unsigned char key[]);

// can read specified file or not
// if readingcount internal variable is not 0, then it returns false
// or when writing internal variable is false, then it returns false
bool canRead(map* mp, unsigned char key[], char** fileName);

// can write specified file or not
// if writing internal variable is true, then it returns false
// or when reading internal variable is not 0, then it returns false
bool canWrite(map* mp, unsigned char key[], char* fileName);


#endif