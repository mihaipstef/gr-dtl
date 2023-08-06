from api import (
    collector_tracker,
    db,
    flask_app,
    sub
)
import api.tracker as tracker
from celery.result import AsyncResult
import datetime as dt
from flask import jsonify, request


@flask_app.after_request
def cors_allow_origin_all(resp):
    resp.headers.add('Access-Control-Allow-Origin', '*')
    return resp


@flask_app.route("/collections", methods=['GET'])
def get_collections_handler():
    collections = db.list_collection_names(
        filter={"name": {"$nin": ["collector_tracker"]}})
    return jsonify(json_list=list(collections)), 200


@flask_app.route("/collection/<collection_name>/probes", methods=['GET'])
def get_collection_probes_handler(collection_name):
    pass


@flask_app.route("/collector/all", methods=['GET'])
def get_all_collectors_handler():
    tracker.update_all_collectors_state()
    tracked_collectors = tracker.find_tracked_collectors()
    return jsonify(json_list=list(tracked_collectors)), 200


@flask_app.route("/collector/active", methods=['GET'])
def get_active_collectors_handler():
    tracker.update_all_collectors_state()
    tracked_collectors = tracker.find_tracked_collectors(states=["PENDING", "STARTED"])
    return jsonify(json_list=list(tracked_collectors)), 200


@flask_app.route("/collector/<collection_name>/start", methods=['POST'])
def collect_start_handler(collection_name):
    collection_overwrite = request.form.get("collection_overwrite", False)
    probe_url = request.form.get("probe_url", "tcp://127.0.0.1:5555")

    # Stop collector
    collector = tracker.get_tracked_collector(collection_name)
    if collector is not None:
        result = AsyncResult(collector["collector_id"])
        result.revoke()

    # Delete collection data if is to overwrite
    collection = db[collection_name]
    if collection is not None and collection_overwrite:
        collection.remove({})

    result = sub.subscriber.delay(collection_name, 60000, probe_url)
    collector_id = result.id

    # Update tracker data
    if collector is None:
        tracker.insert_tracked_collector(collection_name, collector_id, probe_url, result.state)
    else:
        tracker.update_tracked_collector(collection_name, collector_id, probe_url, result.state)
    return {"collector_state": result.state}, 200


@flask_app.route("/collector/<collection_name>/stop", methods=['POST'])
def collect_stop_handler(collection_name):
    collector = tracker.get_tracked_collector(collection_name)
    if collector is not None:
        result = AsyncResult(collector["collector_id"])
        result.revoke()
        return {"collector_state": result.state}, 200
    else:
        return {}, 404


@flask_app.route("/collector/<collection_name>/status", methods=['GET'])
def collect_status_handler(collection_name):
    collector = tracker.get_tracked_collector(collection_name)
    if collector is not None:
        result = AsyncResult(collector["collector_id"])
        return {"collector_state": result.state}, 200
    else:
        return {"error": "collection_not_found"}, 404
