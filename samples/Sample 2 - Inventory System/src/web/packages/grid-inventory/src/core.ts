export type Position = {
  container: string;
  x?: number;
  y?: number;
  rotation?: number;
};
export type Definition = {
  id: string;
  width: number;
  height: number;
  rotatable?: boolean;
  maxStack?: number;
  stackKey?: string;
  slots?: string[];
  twoHanded?: boolean;
};
export type Container = {
  id: string;
  kind: "grid" | "slot" | "socket";
  width?: number;
  height?: number;
  slot?: string;
  owner?: string;
  accepts?: string[];
};
export type Item = {
  id: string;
  definition: string;
  quantity: number;
  position: Position;
};
export type Snapshot = {
  schemaVersion: 1;
  revision: number;
  containers: Container[];
  items: Item[];
  shortcuts: Record<string, { item?: string; action?: string }>;
};
export type Command =
  | { type: "equip"; id: string; slot: string }
  | { type: "socket"; id: string; socket: string; newId?: string }
  | { type: "unsocket"; id: string; container: string }
  | { type: "move"; id: string; to: Position }
  | { type: "add"; item: Item }
  | { type: "remove"; id: string }
  | { type: "split"; id: string; quantity: number; newId: string; to: Position }
  | { type: "merge"; id: string; target: string }
  | { type: "bind"; key: string; reference: { item?: string; action?: string } }
  | { type: "consume"; id: string; quantity: number };
export type Result =
  | { ok: true; revision: number; snapshot: Snapshot; affected: string[] }
  | { ok: false; code: string; message: string };
