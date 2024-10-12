#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>

#define MAX_HEADERS 20
#define MAX_HEADER_LENGTH 200
#define MAX_ENDPOINTS 10
#define MAX_PATH_LENGTH 10

typedef struct {
    char method[10];
    char path[256];
    char version[10];
    char headers[MAX_HEADERS][MAX_HEADER_LENGTH];
    int header_count;
    const char *body;
} HttpRequest;

typedef struct {
    char path[MAX_PATH_LENGTH];
    char *data;
    pthread_mutex_t mutex;
} Endpoint;

HttpRequest parse_request(const char *request);
int find_or_create_endpoint(const char* path);
char* get_endpoint_data(int index);
void print_request(HttpRequest *req);

#endif
