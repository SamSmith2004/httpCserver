#include "request_handler.h"

Endpoint endpoints[MAX_ENDPOINTS]; // Array of endpoints
int endpoint_count = 0;
pthread_mutex_t endpoints_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for endpoints

int find_or_create_endpoint(const char* path) {
    pthread_mutex_lock(&endpoints_mutex);

    // Find the endpoint with the given path
    int index = -1;
    for (int i = 0; i < endpoint_count; i++) {
        if (strcmp(endpoints[i].path, path) == 0) {
            index = i;
            break;
        }
    }

    // Create a new endpoint if it does not exist
    if (index == -1 && endpoint_count < MAX_ENDPOINTS) {
        index = endpoint_count;
        strncpy(endpoints[index].path, path, MAX_PATH_LENGTH - 1);
        endpoints[index].data = NULL;
        pthread_mutex_init(&endpoints[index].mutex, NULL);
        endpoint_count++;
    }

    pthread_mutex_unlock(&endpoints_mutex);
    return index;
}

// Parse the HTTP request and extrat the method, path, and body
HttpRequest parse_request(const char *request) {
  HttpRequest req = {0};
  const char *curr = request;
  const char *end;

  // Parse request line (e.g., "GET /path HTTP/1.1")
  end = strstr(curr, "\r\n"); // Gets the first occurence of "\r\n"
  if (end) {
    // When end is not NULL
    sscanf(curr, "%9s %255s %9s", req.method,
    req.path, req.version); // scan the request line and store the method and path
    curr = end + 2;   // Move past \r\n
  }

  int index = find_or_create_endpoint(req.path);

  if (index == -1) {
    printf("Error: No space for new endpoint\n");
    return req;
  }

  // Parse headers
  req.header_count = 0;
  while (curr[0] != '\r' || curr[1] != '\n') { // Exit on empty line (i.e., \r\n\r\n)
    end = strstr(curr, "\r\n"); // End of current header
    if (!end || req.header_count >= MAX_HEADERS) break; // max headers reached

    int header_length = end - curr;
    if (header_length < MAX_HEADER_LENGTH) {
        strncpy(req.headers[req.header_count], curr, header_length); // copy into arr
        req.headers[req.header_count][header_length] = '\0';
        req.header_count++;
    }

    curr = end + 2;  // Move past \r\n
  }

  // Move past the empty line separating headers from body
  if (curr[0] == '\r' && curr[1] == '\n') {
      curr += 2;
  }

  // Body is the rest of the request
  if (*curr != '\0') {
    req.body = curr;
    size_t body_length = strlen(req.body);

    pthread_mutex_lock(&endpoints[index].mutex);

    // Free previous data if it exists
    if (endpoints[index].data != NULL) {
      free(endpoints[index].data);
    }

    // Allocate memory for the new data
    endpoints[index].data = malloc(body_length + 1);
    if (endpoints[index].data == NULL) {
      fprintf(stderr, "Failed to allocate memory for endpoint data\n");
    } else {
      strncpy(endpoints[index].data, req.body, body_length);
      endpoints[index].data[body_length] = '\0';  // Null-terminate the string
    }

    pthread_mutex_unlock(&endpoints[index].mutex);
  } else {
    req.body = NULL;
  }

  return req;
}

char* get_endpoint_data(int index) {
    char* data_copy = NULL;
    pthread_mutex_lock(&endpoints[index].mutex);

    if (endpoints[index].data != NULL) {
        size_t data_length = strlen(endpoints[index].data);
        data_copy = malloc(data_length + 1);
        if (data_copy != NULL) {
            strcpy(data_copy, endpoints[index].data);
        }
    }

    pthread_mutex_unlock(&endpoints[index].mutex);
    return data_copy;
}

void print_request(HttpRequest *req) {
  printf("Method: %s\n", req->method);
  printf("Path: %s\n", req->path);

  printf("Headers:\n");
  for (int i = 0; i < req->header_count; i++) {
    printf("  %s\n", req->headers[i]);
  }
  if (strcmp(req->method, "POST") == 0 || strcmp(req->method, "PUT") == 0 ||
    strcmp(req->method, "PATCH") == 0 || strcmp(req->method, "DELETE") == 0) {
    printf("Body: %s\n", req->body);
  } else {
    // Methods GET, HEAD, OPTIONS, TRACE, CONNECT, LINK, UNLINK do not have a
    // body
    printf("Body: N/A\n");
  }
}
