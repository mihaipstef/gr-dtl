<script lang="ts">
  import { onMount } from "svelte";
  import { API_COLLECTOR_BASE, collections } from "../store";
  import type { ICollections, ICollector } from "../types";

  interface ICollectorRow extends ICollector{
    element: HTMLElement;
  };

  var probe_url: string = "tcp://ip:port";
  var collection_name: string = "name";
  let collectors: ICollectorRow[] = [];

  onMount(() => {
    collections.actions.fetch();
    setInterval(async () => {
        collections.actions.refresh_state();
      }, 5000);
    });

  const handle_submit = (e) => {
    let formData = new FormData(e.target);
    let request_params = new URLSearchParams()
    for (let field of formData) {
        const [key, value] = field;
        request_params.append(key, value);
    }
    // fetch("http://127.0.0.1:5000/pair/subscribe", { method: "POST", body: request_params }).then((resp) => {
    //     pairs.actions.fetch();
    // });
  };

  const collectorRowColor = (state: string) => {
    if (state == "STARTED") {
      return "green";
    } else if (state == "PENDING") {
      return "orange";
    }
    return "red";
  }

  $: collections.data.subscribe((data: ICollector[]) => {
    collectors = data as ICollectorRow[];
  });
</script>

<h1>DTL monitoring</h1>

<h2>Create new collection</h2>

<form id="subscribe_form" on:submit|preventDefault={handle_submit}>
  <label for="collection_name" class="field_label">Collection name</label>
  <input class="field_label" name="collection_name" placeholder="collection_name" value={collection_name} />
  <label for="probe_url" class="field_label">Probe</label>
  <input class="field_label" name="probe_url" placeholder="tcp://ip:port" value={probe_url} />
  <button type="submit"> Create </button>
</form>

<h2>Collectors</h2>

<table>
  <tr>
    <th>Collection</th>
    <th>Last Insert</th>
    <th>Collector Id</th>
    <th>Collector Status</th>
    <th>Probe url</th>
    <th>Actions</th>
  </tr>
  {#if collectors !== undefined}
    {#each collectors as c}
      <tr style="background-color:{collectorRowColor(c.last_state)}">
        <td>{c.collection_name}</td>
        <td>{c.last_insert}</td>
        <td>{c.collector_id}</td>
        <td>{c.last_state}</td>
        <td>{c.probe_url}</td>
        <td>
          <form action="{API_COLLECTOR_BASE}/{c.collection_name}/start" method="post">
            <button type="submit" name="restart_btn" class="btn-link">Restart</button>
          </form>
          <form action="{API_COLLECTOR_BASE}/{c.collection_name}/stop" method="post">
            <button type="submit" name="restart_btn" class="btn-link">Stop</button>
          </form>
        </td>
      </tr>
    {/each}
  {/if}
</table>


<style>
  table {
    width: 100%;
  }
  table th {
    background-color: plum;
  }
  .btn-link {
    border: none;
    outline: none;
    background: none;
    cursor: pointer;
    color: #0000EE;
    padding: 0;
    text-decoration: underline;
    font-family: inherit;
    font-size: inherit;
  }
</style>