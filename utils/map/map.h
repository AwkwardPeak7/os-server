#ifndef MAP_H
#define MAP_H

// TODO: dynamically allocate memory for keys and values
#define MAXSIZE 10
#define shaLength 32

typedef struct map
{
    int size;
    unsigned char keys[MAXSIZE][shaLength];
    int values [MAXSIZE];

} map;

map* createMap();
int getIndex(map * mp, unsigned char key[]);
void increment(map* mp, unsigned char key[]);
int get(map* mp, unsigned char key[]);
void decrement(map* mp, unsigned char key[]);
//void freeMap(map* mp);

#endif