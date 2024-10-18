#include "request_handler.h"

Endpoint endpoints[MAX_ENDPOINTS]; // Array of endpoints
int endpoint_count = 0;
pthread_mutex_t endpoints_mutex =
    PTHREAD_MUTEX_INITIALIZER; // Mutex for endpoints

static const struct content_type CONTENT_TYPES[] = {
    {".txt", "text/plain"},
    {".html", "text/html"},
    {".htm", "text/html"},
    {".css", "text/css"},
    {".js", "text/javascript"},
    {".json", "application/json"},
    {".xml", "application/xml"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".svg", "image/svg+xml"},
    {".mp3", "audio/mpeg"},
    {".mp4", "video/mp4"},
    {".pdf", "application/pdf"},
    {".bin", "application/octet-stream"},
    {NULL, "application/octet-stream"} // Default type
};

int find_or_create_endpoint(const char *path) {
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
    endpoints[index].path[MAX_PATH_LENGTH - 1] = '\0';
    endpoints[index].data = NULL;
    pthread_mutex_init(&endpoints[index].mutex, NULL);
    endpoint_count++;
  }

  pthread_mutex_unlock(&endpoints_mutex);
  return index;
}

const char *get_status_message(int status_code) {
  switch (status_code) {
  case 200:
    return "OK";
  case 204:
    return "No Content";
  case 400:
    return "Bad Request";
  case 404:
    return "Not Found";
  case 412:
    return "Precondition Failed: Content-Type and Content-Length required";
  case 415:
    return "Unsupported Media Type";
  case 500:
    return "Internal Server Error";
  default:
    return "Unknown Status";
  }
}

HttpRequest body_parser(const char *curr, HttpRequest req, int index) {
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
      endpoints[index].data[body_length] = '\0'; // Null-terminate the string
    }

    pthread_mutex_unlock(&endpoints[index].mutex);
  } else {
    req.body = NULL;
  }

  return req;
}

// Parse the HTTP request and extrat the method, path, and body
HttpRequest parse_request(const char *request) {
  HttpRequest req = {0};
  const char *curr = request;
  const char *end;
  req.response_code = 200;

  // Parse request line (e.g., "GET /path HTTP/1.1")
  end = strstr(curr, "\r\n"); // Gets the first occurence of "\r\n"
  if (end) {
    // When end is not NULL
    sscanf(curr, "%9s %255s %9s", req.method, req.path,
           req.version); // scan the request line and store the method and path
    curr = end + 2;      // Move past \r\n
  }

  int index = find_or_create_endpoint(req.path);

  if (index == -1) {
    printf("Error: No space for new endpoint\n");
    req.response_code = 500;
    return req;
  }

  if (strcmp(req.method, "DELETE") == 0) {
    pthread_mutex_lock(&endpoints[index].mutex);
    if (endpoints[index].data != NULL) {
      free(endpoints[index].data);
      endpoints[index].data = NULL;
      req.response_code = 204;
    } else {
      req.response_code = 404;
    }
    pthread_mutex_unlock(&endpoints[index].mutex);
  }

  _Bool requireBody = strcmp(req.method, "POST") == 0 ||
                      strcmp(req.method, "PUT") == 0 ||
                      strcmp(req.method, "PATCH") == 0;

  // Parse headers
  req.header_count = 0;
  while (curr[0] != '\r' ||
         curr[1] != '\n') {     // Exit on empty line (i.e., \r\n\r\n)
    end = strstr(curr, "\r\n"); // End of current header
    if (!end || req.header_count >= MAX_HEADERS)
      break; // max headers reached

    int header_length = end - curr;
    if (header_length < MAX_HEADER_LENGTH) {
      strncpy(req.headers[req.header_count], curr,
              header_length); // copy into arr
      req.headers[req.header_count][header_length] = '\0';
      req.header_count++;
    }

    curr = end + 2; // Move past \r\n
  }

  char *contentType = NULL;
  if (requireBody) {
    _Bool hasContentLength = 0;
    _Bool hasContentType = 0;
    for (int i = 0; i < req.header_count; i++) {
      char *header = req.headers[i];
      char *value = strchr(header, ':');
      if (value) {
        *value = '\0';
        value++; // Move past the colon
        while (*value == ' ')
          value++; // Skip spaces

        if (strcmp(header, "Content-Length") == 0)
          hasContentLength = 1;
        else if (strcmp(header, "Content-Type") == 0) {
          contentType = value;
          hasContentType = 1;
        }
      }
    }
    if (!hasContentType && !hasContentLength) {
      printf("Error: No space for new endpoint\n");
      req.response_code = 412;
      return req;
    }
  } else {
    return req;
  }

  if (index != -1) {
    if (contentType != NULL && strcmp(contentType, "text/plain") == 0 ||
        strcmp(contentType, "text/html") == 0 ||
        strcmp(contentType, "text/css") == 0 ||
        strcmp(contentType, "text/javascript") == 0 ||
        strcmp(contentType, "application/json") == 0 ||
        strcmp(contentType, "application/xml") == 0) {
      req = body_parser(curr, req, index);
    } else {
      // Add other media types later
      req.response_code = 415; // Unsupported Media Type
    }
  } else {
    req.response_code = 500; // Internal Server Error
  }
  return req;
}

