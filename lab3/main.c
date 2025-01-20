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
    HANDLE main_process_mutex = NULL;
#else
    pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t main_process_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

#define LOG_FILE "../logs/log.txt"
#define COUNTER_SHM_NAME "/counter_shared_memory"
#define MAIN_SHM_NAME "/main_shared_memory"

int *counter = NULL;
int *main_process_flag = NULL;

#ifdef _WIN32
    #define NULL 0
#endif

void signal_handler(int sig) {
    #ifdef _WIN32
        if (sig == CTRL_C_EVENT) {
            printf("Caught interrupt signal\n");
            if (main_process_flag) {
              *main_process_flag = 0;
            }
            exit(0);
        }
    #else
        if (sig == SIGINT) {
            printf("Caught interrupt signal\n");
            if (main_process_flag) {
                *main_process_flag = 0;
            }
            exit(0);
        }
    #endif
}

void setup_signal_handler() {
#ifdef _WIN32
    if (!SetConsoleCtrlHandler(signal_handler, TRUE)) {
        printf("Error setting control handler\n");
        exit(1);
    }
#else
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction failed");
        exit(1);
    }
#endif
}

void lock_mutex(void* mutex) {
#ifdef _WIN32
    WaitForSingleObject((HANDLE)mutex, INFINITE);
#else
    pthread_mutex_lock((pthread_mutex_t*)mutex);
#endif
}

void unlock_mutex(void* mutex) {
#ifdef _WIN32
    ReleaseMutex((HANDLE)mutex);
#else
    pthread_mutex_unlock((pthread_mutex_t*)mutex);
#endif
}

// Поток, который запускает таймер и раз в 300 мс увеличивает счетчик на 1:
#ifdef _WIN32
    DWORD WINAPI increment(LPVOID arg)
#else
    void* increment(void* arg)
#endif
{
    while (1) {
        lock_mutex(&counter_mutex);
        (*counter)++;
        unlock_mutex(&counter_mutex);

        #ifdef _WIN32
            Sleep(300);
        #else
            usleep(300000);
        #endif
    }

    return NULL;
}

// Поток, который позволяет пользователю через интерфейс командной строки установить любое значение счетчика
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
        lock_mutex(&counter_mutex);
        (*counter)++;
        unlock_mutex(&counter_mutex);
    }

    return NULL;
}

// Поток, который раз в 1 секунду пишет в лог-файл текущее время (дата, часы, минуты, секунды, миллисекунды), свой идентификатор процесса и значение счетчика
#ifdef _WIN32
    DWORD WINAPI log_status(LPVOID arg)
#else
    void* log_status(void* arg)
#endif
{
    while (1) {
        #ifdef _WIN32
            Sleep(1000);
        #else
            usleep(1000000);
        #endif

        char log_msg[64];
        lock_mutex(&counter_mutex);
        snprintf(log_msg, sizeof(log_msg), "Counter = %d, ", *counter);
        unlock_mutex(&counter_mutex);

        write_log(log_msg);
    }

    return NULL;
}

// Копия 1 записывает в тот же лог файл строку со своим идентификатором процесса, временем своего запуска, увеличивает текущее значение счетчика на 10, записывает в лог файл время выхода и завершается
void copy1_func() {
    char log_msg[64];
    snprintf(log_msg, sizeof(log_msg), "Child1 start:");
    write_log(log_msg);

    lock_mutex(&counter_mutex);
    *counter += 10;
    unlock_mutex(&counter_mutex);

    snprintf(log_msg, sizeof(log_msg), "Child1 exit:");
    write_log(log_msg);
    exit(0);
}

// Копия 2 записывает в тот же лог файл строку со своим идентификатором процесса, временем своего запуска, увеличивает текущее значение счетчика в 2 раза, через 2 секунды уменьшает значение счетчика в 2 раза, записывает в лог файл время выхода и завершается
void copy2_func() {
    char log_msg[64];

    snprintf(log_msg, sizeof(log_msg), "Child2 start:");
    write_log(log_msg);

    lock_mutex(&counter_mutex);
    *counter = *counter << 1;
    unlock_mutex(&counter_mutex);

    #ifdef _WIN32
        Sleep(2000);
    #else
        usleep(2000000);
    #endif

    lock_mutex(&counter_mutex);
    *counter = *counter >> 1;
    unlock_mutex(&counter_mutex);

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
    char program_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", program_path, sizeof(program_path) - 1);
    if (len == -1) {
        perror("Failed to get the program path");
        exit(1);
    }
    program_path[len] = '\0';

    char *new_argv[] = {program_path, param, NULL};
    pid_t pid = fork();

    if (pid == 0) {
        execv(program_path, new_argv);
        perror("execv failed");
        exit(1);
    } else if (pid < 0) {
        perror("Failed to create child process");
        exit(1);
    }
    return pid;
#endif
}

