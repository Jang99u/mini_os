#include "../header/header.h"

void initQueue(Queue* queue) {
    queue->front = 0;
    queue->rear = -1;
    queue->size = 0;
}

int isEmpty(Queue* queue) {
    return queue->size == 0;
}

int isFull(Queue* queue) {
    return queue->size == MAX_QUEUE_SIZE;
}

void enqueue(Queue* queue, const char* str) {
    if (isFull(queue)) {
        fprintf(stderr, "Queue overflow\n");
        return;
    }
    queue->rear = (queue->rear + 1) % MAX_QUEUE_SIZE;
    queue->items[queue->rear] = strdup(str);
    queue->size++;
}

char* dequeue(Queue* queue) {
    if (isEmpty(queue)) {
        fprintf(stderr, "Queue underflow\n");
        return NULL;
    }
    char* str = queue->items[queue->front];
    queue->front = (queue->front + 1) % MAX_QUEUE_SIZE;
    queue->size--;
    return str;
}

char* peek(Queue* queue) {
    if (isEmpty(queue)) {
        fprintf(stderr, "Queue is empty\n");
        return NULL;
    }
    return queue->items[queue->front];
}

void freeQueue(Queue* queue) {
    while (!isEmpty(queue)) {
        free(dequeue(queue));
    }
}

// 문자열을 '/' 문자를 기반으로 큐에 집어 넣어 큐를 형성
void buildQueue(Queue* queue, char* queuestr) {
    if(queuestr == NULL) {
        return;
    } else {
        enqueue(queue, queuestr);
        queuestr = strtok(NULL, "/");
        buildQueue(queue, queuestr);
    }
}