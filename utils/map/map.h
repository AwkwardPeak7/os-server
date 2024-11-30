#ifndef MAP_H
#define MAP_H

#include <stdbool.h>
#include <pthread.h>
#include "../arena/arena.h"

typedef struct fileEntry
{
    unsigned char* fileName;

    unsigned int readingCount;

    pthread_mutex_t readingLock;
    pthread_mutex_t writingLock;

    unsigned int referenceCount;
    pthread_mutex_t referenceLock;
} fileEntry;

typedef struct mapEntry
{
    fileEntry *fileEntries;

    pthread_mutex_t fileEntriesLock;

    unsigned int userCount;

    Arena* arena;
} mapEntry;

typedef struct map
{
    int maxSize;
    char* *keys;
    mapEntry* *values;
    pthread_mutex_t lock;
    Arena* arena;
} map;

map* createMap(int maxSize, Arena* _arena);
void freeMap(map* mp);

void addUser(map* mp, unsigned char key[]);
void removeUser(map* mp, unsigned char key[]);

// can read specified file or not
// if readingcount internal variable is not 0, then it returns false
// or when writing internal variable is false, then it returns false
void startRead(map* mp, unsigned char key[], char* fileName);

void stopRead(map* mp, unsigned char key[], char* fileName);

// can write specified file or not
// if writing internal variable is true, then it returns false
// or when reading internal variable is not 0, then it returns false
void startWrite(map* mp, unsigned char key[], char* fileName);

void stopWrite(map* mp, unsigned char key[], char* fileName);


#endif