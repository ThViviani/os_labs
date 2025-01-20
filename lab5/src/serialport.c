#include "serialport.h"

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
        char buffer[256];
        int bytes_read = read(fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            return strtof(buffer, NULL);
        }
        return -1.0;
    }
#endif