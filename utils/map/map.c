#include "map.h"
#include <stdlib.h>
#include <string.h> 

map* createMap(int maxSize) 
{
    // Allocate memory for the map struct
    map *m = (map *)malloc(sizeof(map));
    m->size = 0;
	m->maxSize = maxSize;

    m->keys = (unsigned char **)malloc(maxSize * sizeof(unsigned char *));

    for (int i = 0; i < maxSize; i++) 
	{
        m->keys[i] = (unsigned char *)malloc(shaLength * sizeof(unsigned char));
        memset(m->keys[i], 0, shaLength);  // Initialize the memory to 0
    }

    // Allocate memory for values array
    m->values = (int *)malloc(maxSize * sizeof(int));
    memset(m->values, 0, maxSize * sizeof(int)); 

    return m;
}

int getIndex(map * mp, unsigned char key[]) 
{ 
	for (int i = 0; i < mp->size; i++) 
    { 
		if (strcmp(mp->keys[i], key) == 0) 
        { 
			return i; 
		} 
	} 
	return -1;
} 

void increment(map* mp, unsigned char key[]) 
{ 
	int index = getIndex(mp, key); 
	if (index == -1) 
    {
		strcpy(mp->keys[mp->size], key); 
		mp->values[mp->size] = 1; 
		mp->size++; 
	} 
	else 
    {
		mp->values[index] = mp->values[index]+1; 
	} 
} 

int get(map* mp, unsigned char key[]) 
{ 
	int index = getIndex(mp, key); 
	if (index == -1) 
    {
		return -1; 
	} 
	else 
    {
		return mp->values[index]; 
	} 
} 

void decrement(map* mp, unsigned char key[]) 
{ 
	int index = getIndex(mp, key); 
	if (index == -1) 
    {
        return;
	} 
	else 
    {
		mp->values[index] = mp->values[index]-1;

        if(mp->values[index] == 0)
        {
            for (int i = index; i < mp->size - 1; i++) {
                memcpy(mp->keys[i], mp->keys[i + 1], shaLength);
                mp->values[i] = mp->values[i + 1];
            }
            mp->size--;
        } 
	} 
}

void freeMap(map *mp) 
{
    if (mp) 
	{
        for (int i = 0; i < mp->maxSize; i++) 
		{
            free(mp->keys[i]);
        }
        free(mp->keys);
        free(mp->values);
        free(mp);
    }
}
