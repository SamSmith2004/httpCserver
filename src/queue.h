#ifndef QUEUE_H
#define QUEUE_H

#include <stdlib.h>

typedef struct task_node task_node_t;
typedef struct task_queue task_queue_t;

task_queue_t* create_queue();
void enqueue(task_queue_t *queue, int value);
int dequeue(task_queue_t *queue);
int is_empty(task_queue_t *queue);
void print_queue(task_queue_t *queue);
void free_queue(task_queue_t *queue);

struct task_node {
    int client_socket;
    struct task_node *next;
};

struct task_queue {
    task_node_t *front;
    task_node_t *rear;
};

#endif
