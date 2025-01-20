#include <time.h>
#include <string.h>

#include "serialport.h"
#include "database.h"
#include "handlers.h"

#ifdef _WIN32
    #include <windows.h>
    #include <winsock2.h>
    typedef int socklen_t;
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <pthread.h>
#endif

#define HOUR 3600
#define DAY 86400
#define MAX_LINE_LENGTH 256

#define PORT 8080

volatile int terminate = 0;

void get_timestamp(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);
    if (local_time) {
        strftime(buffer, size, "%Y-%m-%d %H:%M:%S", local_time);
    } else {
        snprintf(buffer, size, "Invalid time");
    }
}

#if _WIN32
    BOOL WINAPI custom_signal_handler(DWORD signal) {
        if (signal == CTRL_C_EVENT) {
            terminate = 1;
            return TRUE;
        }
        return FALSE;
    }
#else
    void custom_signal_handler(int signum) {
        terminate = 1;
    }
#endif

void close_server(int sock) {
    #ifdef _WIN32
        closesocket(sock);
        WSACleanup();
    #else
        close(sock);
    #endif
}

void close_client(int sock) {
    #ifdef _WIN32
        closesocket(sock);
    #else
        close(sock);
    #endif
}

void process_client_request(int client_sock) {
    sqlite3 *database;
    char *error_message = NULL;

    int status = sqlite3_open("../db/temp.db", &database);
    if (status != SQLITE_OK) {
        fprintf(stderr, "Failed to open database: %s\n", sqlite3_errmsg(database));
        sqlite3_close(database);
        return;
    }

    char request_buffer[256];
    #ifdef _WIN32
        recv(client_sock, request_buffer, sizeof(request_buffer), 0);
    #else
        read(client_sock, request_buffer, sizeof(request_buffer));
    #endif

    printf("Received request: %s\n", request_buffer);

    char *request_method = strtok(request_buffer, " ");
    char *request_route = strtok(NULL, " ");

    if (request_method == NULL || request_route == NULL || strcmp(request_method, "GET") != 0) {
        send_400_error_response(client_sock);
        return;
    }

    printf("Request Method: %s, Request Route: %s\n", request_method, request_route);

    if (strcmp(request_route, "/v1/temperature/current") == 0) {
        fetch_current_temperature(client_sock, database);
    } else if (strcmp(request_route, "/v1/temperature/average/hourly") == 0) {
        fetch_avg_temperature(client_sock, database, "hourly_avg");
    } else if (strcmp(request_route, "/v1/temperature/average/daily") == 0) {
        fetch_avg_temperature(client_sock, database, "daily_avg");
    } else {
        send_404_error_response(client_sock);
    }

    sqlite3_close(database);
}


#ifdef _WIN32
DWORD WINAPI run_server(LPVOID arg)
#else
void* run_server(void *arg)
#endif
{
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        fprintf(stderr, "Error initializing Winsock. Code: %d\n", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
#endif

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
#ifdef _WIN32
        WSACleanup();
#endif
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Binding failed");
        close_server(server_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, 10) < 0) {
        perror("Listen failed");
        close_server(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Server is running at http://localhost:%d\n", PORT);

    while (!terminate) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_sock, &read_fds);

        struct timeval timeout = {1, 0};
        int select_result = select(server_sock + 1, &read_fds, NULL, NULL, &timeout);

        if (select_result < 0 && !terminate) {
            perror("Select failed");
            break;
        }

        if (select_result > 0 && FD_ISSET(server_sock, &read_fds)) {
            client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_len);
            if (client_sock < 0) {
                perror("Accept failed");
                continue;
            }

            process_client_request(client_sock);
            close_client(client_sock);
        }
    }

    close_server(server_sock);
    return NULL;
}


int main() {
    #ifdef _WIN32
        HANDLE port = open_port();
        if (port == INVALID_HANDLE_VALUE) return EXIT_FAILURE;

        if (!SetConsoleCtrlHandler(custom_signal_handler, TRUE)) {
            fprintf(stderr, "Error setting signal handler.\n");
            return EXIT_FAILURE;
        }
    #else
        int port = open_port();
        if (port == -1) return EXIT_FAILURE;

        struct sigaction sa;
        sa.sa_handler = custom_signal_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        if (sigaction(SIGINT, &sa, NULL) == -1) {
            perror("Error setting signal handler");
            exit(EXIT_FAILURE);
        }
    #endif

    sqlite3 *db = init_database();

    #ifdef _WIN32
        HANDLE server_thread;
        DWORD thread_id;
        server_thread = CreateThread(NULL, 0, run_server, NULL, 0, &thread_id);
        if (server_thread == NULL) {
            printf("Failed to create server thread. Error: %lu\n", GetLastError());
            return -1;
        }
    #else
        pthread_t server_thread;
        if (pthread_create(&server_thread, NULL, run_server, NULL) != 0) {
             perror("Failed to create server thread");
             return 1;
        }
    #endif

    time_t start_time = time(NULL);
    time_t last_hour_time = start_time;
    time_t last_day_time = start_time;

    float hourly_sum = 0.0;
    int hourly_count = 0;
    float daily_sum = 0.0;
    int daily_count = 0;

    while (!terminate) {
        float temperature = read_port(port);

        char timestamp[64];
        get_timestamp(timestamp, sizeof(timestamp));

        char log_entry[128];
        snprintf(log_entry, sizeof(log_entry), "%s %.2f", timestamp, temperature);
        add_temperature_to_db(db, CURR_TEMP_TABLE, timestamp, temperature);

        hourly_sum += temperature;
        hourly_count++;
        daily_sum += temperature;
        daily_count++;

        time_t now = time(NULL);

        if ((int)difftime(now, last_hour_time) % HOUR == 0 && now != last_hour_time) {
            float hourly_avg = hourly_sum / hourly_count;
            snprintf(log_entry, sizeof(log_entry), "%s %.2f", timestamp, hourly_avg);
            add_temperature_to_db(db, HOURLY_AVG_TABLE, timestamp, temperature);

            hourly_sum = 0.0;
            hourly_count = 0;
            last_hour_time = now;
        }

        if ((int)difftime(now, last_day_time) % DAY == 0 && now != last_day_time) {
            float daily_avg = daily_sum / daily_count;
            snprintf(log_entry, sizeof(log_entry), "%s Daily Avg: %.2f", timestamp, daily_avg);
            add_temperature_to_db(db, DAILY_AVG_TABLE, timestamp, temperature);

            daily_sum = 0.0;
            daily_count = 0;
            last_day_time = now;
        }

        #ifdef _WIN32
            Sleep(1000);
        #else
            sleep(1);
        #endif
    }

    #ifdef _WIN32
        CloseHandle(port);
    #else
        close(port);
    #endif

    sqlite3_close(db);

#ifdef _WIN32
        WaitForSingleObject(server_thread, INFINITE);
        CloseHandle(server_thread);
    #else
        pthread_join(server_thread, NULL);
    #endif
    return 0;
}
