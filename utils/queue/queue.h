#ifndef QUEUE_H
#define QUEUE_H

typedef struct queueCell
{
    char* command;
} queueCell;

typedef struct Queue 
{
    queueCell *array;
    int capacity;
    int size;
    int front;
    int rear;
} Queue;

Queue *createQueue(int initialCapacity);
void enqueue(Queue *queue, queueCell* element);
queueCell *dequeue(Queue *queue);
char *peek(Queue *queue);
int size(Queue *queue);
void freeQueue(Queue *queue);

#endif