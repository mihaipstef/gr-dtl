#!/bin/bash

# Start SQLite
# /usr/bin/sqlite3 /db/monitor.sqlite &

# Start backend (dev web server should be ok as is for internal use)
FLASK_APP=collector/app.py flask run --host 0.0.0.0 &

# Start frontend
npm --prefix ui/ run preview -- --host 0.0.0.0