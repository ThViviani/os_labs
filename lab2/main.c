#include <stdio.h>
#include "backgroundRunner.h"

int main() {
    #ifdef _WIN32
        char *command = "test_program.exe";
    #else
        char *command = "./test_program";
    #endif

    printf("Starting the child process...\n");
    int result = backgroundRunner(command);

    printf("Child process has ended with exit_code = %d", result);
    return 0;
}
