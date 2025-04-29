#include "lab.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// queue structure
struct queue {
    void **buffer;          
    int capacity;           
    int size;               
    int front;              
    int rear;               
    bool shutdown;          
    pthread_mutex_t lock;  
    pthread_cond_t not_empty; 
    pthread_cond_t not_full; 
};

// Initialize a new queue
queue_t queue_init(int capacity) {
    if (capacity <= 0) {
        return NULL;
    }

    queue_t q = malloc(sizeof(struct queue));
    if (!q) {
        return NULL;
    }

    q->buffer = malloc(sizeof(void *) * capacity);
    if (!q->buffer) {
        free(q);
        return NULL;
    }

    q->capacity = capacity;
    q->size = 0;
    q->front = 0;
    q->rear = 0;
    q->shutdown = false;

    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);

    return q;
}

// Frees memory and signals waiting threads
void queue_destroy(queue_t q) {
    if (!q) {
        return;
    }

    pthread_mutex_lock(&q->lock);

    q->shutdown = true;
    pthread_cond_broadcast(&q->not_empty);
    pthread_cond_broadcast(&q->not_full);

    pthread_mutex_unlock(&q->lock);

    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);

    free(q->buffer);
    free(q);
}

// Adds to the back of the queue
void enqueue(queue_t q, void *data) {
    if (!q || !data) {
        return;
    }

    pthread_mutex_lock(&q->lock);

    while (q->size == q->capacity && !q->shutdown) {
        pthread_cond_wait(&q->not_full, &q->lock);
    }

    if (q->shutdown) {
        pthread_mutex_unlock(&q->lock);
        return;
    }

    q->buffer[q->rear] = data;
    q->rear = (q->rear + 1) % q->capacity;
    q->size++;

    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
}

// Removes the first element in the queue
void *dequeue(queue_t q) {
    if (!q) {
        return NULL;
    }

    pthread_mutex_lock(&q->lock);

    while (q->size == 0 && !q->shutdown) {
        pthread_cond_wait(&q->not_empty, &q->lock);
    }

    if (q->size == 0 && q->shutdown) {
        pthread_mutex_unlock(&q->lock);
        return NULL;
    }

    void *data = q->buffer[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->size--;

    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->lock);

    return data;
}

// Set the shutdown flag in the queue so threads can complete
void queue_shutdown(queue_t q) {
    if (!q) {
        return;
    }

    pthread_mutex_lock(&q->lock);

    q->shutdown = true;
    pthread_cond_broadcast(&q->not_empty);
    pthread_cond_broadcast(&q->not_full);

    pthread_mutex_unlock(&q->lock);
}

// Returns true if the queue is empty
bool is_empty(queue_t q) {
    if (!q) {
        return true;
    }

    pthread_mutex_lock(&q->lock);
    bool empty = (q->size == 0);
    pthread_mutex_unlock(&q->lock);

    return empty;
}

// Returns true if the queue is shutdown
bool is_shutdown(queue_t q) {
    if (!q) {
        return true;
    }

    pthread_mutex_lock(&q->lock);
    bool shutdown = q->shutdown;
    pthread_mutex_unlock(&q->lock);

    return shutdown;
}