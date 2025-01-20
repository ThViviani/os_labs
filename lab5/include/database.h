#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>

#define CURR_TEMP_TABLE "temp"
#define HOURLY_AVG_TABLE "hourly_avg"
#define DAILY_AVG_TABLE "daily_avg"

void add_temperature_to_db(sqlite3 *db, const char *table_name, const char *timestamp, double temperature);

sqlite3 *init_database();

#endif // DATABASE_H