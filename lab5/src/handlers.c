#include "handlers.h"

#define BUFFER_SIZE 1024

void send_http_response(int sock, const char *status_code, const char *body)
{
    char headers[BUFFER_SIZE];
    int header_length = snprintf(headers, sizeof(headers),
        "HTTP/1.1 %s\r\n"
        "Content-Type: application/json\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Content-Length: %zu\r\n"
        "\r\n", status_code, strlen(body));

    if (header_length < 0 || header_length >= BUFFER_SIZE)
    {
        fprintf(stderr, "Error formatting HTTP headers\n");
        return;
    }

    if (send(sock, headers, header_length, 0) < 0)
    {
        perror("Error sending HTTP headers");
        return;
    }

    if (send(sock, body, strlen(body), 0) < 0)
    {
        perror("Error sending HTTP body");
    }
}

void send_404_error_response(int sock)
{
    cJSON *response_object = cJSON_CreateObject();
    cJSON_AddStringToObject(response_object, "error", "404 Not Found");
    cJSON_AddStringToObject(response_object, "message", "The requested resource could not be found.");

    char *response_body = cJSON_PrintUnformatted(response_object);
    send_http_response(sock, "404 Not Found", response_body);

    cJSON_Delete(response_object);
    free(response_body);
}

void send_400_error_response(int sock)
{
    cJSON *response_object = cJSON_CreateObject();
    cJSON_AddStringToObject(response_object, "error", "400 Bad Request");
    cJSON_AddStringToObject(response_object, "message", "The request is malformed. Please check the syntax.");

    char *response_body = cJSON_PrintUnformatted(response_object);
    send_http_response(sock, "400 Bad Request", response_body);

    cJSON_Delete(response_object);
    free(response_body);
}

void fetch_avg_temperature(int sock, sqlite3 *db, const char *table_name)
{
    sqlite3_stmt *stmt;
    char query[128];

    snprintf(query, sizeof(query), "SELECT timestamp, temp FROM %s;", table_name);

    if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK)
    {
        fprintf(stderr, "Database query preparation failed: %s\n", sqlite3_errmsg(db));
        send_400_error_response(sock);
        return;
    }

    cJSON *response_array = cJSON_CreateArray();

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const unsigned char *timestamp = sqlite3_column_text(stmt, 0);
        double temperature = sqlite3_column_double(stmt, 1);

        cJSON *record = cJSON_CreateObject();
        cJSON_AddStringToObject(record, "timestamp", (const char *)timestamp);
        cJSON_AddNumberToObject(record, "temp", temperature);
        cJSON_AddItemToArray(response_array, record);
    }

    sqlite3_finalize(stmt);

    char *json_response = cJSON_Print(response_array);
    send_http_response(sock, "200 OK", json_response);

    cJSON_Delete(response_array);
    free(json_response);
}

void fetch_current_temperature(int sock, sqlite3 *db)
{
    const char *query = "SELECT timestamp, temp FROM temp ORDER BY timestamp DESC LIMIT 1;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK)
    {
        fprintf(stderr, "Database query preparation failed: %s\n", sqlite3_errmsg(db));
        send_http_response(sock, "500 Internal Server Error", "{\"error\": \"Database query failed\"}");
        return;
    }

    cJSON *response_object = cJSON_CreateObject();

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const unsigned char *timestamp = sqlite3_column_text(stmt, 0);
        double temperature = sqlite3_column_double(stmt, 1);

        cJSON_AddStringToObject(response_object, "timestamp", (const char *)timestamp);
        cJSON_AddNumberToObject(response_object, "temp", temperature);
    }
    else
    {
        cJSON_AddStringToObject(response_object, "error", "No data available");
    }

    char *response_body = cJSON_PrintUnformatted(response_object);
    send_http_response(sock, "200 OK", response_body);

    sqlite3_finalize(stmt);
    cJSON_Delete(response_object);
    free(response_body);
}
