#!/usr/bin/env python3

import argparse
import monitoring
import pymongo


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--probe", type=str, default="tcp://127.0.0.1:5555", help="Address of the monitoring probe")
    parser.add_argument("--db", type=str, default="mongodb://probe:probe@127.0.0.1:27017/", help="URI of the database that stores probe data")
    parser.add_argument("--collection", type=str, default="test", help="Collection to use")

    args = parser.parse_args()

    db_client = pymongo.MongoClient(args.db)
    db = db_client["probe_data"]

    monitoring.start_collect(args.probe, db, args.collection)
