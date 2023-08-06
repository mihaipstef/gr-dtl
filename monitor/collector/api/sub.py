from api import flask_app, collector_tracker, db
from celery import shared_task
from celery.result import AsyncResult
import datetime as dt
import pmt
import time
import zmq
import traceback

def parse_msg(msg):
    try:
        msg = pmt.deserialize_str(msg)
        if pmt.is_dict(msg):
            return pmt.to_python(msg)
    except Exception as e:
        print(msg)
        print(str(e))
    return None


@shared_task(ignore_result=False, track_started=True)
def subscriber(collection_name, timeout, url):
    step = 10
    time_wait = 0
    ctx = zmq.Context()
    socket = ctx.socket(zmq.SUB)
    socket.setsockopt_string(zmq.SUBSCRIBE, "")
    socket.connect(url)
    collection = db[collection_name]
    print(f"Subscribe to: {url}\nWaiting for data...")
    with flask_app.app_context():
        while time_wait <= timeout:
            try:
                msg = socket.recv(zmq.NOBLOCK)
                msg_dict = parse_msg(msg)
                if msg_dict is not None:
                    ts = dt.datetime.utcnow()
                    msg_dict["insert_ts"] = ts
                    collection.insert_one(msg_dict)
                    print(msg_dict)
                    collector_tracker.update_one({"collection_name": collection_name}, {
                        "$set": {"last_insert": ts}})
                time_wait = 0
            except Exception as e:
                time_wait += step
                time.sleep(step/1000)
    return True
