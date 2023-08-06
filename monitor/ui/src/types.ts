
interface IMonitorData {
    tx_frame_count?: number;
    rx_frame_count?: number;
    payload_crc_success?: number;
    payload_crc_failed?: number;
    header_crc_success?: number;
    header_crc_failed?: number;
    snr_est?: number;
    current_bps?: number
    last_update?: Date;
}

interface ICollector {
    collection_name: string;
    collector_id: string;
    probe_url: string;
    last_state: string;
    last_insert: string;
}

interface ICollections {
    json_list: [];
}

export type { IMonitorData, ICollector, ICollections};