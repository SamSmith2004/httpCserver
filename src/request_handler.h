#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include <stdio.h>
#include <string.h>

#define MAX_HEADERS 20
#define MAX_HEADER_LENGTH 200

typedef struct {
    char method[10];
    char path[256];
    char version[10];
    char headers[MAX_HEADERS][MAX_HEADER_LENGTH];
    int header_count;
    const char *body;
} HttpRequest;

HttpRequest parse_request(const char *request);
void print_request(HttpRequest *req);

#endif
