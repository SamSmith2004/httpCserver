#include "request_handler.h"

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
  } else {
    req.body = NULL;
  }

  return req;
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
