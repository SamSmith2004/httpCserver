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
    sscanf(curr, "%9s %255s", req.method,
           req.path); // scan the request line and store the method and path
    curr = end + 2;   // Move past \r\n
  }

  // Find the body (separated from headers by \r\n\r\n)
  end = strstr(curr, "\r\n\r\n");
  if (end) {
    req.body = (char *)end + 4; // Point to start of body
  }

  return req;
}

void print_request(HttpRequest *req) {
  printf("Method: %s\n", req->method);
  printf("Path: %s\n", req->path);
  if (strcmp(req->method, "POST") == 0 || strcmp(req->method, "PUT") == 0 ||
      strcmp(req->method, "PATCH") == 0 || strcmp(req->method, "DELETE") == 0) {
    printf("Body: %s\n", req->body);
  } else {
    // Methods GET, HEAD, OPTIONS, TRACE, CONNECT, LINK, UNLINK do not have a
    // body
    printf("Body: N/A\n");
  }
}
