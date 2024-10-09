#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void handle_client(int client_socket);

#endif
