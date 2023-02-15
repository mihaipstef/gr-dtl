import { derived, writable } from "svelte/store";
import type { IMonitorData, IPairs } from "./types";

// Endpoints

const PAIR_API: string = "http://127.0.0.1:5000/pair";

//Monitor data store
let _monitor_data = writable({});

export const monitor_data = {
    data: _monitor_data,
    actions: {
        fetch: async (pair_id: string) => {
            let resp = await fetch(PAIR_API + "/" + pair_id + "/monitor_data");
            let data_json = await resp.json();
            let data : IMonitorData = data_json;
            _monitor_data.set(data);
        },
    },
}; 

// Subscribers (pairs) list
let _pairs = writable({});

export const pairs = {
    data: _pairs,
    actions: {
        fetch: async () => {
            let resp = await fetch(PAIR_API);
            let data_json = await resp.json();
            let data : IPairs = data_json;
            _pairs.set(data);
        },
    },
}; 
