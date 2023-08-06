from api import (
    collector_tracker,
)
from celery.result import (
    AsyncResult,
)
import datetime as dt


def update_collector_state(collector):
    result = AsyncResult(collector["collector_id"])
    if result.state != collector["last_state"]:
        collector_tracker.update_one(
            {"collection_name": collector["collection_name"]}, {
                "$set": {
                    "last_update": dt.datetime.utcnow(),
                    "last_state": result.state}})
        return True
    return False


def update_all_collectors_state():
    updated = False
    all_collectors = collector_tracker.find({})
    for collector in all_collectors:
        updated = updated or update_collector_state(collector)
    return updated


def get_tracked_collector(collection_name):
    collector = collector_tracker.find_one(
        {"collection_name": {"$in": [collection_name]}})
    return collector


def find_tracked_collectors(states=None, collections=None):

    query_and = []
    if states:
        query_and.append({"last_state": {"$in": states}})
    if collections:
        query_and.append({"collection_name": {"$in": collections}})
    query = {}
    if len(query_and):
        query = {"$and": query_and}
    tracked_collectors = collector_tracker.find(query, {"_id": 0})
    return tracked_collectors


def insert_tracked_collector(collection_name, collector_id, probe_url, state):
    collector_tracker.insert_one(
        {"collection_name": collection_name,
         "last_update": dt.datetime.utcnow(),
         "last_insert": None,
         "collector_id": collector_id,
         "probe_url": probe_url,
         "last_state": state})


def update_tracked_collector(collection_name, collector_id, probe_url, state):
     collector_tracker.update_one(
        {"collection_name": collection_name}, {
            "$set": {
                "last_update": dt.datetime.utcnow(),
                "collector_id": collector_id,
                "probe_url": probe_url,
                "last_state": state}})