#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
    #include <fcntl.h>
    #include <termios.h>
    #include <signal.h>
#endif

#ifdef _WIN32
    #define SERIAL_PORT "COM1"
#else
    #define SERIAL_PORT "/dev/ttys010"
#endif

volatile int terminate = 0;

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
        if (!SetConsoleCtrlHandler(custom_signal_handler, TRUE)) {
            fprintf(stderr, "Error setting signal handler.\n");
            return EXIT_FAILURE;
        }

        HANDLE serial_port = CreateFileA(SERIAL_PORT, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (serial_port == INVALID_HANDLE_VALUE) {
            fprintf(stderr, "Failed to open serial port: %s\n", SERIAL_PORT);
            return -1;
        }

        DCB serial_config = {0};
        serial_config.DCBlength = sizeof(serial_config);
        GetCommState(serial_port, &serial_config);
        serial_config.BaudRate = CBR_9600;
        serial_config.ByteSize = 8;
        serial_config.Parity = NOPARITY;
        serial_config.StopBits = ONESTOPBIT;
        SetCommState(serial_port, &serial_config);
    #else
        struct sigaction sa;
        sa.sa_handler = custom_signal_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        if (sigaction(SIGINT, &sa, NULL) == -1) {
            perror("Error setting signal handler");
            exit(EXIT_FAILURE);
        }

        int serial_port = open(SERIAL_PORT, O_WRONLY | O_NOCTTY);
        if (serial_port < 0) {
            perror("Failed to open serial port");
            return -1;
        }

        struct termios serial_options = {0};
        if (tcgetattr(serial_port, &serial_options) != 0) {
            perror("Failed to get attributes");
            close(serial_port);
            return -1;
        }
        cfsetospeed(&serial_options, B9600);
        cfsetispeed(&serial_options, B9600);

        serial_options.c_cflag = (serial_options.c_cflag & ~CSIZE) | CS8;
        serial_options.c_cflag |= CLOCAL | CREAD;
        serial_options.c_cflag &= ~(PARENB | PARODD);
        serial_options.c_cflag &= ~CSTOPB;
        if (tcsetattr(serial_port, TCSANOW, &serial_options) != 0) {
            perror("Failed to set attributes");
            close(serial_port);
            return -1;
        }
    #endif

    while (!terminate) {
        srand(time(NULL));
        float current_temperature = (rand() % 61) - 30;
        char temp_message[32];
        snprintf(temp_message, sizeof(temp_message), "%.2f\n", current_temperature);

        #ifdef _WIN32
            DWORD bytes_written;
            WriteFile(serial_port, temp_message, strlen(temp_message), &bytes_written, NULL);
        #else
            write(serial_port, temp_message, strlen(temp_message));
        #endif
        printf("Sent to port %d message: %s", serial_port, temp_message);

        #ifdef _WIN32
            Sleep(1000);
        #else
            sleep(1);
        #endif
    }

    #ifdef _WIN32
        CloseHandle(serial_port);
    #else
        close(serial_port);
    #endif

    return 0;
}
