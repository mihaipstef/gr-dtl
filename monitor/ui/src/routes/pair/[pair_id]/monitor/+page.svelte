<script lang="ts">
    import { page } from '$app/stores';
    import { onMount } from 'svelte';
    import { monitor_data } from "../../../../store"
    import type { IMonitorData } from "../../../../types"

    let pair_id = $page.params["pair_id"];

    var tx_frame_count: number | undefined;
    var rx_frame_count: number | undefined;
    var payload_crc_success: number | undefined;
    var payload_crc_failed: number | undefined;
    var header_crc_success: number | undefined;
    var header_crc_failed: number | undefined;
    var current_bps: number | undefined;
    var snr_est: string | undefined;
    var last_update: Date | undefined;
    var header_error_rate: number | undefined;
    var payload_error_rate: number | undefined;


    onMount(() => {
        const fetch_data = async () => {
            monitor_data.actions.fetch(pair_id);
        };
        const interval = setInterval(fetch_data, 1000);
        fetch_data();
        return () => clearInterval(interval);
    });

    $: monitor_data.data.subscribe((data: IMonitorData) => {
        tx_frame_count = data?.tx_frame_count;
        rx_frame_count = data?.rx_frame_count;
        payload_crc_success = data?.payload_crc_success;
        payload_crc_failed = data?.payload_crc_failed;
        header_crc_success = data?.header_crc_success;
        header_crc_failed = data?.header_crc_failed;
        snr_est = data?.snr_est?.toString();
        last_update = data?.last_update;
    });

    $: if ((payload_crc_success != undefined) && (payload_crc_failed != undefined)) {
        payload_error_rate = 100 * payload_crc_failed / (payload_crc_failed + payload_crc_success);
    }

    $: if ((header_crc_success != undefined) && (header_crc_failed != undefined)) {
        header_error_rate = 100 * header_crc_failed / (header_crc_failed + header_crc_success);
    }

</script>

<style>
</style>

<h1>Pair {pair_id}</h1>

<table id="monitor_data">
    <tr>
        <td>Current estimated SNR [dB]</td>
        <td>{snr_est}</td>
    </tr>
    <tr>
        <td>Current symbol size [bps]</td>
        <td>{current_bps}</td>
    </tr>
    <tr>
        <td>TX frame number  [frames]</td>
        <td>{tx_frame_count}</td>
    </tr>
    <tr>
        <td>RX frame number [frames]</td>
        <td>{rx_frame_count}</td>
    </tr>
    <tr>
        <td>Header error rate [%]</td>
        <td>{header_error_rate} (failed: {header_crc_failed}, success: {header_crc_success})</td>
    </tr>
    <tr>
        <td>Payload error rate [%]</td>
        <td>{payload_error_rate} (failed: {payload_crc_failed}, success: {payload_crc_success})</td>
    </tr>
    <tr>
        <td>Last update</td>
        <td>{last_update}</td>
    </tr>
</table>
