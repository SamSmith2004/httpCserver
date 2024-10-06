#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    char choice[10];
    char command[256];

    printf("GET or POST: ");
    scanf("%9s", choice);

    if (strcmp(choice, "GET") == 0) {
        snprintf(command, sizeof(command), "curl http://localhost:8080");
        system(command);
    } else if (strcmp(choice, "POST") == 0) {
        char body[100];
        printf("Enter POST body: ");
        scanf(" %99[^\n]", body);

        snprintf(command, sizeof(command), "curl -X POST -d '%s' http://localhost:8080", body);
        system(command);
    } else {
        printf("Invalid request\n");
        return 1;
    }
    return 0;
}
