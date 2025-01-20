#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shared_memory.h"
#include "utils.h"
#ifdef _WIN32
  #include <windows.h>
#else
  #include <fcntl.h>
  #include <pthread.h>
  #include <unistd.h>
  #include <signal.h>
  #include <sys/stat.h>
  #include <limits.h>
#endif

#ifdef _WIN32
    HANDLE counter_mutex = NULL;
#else
    pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

#define MASTER_LOCK_FILE "master.lock"
#define LOG_FILE "../logs/log.txt"

int *counter = NULL;

#ifdef _WIN32
    #define NULL 0
#endif

int is_master_thread() {
#ifdef _WIN32
    return (GetFileAttributes(MASTER_LOCK_FILE) == INVALID_FILE_ATTRIBUTES);
#else
    return (access(MASTER_LOCK_FILE, F_OK) != 0);
#endif
}

void create_master_lock() {
#ifdef _WIN32
    HANDLE hFile = CreateFile(MASTER_LOCK_FILE, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Не удалось создать master.lock, возможно, файл уже существует.\n");
    } else {
        printf("master.lock создан, главный поток.\n");
        CloseHandle(hFile);
    }
#else
    int fd = open(MASTER_LOCK_FILE, O_CREAT | O_EXCL | O_WRONLY, 0644);
    if (fd == -1) {
        perror("Не удалось создать master.lock, возможно, файл уже существует.");
    } else {
        printf("master.lock создан, главный поток.\n");
        close(fd);
    }
#endif
}

void remove_master_lock() {
#ifdef _WIN32
    if (!DeleteFile(MASTER_LOCK_FILE)) {
        printf("Не удалось удалить master.lock.\n");
    } else {
        printf("master.lock успешно удалён.\n");
    }
#else
    if (unlink(MASTER_LOCK_FILE) == -1) {
        perror("Не удалось удалить master.lock");
    } else {
        printf("master.lock успешно удалён.\n");
    }
#endif
}

void lock_mutex() {
#ifdef _WIN32
    WaitForSingleObject(counter_mutex, INFINITE);
#else
    pthread_mutex_lock(&counter_mutex);
#endif
}

void unlock_mutex() {
    #ifdef _WIN32
        ReleaseMutex(counter_mutex);
    #else
        pthread_mutex_unlock(&counter_mutex);
    #endif
}

void copy1_func() {
    char log_msg[64];
    snprintf(log_msg, sizeof(log_msg), "Child1 start:");
    write_log(log_msg);

    lock_mutex();
    *counter += 10;
    unlock_mutex();

    snprintf(log_msg, sizeof(log_msg), "Child1 exit:");
    write_log(log_msg);
    exit(0);
}

void copy2_func() {
    char log_msg[64];

    snprintf(log_msg, sizeof(log_msg), "Child2 start:");
    write_log(log_msg);

    lock_mutex();
    *counter = *counter << 1;
    unlock_mutex();

    #ifdef _WIN32
        Sleep(2000);
    #else
        usleep(2000000);
    #endif

    lock_mutex();
    *counter = *counter >> 1;
    unlock_mutex();

    snprintf(log_msg, sizeof(log_msg), "Child2 exit:");
    write_log(log_msg);
    exit(0);
}

int create_copy(char* param) {
#ifdef _WIN32
    char program_path[MAX_PATH];
    if (GetModuleFileName(NULL, program_path, MAX_PATH) == 0) {
        printf("Failed to get the program path\n");
        exit(1);
    }

    char command_line[MAX_PATH + 256];
    snprintf(command_line, sizeof(command_line), "\"%s\" %s", program_path, param);

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcess(
            NULL,
            command_line,
            NULL,
            NULL,
            FALSE,
            0,
            NULL,
            NULL,
            &si,
            &pi
    )) {
        printf("Failed to create process\n");
        exit(1);
    }

    return pi.dwProcessId;

#else
    int pid = fork();
    if (pid == 0) {
        char program_path[PATH_MAX];
        if (readlink("/proc/self/exe", program_path, sizeof(program_path)) == -1) {
            perror("Failed to get the program path");
            exit(1);
        }
        char *new_argv[] = {program_path, param, NULL};
        execv(program_path, new_argv);
    } else if (pid < 1) {
        printf("Failed to create child process\n");
        exit(0);
    }
    return pid;
#endif
}

int check_process_finished(int pid) {
#ifdef _WIN32
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (hProcess == NULL) {
        return 1;
    }

    DWORD exitCode;
    if (GetExitCodeProcess(hProcess, &exitCode)) {
        CloseHandle(hProcess);
        return (exitCode == STILL_ACTIVE) ? 0 : 1;
    }
    CloseHandle(hProcess);
    return 1;
#else
    int status;
    int result = waitpid(pid, &status, WNOHANG);

    if (result == 0)
        return 0;
    if (result == pid)
        return 1;

    perror("waitpid");
    return -1;
#endif
}


#ifdef _WIN32
    DWORD WINAPI run_copies(LPVOID arg)
#else
    void* run_copies(void* arg)