int check_process_ended(int pid) {
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

// Поток, который раз в 3 секунды запускает 2 копии:
#ifdef _WIN32
    DWORD WINAPI run_copies(LPVOID arg)
#else
    void* run_copies(void* arg)
#endif
{
    int copy_1_pid = 0;
    int copy_2_pid = 0;

    while(1) {
        if (copy_1_pid == 0) {
            copy_1_pid = create_copy("--copy1");
        } else {
            write_log("Copy 1 is still running.");
        }

        if (copy_2_pid == 0) {
            copy_2_pid = create_copy("--copy2");
        } else {
            write_log("Copy 2 is still running.");
        }

        #ifdef _WIN32
            Sleep(3000);
        #else
            usleep(3000000);
        #endif

        if (check_process_ended(copy_1_pid)) {
            copy_1_pid = 0;
        }
        if (check_process_ended(copy_2_pid)) {
            copy_2_pid = 0;
        }
    }
}

int main(int argc, char *argv[]) {
    setup_signal_handler();

   counter = create_shared_memory(COUNTER_SHM_NAME);
    main_process_flag = create_shared_memory(MAIN_SHM_NAME);
    *main_process_flag = 0;

    if (argc > 1 && strcmp(argv[1], "--copy1") == 0) {
        copy1_func();
    } else if (argc > 1 && strcmp(argv[1], "--copy2") == 0) {
        copy2_func();
    }

    lock_mutex(&main_process_mutex);
    if (*main_process_flag == 0) {
        *main_process_flag = 1;
        printf("This is the main process.\n");
        write_log("Main Process starts:");

        #ifdef _WIN32
            HANDLE increment_thread, input_thread, log_thread, run_copies_thread;

            increment_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)increment, NULL, 0, NULL);
            input_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)input, NULL, 0, NULL);
            log_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)log_status, NULL, 0, NULL);
            run_copies_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)run_copies, NULL, 0, NULL);

            WaitForSingleObject(increment_thread, INFINITE);
            WaitForSingleObject(input_thread, INFINITE);
            WaitForSingleObject(log_thread, INFINITE);
            WaitForSingleObject(run_copies_thread, INFINITE);
        #else
            pthread_t increment_thread, input_thread, log_thread, run_copies_thread;
            pthread_create(&increment_thread, NULL, increment, NULL);
            pthread_create(&input_thread, NULL, input, NULL);
            pthread_create(&log_thread, NULL, log_status, NULL);
            pthread_create(&run_copies_thread, NULL, run_copies, NULL);

            pthread_join(increment_thread, NULL);
            pthread_join(input_thread, NULL);
            pthread_join(log_thread, NULL);
            pthread_join(run_copies_thread, NULL);
        #endif
    } else {
       printf("This is a child process.\n");
       unlock_mutex(&main_process_mutex);

        #ifdef _WIN32
            HANDLE increment_thread, input_thread;

            increment_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)increment, NULL, 0, NULL);
            input_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)input, NULL, 0, NULL);

            WaitForSingleObject(increment_thread, INFINITE);
            WaitForSingleObject(input_thread, INFINITE);
        #else
            pthread_t increment_thread, input_thread;
            pthread_create(&increment_thread, NULL, increment, NULL);
            pthread_create(&input_thread, NULL, input, NULL);

            pthread_join(increment_thread, NULL);
            pthread_join(input_thread, NULL);
        #endif
    }

    while (1) {
        if (*main_process_flag == 0) {
            lock_mutex(&main_process_mutex);
            *main_process_flag = 1;
            printf("Process %d is now the main process.\n", getpid());
            unlock_mutex(&main_process_mutex);
            break;
        }
    }

    cleanup_shared_memory(counter, COUNTER_SHM_NAME);
    cleanup_shared_memory(main_process_flag, MAIN_SHM_NAME);
    return 0;
}

