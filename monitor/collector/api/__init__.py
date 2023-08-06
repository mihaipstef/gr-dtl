from celery import Celery, current_app
from flask import Flask
from flask_celeryext import FlaskCeleryExt 
import pymongo

class Config:
    DATABASE_URI = "mongodb://probe:probe@127.0.0.1:27017/"
    CELERY_BROKER_URL="sqla+sqlite:///celerydb.sqlite"
    CELERY_RESULT_BACKEND="db+sqlite:///results.sqlite"


config = {
    "default": Config(),
}

db_client = pymongo.MongoClient(config["default"].DATABASE_URI)
db = db_client["probe_data"]
collector_tracker = db["collector_tracker"]
collector_tracker.create_index(
    [("collection_name", pymongo.ASCENDING)], unique=True)

def celery_create_app(app: Flask) -> Celery:
    _celery_app = current_app
    _celery_app.config_from_object(app.config, namespace="CELERY")
    return _celery_app

celery_app = FlaskCeleryExt(create_celery_app=celery_create_app)

def flask_create_app():
    flask_app = Flask(__name__)
    flask_app.config.from_object(config["default"])
    celery_app.init_app(flask_app)
    return flask_app

flask_app = flask_create_app()