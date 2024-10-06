#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <stdint.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_HEADERS 20
#define MAX_HEADER_LENGTH 200

typedef struct {
    char method[10];
    char path[256];
    char version[10];
    char headers[MAX_HEADERS][MAX_HEADER_LENGTH];
    int header_count;
    char *body;
} HttpRequest;

HttpRequest parse_request(const char *request);
void print_request(HttpRequest *req);

#endif
