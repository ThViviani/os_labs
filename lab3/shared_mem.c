#include "shared_mem.h"
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

#define SHARED_MEMORY_NAME "/shared_memory_counter"
#define SHARED_MEMORY_SIZE 4096

int initialize_shared_memory() {
    int shm_fd = shm_open(SHARED_MEMORY_NAME, O_CREAT | O_RDWR, 0666);
        if (shm_fd == -1) {
            perror("shm_open");
            return -1;
        }

        if (ftruncate(shm_fd, SHARED_MEMORY_SIZE) == -1) {
                perror("ftruncate");
                close(shm_fd);
                return -1;
            }

        counter = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
            if (counter == MAP_FAILED) {
                perror("mmap failed");
                close(shm_fd);
                shm_unlink(name);
                exit(1);
            }

            return counter;
        }

}
