#ifndef MAP_H
#define MAP_H

#include <stdbool.h>
#include <pthread.h>

typedef struct map
{
    int maxSize;
    char* *keys;
    mapEntry* *values;
    pthread_mutex_t lock;
} map;

typedef struct mapEntry
{
    unsigned char* *fileNames;
    pthread_mutex_t fileNamesLock;

    unsigned int userCount;
    unsigned int *readingCount;

    pthread_mutex_t *readingLock;
    pthread_mutex_t *writingLock;
} mapEntry;

map* createMap(int maxSize);
void freeMap(map* mp);

void addUser(map* mp, unsigned char key[]);
void removeUser(map* mp, unsigned char key[]);

// can read specified file or not
// if readingcount internal variable is not 0, then it returns false
// or when writing internal variable is false, then it returns false
void startRead(map* mp, unsigned char key[], char* fileName);

// can write specified file or not
// if writing internal variable is true, then it returns false
// or when reading internal variable is not 0, then it returns false
void startWrite(map* mp, unsigned char key[], char* fileName);


#endif