#!/bin/bash

# Start backend (dev web server should be ok as is for internal use)
cd collector
FLASK_APP=app.py flask run --host 0.0.0.0 &
celery -A app.celery worker &
cd ..

# Start frontend
npm --prefix ui/ run preview -- --host 0.0.0.0