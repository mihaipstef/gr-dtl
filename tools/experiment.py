#!/usr/bin/env python3

import argparse
import json
import monitoring
import multiprocessing
import os
import pymongo
import sim
import subprocess
import sys
import time
import timeit
import traceback
import uuid


class capture_stdout():
    def __init__(self, log_fname):
        self.log_fname = log_fname
        sys.stdout.flush()
        self.log = os.open(self.log_fname, os.O_WRONLY |
                           os.O_TRUNC | os.O_CREAT)

    def __enter__(self):
        self.orig_stdout = os.dup(1)
        self.new_stdout = os.dup(1)
        os.dup2(self.log, 1)
        sys.stdout = os.fdopen(self.new_stdout, 'w')

    def __exit__(self, exc_type, exc_val, exc_tb):
        sys.stdout.flush()
        os.dup2(self.orig_stdout, 1)
        os.close(self.orig_stdout)
        os.close(self.log)


parser = argparse.ArgumentParser()
parser.add_argument("--logs", type=str, default=".",
                    help="Logs and other artifacts location")
parser.add_argument("--config", type=str, default="config.json",
                    help="Experiment configuration json file")
parser.add_argument("--sim_cls", type=str, default="ofdm_adaptive_loopback_src",
                    help="Simulator class used for the experiment")

args = parser.parse_args()

logs_folder = args.logs
experiments_file = args.config
sim_cls = getattr(sim, args.sim_cls, sim.ofdm_adaptive_sim_src)

logs_store = f"{logs_folder}"
current_log = f"{logs_folder}/sim.log"

# Load experiments
experiments = []
with open(experiments_file, "r") as f:
    experimets_path = os.path.dirname(experiments_file)
    content = f.read()
    experiments = json.loads(content)
    for e in experiments:
        if "fec_codes" in e and len(e["fec_codes"]):
            e["fec_codes"] = [(name, f"{experimets_path}/{fn}")
                              for name, fn in e["fec_codes"]]

run_timestamp = int(time.time())
run_timestamp = 0

for i, e in enumerate(experiments):

    name = e.get("name", uuid.uuid4())

    if "skip" in e and e["skip"]:
        print(f"Skip experiment {name}, number: {i}")
        continue

    db_url = e.get("monitor_db", None)
    probe_url = e.get("monitor_probe", None)

    monitor_process = None
    monitor_process_pid = None
    if probe_url and db_url:
        print(db_url)
        db_client = pymongo.MongoClient(db_url)
        db = db_client["probe_data"]
        monitor_process = multiprocessing.Process(
            target=monitoring.start_collect, args=(probe_url, db, f"{name}_{run_timestamp}",))
        monitor_process.start()
        monitor_process_pid = monitor_process.pid

    print(f"Run experiment {name}, number: {i}, monitoring PID: {monitor_process_pid}")
    print(e)

    try:

        log_store_fname = f"{logs_store}/experiment_{run_timestamp}_{name}.log"
        result_fname = f"{logs_store}/experiment_{run_timestamp}_{name}.result"
        config_fname = f"{logs_store}/experiment_{run_timestamp}_{name}.run.json"
        experiment_fname = f"{logs_store}/experiment_{run_timestamp}_{name}.json"

        with open(experiment_fname, "w") as f:
            f.write(json.dumps(e))

        if "config" in e:
            with open(config_fname, "w") as f:
                f.write(json.dumps(e["config"]))

        with capture_stdout(log_store_fname) as _:
            print(
                timeit.timeit(
                    lambda: sim.main(
                        top_block_cls=sim_cls,
                        config_dict=e,
                        run_config_file=config_fname,),
                    number=1))

        result = subprocess.check_output(
            [f"{os.path.dirname(__file__)}/log.sh", log_store_fname, log_store_fname])

        with open(result_fname, "w") as f:
            f.write(result.decode("utf-8"))

        if monitor_process and monitor_process.is_alive():
            monitor_process.terminate()
            time.sleep(1)

    except Exception as ex:
        print("experiment failed")
        print(str(ex))
        print(traceback.format_exc())
