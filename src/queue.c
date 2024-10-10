#include "queue.h"
#include <stdio.h>

task_queue_t* create_queue() {
    task_queue_t *queue = (task_queue_t*)malloc(sizeof(task_queue_t));
    if (queue == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    queue->front = queue->rear = NULL;
    return queue;
}

void enqueue(task_queue_t *queue, int value) {
    task_node_t *new_node = (task_node_t*)malloc(sizeof(task_node_t));
    if (new_node == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    new_node->client_socket = value;
    new_node->next = NULL;

    if (is_empty(queue)) {
        queue->front = queue->rear = new_node;
    } else {
        queue->rear->next = new_node;
        queue->rear = new_node;
    }
}

int dequeue(task_queue_t *queue) {
    if (is_empty(queue)) {
        fprintf(stderr, "Queue is empty\n");
        exit(1);
    }
    task_node_t *temp = queue->front;
    int value = temp->client_socket;
    queue->front = queue->front->next;
    free(temp);

    if (queue->front == NULL) {
        queue->rear = NULL;
    }
    return value;
}

int is_empty(task_queue_t *queue) {
    return (queue->front == NULL);
}

void print_queue(task_queue_t *queue) {
    task_node_t *current = queue->front;
    while (current != NULL) {
        printf("%d ", current->client_socket);
        current = current->next;
    }
    printf("\n");
}

void free_queue(task_queue_t *queue) {
    while (!is_empty(queue)) {
        dequeue(queue);
    }
    free(queue);
}
