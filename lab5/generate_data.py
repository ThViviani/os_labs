import sqlite3
import random
from datetime import datetime, timedelta

DATABASE_PATH = "db/temp.db"

HOUR_START = datetime(2024, 8, 4)
HOUR_END = datetime(2025, 1, 19)

DAY_START = datetime(2024, 8, 4)
DAY_END = datetime(2025, 1, 19)

def create_database():
    conn = sqlite3.connect(DATABASE_PATH)
    cur = conn.cursor()

    cur.execute("""
        CREATE TABLE IF NOT EXISTS hourly_avg (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp DATETIME,
            temp REAL
        );
    """)

    cur.execute("""
        CREATE TABLE IF NOT EXISTS daily_avg (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp DATETIME,
            temp REAL
        );
    """)

    conn.commit()
    conn.close()

def random_temperature(low=-20, high=32):
    return round(random.uniform(low, high), 2)  # Возвращаем просто число с двумя знаками после запятой

def populate_hourly_data():
    conn = sqlite3.connect(DATABASE_PATH)
    cur = conn.cursor()

    date_iter = HOUR_START

    while date_iter <= HOUR_END:
        temp = random_temperature()
        cur.execute("""
            INSERT INTO hourly_avg (timestamp, temp)
            VALUES (?, ?);
        """, (date_iter.strftime("%Y-%m-%d %H:%M:%S"), temp))
        date_iter += timedelta(hours=1)

    conn.commit()
    conn.close()

def populate_daily_data():
    conn = sqlite3.connect(DATABASE_PATH)
    cur = conn.cursor()

    date_iter = DAY_START

    while date_iter <= DAY_END:
        temp = random_temperature()
        cur.execute("""
            INSERT INTO daily_avg (timestamp, temp)
            VALUES (?, ?);
        """, (date_iter.strftime("%Y-%m-%d %H:%M:%S"), temp))
        date_iter += timedelta(days=1)

    conn.commit()
    conn.close()

create_database()
populate_hourly_data()
populate_daily_data()

print(f"Temperature data successfully generated and saved to {DATABASE_PATH}.")
