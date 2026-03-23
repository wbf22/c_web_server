#include <stdio.h>
#include "CServer.h"

/*

valgrind --leak-check=full --show-leak-kinds=all ./test_exe

leaks -atExit -- ./test_exe
*/

char* ANSII_RESET = "\033[0m";
void assert(int condition, char* message) {
    if (!condition) {
        fprintf(stderr, "\033[38;2;255;0;0m");
        fprintf(stderr, "failed: ");
        fprintf(stderr, message);
        fprintf(stderr, ANSII_RESET);
        fprintf(stderr, "\n");
        exit(EXIT_FAILURE);
    }
    else {
        fprintf(stderr, "\033[38;2;40;133;0m");
        fprintf(stderr, "passed: ");
        fprintf(stderr, message);
        fprintf(stderr, ANSII_RESET);
        fprintf(stderr, "\n");
    }
}


CServer* server;

void handler_function(Response* response, char* method, HttpHeader* headers, int num_headers, char* path, char* body) {

    char* res_body = "hi man";

    response->status = HTTP_OK;
    response->headers = NULL;
    response->num_headers = 0;
    response->body = strdup(res_body);


    if (strcmp("hi", body) == 0) {
        c_server_stop(server);
    }
}

SecurityResult* security_function(char* ip_address, int port, char* method, HttpHeader* headers, int num_headers, char* path, void* security_data) {
    SecurityResult* result = malloc(sizeof(SecurityResult));
    result->request_failed_security = 0;

    return result;
}


int main() {

    server = c_server_start_http(8080, handler_function, security_function, NULL);

    while(!server->shutdown) {
        sleep(2);
    }

    while(!server->shutdown_done) {
        sleep(1);
    }

    c_server_free(server);
    
    return 0;
}
