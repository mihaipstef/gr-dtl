from api import (
    celery_app,
    crud,
    flask_create_app,
)
from celery.result import AsyncResult
from flask import jsonify, request
import sub


app = flask_create_app()
celery = celery_app.celery

# Routes

@app.after_request
def cors_allow_origin_all(resp):
    resp.headers.add('Access-Control-Allow-Origin', '*')
    return resp

@app.route("/pair", methods=['GET'])
def get_pairs_handler():
    subscribers = crud.retrieve_subscribers(True)
    return jsonify(json_list=[s.to_dict() for s in subscribers]), 200


@app.route("/pair/<pair_id>/monitor_data", methods=['GET'])
def get_monitor_data_handler(pair_id):
    stats = crud.retrieve_monitor_data(pair_id)
    if stats:
        return jsonify(stats.to_dict()), 200
    return jsonify({}), 400


@app.route("/pair/subscribe", methods=['POST'])
def set_subscriber_handler():
    tx_url = request.form.get("tx_url", None)
    rx_url = request.form.get("rx_url", None)
    pair_id = request.form.get("pair_id", None)
    description = request.form.get("description", "")
    if pair_id is None:
        return "missing pair id", 400

    subscribers = {"pair_id": pair_id, "rx_id": None, "tx_id": None}
    # Makes the API call idempotent
    if (s := crud.retrieve_subscriber(pair_id)):
        if s.is_active:
            subscribers["rx_id"] = s.rx_sub_id
            subscribers["tx_id"] = s.tx_sub_id
            return jsonify(subscribers), 200
        else:
            return "pair_id exists", 400
    # Start subscriber tasks
    for id_key, url in [("rx_id", rx_url), ("tx_id", tx_url)]:
        if url is not None:
            result = sub.subscriber.delay(pair_id, 10000, url)
            subscribers[id_key] = result.id
    
    # Create subscriber entry in DB
    crud.create_subscibers(
        pair_id, rx_sub_id=subscribers["rx_id"], tx_sub_id=subscribers["tx_id"], is_active=True, description=description)
    if not crud.create_monitor_data(pair_id=pair_id):
        return "failed", 500
    return jsonify(subscribers), 200


@app.route("/pair/<id>", methods=["GET"])
def get_subscriber_status(id):
    result = AsyncResult(id)
    return {
        "ready": result.ready(),
        "successful": result.successful(),
    }
