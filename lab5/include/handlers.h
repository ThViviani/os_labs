#ifndef HANDLERS_H
#define HANDLERS_H

#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string.h>
#include "cJSON.h"
#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <sys/socket.h>
#endif

void send_http_response(int sock, const char *status_code, const char *body);

void send_404_error_response(int sock);

void send_400_error_response(int sock);

void fetch_avg_temperature(int sock, sqlite3 *db, const char *table_name);

void fetch_current_temperature(int sock, sqlite3 *db);

#endif // HANDLERS_H