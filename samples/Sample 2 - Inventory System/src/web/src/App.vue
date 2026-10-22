<script setup>
import { ref, computed, onMounted, onBeforeUnmount } from "vue";
import { InventoryView } from "grid-inventory/dom";
const props = defineProps({
  session: { type: Object, required: true },
  hostName: { type: String, required: true },
});
const { session } = props;
const { engine, definitions, addRelic, save, activate, attributes, comparison } = session;
const quickbar = ref(null),
  grids = ref(null),
  status = ref(
    session.persistenceError ||
      "Drag · R rotate · double click equip · Shift click transfer · Ctrl drag split · wheel quantity"
  ),
  hover = ref(null),
  selected = ref(null),
  snapshot = ref(engine.snapshot()),
  groundOpen = ref(false);
let view;
const unsub = [];
const tooltipStyle = computed(() =>
  hover.value
    ? {
        left:
          Math.max(
            8,
            Math.min(hover.value.anchor.right + 12, window.innerWidth - 245)
          ) + "px",
        top:
          Math.max(
            8,
            Math.min(hover.value.pointer.y, window.innerHeight - 140)
          ) + "px",
      }
    : {}
);
const stats = computed(() => attributes(snapshot.value));
const selectedItem = computed(() =>
  snapshot.value.items.find((i) => i.id === selected.value)
);
const selectedDefinition = computed(() =>
  definitions.find((d) => d.id === selectedItem.value?.definition)
);
function persist() {
  status.value = save() ? "Snapshot saved" : session.persistenceError;
}
function reload() {
  location.reload();
}
function report(result) {
  status.value = result.ok
    ? session.persistenceError || "Transaction committed"
    : result.message;
}
const shortcutSlots = computed(() => Array.from({ length: 6 }, (_, index) => {
  const key = String(index + 1);
  const reference = snapshot.value.shortcuts[key];
  const item = snapshot.value.items.find((i) => i.id === reference?.item);
  const definition = definitions.find((d) => d.id === item?.definition);
  return {
    key,
    name: reference?.action || definition?.name || (reference ? "Unavailable" : "Unbound"),
    icon: definition ? "./reliquary/items/" + definition.id + ".webp" : null,
    quantity: item?.quantity || 0,
    rarity: definition?.rarity,
    action: reference?.action,
  };
}));
function unsocket() {
  if (selected.value)
    report(
      engine.execute({ type: "unsocket", id: selected.value, container: "bag" })
    );
}
function discard() {
  if (selected.value)
    report(engine.execute({ type: "remove", id: selected.value }));
}
onMounted(() => {
  view = new InventoryView(engine, grids.value, {
    cellSize: 38,
    gap: 2,
    dropTarget(item, hit) {
      const slot = hit?.closest("[data-shortcut]");
      if (!slot || !quickbar.value?.contains(slot)) return;
      return {
        element: slot,
        command: { type: "bind", key: slot.dataset.shortcut, reference: { item: item.id } },
      };
    },
    command: session.dropCommand,
    render(item) {
      const d = definitions.find((d) => d.id === item.definition),
        el = document.createElement("div");
      el.className = "relic-card rarity-" + d.rarity;
      const img = document.createElement("img");
      img.src = "./reliquary/items/" + d.id + ".webp";
      img.alt = d.name;
      img.draggable = false;
      const rotation = item.position.rotation || 0;
      const container = engine
        .snapshot()
        .containers.find((c) => c.id === item.position.container);
      const [w, h] =
        container?.kind === "grid" ? engine.dimensions(item) : [2, 2];
      const ratio = (h * 40 - 2) / (w * 40 - 2);
      img.style.cssText = `position:absolute;left:50%;top:50%;width:${
        rotation % 2 ? ratio * 100 : 100
      }%;height:${
        rotation % 2 ? 100 / ratio : 100
      }%;transform:translate(-50%,-50%) rotate(${rotation * 90}deg)`;
      el.append(img);
      if (item.quantity > 1) {
        const count = document.createElement("b");
        count.textContent = item.quantity;
        el.append(count);
      }
      return el;
    },
  }).mount();
  window.Reliquary.view = view;
  const keyboard = (event) => {
    if (
      !["INPUT", "TEXTAREA", "SELECT"].includes(event.target.tagName) &&
      /^[1-6]$/.test(event.key)
    )
      activate(event.key);
  };
  document.addEventListener("keydown", keyboard);
  unsub.push(() => document.removeEventListener("keydown", keyboard));
  unsub.push(
    engine.on("hover", (event) => {
      hover.value = event.phase === "leave" ? null : event;
      if (event.phase !== "leave") selected.value = event.itemId;
    }),
    engine.on("change", (value) => {
      snapshot.value = value;
      if (session.persistenceError) status.value = session.persistenceError;
    }),
    engine.on("persistence", ({ error }) => {
      if (error) status.value = error;
    }),
    engine.on("reject", report)
  );
});
onBeforeUnmount(() => {
  view?.destroy();
  session.dispose();
  unsub.forEach((fn) => fn());
});
</script>
<template>
  <main class="reliquary-app">
    <header class="masthead">
      <span class="eyebrow">THE ASHEN VIGIL</span>
      <h1>Ashen Reliquary</h1>
      <p>Arms, relics &amp; the spoils of a forgotten age</p>
    </header>
    <nav class="demo-toolbar">
      <span>{{ hostName }}</span
      ><button
        @click="report(addRelic())"
      >
        New relic</button
      ><button @click="persist">Save / Retry</button
      ><button @click="reload">Reload</button
      ><button @click="groundOpen = !groundOpen">
        {{ groundOpen ? "Close" : "Open" }} Ground</button
      ><span role="status">{{ status }}</span>
    </nav>
    <div class="ashen-layout">
      <aside class="character-panel">
        <span class="eyebrow">PENITENT</span>
        <h2>Odran Vael</h2>
        <p>Level 24 · Ashen Vigil</p>
        <dl>
          <dt>Power</dt>
          <dd>{{ stats.power }}</dd>
          <dt>Ward</dt>
          <dd>{{ stats.ward }}</dd>
        </dl>
        <hr />
        <h3>{{ selectedDefinition?.name || "Inspect a relic" }}</h3>
        <p>{{ selectedDefinition?.rarity }}</p>
        <p v-if="selectedDefinition">
          Size {{ selectedDefinition.width }} ×
          {{ selectedDefinition.height }} · Quantity {{ selectedItem.quantity }}
        </p>
        <p v-if="hover">{{ comparison(hover.itemId) }}</p>
        <p v-if="selectedItem && selectedItem.quantity > 1" class="hint">
          Hold Ctrl at release to split half; toggle Ctrl anytime during dragging. Wheel adjusts split quantity.
          Escape cancels.
        </p>
        <button v-if="selectedItem" @click="unsocket">Remove gem to Bag</button
        ><button v-if="selectedItem" @click="discard">
          Offer to the ashes
        </button>
        <p class="hint">
          Select by hovering. Drag gems to the weapon socket. Double click
          equipment to equip or return to Bag.
        </p>
      </aside>
      <div
        ref="grids"
        :class="['framework-grids', { 'ground-open': groundOpen }]"
      ></div>
    </div>
    <nav ref="quickbar" class="quickbar" aria-label="Quickbar">
      <div v-for="slot in shortcutSlots" :key="slot.key" :data-shortcut="slot.key">
        <button
          class="quickbar-slot"
          :class="slot.rarity ? 'rarity-' + slot.rarity : ''"
          :title="slot.name + ' · Drag an item here to bind'"
          :aria-label="slot.key + ' · ' + slot.name"
          @click="activate(slot.key)"
        >
          <img v-if="slot.icon" :src="slot.icon" :alt="slot.name" draggable="false" />
          <span v-else-if="slot.action" class="quickbar-action">{{ slot.name }}</span>
          <span class="quickbar-key">{{ slot.key }}</span>
          <span v-if="slot.quantity > 1" class="quickbar-count">{{ slot.quantity }}</span>
        </button>
      </div>
    </nav>
    <div
      v-if="hover && selectedDefinition"
      role="tooltip"
      class="relic-tooltip"
      :style="tooltipStyle"
    >
      <strong>{{ selectedDefinition.name }}</strong>
      <p>{{ selectedDefinition.rarity }} · {{ selectedItem.quantity }}</p>
      <p>{{ comparison(hover.itemId) }}</p>
    </div>
    <footer>
      ODRAN VAEL ✦ PENITENT OF THE ASHEN VIGIL
      <p><a href="https://github.com/shabbywu/ultralight-godot/tree/main/samples/Sample%202%20-%20Inventory%20System">Source &amp; build instructions</a> · <a href="./reliquary/LICENSES.md">Artwork &amp; font notices</a></p>
    </footer>
  </main>
