
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

interface IPair {
    pair_id: string;
    rx_sub_id?: string;
    tx_sub_id?: string;
    is_active: boolean;
}

interface IPairs {
    json_list: [];
}

export type { IMonitorData, IPair, IPairs};