const copy = <T>(value: T): T => JSON.parse(JSON.stringify(value));
export class Inventory {
  private state: Snapshot;
  private occupied = new Map<string, string>();
  private listeners = new Map<string, Set<(event: any) => void>>();
  readonly definitions: Map<string, Definition>;
  constructor(
    definitions: Definition[],
    containers: Container[],
    private rules: {
      stackKey?: (item: Item, state: Snapshot) => string;
      accept?: (item: Item, container: Container, state: Snapshot) => boolean;
      validate?: (commands: Command[], state: Snapshot) => string | undefined;
    } = {}
  ) {
    this.definitions = new Map(copy(definitions).map((d) => [d.id, d]));
    if (
      this.definitions.size !== definitions.length ||
      definitions.some(
        (d) =>
          !d.id ||
          !Number.isInteger(d.width) ||
          d.width < 1 ||
          !Number.isInteger(d.height) ||
          d.height < 1 ||
          (d.maxStack !== undefined &&
            (!Number.isInteger(d.maxStack) || d.maxStack < 1))
      )
    )
      throw Error("Invalid definitions");
    this.state = {
      schemaVersion: 1,
      revision: 0,
      containers: copy(containers),
      items: [],
      shortcuts: {},
    };
  }
  occupant(container: string, x?: number, y?: number) {
    return this.occupied.get(
      JSON.stringify(x === undefined ? [container] : [container, x, y])
    );
  }
  private stackKey(item: Item, state: Snapshot) {
    const d = this.definitions.get(item.definition)!;
    return this.rules.stackKey?.(item, state) || d.stackKey || d.id;
  }
  on(event: string, handler: (event: any) => void) {
    const set = this.listeners.get(event) || new Set();
    set.add(handler);
    this.listeners.set(event, set);
    return () => {
      set.delete(handler);
    };
  }
  emit(event: string, value: any) {
    this.listeners.get(event)?.forEach((fn) => fn(value));
  }
  snapshot() {
    return copy(this.state);
  }
  dimensions(item: Item) {
    const d = this.definitions.get(item.definition)!;
    return (item.position.rotation || 0) % 2
      ? [d.height, d.width]
      : [d.width, d.height];
  }
  private validate(s: Snapshot) {
    const fields = (record: object, allowed: string[]) => {
      if (
        !record ||
        typeof record !== "object" ||
        Array.isArray(record) ||
        Object.keys(record).some((key) => !allowed.includes(key))
      )
        throw Error("Unexpected snapshot fields");
    };
    fields(s, [
      "schemaVersion",
      "revision",
      "containers",
      "items",
      "shortcuts",
    ]);
    if (
      !Array.isArray(s.containers) ||
      !Array.isArray(s.items) ||
      !s.shortcuts ||
      typeof s.shortcuts !== "object" ||
      Array.isArray(s.shortcuts)
    )
      throw Error("Invalid snapshot records");
    for (const c of s.containers) {
      fields(c, ["id", "kind", "width", "height", "slot", "owner", "accepts"]);
      if (
        typeof c.id !== "string" ||
        !c.id ||
        (c.kind !== "socket" && c.owner !== undefined)
      )
        throw Error("Invalid container identity");
    }
    for (const i of s.items) {
      fields(i, ["id", "definition", "quantity", "position"]);
      fields(i.position, ["container", "x", "y", "rotation"]);
      if (
        typeof i.id !== "string" ||
        typeof i.definition !== "string" ||
        typeof i.position.container !== "string"
      )
        throw Error("Invalid item identity");
    }
    for (const ref of Object.values(s.shortcuts)) {
      fields(ref, ["item", "action"]);
      if (
        (ref.item !== undefined && typeof ref.item !== "string") ||
        (ref.action !== undefined && typeof ref.action !== "string")
      )
        throw Error("Invalid shortcut type");
    }

    if (
      s.schemaVersion !== 1 ||
      !Number.isInteger(s.revision) ||
      s.revision < 0
    )
      throw Error("Invalid schema or revision");
    const ids = new Set<string>(),
      cells = new Map<string, string>(),
      containers = new Map(s.containers.map((c) => [c.id, c]));
    if (containers.size !== s.containers.length)
      throw Error("Duplicate container");
    for (const c of s.containers) {
      if (!["grid", "slot", "socket"].includes(c.kind))
        throw Error("Invalid container");
      if (
        c.kind === "grid" &&
        (!Number.isInteger(c.width) ||
          !Number.isInteger(c.height) ||
          c.width! <= 0 ||
          c.height! <= 0)
      )
        throw Error("Invalid grid dimensions");
      if (c.kind === "socket" && !s.items.some((i) => i.id === c.owner))
        throw Error("Missing socket owner");
    }
    for (const i of s.items) {
      const d = this.definitions.get(i.definition),
        c = containers.get(i.position.container),
        r = i.position.rotation || 0;
      if (ids.has(i.id) || !i.id) throw Error("Duplicate item ID");
      ids.add(i.id);
      if (!d || !c) throw Error("Unknown definition or container: " + i.id);
      if (
        !Number.isInteger(i.quantity) ||
        i.quantity < 1 ||
        i.quantity > (d.maxStack || 1)
      )
        throw Error("Invalid quantity: " + i.id);
      if (!Number.isInteger(r) || r < 0 || r > 3 || (!d.rotatable && r !== 0))
        throw Error("Invalid rotation");
      if (
        (c.accepts && !c.accepts.includes(i.definition)) ||
        this.rules.accept?.(i, c, s) === false
      )
        throw Error("Container rejected item");
      let keys: string[] = [];
      if (c.kind === "grid") {
        const { x, y } = i.position,
          [w, h] = this.dimensions(i);
        if (
          !Number.isInteger(x) ||
          !Number.isInteger(y) ||
          x! < 0 ||
          y! < 0 ||
          x! + w > c.width! ||
          y! + h > c.height!
        )
          throw Error("Out of bounds");
        for (let a = x!; a < x! + w; a++)
          for (let b = y!; b < y! + h; b++)
            keys.push(JSON.stringify([c.id, a, b]));
      } else {
        if (c.kind === "slot" && !d.slots?.includes(c.slot!))
          throw Error("Invalid equipment slot");
        if (c.kind === "socket" && i.quantity !== 1)
          throw Error("Socket takes one gem");
        keys = [JSON.stringify([c.id])];
        if (d.twoHanded && c.slot === "mainhand") {
          const off = s.containers.find((v) => v.slot === "offhand");
          if (off) keys.push(JSON.stringify([off.id]));
        }
      }
      for (const key of keys) {
        if (cells.has(key)) throw Error("Occupied: " + key);
        cells.set(key, i.id);
      }
      let owner = c.owner;
      const seen = new Set([i.id]);
      while (owner) {
        if (seen.has(owner)) throw Error("Socket cycle");
        seen.add(owner);
        owner = containers.get(
          s.items.find((v) => v.id === owner)?.position.container || ""
        )?.owner;
      }
    }
    for (const ref of Object.values(s.shortcuts))
      if (
        !ref ||
        Boolean(ref.item) === Boolean(ref.action) ||
        (ref.item && !ids.has(ref.item))
      )
        throw Error("Invalid shortcut");
    return cells;
  }
  load(snapshot: Snapshot): Result {
    try {
      const occupied = this.validate(snapshot);
      this.state = copy(snapshot);
      this.occupied = occupied;
      this.emit("change", this.snapshot());
      return {
        ok: true,
        revision: this.state.revision,
        snapshot: this.snapshot(),
        affected: snapshot.items.map((i) => i.id),
      };
    } catch (e) {
      return { ok: false, code: "INVALID_SNAPSHOT", message: String(e) };
    }
  }
  private calculate(input: Command | Command[]): Result {
    const commands = Array.isArray(input) ? input : [input],
      s = this.snapshot(),
      affected = new Set<string>();
    try {
      for (const command of commands) {
        if (
          !command ||
          ![
            "equip",
            "socket",
            "unsocket",
            "add",
            "move",
            "remove",
            "split",
            "merge",
            "bind",
            "consume",
          ].includes(command.type)
        )
          throw Error("Unknown command");
        const c = command;
        const item = "id" in c ? s.items.find((i) => i.id === c.id) : undefined;
        if ("id" in c && !item) throw Error("Unknown item: " + c.id);
        if (
          c.type === "equip" ||
          c.type === "socket" ||
          c.type === "unsocket"
        ) {
          const trial = new Inventory(
            [...this.definitions.values()],
            s.containers,
            { ...this.rules, validate: undefined }
          );
          const loaded = trial.load(s);
          if (!loaded.ok) throw Error(loaded.message);
          const result =
            c.type === "equip"
              ? trial.planEquip(c.id, c.slot)
              : c.type === "socket"
              ? trial.placeGem(c.id, c.socket, c.newId)
              : trial.removeGem(c.id, c.container);
          if (!result.ok) throw Error(result.message);
          Object.assign(s, result.snapshot);
          result.affected.forEach((id) => affected.add(id));
        }
        if (c.type === "add") {
          s.items.push(copy(c.item));
          affected.add(c.item.id);
        }
        if (c.type === "move") {
          item!.position = copy(c.to);
          affected.add(c.id);
        }
        if (c.type === "bind") {
          if (["__proto__", "prototype", "constructor"].includes(c.key))
            throw Error("Invalid shortcut key");
          s.shortcuts[c.key] = copy(c.reference);
        }
        if (c.type === "split") {
          if (
            !Number.isInteger(c.quantity) ||
            c.quantity <= 0 ||
            c.quantity >= item!.quantity
          )
            throw Error("Invalid split");
          item!.quantity -= c.quantity;
          s.items.push({
            ...copy(item!),
            id: c.newId,
            quantity: c.quantity,
            position: copy(c.to),
          });
          affected.add(c.newId);
          affected.add(c.id);
        }
        if (c.type === "merge") {
          const target = s.items.find((i) => i.id === c.target);
          if (!target || target === item) throw Error("Invalid merge");
          const a = this.definitions.get(item!.definition)!,
            b = this.definitions.get(target.definition)!;
          if (this.stackKey(item!, s) !== this.stackKey(target, s))
            throw Error("Incompatible stack");
          const count = Math.min(
            item!.quantity,
            (b.maxStack || 1) - target.quantity
          );
          if (count <= 0) throw Error("Stack full");
          item!.quantity -= count;
          target.quantity += count;
          affected.add(c.id);
          affected.add(c.target);
          if (!item!.quantity) {
            s.items = s.items.filter((i) => i !== item);
            for (const ref of Object.values(s.shortcuts))
              if (ref.item === c.id) ref.item = c.target;
          }
        }
        if (c.type === "consume") {
          if (
            !Number.isInteger(c.quantity) ||
            c.quantity <= 0 ||
            c.quantity > item!.quantity
          )
            throw Error("Invalid consumption");
          item!.quantity -= c.quantity;
          affected.add(c.id);
        }
        if (c.type === "remove" || (c.type === "consume" && !item!.quantity)) {
          const remove = new Set([c.id]);
          let previous = 0;
          while (previous !== remove.size) {
            previous = remove.size;
            for (const i of s.items)
              if (
                s.containers.some(
                  (v) =>
                    v.id === i.position.container &&
                    v.owner &&
                    remove.has(v.owner)
                )
              )
                remove.add(i.id);
          }
          s.items = s.items.filter((i) => !remove.has(i.id));
          s.containers = s.containers.filter(
            (v) => !v.owner || !remove.has(v.owner)
          );
          for (const [key, ref] of Object.entries(s.shortcuts))
            if (ref.item && remove.has(ref.item)) delete s.shortcuts[key];
          remove.forEach((id) => affected.add(id));
        }
      }
      const rejection = this.rules.validate?.(commands, s);
      if (rejection) throw Error(rejection);
      this.validate(s);
      s.revision = this.state.revision + 1;
      return {
        ok: true,
        revision: this.state.revision,
        snapshot: s,
        affected: [...affected],
      };
    } catch (e) {
      return { ok: false, code: "REJECTED", message: String(e) };
    }
  }
  preview(commands: Command | Command[]) {
    return this.calculate(commands);
  }
  execute(commands: Command | Command[], expectedRevision?: number): Result {
    if (
      expectedRevision !== undefined &&
      expectedRevision !== this.state.revision
    )
      return { ok: false, code: "STALE", message: "State changed" };
    const result = this.calculate(commands);
    if (result.ok) {
      this.occupied = this.validate(result.snapshot);
      this.state = copy(result.snapshot);
      this.emit("change", this.snapshot());
    }
    return result;
  }
  /** Plan equipment returns against only the affected items, including vacated source cells. */
  equip(id: string, slot: string): Result {
    return this.execute({ type: "equip", id, slot });
  }
  socket(id: string, socket: string, newId?: string): Result {
    return this.execute({ type: "socket", id, socket, newId });
  }
  unsocket(id: string, container: string): Result {
    return this.execute({ type: "unsocket", id, container });
  }
  private planEquip(id: string, slot: string): Result {
    const s = this.snapshot(),
      item = s.items.find((i) => i.id === id),
      dest = s.containers.find((c) => c.id === slot);
    if (!item || !dest || dest.kind !== "slot")
      return {
        ok: false,
        code: "INVALID_SLOT",
        message: "Unknown item or slot",
      };
    const source = s.containers.find((c) => c.id === item.position.container);
    if (source?.kind !== "grid")
      return {
        ok: false,
        code: "INVALID_SOURCE",
        message: "Equip from a grid",
      };
    const definition = this.definitions.get(item.definition)!;
    if (!definition.slots?.includes(dest.slot!))
      return {
        ok: false,
        code: "INVALID_SLOT",
        message: "Definition cannot occupy this equipment slot",
      };
    const conflicts = s.items.filter((i) => {
      const c = s.containers.find((c) => c.id === i.position.container);
      return (
        c?.kind === "slot" &&
        (c.id === slot ||
          (definition.twoHanded && c.slot === "offhand") ||
          (dest.slot === "offhand" &&
            this.definitions.get(i.definition)?.twoHanded))
      );
    });
    conflicts.sort((a, b) => {
      const da = this.definitions.get(a.definition)!,
        db = this.definitions.get(b.definition)!;
      return (
        db.width * db.height - da.width * da.height ||
        (a.id < b.id ? -1 : a.id > b.id ? 1 : 0)
      );
    });
    const commands: Command[] = [{ type: "move", id, to: { container: slot } }];
    // Validate partial plans in an isolated engine, with unplaced conflicts temporarily removed.
    const plan = (index: number): Command[] | undefined => {
      if (index === conflicts.length)
        return this.preview(commands).ok ? copy(commands) : undefined;
      const old = conflicts[index];
      for (let y = 0; y < source.height!; y++)
        for (let x = 0; x < source.width!; x++)
          for (const rotation of this.definitions.get(old.definition)!.rotatable
            ? [0, 1]
            : [0]) {
            commands.push({
              type: "move",
              id: old.id,
              to: { container: source.id, x, y, rotation },
            });
            const trial = new Inventory(
              [...this.definitions.values()],
              s.containers,
              this.rules
            );
            trial.load({
              ...s,
              items: s.items.filter(
                (i) => !conflicts.slice(index + 1).some((c) => c.id === i.id)
              ),
              shortcuts: {},
            });
            if (trial.preview(commands).ok) {
              const result = plan(index + 1);
              if (result) return result;
            }
            commands.pop();
          }
      return undefined;
    };
    const planned = plan(0);
    return planned
      ? this.execute(planned, s.revision)
      : {
          ok: false,
          code: "NO_SPACE",
          message: "Cannot return all equipment to source grid",
        };
  }
  private placeGem(id: string, socket: string, newId?: string): Result {
    const item = this.state.items.find((i) => i.id === id),
      container = this.state.containers.find((c) => c.id === socket);
    if (!item || container?.kind !== "socket")
      return {
        ok: false,
        code: "INVALID_SOCKET",
        message: "Unknown gem or socket",
      };
    if (item.quantity > 1) {
      if (!newId)
        return {
          ok: false,
          code: "ID_REQUIRED",
          message: "A split gem requires a deterministic ID",
        };
      return this.execute({
        type: "split",
        id,
        quantity: 1,
        newId,
        to: { container: socket },
      });
    }
    return this.execute({ type: "move", id, to: { container: socket } });
  }
  private removeGem(id: string, container: string): Result {
    const item = this.state.items.find((i) => i.id === id),
      source = this.state.containers.find(
        (c) => c.id === item?.position.container
      );
    if (!item || source?.kind !== "socket")
      return {
        ok: false,
        code: "INVALID_SOCKET",
        message: "Item is not socketed",
      };
    const d = this.definitions.get(item.definition)!;
    const target = this.state.items.find(
      (i) =>
        i.id !== id &&
        i.position.container === container &&
        i.quantity < (this.definitions.get(i.definition)?.maxStack || 1) &&
        this.stackKey(i, this.state) === this.stackKey(item, this.state)
    );
    if (target) return this.execute({ type: "merge", id, target: target.id });
    const to = this.findSpace(id, container);
    return to
      ? this.execute({ type: "move", id, to })
      : { ok: false, code: "NO_SPACE", message: "No room for the gem" };
  }
  findSpace(id: string, container: string): Position | undefined {
    const c = this.state.containers.find((c) => c.id === container);
    if (!c || c.kind !== "grid") return;
    for (let y = 0; y < c.height!; y++)
      for (let x = 0; x < c.width!; x++)
        for (const rotation of this.definitions.get(
          this.state.items.find((i) => i.id === id)?.definition || ""
        )?.rotatable
          ? [0, 1]
          : [0]) {
          const to = { container, x, y, rotation };
          if (this.preview({ type: "move", id, to }).ok) return to;
        }
  }
}
