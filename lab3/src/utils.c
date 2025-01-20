#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
    #include <windows.h>
    #include <time.h>
#else
    #include <fcntl.h>
    #include <unistd.h>
    #include <sys/time.h>
    #include <sys/types.h>
    #include <sys/stat.h>
#endif

void write_log(const char* message) {
    char buffer[256];

    #ifdef _WIN32
        SYSTEMTIME st;
        GetLocalTime(&st);

        int n = snprintf(buffer, sizeof(buffer),
                         "%s PID=%d, time=%04d-%02d-%02d %02d:%02d:%02d.%03d\n",
                         message,
                         GetCurrentProcessId(),
                         st.wYear,
                         st.wMonth,
                         st.wDay,
                         st.wHour,
                         st.wMinute,
                         st.wSecond,
                         st.wMilliseconds);

        HANDLE hFile = CreateFile(LOG_FILE_PATH, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            DWORD error = GetLastError();
            fprintf(stderr, "Failed to open log file. Error code: %lu\n", error);
            return;
        }

        DWORD bytesWritten;
        if (!WriteFile(hFile, buffer, n, &bytesWritten, NULL)) {
            DWORD error = GetLastError();
            fprintf(stderr, "Failed to write to log file. Error code: %lu\n", error);
        }

        CloseHandle(hFile);

    #else
        struct timeval tv;
        gettimeofday(&tv, NULL);

        struct tm *tm_info = localtime(&tv.tv_sec);


        int n = snprintf(buffer, sizeof(buffer),
                         "%s PID=%d, [%04d-%02d-%02d %02d:%02d:%02d.%03d]\n",
                         message,
                         getpid(),
                         tm_info->tm_year + 1900,
                         tm_info->tm_mon + 1,
                         tm_info->tm_mday,
                         tm_info->tm_hour,
                         tm_info->tm_min,
                         tm_info->tm_sec,
                         tv.tv_usec / 1000);

        int fd = open(LOG_FILE_PATH, O_RDWR | O_CREAT | O_APPEND, 0644);
        if (fd == -1) {
            perror("open");
            return;
        }

        if (write(fd, buffer, n) == -1) {
            perror("write");
        }

        close(fd);
        printf("data was sent\n");
    #endif
}
