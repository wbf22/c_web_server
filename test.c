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



Response handler_function(char* method, Map* headers, char* path, char* body) {


    char* res_body = "hi man";

    Response res = {
        HTTP_OK,
        "",
        NULL,
        res_body,
        strlen(res_body)+1
    };

    return res;
}

int main() {

    CServer* server = c_server_start_tcp(8080, handler_function);

    sleep(1000*60*10);
    
    return 0;
}
