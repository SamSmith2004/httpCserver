#include "http_server.h"
#include "request_handler.h"
#include "thread_pool.h"
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

// volatile is used to ensure changes made by the signal handler are immediately
// visible in the main loop prevents missing the signal to stop the server
volatile sig_atomic_t keep_running = 1;
// sig_atomic_t is an atomically accessible int that always performs a single,
// uninterruptible operation

void sigint_handler(int sig) { keep_running = 0; }

void cleanup(int server_fd) { // Shutdown
  printf("\nShutting down the server... \n");
  close(server_fd);
  free_endpoint_data();
}

void handle_client(int client_socket) {
  char buffer[BUFFER_SIZE] = {0};
  HttpResponse *response = NULL;
  int index = -1;
  int mutex_locked = 0;

  ssize_t bytes_read = read(client_socket, buffer, BUFFER_SIZE);
  if (bytes_read < 0) {
    perror("Read failed");
    response = create_response(500, NULL, 0);
    set_response_body(response, "Internal Server Error\n", 22);
    goto send_response;
  }
  buffer[bytes_read] = '\0'; // Null terminate the buffer

  // Parse and print the request
  HttpRequest req = parse_request(buffer);
  print_request(&req);
  printf("---------------\n");

  index = find_or_create_endpoint(req.path);
  if (index == -1) {
    response = create_response(500, NULL, 0);
    set_response_body(response, "Internal Server Error\n", 22);
    goto send_response;
  }

  pthread_mutex_lock(&endpoints[index].mutex);
  mutex_locked = 1;

  if (strcmp(req.method, "GET") == 0) {
    if (endpoints[index].file != NULL && endpoints[index].is_binary) {
      // Read from the file and send its contents
      char buffer[1024];
      size_t bytes_read;

      // Send headers first
      const char *headers[] = {"Content-Type: application/octet-stream",
                               "Transfer-Encoding: chunked"};
      response = create_response(200, headers, 2);
      size_t response_length;
      char *serialized_response =
          serialize_response(response, &response_length);

      size_t bytes_serial =
          write(client_socket, serialized_response, response_length);
      if (bytes_serial < 0) {
        perror("Failed to write serialized response\n");
        goto send_response;
      } else if (bytes_serial < response_length) {
        perror("Pariak write occurred when sending serialized response\n");
        goto send_response;
      }
      free(serialized_response);
      free_response(response);
      response = NULL;

      // Send file contents in chunks
      rewind(endpoints[index].file);
      while ((bytes_read = fread(buffer, 1, sizeof(buffer),
                                 endpoints[index].file)) > 0) {
        char chunk_header[16];
        snprintf(chunk_header, sizeof(chunk_header), "%zx\r\n", bytes_read);

        ssize_t bytes_written_h =
            write(client_socket, chunk_header, strlen(chunk_header));
        if (bytes_written_h < 0) {
          perror("Failed to write chunk header");
          goto send_response;
        } else if (bytes_written_h < strlen(chunk_header)) {
          perror("Pariak write occurred when sending chunk header\n");
          goto send_response;
        }

        ssize_t bytes_written_b = write(client_socket, buffer, bytes_read);
        if (bytes_written_b < 0) {
          perror("Failed to write buffer");
          goto send_response;
        } else if (bytes_written_b < bytes_read) {
          perror("Pariak write occurred when sending buffer\n");
          goto send_response;
        }

        ssize_t bytes_written_e = write(client_socket, "\r\n", 2);
        if (bytes_written_e < 0) {
          perror("Failed to write '\r\n'");
          goto send_response;
        } else if (bytes_written_e < 2) {
          perror("Pariak write occurred when sending '\r\n'\n");
          goto send_response;
        }
      }

      // Send the final chunk
      ssize_t bytes_written = write(client_socket, "0\r\n\r\n", 5);
      if (bytes_written < 0) {
        perror("Failed to write final chunk");
        goto send_response;
      } else if (bytes_written < 5) {
        perror("Partial write occurred when sending final chunk\n");
        goto send_response;
      }
      goto send_response;

    } else if (endpoints[index].data != NULL && !endpoints[index].is_binary) {
      // Handle text data
      char *endpoint_data = get_endpoint_data(index);
      if (endpoint_data != NULL) {
        const char *content_type = "Content-Type: text/plain";
        // Will add better content type detection later
        if (strstr(endpoint_data, "{") != NULL &&
            strstr(endpoint_data, "}") != NULL) {
          content_type = "Content-Type: application/json";
        }

        const char *headers[] = {content_type, "Server: MyServer/1.0"};
        response = create_response(200, headers, 2);
        set_response_body(response, endpoint_data, strlen(endpoint_data));
        free(endpoint_data);
      } else {
        response = create_response(500, NULL, 0);
        set_response_body(response, "Internal Server Error\n", 22);
      }
    } else {
      response = create_response(404, NULL, 0);
      set_response_body(response, "Not Found\n", 10);
    }
  } else if (strcmp(req.method, "DELETE") == 0) {
    if (endpoints[index].data != NULL) {
      free(endpoints[index].data);
      endpoints[index].data = NULL;
      response = create_response(204, NULL, 0);
    } else {
      response = create_response(404, NULL, 0);
      set_response_body(response, "Not Found\n", 10);
    }
  } else {
    // Handle other methods (POST, PUT, etc.)
    response = create_response(req.response_code, NULL, 0);
    set_response_body(response, get_status_message(req.response_code),
                      strlen(get_status_message(req.response_code)));
  }
  goto send_response;

send_response:
  if (mutex_locked && index != -1) {
    pthread_mutex_unlock(&endpoints[index].mutex);
  }
  if (response == NULL) {
    response = create_response(500, NULL, 0);
    set_response_body(response, "Internal Server Error\n", 22);
  }

  char *content_length = NULL;
  int content_length_size =
      snprintf(NULL, 0, "Content-Length: %zu", response->body_length) + 1;
  content_length = (char *)malloc(content_length_size);
  if (content_length != NULL) {
    snprintf(content_length, content_length_size, "Content-Length: %zu",
             response->body_length);
    if (response->header_count < MAX_HEADERS) {
      strncpy(response->headers[response->header_count], content_length,
              MAX_HEADER_LENGTH - 1);
      response->headers[response->header_count][MAX_HEADER_LENGTH - 1] = '\0';
      response->header_count++;
    }
    free(content_length);
  }

  size_t response_length;
  char *serialized_response = serialize_response(response, &response_length);
  if (serialized_response == NULL) {
    fprintf(stderr, "Failed to serialize response\n");
    goto send_response;
  }
  ssize_t bytes_written =
      write(client_socket, serialized_response, response_length);
  if (bytes_written < 0) {
    perror("Failed to write response");
  } else if ((size_t)bytes_written < response_length) {
    perror("Partial write occurred when sending response");
  }
  free(serialized_response);

  free_response(response);
  close(client_socket);
}

