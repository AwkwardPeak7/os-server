#include "map.h"
#include <stdlib.h>
#include <string.h> 

map* createMap()
{
    map * mp= (map *)malloc(sizeof(map));
    if (mp == NULL) {
        return NULL;
    }

    mp->size=0;

    return mp;
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