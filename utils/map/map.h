#ifndef MAP_H
#define MAP_H

#define shaLength 32

typedef struct map
{
    int size;
    int maxSize;
    unsigned char **keys;
    int *values;   

} map;

map* createMap(int maxSize);
int getIndex(map * mp, unsigned char key[]);
void increment(map* mp, unsigned char key[]);
int get(map* mp, unsigned char key[]);
void decrement(map* mp, unsigned char key[]);
void freeMap(map* mp);

#endif