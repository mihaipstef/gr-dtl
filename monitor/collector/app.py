from api import (
    flask_app,
    celery_app
)
from api.routes import *

app = flask_app
celery = celery_app.celery

