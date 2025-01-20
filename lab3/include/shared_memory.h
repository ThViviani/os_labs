#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#define SHM_NAME "/shared_memory"
#define SHM_SIZE sizeof(int)

int* create_shared_memory();
void cleanup_shared_memory(int* ptr);

#endif // SHARED_MEMORY_H