#ifndef SERIALPORT_H
#define SERIALPORT_H

#include <stdio.h>
#include <stdlib.h>
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

    HANDLE open_port();

    float read_port(HANDLE hComm);
#else
    #define SERIAL_PORT "/dev/ttys011"

    int open_port();

    float read_port(int fd);
#endif

#endif // SERIALPORT_H