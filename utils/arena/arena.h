#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#define BINS 5

typedef struct {
    unsigned int totalThreads;
    size_t bin_sizes[BINS];
    bool* free_bins_map[BINS];
    void* binsArray[BINS];
    pthread_mutex_t mutexes[BINS];
    pthread_mutex_t extraMutex;

    bool* extraMemory;
    void* extraBin;
    bool extraEnabled;
} Arena;

Arena* create_arena(unsigned int totalThreads);
void destroy_arena(Arena* arena);

void* custom_malloc(Arena* arena, size_t size);
void custom_free(Arena* arena, void* ptr);
void print_bool_pool(Arena* arena);

void allocate_extra_bin(Arena* arena);
bool is_bin_full(Arena* arena, int bin);

#endif // ARENA_H