#endif
{
    int copy_1_pid = 0;
    int copy_2_pid = 0;

    while(1) {
        if (copy_1_pid == 0)
            copy_1_pid = create_copy("--copy1");
        else
            write_log("Copy 1 is still active.");

        if (copy_2_pid == 0)
            copy_2_pid = create_copy("--copy2");
        else
            write_log("Copy 2 is still active.");

        #ifdef _WIN32
            Sleep(3000);
        #else
            usleep(3000000);
        #endif

        if (check_process_finished(copy_1_pid))
            copy_1_pid = 0;
        if (check_process_finished(copy_2_pid))
            copy_2_pid = 0;
    }
}

#ifdef _WIN32
void signal_handler(int sig) {
    printf("Получен сигнал %d, завершение программы\n", sig);
    write_log("Программа завершена по сигналу");
    remove_master_lock();
    exit(0);
}
#else
void sigint_handler(int sig) {
    printf("Получен сигнал %d, завершение программы\n", sig);
    write_log("Программа завершена по сигналу");
    remove_master_lock();
    exit(0);
}
#endif

#ifdef _WIN32
    DWORD WINAPI increment(LPVOID arg)
#else
    void* increment(void* arg)
#endif
{
    while (1) {
        lock_mutex();
        (*counter)++;
        unlock_mutex();

        #ifdef _WIN32
            Sleep(300);
        #else
            usleep(300000);
        #endif
    }

    return NULL;
}

#ifdef _WIN32
    DWORD WINAPI input(LPVOID arg)
#else
    void* input(void* arg)
#endif
{
    int new_counter_value;
    while (1) {
        printf("Enter new counter value: ");
        if (scanf("%d", &new_counter_value) == 1) {
            (*counter) = new_counter_value;
        }
        lock_mutex();
        (*counter)++;
        unlock_mutex();
    }

    return NULL;
}

#ifdef _WIN32
    DWORD WINAPI log_status(LPVOID arg)
#else
    void* log_status(void* arg)
#endif
{
    #ifdef _WIN32
        Sleep(1000);
    #else
        usleep(1000000);
    #endif

    char log_msg[64];
    lock_mutex();
    snprintf(log_msg, sizeof(log_msg), "Counter = %d, ", *counter);
    unlock_mutex();

    write_log(log_msg);

    return NULL;
}

int main(int argc, char *argv[]) {
    counter = create_shared_memory();

    if (argc > 1) {
        if (strcmp(argv[1], "--copy1") == 0)
            copy1_func();
        if (strcmp(argv[1], "--copy2") == 0)
            copy2_func();
    }

    int is_master = 1;

#ifdef _WIN32
    signal(SIGINT, signal_handler);
#else
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
#endif

    if (is_master_thread()) {
        create_master_lock();
    } else {
        printf("Этот поток не главный. Файл master.lock уже существует.\n");
        is_master = 0;
    }

    FILE *log_file = fopen(LOG_FILE, "a");
    if (log_file == NULL) {
        perror("Не удалось открыть лог-файл");
        return 1;
    }

    time_t now = time(NULL);
    fprintf(log_file, "Процесс %d запущен в %s", getpid(), ctime(&now));
    fclose(log_file);

#ifdef _WIN32
    HANDLE increment_thread, log_thread, input_thread, run_copies_thread;

    increment_thread = CreateThread(NULL, 0, increment, NULL, 0, NULL);
    if (increment_thread == NULL) {
        fprintf(stderr, "Ошибка при создании потока increment\n");
        return 1;
    }

    input_thread = CreateThread(NULL, 0, input, NULL, 0, NULL);
    if (input_thread == NULL) {
        fprintf(stderr, "Ошибка при создании потока input\n");
        return 1;
    }

    WaitForSingleObject(increment_thread, INFINITE);
    WaitForSingleObject(input_thread, INFINITE);

    if (is_master) {
        log_thread = CreateThread(NULL, 0, log_status, NULL, 0, NULL);
        if (log_thread == NULL) {
            fprintf(stderr, "Ошибка при создании потока log_status\n");
            return 1;
        }

        run_copies_thread = CreateThread(NULL, 0, run_copies, NULL, 0, NULL);
        if (run_copies_thread == NULL) {
            fprintf(stderr, "Ошибка при создании потока run_copies\n");
            return 1;
        }

        WaitForSingleObject(log_thread, INFINITE);
        WaitForSingleObject(run_copies_thread, INFINITE);
    }
#else
    pthread_t increment_thread, log_thread, input_thread, run_copies_thread;

    if (pthread_create(&increment_thread, NULL, increment, NULL) != 0) {
        perror("increment thread");
        return 1;
    }

    if (pthread_create(&input_thread, NULL, input, NULL) != 0) {
        perror("input thread");
        return 1;
    }

    pthread_join(increment_thread, NULL);
    pthread_join(input_thread, NULL);

    if (is_master) {
        if (pthread_create(&log_thread, NULL, log_status, NULL) != 0) {
            perror("log_status thread");
            return 1;
        }

        if (pthread_create(&run_copies_thread, NULL, run_copies, NULL) != 0) {
            perror("run_copies thread");
            return 1;
        }

        pthread_join(log_thread, NULL);
        pthread_join(run_copies_thread, NULL);
    }
    #endif
    cleanup_shared_memory(counter);

    return 0;
}
