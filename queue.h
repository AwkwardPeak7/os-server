#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct queueCell
{
    char* command;
}queueCell;

typedef struct Queue 
{
    queueCell *array;
    int capacity;
    int size;
    int front;
    int rear;
} Queue;

Queue *createQueue(int initialCapacity)
{
    Queue *queue = (Queue *)malloc(sizeof(Queue));
    if (queue == NULL) {
        printf("Memory allocation failed\n");
        return NULL;
    }
    queue->array = (queueCell *)malloc(initialCapacity * sizeof(queueCell));
    if (queue->array == NULL) 
    {
        printf("Memory allocation failed\n");
        free(queue);
        return NULL;
    }
    queue->capacity = initialCapacity;
    queue->front = -1;
    queue->rear = -1;
    queue->size = 0;
    return queue;
}

void enqueue(Queue *queue, queueCell* element) 
{
    if (queue->size == queue->capacity) 
    {
        printf("Queue is full\n");
        return;
    }
    if (queue->size == 0)
    {
        queue->front = 0;
    }
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->array[queue->rear].command = strdup(element->command); // Copy the command string
    queue->size++;
}

queueCell *dequeue(Queue *queue) 
{
    if (queue->size == 0) 
    {
        printf("Queue is empty\n");
        return NULL;
    }
    queueCell *dequeued = &queue->array[queue->front];
    if (queue->size == 1) {
        queue->front = -1;
        queue->rear = -1;
    } 
    else 
    {
        queue->front = (queue->front + 1) % queue->capacity;
    }
    queue->size--;
    return dequeued;
}

char *peek(Queue *queue) 
{
    if (queue->size == 0) 
    {
        printf("Queue is empty\n");
        return NULL;
    }
    return queue->array[queue->front].command;
}

int size(Queue *queue) 
{
    return queue->size;
}

void freeQueue(Queue *queue) 
{
    for (int i = 0; i < queue->size; i++) 
    {
        free(queue->array[i].command);
    }
    free(queue->array);
    free(queue);
}