char *get_endpoint_data(int index) {
  char *data_copy = NULL;
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

HttpResponse *create_response(int status_code, const char **headers,
                              int header_count) {
  HttpResponse *response = malloc(sizeof(HttpResponse));
  if (response == NULL) {
    return NULL;
  }

  response->status_code = status_code;
  const char *status_message = get_status_message(status_code);
  strncpy(response->status_message, status_message,
          sizeof(response->status_message) - 1);
  response->status_message[sizeof(response->status_message) - 1] = '\0';

  response->header_count = 0;
  for (int i = 0; i < header_count && i < MAX_HEADERS; i++) { // Copy headers
    strncpy(response->headers[i], headers[i], MAX_HEADER_LENGTH - 1);
    response->headers[i][MAX_HEADER_LENGTH - 1] = '\0';
    response->header_count++;
  }

  // Initialize body to NULL
  response->body = NULL;
  response->body_length = 0;

  return response;
}

void set_response_body(HttpResponse *response, const char *body,
                       size_t body_length) {
  if (response->body != NULL) {
    free(response->body);
  }
  // Allocate space for body, newline and  null term
  response->body = malloc(body_length + 2);
  if (response->body != NULL) {
    memcpy(response->body, body, body_length);
    response->body[body_length] = '\n';
    response->body[body_length + 1] = '\0';
    response->body_length = body_length + 1; // +1 for newline
  }
}

char *serialize_response(HttpResponse *response, size_t *total_length) {
  // Calculate the total length of the response
  *total_length = snprintf(NULL, 0, "HTTP/1.1 %d %s\r\n", response->status_code,
                           response->status_message);
  for (int i = 0; i < response->header_count; i++) {
    *total_length += strlen(response->headers[i]) + 2; // +2 for \r\n
  }
  *total_length += 2; // For the empty line between headers and body
  *total_length += response->body_length;

  char *serialized = malloc(*total_length + 1); // +1 for null terminator
  if (serialized == NULL) {
    return NULL;
  }

  char *current = serialized;
  current += sprintf(current, "HTTP/1.1 %d %s\r\n", response->status_code,
                     response->status_message);
  for (int i = 0; i < response->header_count; i++) {
    current += sprintf(current, "%s\r\n", response->headers[i]);
  }
  current += sprintf(current, "\r\n");
  memcpy(current, response->body, response->body_length);

  return serialized;
}

void update_response_status(HttpResponse *response, int status_code,
                            const char *status_message) {
  if (response != NULL) {
    response->status_code = status_code;
    strncpy(response->status_message, status_message,
            sizeof(response->status_message) - 1);
    response->status_message[sizeof(response->status_message) - 1] = '\0';
  }
}

void free_response(HttpResponse *response) {
  if (response != NULL) {
    if (response->body != NULL) {
      free(response->body);
    }
    free(response);
  }
}

void print_request(HttpRequest *req) {
  printf("Method: %s\n", req->method);
  printf("Path: %s\n", req->path);

  printf("Headers:\n");
  for (int i = 0; i < req->header_count; i++) {
    printf("  %s\n", req->headers[i]);
  }
  printf("Status Code: %d\n", req->response_code);
  printf("Body: %s\n", req->body);
}

void free_endpoint_data() {
  for (int i = 0; i < endpoint_count; i++) {
    pthread_mutex_lock(&endpoints[i].mutex);
    free(endpoints[i].data);
    endpoints[i].data = NULL;
    pthread_mutex_unlock(&endpoints[i].mutex);
    pthread_mutex_destroy(&endpoints[i].mutex);
  }
}
