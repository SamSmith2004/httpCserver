#include "http_server.h"
#include "thread_pool.h"
#include "request_handler.h"
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

// volatile is used to ensure changes made by the signal handler are immediately visible in the main loop
// prevents missing the signal to stop the server
volatile sig_atomic_t keep_running = 1;
// sig_atomic_t is an atomically accessible int that always performs a single, uninterruptible operation

void sigint_handler(int sig) {
    keep_running = 0;
}

void cleanup(int server_fd) { // Shutdown
  printf("\nShutting down the server... \n");
  close(server_fd);
}

void handle_client(int client_socket) {
  char buffer[BUFFER_SIZE] = {0};

  // Read the request
  ssize_t bytes_read = read(client_socket, buffer, BUFFER_SIZE);
  if (bytes_read < 0) {
    perror("Read failed");
    close(client_socket);
    return;
  }
  buffer[bytes_read] = '\0'; // Null terminate the buffer

  // Parse and print the request
  HttpRequest req = parse_request(buffer);
  print_request(&req);
  printf("---------------\n");

  int index = find_or_create_endpoint(req.path);

  if (index != -1) {
     // Get the endpoint data
     char* endpoint_data = get_endpoint_data(index);

     // Prepare and send the HTTP response
     char response[BUFFER_SIZE];
     if (endpoint_data != NULL) {
       snprintf(response, BUFFER_SIZE,
                "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %zu\r\n\r\n%s\n",
                strlen(endpoint_data) + 1, endpoint_data);
       free(endpoint_data);
     } else {
       snprintf(response, BUFFER_SIZE,
                "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n\r\nNo data found\n");
     }

     ssize_t bytes_written = write(client_socket, response, strlen(response));
     if (bytes_written < 0) {
       perror("Write failed");
     } else if (bytes_written < strlen(response)) {
       fprintf(stderr, "Partial write occurred\n");
     }
   } else {
     // Handle the case when no endpoint was found or created
     const char *error_response = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\nContent-Length: 21\r\n\r\nInternal Server Error\n";
     write(client_socket, error_response, strlen(error_response));
   }

  close(client_socket);
}

// Setup scoket, bind, listen, accept, and handle client
int main() {
  int server_fd, new_socket;
  struct sockaddr_in address;
  int addrlen = sizeof(address);

  // Create TCP/IP socket
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    // AF_INET = IPv4, SOCK_STREAM = TCP, 0 = 'use default protocol for address
    // family & socket type'
    perror("Socket creation failed");
    return -1;
  }

  // Set SO_REUSEADDR, allows server to bind to address that was recently used by another socket
  int opt = 1;
  // preventing "Address already in use" errors
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
      // SOL_SOCKET is the socket layer itself
      // &opt is a pointer to the option value (1 means enable)
      perror("setsockopt SO_REUSEADDR failed");
      return -1;
  }

  // Config server address
  address.sin_family = AF_INET;                     // Domain is IPv4
  address.sin_addr.s_addr = inet_addr("127.0.0.1"); // Listen on Localhost
  address.sin_port = htons(PORT);                   // Port 8080
  // sin = socket internet

  // bind() takes socket file descriptor, address to bind to, and size of
  // address
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    // `(struct sockaddr *)` casts the address to a pointer of type `struct
    // sockaddr`
    perror("Bind failed");
    return -1;
  }

  // Listen for incoming connections
  if (listen(server_fd, 3) == -1) { // 3 = max number of pending connections
    perror("Listen failed");
    return -1;
  }

  // Set up signal handling
  signal(SIGINT, sigint_handler);

  thread_pool_t* pool = thread_pool_create(6); // Create a thread pool with 6 threads
  if (pool == NULL) {
      perror("Failed to create thread pool");
      return -1;
  }

  printf("Server listening on localhost:%d\n", PORT);
  printf("Press Ctrl+C to stop the server.\n");

  while (keep_running) {
      // Set a timeout for the accept() call
      struct timeval tv = {.tv_sec = 1, .tv_usec = 0};
      // tv specifies the timeout: 1 second and 0 microseconds (tv_usec)
      if (setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) < 0) {
          // SO_RCVTIMEO sets a timeout for receive operations ex accept()
          // allows the server to periodically check the keep_running flag
          perror("setsockopt SO_RCVTIMEO failed");
          continue;
      }
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,(socklen_t *)&addrlen)) < 0) {
      // cast address to pointer of type `struct sockaddr` and cast addrlen to
      // pointer of type `socklen_t`
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
        // EWOULDBLOCK and EAGAIN indicate a timeout
        continue; // continue to check 'keep_running' again
      }
      perror("Accept failed");
      continue; // Try again on next iteration
    }

    // Add the new socket to the thread pool
    thread_pool_add_task(pool, new_socket);
  }
  thread_pool_destroy(pool);
  cleanup(server_fd); // Close the server socket
  return 0;
}
