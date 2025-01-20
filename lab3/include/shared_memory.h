#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#define SHM_SIZE sizeof(int)

int* create_shared_memory(char* name);
void cleanup_shared_memory(int* ptr, char* name);

#endif // SHARED_MEMORY_H