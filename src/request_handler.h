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

struct content_type {
    const char *extension;
    const char *type;
};

typedef struct {
    char method[10];
    char path[256];
    char version[10];
    char headers[MAX_HEADERS][MAX_HEADER_LENGTH];
    int header_count;
    const char *body;
    int response_code;
} HttpRequest;

typedef struct {
    int status_code;
    char status_message[32];
    char headers[MAX_HEADERS][MAX_HEADER_LENGTH];
    int header_count;
    char *body;
    size_t body_length;
} HttpResponse;

typedef struct {
    char path[MAX_PATH_LENGTH];
    char *data;
    pthread_mutex_t mutex;
} Endpoint;

const char *get_status_message(int status_code);
HttpRequest parse_request(const char *request);
int find_or_create_endpoint(const char* path);
char* get_endpoint_data(int index);
HttpResponse *create_response(int status_code, const char **headers, int header_count);
void set_response_body(HttpResponse *response, const char *body, size_t body_length);
char *serialize_response(HttpResponse *response, size_t *total_length);
void update_response_status(HttpResponse *response, int status_code, const char *status_message);
void free_response(HttpResponse *response);
void print_request(HttpRequest *req);
void free_endpoint_data();

#endif
