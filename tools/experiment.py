#!/usr/bin/env python3

import json
import os
import shutil
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
        self.log = os.open(self.log_fname, os.O_WRONLY|os.O_TRUNC|os.O_CREAT)
    
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


logs_folder = sys.argv[1]
experiments_file = sys.argv[2]

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
            e["fec_codes"] = [(name, f"{experimets_path}/{fn}") for name, fn in e["fec_codes"]]

run_timestamp = int(time.time())
run_timestamp = 0

for i, e in enumerate(experiments):

    name = e.get("name", None)

    if "skip" in e and e["skip"]:
        print(f"Skip experiment {name}, number: {i}")
        continue

    print(f"Run experiment {name}, number: {i}")
    print(e)

    try:

        if name is None:
            name = uuid.uuid4()

        log_store_fname = f"{logs_store}/experiment_{run_timestamp}_{name}.log"
        result_fname = f"{logs_store}/experiment_{run_timestamp}_{name}.result"
        config_fname = f"{logs_store}/experiment_{run_timestamp}_{name}.run.json"
        experiment_fname = f"{logs_store}/experiment_{run_timestamp}_{name}.json"

        data_bytes = e.get("data_bytes", None)
        propagation_paths = e.get("propagation_paths", [(0,0,0,1)])
        use_sync_correct =  e.get("use_sync_correct", True)
        frame_length = e.get("frame_length", 20)

        with open(experiment_fname, "w") as f:
            f.write(json.dumps(e))

        if "config" in e:
            with open(config_fname, "w") as f:
                f.write(json.dumps(e["config"]))

        with capture_stdout(log_store_fname) as _:
            print(
                timeit.timeit(
                    lambda: sim.main(
                        config_dict=e,
                        run_config_file=config_fname,
                        data_bytes=data_bytes,
                        propagation_paths=propagation_paths,
                        use_sync_correct=use_sync_correct,
                        frame_length=frame_length),
            number=1))

        result = subprocess.check_output([f"{os.path.dirname(__file__)}/log.sh", log_store_fname, log_store_fname])

        with open(result_fname, "w") as f:
            f.write(result.decode("utf-8"))

    except Exception as ex:
        print("experiment failed")
        print(str(ex))
        print(traceback.format_exc())
