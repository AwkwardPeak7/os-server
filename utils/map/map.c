#include "map.h"
#include <stdlib.h>
#include <string.h> 

#define shaLength 32

map* createMap(int maxSize) {
    // Allocate memory for the map struct
    map *m = (map *)malloc(sizeof(map));

	m->maxSize = maxSize;

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

// private function to create a map entry
mapEntry* __createMapEntry(int maxSize) {
    mapEntry *entry = (mapEntry *)malloc(sizeof(mapEntry));

    entry->userCount = 0;
    entry->fileNames = (char **)malloc(maxSize * sizeof(char *));
    entry->readingCount = (int*)malloc(maxSize * sizeof(int));
    entry->writing = (bool *)malloc(maxSize * sizeof(bool));

    for (size_t i = 0; i < maxSize; i++) {
        entry->fileNames[i] = NULL;
        entry->readingCount[i] = 0;
        entry->writing[i] = false;
    }

    return entry;
}

// private function to free a map entry
void __freeMapEntry(mapEntry *entry, int maxSize) {
    for (size_t i = 0; i < maxSize; i++) {
        free(entry->fileNames[i]);
    }
    free(entry->fileNames);
    free(entry->readingCount);
    free(entry->writing);
    free(entry);
}

void addUser(map *mp, unsigned char key[]) {
    // find existing or new index
    int idx = __getExistingOrNewIndex(mp, key);
    if (idx == -1) {
        // map is full, and we shouldn't be here...
        return;
    }
    
    // if key is already in the map, increment user count
    if (mp->keys[idx] != NULL) {
        mp->values[idx]->userCount++;
        return;
    }

    // if key is not in the map, add it and also malloc for values
    mp->keys[idx] = (char *)malloc(shaLength);
    memcpy(mp->keys[idx], key, shaLength);
    mp->values[idx] = __createMapEntry(mp->maxSize);

    // increment user count
    mp->values[idx]->userCount++;
}

void removeUser(map *mp, unsigned char key[]) {
    // find existing or new index
    int idx = __getExistingOrNewIndex(mp, key);
    if (idx == -1) {
        // key not found
        return;
    }

    // if key is not in the map, return
    if (mp->keys[idx] == NULL) {
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
