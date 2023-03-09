from api import crud, flask_app
from celery import shared_task
from celery.result import AsyncResult
import pmt
import time
import zmq

_db_mapping = {
    "": "tx_frame_count",
    "": "rx_frame_count",
    "payload_crc_success": "payload_crc_success",
    "payload_crc_failed": "payload_crc_failed",
    "": "header_crc_success",
    "": "header_crc_failed",
    "estimated_snr_tag_key": "snr_est",
    "feedback_constellation_key": "current_bps",
}

def parse_msg(msg):
    try:
        msg = pmt.deserialize_str(msg.decode('ascii'))
        monitor_data = {}

        if pmt.is_dict(msg):
            d = pmt.to_python(msg)
            print(d)
            for k in _db_mapping.keys():
                if k in d:
                    monitor_data[_db_mapping[k]] = d[k]
        return monitor_data
    except Exception as e:
        # We get weird encoding even when serialize_str is used
        pass
    return {}


@shared_task(ignore_result=False)
def subscriber(pair_id, timeout, url="tcp://*:5551", bind=False):
    step = 10
    time_wait = 0
    ctx = zmq.Context()
    socket = ctx.socket(zmq.SUB)
    if bind:
        socket.bind(url)
    else:
        socket.connect(url)
    socket.setsockopt(zmq.SUBSCRIBE, "".encode("ascii"))
    print(f"Subscribe to: {url}\nWaiting for data...")
    with flask_app.app_context():
        while time_wait <= timeout:
            try:
                msg = socket.recv(zmq.NOBLOCK)
                msg_dict = parse_msg(msg)
                crud.update_monitor_data(pair_id, **msg_dict)
                time_wait = 0
            except Exception as e:
                time_wait += step
                time.sleep(step/1000)
        db_sub = crud.retrieve_subscriber(pair_id=pair_id)
        if db_sub:
            active_count = 0
            for s_id in [db_sub.rx_sub_id, db_sub.tx_sub_id]:
                if s_id:
                    r = AsyncResult(s_id)
                    if not r.ready():
                        active_count += 1
            print(f"active subscribers={active_count}")
            crud.update_subscribers(pair_id, is_active=(active_count > 0))
        print("done")
    return True
