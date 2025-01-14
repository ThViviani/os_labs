#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
    #include <signal.h>
    #include <fcntl.h>
    #include <termios.h>
#endif

#ifdef _WIN32
    #define SERIAL_PORT "COM2"
#else
    #define SERIAL_PORT "/dev/ttys009"
#endif

#define LOG_ALL "../logs/temp_all.log"
#define LOG_HOURLY "../logs/temp_hourly.log"
#define LOG_DAILY "../logs/temp_daily.log"

#define HOUR 3600
#define DAY 86400
#define MAX_LINE_LENGTH 256

volatile int terminate = 0;

#ifdef _WIN32
    HANDLE open_port() {
        HANDLE hComm = CreateFile(SERIAL_PORT, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (hComm == INVALID_HANDLE_VALUE) {
            fprintf(stderr, "Error: Cannot open serial port.\n");
            return INVALID_HANDLE_VALUE;
        }

        DCB serial_config = {0};
        serial_config.DCBlength = sizeof(serial_config);
        GetCommState(hComm, &serial_config);
        serial_config.BaudRate = CBR_9600;
        serial_config.ByteSize = 8;
        serial_config.Parity = NOPARITY;
        serial_config.StopBits = ONESTOPBIT;
        SetCommState(hComm, &serial_config);

        COMMTIMEOUTS timeouts = { 0 };
        timeouts.ReadIntervalTimeout = 50;
        timeouts.ReadTotalTimeoutConstant = 50;
        timeouts.ReadTotalTimeoutMultiplier = 10;

        if (!SetCommTimeouts(hComm, &timeouts)) {
            fprintf(stderr, "Failed to set timeouts. Error: %ld\n", GetLastError());
            CloseHandle(hComm);
            return INVALID_HANDLE_VALUE;
        }

        return hComm;
    }

    float read_port(HANDLE hComm) {
        char buffer[MAX_LINE_LENGTH] = {0};
        DWORD bytesRead = 0;
        if (ReadFile(hComm, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
            buffer[bytesRead] = '\0';
            printf("Read from port %d: %s", hComm, buffer);
            return strtof(buffer, NULL);
        }
        return -1.0;
    }

#else
    int open_port() {
        int fd = open(SERIAL_PORT, O_RDONLY | O_NOCTTY);
        if (fd == -1) {
            perror("Error: Cannot open serial port");
            return -1;
        }

        struct termios serial_options;
        if (tcgetattr(fd, &serial_options) != 0) {
            perror("Failed to get attributes");
            close(fd);
            return -1;
        }

        cfsetispeed(&serial_options, B9600);
        cfsetospeed(&serial_options, B9600);

        serial_options.c_cflag |= (CLOCAL | CREAD);
        serial_options.c_cflag &= ~PARENB;
        serial_options.c_cflag &= ~CSTOPB;
        serial_options.c_cflag &= ~CSIZE;
        serial_options.c_cflag |= CS8;

        serial_options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        serial_options.c_iflag &= ~(IXON | IXOFF | IXANY);
        serial_options.c_oflag &= ~OPOST;

        if (tcsetattr(fd, TCSANOW, &serial_options) != 0) {
            perror("Failed to set attributes");
            close(fd);
            return -1;
        }

        return fd;
    }

    float read_port(int fd) {
        char buffer[MAX_LINE_LENGTH];
        int bytes_read = read(fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            printf("Read from port %d: %s", fd, buffer);
            return strtof(buffer, NULL);
        }
        return -1.0;
    }
#endif

void get_timestamp(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);
    if (local_time) {
        strftime(buffer, size, "%Y-%m-%d %H:%M:%S", local_time);
    } else {
        snprintf(buffer, size, "Invalid time");
    }
}

void write_log(const char *filename, const char *message) {
    FILE *file = fopen(filename, "a");
    if (!file) {
        printf("Error: can't open file");
    }
    fprintf(file, "%s\n", message);
    fclose(file);
}

#include <stdio.h>
#include <time.h>
#include <string.h>

#define MAX_LINE_LENGTH 256

void trim_log(const char *filename, int max_age_seconds) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening log file");
        return;
    }

    FILE *temp = fopen("temp.log", "w");
    if (!temp) {
        perror("Error opening temporary file");
        fclose(file);
        return;
    }

    time_t now = time(NULL);
    char line[MAX_LINE_LENGTH];

    while (fgets(line, sizeof(line), file)) {
        struct tm timestamp = {0};
        int parsed = sscanf(line, "%d-%d-%d %d:%d:%d",
                            &timestamp.tm_year, &timestamp.tm_mon, &timestamp.tm_mday,
                            &timestamp.tm_hour, &timestamp.tm_min, &timestamp.tm_sec);

        if (parsed == 6) {
            timestamp.tm_year -= 1900;
            timestamp.tm_mon -= 1;
            time_t entry_time = mktime(&timestamp);

            if (entry_time != -1 && difftime(now, entry_time) > max_age_seconds) {
                continue;
            }
        }

        fputs(line, temp);
    }

    fclose(file);
    fclose(temp);

    if (remove(filename) != 0) {
        perror("Error removing original log file");
        return;
    }
    if (rename("temp.log", filename) != 0) {
        perror("Error renaming temporary file");
        return;
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
        write_log(LOG_ALL, log_entry);
        trim_log(LOG_ALL, DAY);

        hourly_sum += temperature;
        hourly_count++;
        daily_sum += temperature;
        daily_count++;

        time_t now = time(NULL);

        if ((int)difftime(now, last_hour_time) % HOUR == 0 && now != last_hour_time) {
            float hourly_avg = hourly_sum / hourly_count;
            snprintf(log_entry, sizeof(log_entry), "%s %.2f", timestamp, hourly_avg);
            write_log(LOG_HOURLY, log_entry);
            trim_log(LOG_HOURLY, 30 * DAY);

            hourly_sum = 0.0;
            hourly_count = 0;
            last_hour_time = now;
        }

        if ((int)difftime(now, last_day_time) % DAY == 0 && now != last_day_time) {
            float daily_avg = daily_sum / daily_count;
            snprintf(log_entry, sizeof(log_entry), "%s Daily Avg: %.2f", timestamp, daily_avg);
            write_log(LOG_DAILY, log_entry);
            trim_log(LOG_HOURLY, 365 * DAY);

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

    return 0;
}
