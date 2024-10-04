#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <queue.h>

int main() {
    // Create a queue with a small capacity to test edge cases
    int capacity = 3;
    Queue *queue = createQueue(capacity);

    if (queue == NULL) {
        printf("Failed to create queue\n");
        return 1;
    }

    printf("Testing queue operations:\n");

    // Test dequeue on an empty queue
    printf("Dequeue on empty queue:\n");
    queueCell *dequeued = dequeue(queue);
    if (dequeued == NULL) {
        printf("Correctly handled dequeue on empty queue.\n");
    }

    // Test peek on an empty queue
    printf("Peek on empty queue:\n");
    char *peekedCommand = peek(queue);
    if (peekedCommand == NULL) {
        printf("Correctly handled peek on empty queue.\n");
    }

    // Test enqueueing elements
    printf("Enqueueing elements...\n");

    queueCell element1 = { .command = "Command 1" };
    enqueue(queue, &element1);
    printf("Enqueued: %s\n", element1.command);

    queueCell element2 = { .command = "Command 2" };
    enqueue(queue, &element2);
    printf("Enqueued: %s\n", element2.command);

    queueCell element3 = { .command = "Command 3" };
    enqueue(queue, &element3);
    printf("Enqueued: %s\n", element3.command);

    // Test enqueue on full queue
    printf("Enqueue on full queue:\n");
    queueCell element4 = { .command = "Command 4" };
    enqueue(queue, &element4);

    // Test peek when queue is not empty
    printf("Peek on non-empty queue:\n");
    peekedCommand = peek(queue);
    if (peekedCommand != NULL) {
        printf("Peeked command: %s\n", peekedCommand);
    }

    // Test dequeue
    printf("Dequeueing elements...\n");
    dequeued = dequeue(queue);
    if (dequeued != NULL) {
        printf("Dequeued: %s\n", dequeued->command);
    }

    dequeued = dequeue(queue);
    if (dequeued != NULL) {
        printf("Dequeued: %s\n", dequeued->command);
    }

    dequeued = dequeue(queue);
    if (dequeued != NULL) {
        printf("Dequeued: %s\n", dequeued->command);
    }

    // Test dequeue on an empty queue again
    printf("Dequeue on empty queue:\n");
    dequeued = dequeue(queue);
    if (dequeued == NULL) {
        printf("Correctly handled dequeue on empty queue.\n");
    }

    // Free queue resources
    freeQueue(queue);
    
    return 0;
}
