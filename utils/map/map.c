#include "map.h"
#include <stdlib.h>
#include <string.h> 

#define shaLength 32

map* createMap(int maxSize) {
    // Allocate memory for the map struct
    map *m = (map *)malloc(sizeof(map));

	m->maxSize = maxSize;

    m->lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

    m->keys = (char **)malloc(maxSize * sizeof(char *));

    for (int i = 0; i < maxSize; i++)  {
        m->keys[i] = NULL;
    }

    // Allocate memory for values array
    m->values = (mapEntry **)malloc(maxSize * sizeof(mapEntry*));

    for (size_t i = 0; i < maxSize; i++) {
        m->values[i] = NULL;
    }

    return m;
}

// private function to get existing or new index
int __getExistingOrNewIndex(map *mp, unsigned char key[]) {
    int idx = -1;
    for (size_t i = 0; i < mp->maxSize; i++) {
        if (mp->keys[i] == NULL) {
            if (idx == -1)
                idx = i;
            break;
        }
        if (strcmp(mp->keys[i], key) == 0) {
            idx = i;
            return idx;
        }
    }
    return idx;
}

int __getExistingIndex(map *mp, unsigned char key[]) {
    for (size_t i = 0; i < mp->maxSize; i++) {
        if (mp->keys[i] != NULL && strcmp(mp->keys[i], key) == 0) {
            return i;
        }
    }
    return -1;
}

// private function to create a map entry
mapEntry* __createMapEntry(int maxSize) {
    mapEntry *entry = (mapEntry *)malloc(sizeof(mapEntry));

    entry->userCount = 0;
    entry->fileNamesLock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

    entry->fileNames = (char **)malloc(maxSize * sizeof(char *));
    entry->readingCount = (int*)malloc(maxSize * sizeof(int));
    entry->readingLock = (pthread_mutex_t *)malloc(maxSize * sizeof(pthread_mutex_t));
    entry->writingLock = (pthread_mutex_t *)malloc(maxSize * sizeof(pthread_mutex_t));

    for (size_t i = 0; i < maxSize; i++) {
        entry->fileNames[i] = NULL;
        entry->readingCount[i] = 0;
        entry->readingLock[i] = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
        entry->writingLock[i] = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    }

    return entry;
}

// private function to free a map entry
void __freeMapEntry(mapEntry *entry, int maxSize) {
    for (size_t i = 0; i < maxSize; i++) {
        free(entry->fileNames[i]);
        pthread_mutex_destroy(&entry->readingLock[i]);
        pthread_mutex_destroy(&entry->writingLock[i]);
    }
    pthread_mutex_destroy(&entry->fileNamesLock);
    free(entry->fileNames);
    free(entry->readingCount);
    free(entry->readingLock);
    free(entry->writingLock);
    free(entry);
}

void addUser(map *mp, unsigned char key[]) {
    pthread_mutex_lock(&mp->lock);

    // find existing or new index
    int idx = __getExistingOrNewIndex(mp, key);
    if (idx == -1) {
        pthread_mytex_unlock(&mp->lock);
        // map is full, and we shouldn't be here...
        return;
    }
    
    // if key is already in the map, increment user count
    if (mp->keys[idx] != NULL) {
        mp->values[idx]->userCount++;
        pthread_mutex_unlock(&mp->lock);
        return;
    }

    // if key is not in the map, add it and also malloc for values
    mp->keys[idx] = (char *)malloc(shaLength);
    memcpy(mp->keys[idx], key, shaLength);
    
    // we can unlock now since we have reserved the index
    pthread_mutex_unlock(&mp->lock);

    mp->values[idx] = __createMapEntry(mp->maxSize);

    // increment user count
    mp->values[idx]->userCount++;
}

void removeUser(map *mp, unsigned char key[]) {
    // find existing or new index
    int idx = __getExistingIndex(mp, key);
    if (idx == -1) {
        // key not found
        return;
    }

    // decrement user count
    mp->values[idx]->userCount--;

    // if user count is 0, free the entry
    if (mp->values[idx]->userCount == 0) {
        free(mp->keys[idx]);
        __freeMapEntry(mp->values[idx], mp->maxSize);
        mp->keys[idx] = NULL;
        mp->values[idx] = NULL;
    }
}

void freeMap(map *mp) {
    if (mp) {
        for (int i = 0; i < mp->maxSize; i++) {
            if (mp->keys[i] != NULL) {
                free(mp->keys[i]);
            }

            if (mp->values[i] != NULL) {
                __freeMapEntry(mp->values[i], mp->maxSize);
            }
        }
        free(mp->keys);
        free(mp->values);
        free(mp);
    }
}

// private function to get file name index in given map entry
bool __getExistingOrNewFileNameIndex(mapEntry* me, char *fileName, int maxSize) {
    int idx = -1;
    for (size_t i = 0; i < maxSize; i++) {
        if (me->fileNames[i] == NULL) {
            if (idx == -1)
                idx = i;
            break;
        }
        if (strcmp(me->fileNames[i], fileName) == 0) {
            idx = i;
            return idx;
        }
    }
    return idx;
}
