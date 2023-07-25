import datetime
import pmt
import zmq

def parse_msg(msg):
    try:
        msg = pmt.deserialize_str(msg.decode('ascii'))
        monitor_data = {}
        if pmt.is_dict(msg):
            monitor_data = pmt.to_python(msg)

        return monitor_data
    except Exception as e:
        # We get weird encoding even when serialize_str is used
        pass
    return {}


def start_collect(probe, db, collection_name):
    ctx = zmq.Context()
    socket = ctx.socket(zmq.SUB)
    socket.setsockopt_string(zmq.SUBSCRIBE, "")
    socket.bind(probe)
    collection = db[collection_name]

    while True:
        msg = socket.recv()
        probe_data = parse_msg(msg)
        if "msg" in probe_data:
            probe_document = probe_data["msg"]
            probe_document["probe_name"] = probe_data.get("probe_name", "john_doe")
            probe_document["created_date"] = datetime.datetime.utcnow()
            collection.insert_one(probe_document)
