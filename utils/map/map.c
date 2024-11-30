#include "map.h"
#include "../arena/arena.h"
#include <stdlib.h>
#include <string.h> 
#include <stdio.h>
#include <unistd.h>

#define shaLength 32

map* createMap(int maxSize, Arena* _arena) {
    // Allocate memory for the map struct
    map *m = (map *)custom_malloc(_arena, sizeof(map));

	m->maxSize = maxSize;

    m->lock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

    m->keys = (char **)custom_malloc(_arena, maxSize * sizeof(char *));

    for (int i = 0; i < maxSize; i++)  {
        m->keys[i] = NULL;
    }

    // Allocate memory for values array
    m->values = (mapEntry **)custom_malloc(_arena, maxSize * sizeof(mapEntry*));

    for (size_t i = 0; i < maxSize; i++) {
        m->values[i] = NULL;
    }

    m->arena = _arena;

    return m;
}

// private function to get existing or new index
int __getExistingOrNewEntryIndex(map *mp, unsigned char key[]) {
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

int __getExistingEntryIndex(map *mp, unsigned char key[]) {
    for (size_t i = 0; i < mp->maxSize; i++) {
        if (mp->keys[i] != NULL && strcmp(mp->keys[i], key) == 0) {
            return i;
        }
    }
    return -1;
}

// private function to create a map entry
mapEntry* __createMapEntry(int maxSize, Arena* _arena) {
    mapEntry *entry = (mapEntry *)custom_malloc(_arena,sizeof(mapEntry));

    entry->arena = _arena;
    entry->userCount = 0;
    entry->fileEntriesLock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    entry->fileEntries = (fileEntry *)custom_malloc(_arena, maxSize * sizeof(fileEntry));

    entry->fileEntries->fileName = NULL;
    entry->fileEntries->readingCount = 0;
    entry->fileEntries->readingLock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    entry->fileEntries->referenceCount = 0;
    entry->fileEntries->referenceLock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    entry->fileEntries->writingLock = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

    return entry;
}

// private function to free a map entry
void __freeMapEntry(mapEntry *entry, int maxSize) {
    custom_free(entry->arena,entry->fileEntries->fileName);
    custom_free(entry->arena, entry->fileEntries);
    custom_free(entry->arena, entry);
}

void addUser(map *mp, unsigned char key[]) {
    pthread_mutex_lock(&mp->lock);

    // find existing or new index
    int idx = __getExistingOrNewEntryIndex(mp, key);
    if (idx == -1) {
        pthread_mutex_unlock(&mp->lock);
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
    mp->keys[idx] = (char *)custom_malloc( mp->arena, shaLength);
    memcpy(mp->keys[idx], key, shaLength);
    
    // we can unlock now since we have reserved the index
    pthread_mutex_unlock(&mp->lock);

    mp->values[idx] = __createMapEntry(mp->maxSize, mp->arena);

    // increment user count
    mp->values[idx]->userCount++;
}

void removeUser(map *mp, unsigned char key[]) {
    // find existing index
    int idx = __getExistingEntryIndex(mp, key);
    if (idx == -1) {
        // key not found
        return;
    }

    // decrement user count
    mp->values[idx]->userCount--;

    // if user count is 0, free the entry
    if (mp->values[idx]->userCount == 0) {
        pthread_mutex_lock(&mp->lock);
        custom_free(mp->arena, mp->keys[idx]);
        __freeMapEntry(mp->values[idx], mp->maxSize);
        mp->keys[idx] = NULL;
        mp->values[idx] = NULL;
        pthread_mutex_unlock(&mp->lock);
    }
}

void freeMap(map *mp) {
    if (mp) {
        for (int i = 0; i < mp->maxSize; i++) {
            if (mp->keys[i] != NULL) {
                custom_free(mp->arena,mp->keys[i]);
            }

            if (mp->values[i] != NULL) {
                __freeMapEntry(mp->values[i], mp->maxSize);
            }
        }
        custom_free(mp->arena, mp->keys);
        custom_free(mp->arena, mp->values);
        custom_free(mp->arena, mp);
    }
}

// private function to get file name index in given map entry
bool __getExistingOrNewFileNameIndex(mapEntry* me, char *fileName, int maxSize) {
    int idx = -1;
    for (size_t i = 0; i < maxSize; i++) {
        if (me->fileEntries[i].fileName == NULL) {
            if (idx == -1)
                idx = i;
            continue;
        }
        if (strcmp(me->fileEntries[i].fileName, fileName) == 0) {
            idx = i;
            return idx;
        }
    }
    return idx;
}

bool __getExistingFileNameIndex(mapEntry* me, char *fileName, int maxSize) {
    for (size_t i = 0; i < maxSize; i++) {
        if (me->fileEntries[i].fileName != NULL && strcmp(me->fileEntries[i].fileName, fileName) == 0) {
            return i;
        }
    }
    return -1;
}

void __incrementReferenceCount(fileEntry* fe) {
    pthread_mutex_lock(&fe->referenceLock);
    fe->referenceCount++;
    pthread_mutex_unlock(&fe->referenceLock);
}

// void __decrementReferenceCount(fileEntry* fe) {
//     pthread_mutex_lock(&fe->referenceLock);
//     fe->referenceCount--;
//     pthread_mutex_unlock(&fe->referenceLock);
// }

void __removeFileName(mapEntry* me, int fileIdx) {
    pthread_mutex_lock(&me->fileEntriesLock);
    fileEntry* fe = &(me->fileEntries[fileIdx]);

    custom_free(me->arena, fe->fileName);
    fe->fileName = NULL;
    fe->referenceCount = 0;
    fe->readingCount = 0;

    pthread_mutex_unlock(&me->fileEntriesLock);
}

void startRead(map* mp, unsigned char key[], char* fileName) {
    // find existing
    int idx = __getExistingOrNewEntryIndex(mp, key);
    if (idx == -1) {
        puts("startRead: idx == -1\n");
        return; // we shouldn't be here
    }

    if (mp->values[idx] == NULL) {
        printf("startRead: allocate new map entry at %d\n", idx);
        mp->values[idx] = __createMapEntry(mp->maxSize, mp->arena);
        mp->keys[idx] = (char*)custom_malloc(mp->arena,strlen(key));
        strcpy(mp->keys[idx], key);
    }

    mapEntry* me = mp->values[idx];

    pthread_mutex_lock(&me->fileEntriesLock);

    // find existing or new index for file name
    int fileIdx = __getExistingOrNewFileNameIndex(me, fileName, mp->maxSize);

    if (fileIdx == -1) {
        puts("fileIdx == -1\n");
        pthread_mutex_unlock(&me->fileEntriesLock);
        return; // we really shouldn't be here...
    }

    fileEntry* fe = &(me->fileEntries[fileIdx]);

    // set file name if it's not set
    if (fe->fileName == NULL) {
        fe->fileName = (char *)custom_malloc(mp->arena, strlen(fileName));
        strcpy(fe->fileName, fileName);
    }

    __incrementReferenceCount(fe);

    pthread_mutex_unlock(&me->fileEntriesLock);

    // main logic
    pthread_mutex_lock(&fe->readingLock);
    if (fe->readingCount == 0) {
        pthread_mutex_lock(&fe->writingLock);

    }
    fe->readingCount++;
    printf("readcount %d\n",fe->readingCount);
    pthread_mutex_unlock(&fe->readingLock);
}

void stopRead(map* mp, unsigned char key[], char* fileName) {
    // find existing
    int idx = __getExistingEntryIndex(mp, key);
    if (idx == -1) {
        puts("stopRead: idx == -1\n");
        return; // we shouldn't be here
    }

    mapEntry* me = mp->values[idx];

    // find existing or new index for file name
    int fileIdx = __getExistingFileNameIndex(me, fileName, mp->maxSize);

    if (fileIdx == -1) {
        puts("stopRead: fileIdx = -1\n");
        return; // we really shouldn't be here...
    }

    fileEntry* fe = &(me->fileEntries[fileIdx]);

    pthread_mutex_lock(&fe->readingLock);
    fe->readingCount--;

    pthread_mutex_lock(&fe->referenceLock);
    fe->referenceCount--;

    //__decrementReferenceCount(fe);
    if (fe->readingCount == 0) {
        if (fe->referenceCount == 0) {
            __removeFileName(me, fileIdx);
        } 
        pthread_mutex_unlock(&fe->writingLock);
    }
    pthread_mutex_unlock(&fe->referenceLock);

    pthread_mutex_unlock(&fe->readingLock);
}

void startWrite(map* mp, unsigned char key[], char* fileName) {
    // find existing
    int idx = __getExistingOrNewEntryIndex(mp, key);
    if (idx == -1) {
        puts("startWrite: idx == -1\n");
        return; // we shouldn't be here
    }

    if (mp->values[idx] == NULL) {
        printf("startWrite: allocate new map entry at %d\n", idx);
        mp->values[idx] = __createMapEntry(mp->maxSize, mp->arena);
        mp->keys[idx] = (char*)custom_malloc(mp->arena, strlen(key));
        strcpy(mp->keys[idx], key);
    }

    mapEntry* me = mp->values[idx];

    pthread_mutex_lock(&me->fileEntriesLock);

    // find existing or new index for file name
    int fileIdx = __getExistingOrNewFileNameIndex(me, fileName, mp->maxSize);

    if (fileIdx == -1) {
        puts("startWrite: fileIdx == -1\n");
        pthread_mutex_unlock(&me->fileEntriesLock);
        return; // we really shouldn't be here...
    }

    fileEntry* fe = &(me->fileEntries[fileIdx]);

    // set file name if it's not set
    if (fe->fileName == NULL) {
        fe->fileName = (char *)custom_malloc(mp->arena, strlen(fileName));
        strcpy(fe->fileName, fileName);
    }

    __incrementReferenceCount(fe);

    pthread_mutex_unlock(&me->fileEntriesLock);

    pthread_mutex_lock(&fe->writingLock);
}

void stopWrite(map* mp, unsigned char key[], char* fileName) {
    // find existing
    int idx = __getExistingEntryIndex(mp, key);
    if (idx == -1) {
        puts("stopWrite: idx == -1\n");
        return; // we shouldn't be here
    }

    mapEntry* me = mp->values[idx];

    // find existing or new index for file name
    int fileIdx = __getExistingFileNameIndex(me, fileName, mp->maxSize);

    if (fileIdx == -1) {
        puts("stopWrite: fileIdx == -1\n");
        return; // we really shouldn't be here...
    }

    fileEntry* fe = &(me->fileEntries[fileIdx]);

    //__decrementReferenceCount(fe);
    pthread_mutex_lock(&fe->referenceLock);
    
    fe->referenceCount--;

    if (fe->referenceCount == 0) {
        __removeFileName(me, fileIdx);
    }
    
    pthread_mutex_unlock(&fe->referenceLock);

    pthread_mutex_unlock(&fe->writingLock);
}
