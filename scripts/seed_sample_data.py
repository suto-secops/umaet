#!/usr/bin/env python3
"""
Seed script for umaet — writes sample transactions and budgets
directly into the live SQLite database.
"""
import sqlite3, os, sys
from pathlib import Path

DB = Path.home() / ".local/share/umaet/umaet.db"

if not DB.exists():
    print(f"Database not found at {DB}")
    sys.exit(1)

SAMPLES = [
    # Late 2025
    ("2025-10-01","income",3200.00,"Salary","October Salary", "Monthly Salary October 2025"),
    ("2025-10-02","expense",900.00,"Housing","Rent", "Apartment Rent"),
    ("2025-10-03","expense",65.40,"Food & Dining","Lidl", "Weekly Groceries Lidl"),
    ("2025-10-05","expense",55.00,"Transportation","Transit Pass", "Monthly Transit Pass"),
    ("2025-10-12","expense",84.10,"Food & Dining","Mercadona", "Supermarket Mercadona"),
    ("2025-10-15","expense",45.00,"Utilities","Internet", "Fiber Internet"),
    ("2025-10-20","expense",32.00,"Entertainment","Cinema", "Cinema and Popcorn"),
    ("2025-10-25","expense",78.50,"Shopping","Jacket", "Autumn Jacket"),
    ("2025-11-01","income",3200.00,"Salary","November Salary", "Monthly Salary November 2025"),
    ("2025-11-02","expense",900.00,"Housing","Rent", "Apartment Rent"),
    ("2025-11-04","expense",110.00,"Utilities","Power & Water", "Electricity & Water"),
    ("2025-11-10","expense",95.20,"Food & Dining","Groceries", "Groceries"),
    ("2025-11-15","income",450.00,"Investments","Dividends", "Stock Dividends"),
    ("2025-11-20","expense",120.00,"Shopping","Tech Gadget", "Black Friday Tech Gadget"),
    ("2025-12-01","income",3200.00,"Salary","December Salary", "Monthly Salary December 2025"),
    ("2025-12-02","expense",900.00,"Housing","Rent", "Apartment Rent"),
    ("2025-12-10","expense",145.00,"Food & Dining","Holiday Dinner", "Holiday Dinner with family"),
    ("2025-12-18","expense",180.00,"Shopping","Gifts", "Christmas Gifts"),
    ("2025-12-24","expense",85.00,"Entertainment","Theater", "Holiday Theater"),
    # 2026 Q1
    ("2026-01-01","income",3350.00,"Salary","New Year Raise", "Monthly Salary with New Year Raise"),
    ("2026-01-02","expense",920.00,"Housing","Apartment Rent"),
    ("2026-01-04","expense",72.30,"Food & Dining","Weekly Groceries"),
    ("2026-01-08","expense",55.00,"Transportation","Monthly Transit Pass"),
    ("2026-01-14","expense",98.40,"Utilities","Winter Heating & Electricity"),
    ("2026-01-18","expense",64.50,"Food & Dining","Supermarket run"),
    ("2026-01-22","expense",25.00,"Entertainment","Streaming Subscriptions"),
    ("2026-01-28","expense",45.00,"Healthcare","Dental Cleaning"),
    ("2026-02-01","income",3350.00,"Salary","Monthly Salary February 2026"),
    ("2026-02-02","expense",920.00,"Housing","Apartment Rent"),
    ("2026-02-05","expense",88.00,"Food & Dining","Costco Wholesale Bulk"),
    ("2026-02-11","expense",45.00,"Utilities","Fiber Internet"),
    ("2026-02-14","expense",92.50,"Food & Dining","Valentine Dinner"),
    ("2026-02-20","income",600.00,"Salary","Freelance Web Design"),
    ("2026-02-24","expense",55.00,"Transportation","Train Ticket Weekend Trip"),
    ("2026-03-01","income",3350.00,"Salary","Monthly Salary March 2026"),
    ("2026-03-02","expense",920.00,"Housing","Apartment Rent"),
    ("2026-03-04","expense",79.20,"Food & Dining","Groceries & Fresh produce"),
    ("2026-03-09","expense",55.00,"Transportation","Monthly Transit Pass"),
    ("2026-03-15","expense",82.00,"Utilities","Electricity and Water"),
    ("2026-03-18","expense",42.00,"Entertainment","Board Game Night snacks"),
    ("2026-03-23","expense",115.00,"Shopping","Spring Running Shoes"),
    ("2026-03-29","expense",68.30,"Food & Dining","Groceries"),
    # 2026 Q2
    ("2026-04-01","income",3350.00,"Salary","Monthly Salary April 2026"),
    ("2026-04-02","expense",920.00,"Housing","Apartment Rent"),
    ("2026-04-06","expense",85.00,"Food & Dining","Weekly Groceries"),
    ("2026-04-10","expense",45.00,"Utilities","Fiber Internet"),
    ("2026-04-16","expense",38.00,"Entertainment","Museum Exhibition Tickets"),
    ("2026-04-20","expense",74.50,"Food & Dining","Supermarket"),
    ("2026-04-25","expense",45.00,"Transportation","Car refueling"),
    ("2026-05-01","income",3350.00,"Salary","Monthly Salary May 2026"),
    ("2026-05-02","expense",920.00,"Housing","Apartment Rent"),
    ("2026-05-05","expense",91.20,"Food & Dining","Farmer Market & Grocery"),
    ("2026-05-12","expense",75.00,"Utilities","Spring Utility bill"),
    ("2026-05-15","income",500.00,"Investments","Quarterly Dividends"),
    ("2026-05-18","expense",120.00,"Entertainment","Outdoor Music Festival"),
    ("2026-05-22","expense",86.40,"Food & Dining","Weekly Groceries"),
    ("2026-05-27","expense",55.00,"Transportation","Transit card reload"),
    ("2026-06-01","income",3350.00,"Salary","Monthly Salary June 2026"),
    ("2026-06-02","expense",920.00,"Housing","Apartment Rent"),
    ("2026-06-06","expense",68.00,"Food & Dining","Groceries"),
    ("2026-06-11","expense",45.00,"Utilities","Fiber Internet"),
    ("2026-06-15","expense",160.00,"Shopping","Summer clothes"),
    ("2026-06-20","expense",94.10,"Food & Dining","Organic Supermarket"),
    ("2026-06-25","expense",60.00,"Healthcare","Eye exam & new contacts"),
    # 2026 Q3
    ("2026-07-01","income",3350.00,"Salary","Monthly Salary July 2026"),
    ("2026-07-02","expense",920.00,"Housing","Apartment Rent"),
    ("2026-07-05","expense",96.50,"Food & Dining","Weekly Groceries & BBQ"),
    ("2026-07-08","expense",55.00,"Transportation","Transit pass"),
    ("2026-07-14","expense",110.00,"Utilities","AC & Summer electricity"),
    ("2026-07-19","expense",75.00,"Entertainment","Beach weekend outing"),
    ("2026-07-24","expense",82.30,"Food & Dining","Supermarket"),
    ("2026-08-01","income",3350.00,"Salary","Monthly Salary August 2026"),
    ("2026-08-01","income",800.00,"Salary","Summer Performance Bonus"),
    ("2026-08-02","expense",920.00,"Housing","Apartment Rent"),
    ("2026-08-04","expense",85.40,"Food & Dining","Groceries"),
    ("2026-08-10","expense",45.00,"Utilities","Fiber Internet"),
    ("2026-08-14","expense",140.00,"Transportation","Flight ticket domestic visit"),
    ("2026-08-18","expense",92.00,"Food & Dining","Dining out with friends"),
    ("2026-08-25","expense",78.00,"Shopping","Backpack & travel accessories"),
    ("2026-09-01","income",3350.00,"Salary","Monthly Salary September 2026"),
    ("2026-09-02","expense",920.00,"Housing","Apartment Rent September"),
    ("2026-09-03","expense",68.50,"Food & Dining","Weekly Groceries Lidl"),
    ("2026-09-04","expense",55.00,"Transportation","Monthly Transit Pass"),
    ("2026-09-06","expense",45.00,"Utilities","Fiber Internet"),
    ("2026-09-07","expense",34.20,"Food & Dining","Lunch bistro"),
    ("2026-09-08","expense",28.50,"Entertainment","Cinema weekend"),
    ("2026-09-09","expense",52.00,"Shopping","Stationery and tech cable"),
    ("2026-09-10","expense",41.80,"Food & Dining","Supermarket restock"),
]

