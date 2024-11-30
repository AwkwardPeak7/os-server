#include "arena.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

Arena* create_arena(unsigned int _totalThreads)
{
    Arena* arena = (Arena*)malloc(sizeof(Arena));
    arena->totalThreads = _totalThreads;//*2;
    arena->extraEnabled = false;

    // Set bins size in bytes
    arena->bin_sizes[0] = 16;
    arena->bin_sizes[1] = 64;
    arena->bin_sizes[2] = 256;
    arena->bin_sizes[3] = 512;
    arena->bin_sizes[4] = 1024;
    for (int i = 0; i < BINS; i++) 
    {
        arena->free_bins_map[i] = (bool*)malloc(arena->totalThreads * sizeof(bool));
        memset(arena->free_bins_map[i], 1, arena->totalThreads * sizeof(bool));
        arena->binsArray[i] = malloc(arena->bin_sizes[i] * arena->totalThreads);
        arena->mutexes[i] = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    }

    arena->extraMutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

    return arena;
}

void destroy_arena(Arena* arena)
{
    for (int i = 0; i < BINS; i++) 
    {
        free(arena->free_bins_map[i]);
        free(arena->binsArray[i]);
        pthread_mutex_destroy(&arena->mutexes[i]);
    }
    free(arena);
}

bool is_bin_full(Arena* arena, int bin)
{
    for (int i = 0; i < arena->totalThreads; i++) 
    {
        if (arena->free_bins_map[bin][i]) 
        {
            return false;
        }
    }
    return true;
}

void* custom_malloc(Arena* arena, size_t size)
{
    int bin = -1;
    for (int i = 0; i < BINS; i++) 
    {

        if (size <= arena->bin_sizes[i]) 
        {
            if(is_bin_full(arena, i))
            {
                continue;
            }
            bin = i;
            break;
        }
    }

    if(bin == -1)
    {
        if(arena->extraEnabled == false)
        {
            pthread_mutex_lock(&arena->extraMutex);
            if(arena->extraEnabled == false)
            {
                allocate_extra_bin(arena);
                arena->extraEnabled = true;
            }
            pthread_mutex_unlock(&arena->extraMutex);
        }

        pthread_mutex_lock(&arena->extraMutex);
        for (int i = 0; i < arena->totalThreads; i++) 
        {
            if (arena->extraMemory[i]) 
            {
            arena->extraMemory[i] = false;
            pthread_mutex_unlock(&arena->extraMutex);
            return (char*)arena->extraMemory + (128 * i);
            }
        }
        pthread_mutex_unlock(&arena->extraMutex);

        printf("Insufficient memory. Requested size: %zu bytes\n", size);
        return NULL;
    }

    pthread_mutex_lock(&arena->mutexes[bin]);
    for (int i = 0; i < arena->totalThreads; i++) 
    {
        if (arena->free_bins_map[bin][i]) 
        {
            arena->free_bins_map[bin][i] = false;
            pthread_mutex_unlock(&arena->mutexes[bin]);
            return (char*)arena->binsArray[bin] + (arena->bin_sizes[bin] * i);
        }
    }
    pthread_mutex_unlock(&arena->mutexes[bin]);

    printf("Insufficient memory. Requested size: %zu bytes\n", size);

    return NULL;
}

void print_bool_pool(Arena* arena)
{
    for (int i = 0; i < BINS; i++) 
    {
        for (int j = 0; j < arena->totalThreads; j++) {
            printf("%d ", arena->free_bins_map[i][j]);
        }
        printf("\n");
    }
}

void custom_free(Arena* arena, void* ptr)
{
    for (int i = 0; i < BINS; i++) 
    {
        if (ptr >= arena->binsArray[i] && ptr < (char*)arena->binsArray[i] + (arena->bin_sizes[i] * arena->totalThreads)) 
        {
            int index = ((char*)ptr - (char*)arena->binsArray[i]) / arena->bin_sizes[i];
            pthread_mutex_lock(&arena->mutexes[i]);
            arena->free_bins_map[i][index] = true;
            pthread_mutex_unlock(&arena->mutexes[i]);
            break;
        }
    }

    if(arena->extraEnabled)
    {
        for (int i = 0; i < arena->totalThreads; i++) 
        {
            if (ptr == (char*)arena->extraMemory + (128 * i)) 
            {
                pthread_mutex_lock(&arena->extraMutex);
                arena->extraMemory[i] = true;
                pthread_mutex_unlock(&arena->extraMutex);
                break;
            }
        }
    }
}

void allocate_extra_bin(Arena* arena)
{
    printf("Allocating extra memory bin\n");
    arena->extraBin = malloc(arena->totalThreads * sizeof(bool));
    memset(arena->extraBin, 1, arena->totalThreads * sizeof(bool));
    arena->extraMemory = malloc(128 * arena->totalThreads);
}
