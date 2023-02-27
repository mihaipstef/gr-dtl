from celery import Celery, current_app
from flask import Flask
from flask_celeryext import FlaskCeleryExt 
from flask_migrate import Migrate
from flask_sqlalchemy import SQLAlchemy

class Config:
    SQLALCHEMY_DATABASE_URI = "sqlite:///monitor.sqlite"
    CELERY_BROKER_URL="sqla+sqlite:///celerydb.sqlite"
    CELERY_RESULT_BACKEND="db+sqlite:///results.sqlite"


config = {
    "default": Config(),
}

db = SQLAlchemy()
migrate = Migrate()

def celery_create_app(app: Flask) -> Celery:
    _celery_app = current_app
    print(app.config)
    _celery_app.config_from_object(app.config, namespace="CELERY")
    return _celery_app

celery_app = FlaskCeleryExt(create_celery_app=celery_create_app)

def flask_create_app():
    flask_app = Flask(__name__)
    flask_app.config.from_object(config["default"])
    db.init_app(flask_app)
    migrate.init_app(flask_app, db)
    celery_app.init_app(flask_app)

    return flask_app

flask_app = flask_create_app()