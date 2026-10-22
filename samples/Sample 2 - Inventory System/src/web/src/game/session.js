// Presentation adapter. The backend owns identities, transactions and persistence.
const clone = (value) => {
  if (Array.isArray(value)) return value.map(clone);
  if (value && typeof value === "object")
    return Object.fromEntries(Object.entries(value).map(([key, child]) => [key, clone(child)]));
  return value;
};

export function createReliquary({ backend, toValue, subscribe }) {
  const definitions = toValue(backend.definitions);
  const definitionMap = new Map(definitions.map((d) => [d.id, d]));
  const listeners = new Map();
  let state;
  let disposed = false;
  const publish = (event, value) => {
    listeners.get(event)?.forEach((fn) => fn(clone(value)));
  };
  function refresh(event) {
    if (disposed) return;
    state = toValue(backend.read());
    if (event?.type === "stateChanged") publish("change", state.snapshot);
    publish("persistence", { dirty: state.dirty, error: state.persistenceError });
  }
  // Subscribe before the initial read; no state is inferred from command results.
  const unsubscribe = subscribe((event) => refresh(event));
  refresh();
  function invoke(command, preview, revision) {
    // View commands stop here. Only explicit, scalar host methods cross the bridge.
    if (Array.isArray(command))
      return { ok: false, code: "UNSUPPORTED", message: "Use a single UI operation" };
    const c = command;
    const suffix = preview ? [] : [revision];
    const to = c.to;
    let value;
    switch (c.type) {
      case "move": value = (preview ? backend.preview_move_item : backend.move_item)(c.id, to.container, to.x, to.y, to.rotation || 0, ...suffix); break;
      case "remove": value = (preview ? backend.preview_remove_item : backend.remove_item)(c.id, ...suffix); break;
      case "split": value = (preview ? backend.preview_split_stack : backend.split_stack)(c.id, c.quantity, to.container, to.x, to.y, to.rotation || 0, ...suffix); break;
      case "merge": value = (preview ? backend.preview_merge_stacks : backend.merge_stacks)(c.id, c.target, ...suffix); break;
      case "equip": value = (preview ? backend.preview_equip_item : backend.equip_item)(c.id, c.slot, ...suffix); break;
      case "socket": value = (preview ? backend.preview_socket_gem : backend.socket_gem)(c.id, c.socket, ...suffix); break;
      case "unsocket": value = (preview ? backend.preview_unsocket_gem : backend.unsocket_gem)(c.id, c.container, ...suffix); break;
      case "bind":
        if (!c.reference?.item) return {ok:false, code:"UNSUPPORTED", message:"An item reference is required"};
        value = (preview ? backend.preview_bind_item : backend.bind_item)(c.key, c.reference.item, ...suffix); break;
      case "activateItem":
        if (c.kind === "transfer") value = (preview ? backend.preview_transfer_item : backend.transfer_item)(c.id, ...suffix);
        else value = (preview ? backend.preview_equip_best_slot : backend.equip_best_slot)(c.id, ...suffix);
        break;
      default: return {ok:false, code:"UNSUPPORTED", message:"Unsupported UI operation"};
    }
    return toValue(value);
  }
  const execute = (command, revision = state.snapshot.revision) => {
    if (disposed) return { ok: false, code: "CLOSED", message: "View disposed" };
    return invoke(command, false, revision);
  };
  const engine = {
    definitions: definitionMap,
    snapshot: () => clone(state.snapshot),
    preview: (command) => invoke(command, true),
    execute,
    dimensions(item) {
      const d = definitionMap.get(item.definition);
      return (item.position.rotation || 0) % 2 ? [d.height, d.width] : [d.width, d.height];
    },
    occupant(container, x, y) {
      const s = state.snapshot;
      for (const item of s.items) {
        const c = s.containers.find((c) => c.id === item.position.container);
        const d = definitionMap.get(item.definition);
        if (x === undefined) {
          if (item.position.container === container) return item.id;
          const target = s.containers.find((c) => c.id === container);
          if (target?.slot === "offhand" && c?.slot === "mainhand" && d.twoHanded) return item.id;
        } else if (item.position.container === container) {
          const [w, h] = engine.dimensions(item), p = item.position;
          if (x >= p.x && x < p.x + w && y >= p.y && y < p.y + h) return item.id;
        }
      }
    },
    on(event, handler) {
      const set = listeners.get(event) || new Set();
      set.add(handler);
      listeners.set(event, set);
      return () => set.delete(handler);
    },
    emit(event, value) {
      if (event === "change" || event === "persistence") throw Error("Backend events are read-only");
      if (event === "activate") {
        const result = execute({ type: "activateItem", id: value.itemId, kind: value.kind });
        if (!result.ok) publish("reject", result);
      } else publish(event, value);
    },
  };
  return {
    engine, definitions,
    snapshot: engine.snapshot,
    addRelic: () => toValue(backend.create_relic()),
    save: () => backend.save(),
    activate: (key) => toValue(backend.activate_shortcut(key, state.snapshot.revision)),
    attributes: () => clone(state.attributes),
    comparison(id) {
      const value = toValue(backend.comparison(id));
      if (!value) return "";
      if (value.message) return value.message;
      const signed = (n) => (n >= 0 ? "+" : "") + n;
      return `Equip: Power ${signed(value.power)} · Ward ${signed(value.ward)}`;
    },
    dropCommand(item, to, container) {
      if (container.kind === "slot") return { type: "equip", id: item.id, slot: container.id };
      if (container.kind === "socket") return { type: "socket", id: item.id, socket: container.id };
      return { type: "move", id: item.id, to };
    },
    get persistenceError() { return state.persistenceError; },
    get dirty() { return state.dirty; },
    dispose() {
      if (disposed) return;
      disposed = true;
      unsubscribe();
      listeners.clear();
    },
  };
}
