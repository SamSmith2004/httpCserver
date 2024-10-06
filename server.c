#include "http_server.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// Parse the HTTP request and extrat the method, path, and body
HttpRequest parse_request(const char *request) {
    HttpRequest req = {0};
    const char *curr = request;
    const char *end;

    // Parse request line (e.g., "GET /path HTTP/1.1")
    end = strstr(curr, "\r\n"); // Gets the first occurence of "\r\n"
    if (end) {
        // When end is not NULL
        sscanf(curr, "%9s %255s", req.method, req.path); // scan the request line and store the method and path
        curr = end + 2;  // Move past \r\n
    }

    // Find the body (separated from headers by \r\n\r\n)
    end = strstr(curr, "\r\n\r\n");
    if (end) {
        req.body = (char *)end + 4;  // Point to start of body
    }

    return req;
}

void print_request(HttpRequest *req) {
    printf("Method: %s\n", req->method);
    printf("Path: %s\n", req->path);
    if (req->body && strlen(req->body) > 0) {
        printf("Body: %s\n", req->body);
    } else if (strcmp(req->method, "GET") == 0){
        // No body to print for GET requests
    } else {
        printf("Body: (empty)\n");
    }
}

int main() {
  int server_fd, new_socket;
  struct sockaddr_in address;
  int addrlen = sizeof(address);
  char buffer[BUFFER_SIZE] = {0};

  // Create TCP/IP socket
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    // AF_INET = IPv4, SOCK_STREAM = TCP, 0 = 'use default protocol for address
    // family & socket type'
    perror("Socket creation failed");
    return -1;
  }

  // Config server address
  address.sin_family = AF_INET;                     // Domain is IPv4
  address.sin_addr.s_addr = inet_addr("127.0.0.1"); // Listen on Localhost
  address.sin_port = htons(PORT);                   // Port 8080
  // sin = socket internet

  // bind() takes socket file descriptor, address to bind to, and size of address
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    // `(struct sockaddr *)` casts the address to a pointer of type `struct sockaddr`
    perror("Bind failed");
    return -1;
  }

  // Listen for incoming connections
  if (listen(server_fd, 3) == -1) { // 3 = max number of pending connections
    perror("Listen failed");
    return -1;
  }

  printf("Server listening on localhost:%d\n", PORT);

  int keep_running = 1;
  while (keep_running) {
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
        // cast address to pointer of type `struct sockaddr` and cast addrlen to pointer of type `socklen_t`
        perror("Accept failed");
        continue; // Try again on next iteration
    }

    // Read the request
    ssize_t bytes_read = read(new_socket, buffer, BUFFER_SIZE);
    if (bytes_read < 0) {
        perror("Read failed");
        close(new_socket);
        continue;
    }
    buffer[bytes_read] = '\0'; // Null terminate the buffer

    // Parse and print the request
    HttpRequest req = parse_request(buffer);
    print_request(&req);
    printf("---------------\n");

    // Prepare and send a simple HTTP response
    const char *response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 14\r\n\r\nSucess!\n";

    ssize_t bytes_written = write(new_socket, response, strlen(response));
    if (bytes_written < 0) {
      perror("Write failed"); // less than 0 bytes = failure
    } else if (bytes_written < strlen(response)) {
      fprintf(stderr, "Partial write occurred\n");
      // Not all bytes were written hence partial write
    }
    close(new_socket); // Close the client connection
  }
  close(server_fd); // Close the server socket
  return 0;
}
