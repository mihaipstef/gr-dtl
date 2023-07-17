#!/usr/bin/env python3

import argparse
import pmt
import zmq

from datetime import datetime
import pymongo

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


parser = argparse.ArgumentParser()
parser.add_argument("--probe", type=str, default="tcp://127.0.0.1:5555", help="Address of the monitoring probe")
parser.add_argument("--db", type=str, default="mongodb://probe:probe@127.0.0.1:27017/", help="URI of the database that stores probe data")

args = parser.parse_args()

db_client = pymongo.MongoClient(args.db)
db = db_client["probe_data"]

ctx = zmq.Context()
socket = ctx.socket(zmq.SUB)
socket.setsockopt_string(zmq.SUBSCRIBE, "")
socket.bind(args.probe)

while True:
    msg = socket.recv()
    probe_data = parse_msg(msg)
    if "collection" in probe_data:
        collection = db[probe_data["collection"]]
        probe_document = probe_data["msg"]
        probe_document["probe_name"] = probe_data.get("probe_name", "john_doe")
        probe_document["created_date"] = datetime.utcnow()
        collection.insert_one(probe_document)
        print(probe_document)