</template>
<style>
.ashen-layout {
  display: flex;
  gap: 26px;
  align-items: flex-start;
  justify-content: center;
  min-width: 1050px;
}
.character-panel {
  width: 215px;
  padding: 22px;
  border: 1px solid #786343;
  background: #17110de8;
  flex-shrink: 0;
}
.character-panel dl {
  display: grid;
  grid-template-columns: 1fr 1fr;
}
.character-panel input {
  width: 55px;
}
.character-panel button {
  margin: 5px 0;
}
.hint {
  font-size: 13px;
  color: #ac9979;
}
.framework-grids {
  display: grid;
  grid-template-columns: 78px 78px 78px 478px;
  grid-template-rows: 100px 100px 100px 100px 100px;
  gap: 26px 18px;
  padding-top: 24px;
  align-items: start;
}
.framework-grids section[data-container="head"] {
  grid-area: 1/2;
}
.framework-grids section[data-container="chest"] {
  grid-area: 2/2;
}
.framework-grids section[data-container="gloves"] {
  grid-area: 3/1;
}
.framework-grids section[data-container="neck"] {
  grid-area: 1/3;
}
.framework-grids section[data-container="ring1"] {
  grid-area: 3/2;
}
.framework-grids section[data-container="ring2"] {
  grid-area: 3/3;
}
.framework-grids section[data-container="mainhand"] {
  grid-area: 2/1;
}
.framework-grids section[data-container="offhand"] {
  grid-area: 2/3;
}
.framework-grids section[data-container="bag"] {
  grid-area: 1/4/4/5;
  margin-left: 15px;
}
.framework-grids section[data-kind="socket"] {
  grid-area: 4/2;
}
.framework-grids section[data-container="ground"] {
  grid-area: 6/1/7/5;
  display: none;
}
.framework-grids.ground-open section[data-container="ground"] {
  display: block;
}
.relic-card {
  border: 1px solid #8c7040;
  background: radial-gradient(#6b512c33, #17110f);
  display: flex;
  align-items: center;
  justify-content: center;
}
.relic-card img {
  width: 100%;
  height: 100%;
  object-fit: contain;
  pointer-events: none;
}
.relic-card b {
  position: absolute;
  bottom: 2px;
  right: 4px;
  color: #fff;
}
.rarity-legendary {
  border-color: #ce8b35;
}
.rarity-magic {
  border-color: #6586ab;
}
.rarity-relic {
  border-color: #a3739d;
}
.reliquary-app {
  max-width: 1400px;
  margin: auto;
}
.quickbar {
  display: flex;
  justify-content: center;
  gap: 12px;
  margin-top: 20px;
}
.quickbar > div {
  display: flex;
  flex-direction: column;
  max-width: 145px;
}
button {
  cursor: pointer;
  border: 1px solid #786343;
  background: #251b13;
  color: #e9d2a4;
  padding: 6px;
}
body {
  overflow: auto !important;
}
.demo-toolbar {
  flex-wrap: wrap;
}
.inventory-container label {
  color: #c8af7d;
  text-transform: capitalize;
}
</style>

<style>
.framework-grids {
  position: relative;
}
.framework-grids::before {
  content: "";
  position: absolute;
  pointer-events: none;
  left: 65px;
  top: 15px;
  width: 160px;
  height: 470px;
  background: url("/reliquary/items/effigy.webp") center/contain no-repeat;
  opacity: 0.65;
}
.framework-grids section[data-kind="slot"] {
  background-color: #16110aa6 !important;
}
.relic-tooltip {
  position: fixed;
  pointer-events: none;
  z-index: 5000;
  max-width: 235px;
  padding: 14px;
  background: #1c140cf5;
  border: 1px solid #b29051;
  box-shadow: 0 6px 25px #000;
}
.relic-tooltip p {
  margin: 4px 0;
  font-size: 14px;
}
.relic-card {
  overflow: hidden;
}
</style>

<style>
.inventory-container[data-reserved-by]::after {
  content: "Reserved by two-handed weapon";
  position: absolute;
  inset: 8px;
  display: flex;
  align-items: center;
  text-align: center;
  font-size: 12px;
  color: #a99162;
  pointer-events: none;
}
</style>

<style>
.quickbar {
  position: fixed;
  bottom: 12px;
  left: 50%;
  transform: translateX(-50%);
  z-index: 30;
  padding: 8px 12px;
  background: #120c08ed;
  border: 1px solid #6c5438;
  margin: 0;
  max-width: calc(100vw - 24px);
}
.reliquary-app {
  padding-bottom: 110px;
}
</style>

<style>
.quickbar-slot {
  position: relative;
  width: 64px;
  height: 64px;
  padding: 5px;
  background: #19130e;
}
.quickbar-slot img {
  width: 100%;
  height: 100%;
  object-fit: contain;
  pointer-events: none;
}
.quickbar-key, .quickbar-count {
  position: absolute;
  z-index: 1;
  padding: 0 3px;
  background: #120c08d9;
  text-shadow: 0 1px 2px #000;
  pointer-events: none;
}
.quickbar-key { top: 2px; left: 3px; font-size: 12px; }
.quickbar-count { bottom: 2px; right: 3px; font-size: 14px; }
.quickbar-action { font-size: 11px; overflow-wrap: anywhere; }
</style>
