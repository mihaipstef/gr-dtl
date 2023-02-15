<script lang="ts">
  import { onMount } from "svelte";
  import { monitor_data, pairs } from "../store";
  import type { IPairs, IPair } from "../types";

  var tx_url: string = "tcp://ip:port";
  var rx_url: string = "tcp://ip:port";
  var pair_id: string = "name";
  let pair_list: IPair[];

  onMount(() => {
    pairs.actions.fetch();
  });

  $: pairs.data.subscribe((data: IPairs) => {
    pair_list = data.json_list;
    console.log(pair_list);
  });

  const handle_submit = (e) => {
    let formData = new FormData(e.target);
    let request_params = new URLSearchParams()
    for (let field of formData) {
        const [key, value] = field;
        request_params.append(key, value);
    }
    fetch("http://127.0.0.1:5000/pair/subscribe", { method: "POST", body: request_params }).then((resp) => {
        pairs.actions.fetch();
    });
  };
</script>

<h1>DTL monitoring</h1>

<h2>Subscribe to pair</h2>

<form id="subscribe_form" on:submit|preventDefault={handle_submit}>
  <label for="tx_url" class="field_label">TX</label>
  <input class="field_label" name="tx_url" placeholder="tcp://ip:port" value={tx_url} />
  <label for="rx_url" class="field_label">RX</label>
  <input class="field_label" name="rx_url" placeholder="tcp://ip:port" value={rx_url} />
  <label for="pair_id" class="field_label">Pair ID</label>
  <input class="field_label" name="pair_id" placeholder="pair" value={pair_id} />
  <button type="submit"> Subscribe to pair </button>
</form>

<h2>Pairs</h2>

<table>
  <tr>
    <th>Pair Id</th>
    <th>Active</th>
    <th>Tx subscriber Id</th>
    <th>Rx subscriber Id</th>
  </tr>
  {#if pair_list !== undefined}
    {#each pair_list as p}
      <tr>
        <td><a href={"pair/" + p.pair_id + "/monitor"}>{p.pair_id}</a></td>
        <td>{p.is_active}</td>
        <td>{p.tx_sub_id}</td>
        <td>{p.rx_sub_id}</td>
      </tr>
    {/each}
  {/if}
</table>
