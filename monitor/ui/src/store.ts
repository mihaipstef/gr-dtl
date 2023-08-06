import { get, writable } from "svelte/store";
import type { IMonitorData, ICollector } from "./types";

// Endpoints

export const API_COLLECTOR_BASE: string = "http://127.0.0.1:5000/collector/";
const API_COLLECTIONS: string = "http://127.0.0.1:5000/collections";

//Monitor data store
let _monitor_data = writable({});

export const monitor_data = {
    data: _monitor_data,
    actions: {
        fetch: async (collection_name: string) => {
            // let resp = await fetch(PAIR_API + "/" + pair_id + "/monitor_data");
            // let data_json = await resp.json();
            // _monitor_data.set(data_json);
        },
    },
}; 

let _collectors = writable([]);
export const collections = {
    data: _collectors,
    actions: {
        fetch: async () => {
            let resp = await fetch(API_COLLECTOR_BASE+"all");
            let data_json = await resp.json();
            let data: ICollector[] = data_json["json_list"];
            _collectors.set(data);
        },
        refresh_state: async () => {
            var data = get(_collectors);
            for (var collector of data) {
                let resp = await fetch(API_COLLECTOR_BASE + collector["collection_name"] + "/status");
                let data_json = await resp.json();
                collector.last_state = data_json["collector_state"];
            }
            _collectors.set(data);
        },
    },
}; 