// Setup scoket, bind, listen, accept, and handle client
int main() {
  int server_fd, new_socket;
  struct sockaddr_in address;
  int addrlen = sizeof(address);

  // Create TCP/IP socket
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    // AF_INET = IPv4, SOCK_STREAM = TCP, 0 = 'use default protocol for
    // address family & socket type'
    perror("Socket creation failed");
    return -1;
  }

  // Set SO_REUSEADDR, allows server to bind to address that was recently
  // used by another socket
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

  thread_pool_t *pool =
      thread_pool_create(6); // Create a thread pool with 6 threads
  if (pool == NULL) {
    perror("Failed to create thread pool");
    return -1;
  }

  printf("Server listening on localhost:%d\n", PORT);
  printf("Server PID: %d\n", getpid());
  printf("Press Ctrl+C to stop the server.\n");

  while (keep_running) {
    // Set a timeout for the accept() call
    struct timeval tv = {.tv_sec = 1, .tv_usec = 0};
    // tv specifies the timeout: 1 second and 0 microseconds (tv_usec)
    if (setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv,
                   sizeof tv) < 0) {
      // SO_RCVTIMEO sets a timeout for receive operations ex accept()
      // allows the server to periodically check the keep_running flag
      perror("setsockopt SO_RCVTIMEO failed");
      continue;
    }
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                             (socklen_t *)&addrlen)) < 0) {
      // cast address to pointer of type `struct sockaddr` and cast addrlen
      // to pointer of type `socklen_t`
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
        // EWOULDBLOCK and EAGAIN indicate a timeout
        continue; // continue to check 'keep_running' again
      }
      if (errno == EINTR) {
        // This can happen when a signal is received (like Ctrl+C)
        continue;
      }
      perror("Accept failed");
      continue; // Try again on next iteration
    }

    // Add the new socket to the thread pool
    thread_pool_add_task(pool, new_socket);
  }
  thread_pool_destroy(pool);
  cleanup(server_fd); // Close the server socket and free endpoint data
  return 0;
}