BUDGETS = [
    ("_GLOBAL_",       2500.00),
    ("Food & Dining",   450.00),
    ("Housing",         950.00),
    ("Transportation",  180.00),
    ("Utilities",       160.00),
    ("Entertainment",   140.00),
    ("Shopping",        200.00),
]

CATEGORIES = list({row[3] for row in SAMPLES})

con = sqlite3.connect(DB)
cur = con.cursor()

# ensure tables exist (app may not have run yet)
cur.executescript("""
PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE IF NOT EXISTS transactions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    type TEXT NOT NULL CHECK(type IN ('income','expense')),
    amount REAL NOT NULL CHECK(amount >= 0),
    category TEXT NOT NULL,
    date TEXT NOT NULL,
    note TEXT,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS budgets (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    category TEXT UNIQUE NOT NULL,
    monthly_limit REAL NOT NULL CHECK(monthly_limit >= 0)
);

CREATE TABLE IF NOT EXISTS categories (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT UNIQUE NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_trans_date     ON transactions(date);
CREATE INDEX IF NOT EXISTS idx_trans_category ON transactions(category);
""")

# Seed categories
for cat in CATEGORIES:
    cur.execute("INSERT OR IGNORE INTO categories (name) VALUES (?)", (cat,))

# Seed budgets
for cat, limit in BUDGETS:
    cur.execute(
        "INSERT INTO budgets (category, monthly_limit) VALUES (?,?) "
        "ON CONFLICT(category) DO UPDATE SET monthly_limit=?",
        (cat, limit, limit)
    )

# Seed transactions
cur.executemany(
    "INSERT INTO transactions (date,type,amount,category,note) VALUES (?,?,?,?,?)",
    SAMPLES
)

con.commit()
con.close()

total = len(SAMPLES)
print(f"✓ Seeded {total} transactions + {len(BUDGETS)} budgets into {DB}")
