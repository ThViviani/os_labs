#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <unistd.h>
#endif

#include "shared_memory.h"

int* create_shared_memory(char* name) {
    #ifdef _WIN32
        if (SIZE <= 0) {
            fprintf(stderr, "Error: size must be greater than 0\n");
            exit(1);
        }

        HANDLE hSharedMemory;
        int* counter;

        hSharedMemory = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, (DWORD)SIZE, name);

        if (hSharedMemory == NULL) {
            fprintf(stderr, "CreateFileMapping failed with error %lu\n", GetLastError());
            exit(1);
        }

        counter = (int*)MapViewOfFile(hSharedMemory, FILE_MAP_ALL_ACCESS, 0, 0, SIZE);

        if (counter == NULL) {
            fprintf(stderr, "MapViewOfFile failed with error %lu\n", GetLastError());
            CloseHandle(hSharedMemory);
            exit(1);
        }

        return counter;
    #else
        int shm_fd;
        int* counter;

        shm_fd = shm_open(name, O_CREAT | O_RDWR, 0777);
        if (shm_fd == -1) {
            perror("shm_open");
            exit(1);
        }

        if (SHM_SIZE <= 0) {
            fprintf(stderr, "Error: size must be greater than 0\n");
            exit(1);
        }

        if (ftruncate(shm_fd, SHM_SIZE) == -1) {
            perror("ftruncate");
            exit(1);
        }

        counter = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
        if (counter == MAP_FAILED) {
            perror("mmap failed");
            close(shm_fd);
            shm_unlink(name);
            exit(1);
        }

        return counter;
    #endif
}

void cleanup_shared_memory(int* ptr, char* name) {
#ifdef _WIN32
    if (ptr != NULL) {
        UnmapViewOfFile(ptr);
    }
#else
    if (munmap(ptr, SHM_SIZE) == -1) {
        perror("munmap");
    }

    if (shm_unlink(name) == -1) {
        perror("shm_unlink");
    }
#endif
}
