#include <stdio.h>
#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif

int main() {
    printf("This program counts down 5 seconds and shuts down\n");

    for (int i = 1; i <= 5; i++) {
        printf("%d\n", i);
        #ifdef _WIN32
            Sleep(1000);
        #else
            sleep(1);
        #endif
    }

    printf("Program finished\n");
    return 0;
}
