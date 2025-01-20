#include <stdio.h>
#include <stdlib.h>
#include "database.h"

void add_temperature_to_db(sqlite3 *db, const char *table_name, const char *timestamp, double temperature) {
    char query[128];
    sqlite3_stmt *stmt = NULL;

    snprintf(query, sizeof(query), "INSERT INTO %s (timestamp, temp) VALUES (?, ?);", table_name);

    if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return;
    }

    if (sqlite3_bind_text(stmt, 1, timestamp, -1, SQLITE_STATIC) != SQLITE_OK) {
        fprintf(stderr, "Failed to bind timestamp: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return;
    }

    if (sqlite3_bind_double(stmt, 2, temperature) != SQLITE_OK) {
        fprintf(stderr, "Failed to bind temperature: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return;
    }

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
}


sqlite3 *init_database() {
    sqlite3 *db;
    char *err_msg = 0;

    int rc = sqlite3_open("../db/temp.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error opening database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(EXIT_FAILURE);
    }

    const char *sql =
        "CREATE TABLE IF NOT EXISTS temp ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "timestamp DATETIME, "
        "temp REAL);"
        "CREATE TABLE IF NOT EXISTS hourly_avg ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "timestamp DATETIME, "
        "temp REAL);"
        "CREATE TABLE IF NOT EXISTS daily_avg ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "timestamp DATETIME, "
        "temp REAL);";

    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Queries execution error %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Database initialized successfully.\n");
    return db;
}