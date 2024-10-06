#include <stdio.h> 
#include <stdlib.h>
#include <string.h> 

#define MAXSIZE 10
#define shaLength 32

typedef struct map
{
    int size;
    unsigned char keys[MAXSIZE][shaLength];
    int values [MAXSIZE];

} map;

map* createMap()
{
    map * mp= (map *)malloc(sizeof(map));
    if (mp == NULL) {
        printf("Memory allocation failed\n");
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
        printf("SHA doesnt exist \n"); 
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

//Testing MAIN

// int main() {
//     map* myMap = createMap();
//     if (myMap == NULL) {
//         return 1; // Exit if memory allocation failed
//     }

//     // Incrementing keys
//     printf("Incrementing keys...\n");
//     increment(myMap, "abc123");
//     increment(myMap, "def456");
//     increment(myMap, "abc123"); // Incrementing an existing key

//     // Checking the map values
//     printf("\nValues in map:\n");
//     printf("abc123: %d\n", get(myMap, "abc123"));
//     printf("def456: %d\n", get(myMap, "def456"));
//     printf("xyz789: %d (non-existent key)\n", get(myMap, "xyz789")); // Non-existent key

//     // Decrementing keys
//     printf("\nDecrementing keys...\n");
//     decrement(myMap, "abc123"); // Should decrease the value
//     printf("abc123: %d\n", get(myMap, "abc123"));

//     decrement(myMap, "def456"); // Should remove the key after decrement
//     decrement(myMap, "def456"); // Should print "SHA doesn't exist"

//     // Checking final state of the map
//     printf("\nFinal values in map:\n");
//     for (int i = 0; i < myMap->size; i++) {
//         printf("%s: %d\n", myMap->keys[i], myMap->values[i]);
//     }

//     free(myMap); // Free allocated memory
//     return 0;
// }