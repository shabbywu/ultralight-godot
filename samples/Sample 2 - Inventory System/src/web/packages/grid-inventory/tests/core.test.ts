import { test } from "node:test";
import assert from "node:assert/strict";
import { Inventory } from "../src/core";
const make = () =>
  new Inventory(
    [
      { id: "a", width: 2, height: 1, rotatable: true, maxStack: 10 },
      {
        id: "sword",
        width: 1,
        height: 2,
        slots: ["mainhand"],
        twoHanded: true,
      },
      { id: "shield", width: 1, height: 1, slots: ["offhand"] },
    ],
    [
      { id: "bag", kind: "grid", width: 4, height: 4 },
      { id: "main", kind: "slot", slot: "mainhand" },
      { id: "off", kind: "slot", slot: "offhand" },
    ]
  );
test("preview, atomic failure, stale revision, single event", () => {
  const e = make();
  let events = 0;
  e.on("change", () => events++);
  const add = {
    type: "add",
    item: {
      id: "1",
      definition: "a",
      quantity: 5,
      position: { container: "bag", x: 0, y: 0 },
    },
  } as const;
  assert(e.preview(add).ok);
  assert.equal(e.snapshot().items.length, 0);
  assert(e.execute(add).ok);
  const before = e.snapshot();
  assert(
    !e.execute([
      { type: "move", id: "1", to: { container: "bag", x: 3, y: 0 } },
      { type: "remove", id: "missing" },
    ]).ok
  );
  assert.deepEqual(e.snapshot(), before);
  assert(!e.execute({ type: "remove", id: "1" }, 0).ok);
  assert.equal(events, 1);
});
test("split, partial merge, references and quantity conservation", () => {
  const e = make();
  e.execute({
    type: "add",
    item: {
      id: "1",
      definition: "a",
      quantity: 10,
      position: { container: "bag", x: 0, y: 0 },
    },
  });
  e.execute({
    type: "split",
    id: "1",
    newId: "2",
    quantity: 4,
    to: { container: "bag", x: 0, y: 1 },
  });
  e.execute({ type: "bind", key: "1", reference: { item: "2" } });
  assert(e.execute({ type: "merge", id: "2", target: "1" }).ok);
  assert.equal(e.snapshot().items[0].quantity, 10);
  assert.equal(e.snapshot().shortcuts["1"].item, "1");
});
test("two handed equip returns shield and uses vacated source", () => {
  const e = make();
  assert(
    e.execute([
      {
        type: "add",
        item: {
          id: "s",
          definition: "sword",
          quantity: 1,
          position: { container: "bag", x: 0, y: 0 },
        },
      },
      {
        type: "add",
        item: {
          id: "h",
          definition: "shield",
          quantity: 1,
          position: { container: "off" },
        },
      },
    ]).ok
  );
  assert(e.equip("s", "main").ok);
  assert.equal(
    e.snapshot().items.find((i) => i.id === "h")?.position.container,
    "bag"
  );
});
test("random invalid moves never mutate unrelated objects", () => {
  const e = make();
  e.execute({
    type: "add",
    item: {
      id: "1",
      definition: "a",
      quantity: 1,
      position: { container: "bag", x: 0, y: 0 },
    },
  });
  for (let n = 0; n < 500; n++) {
    const before = e.snapshot();
    const result = e.execute({
      type: "move",
      id: "1",
      to: { container: "bag", x: (n % 9) - 3, y: (n % 7) - 2, rotation: n % 4 },
    });
    if (!result.ok) assert.deepEqual(e.snapshot(), before);
    const saved = e.snapshot();
    assert(e.load(saved).ok);
    assert.deepEqual(e.snapshot(), saved);
  }
});
test("failed multi equipment return preserves every item and revision", () => {
  const e = new Inventory(
    [
      { id: "big", width: 1, height: 1, slots: ["mainhand"], twoHanded: true },
      { id: "old", width: 1, height: 1, slots: ["mainhand", "offhand"] },
    ],
    [
      { id: "bag", kind: "grid", width: 1, height: 1 },
      { id: "main", kind: "slot", slot: "mainhand" },
      { id: "off", kind: "slot", slot: "offhand" },
    ]
  );
  assert(
    e.execute([
      {
        type: "add",
        item: {
          id: "new",
          definition: "big",
          quantity: 1,
          position: { container: "bag", x: 0, y: 0 },
        },
      },
      {
        type: "add",
        item: {
          id: "old1",
          definition: "old",
          quantity: 1,
          position: { container: "main" },
        },
      },
      {
        type: "add",
        item: {
          id: "old2",
          definition: "old",
          quantity: 1,
          position: { container: "off" },
        },
      },
    ]).ok
  );
  const before = e.snapshot();
  assert(!e.preview({ type: "equip", id: "new", slot: "main" }).ok);
  assert(!e.execute({ type: "equip", id: "new", slot: "main" }).ok);
  assert.deepEqual(e.snapshot(), before);
});
test("equipment preview and two returns in one notification", () => {
  const e = new Inventory(
    [
      { id: "big", width: 1, height: 2, slots: ["mainhand"], twoHanded: true },
      { id: "old", width: 1, height: 1, slots: ["mainhand", "offhand"] },
    ],
    [
      { id: "bag", kind: "grid", width: 1, height: 2 },
      { id: "main", kind: "slot", slot: "mainhand" },
      { id: "off", kind: "slot", slot: "offhand" },
    ]
  );
  e.execute([
    {
      type: "add",
      item: {
        id: "new",
        definition: "big",
        quantity: 1,
        position: { container: "bag", x: 0, y: 0 },
      },
    },
    {
      type: "add",
      item: {
        id: "old1",
        definition: "old",
        quantity: 1,
        position: { container: "main" },
      },
    },
    {
      type: "add",
      item: {
        id: "old2",
        definition: "old",
        quantity: 1,
        position: { container: "off" },
      },
    },
  ]);
  const before = e.snapshot();
  assert(e.preview({ type: "equip", id: "new", slot: "main" }).ok);
  assert.deepEqual(e.snapshot(), before);
  let count = 0;
  e.on("change", () => count++);
  assert(e.execute({ type: "equip", id: "new", slot: "main" }).ok);
  assert.equal(count, 1);
  assert.equal(
    e.snapshot().items.filter((i) => i.position.container === "bag").length,
    2
  );
});
test("partial merge preserves remainder and references; split cancellation is pure", () => {
  const e = make();
  e.execute([
    {
      type: "add",
      item: {
        id: "a",
        definition: "a",
        quantity: 8,
        position: { container: "bag", x: 0, y: 0 },
      },
    },
    {
      type: "add",
      item: {
        id: "b",
        definition: "a",
        quantity: 7,
        position: { container: "bag", x: 0, y: 1 },
      },
    },
  ]);
  const before = e.snapshot();
  for (let n = 0; n < 50; n++)
    assert(
      e.preview({
        type: "split",
        id: "a",
        quantity: 2,
        newId: "preview",
        to: { container: "bag", x: 0, y: 2 },
      }).ok
    );
  assert.deepEqual(e.snapshot(), before);
  e.execute({ type: "merge", id: "a", target: "b" });
  assert.equal(e.snapshot().items.find((i) => i.id === "a")?.quantity, 5);
  assert.equal(e.snapshot().items.find((i) => i.id === "b")?.quantity, 10);
});
test("socket identity, carrying, removal merge and full destination failure", () => {
  const defs = [
    { id: "gear", width: 1, height: 1 },
    { id: "gem", width: 1, height: 1, maxStack: 10 },
  ];
  const e = new Inventory(defs, [
    { id: "bag", kind: "grid", width: 2, height: 1 },
    { id: "other", kind: "grid", width: 1, height: 1 },
  ]);
  e.execute([
    {
      type: "add",
      item: {
        id: "gear",
        definition: "gear",
        quantity: 1,
        position: { container: "bag", x: 0, y: 0 },
      },
    },
    {
      type: "add",
      item: {
        id: "gems",
        definition: "gem",
        quantity: 5,
        position: { container: "bag", x: 1, y: 0 },
      },
    },
  ]);
  const s = e.snapshot();
  s.containers.push({
    id: "socket",
    kind: "socket",
    owner: "gear",
    accepts: ["gem"],
  });
  assert(e.load(s).ok);
  const before = e.snapshot();
  assert(
    e.preview({ type: "socket", id: "gems", socket: "socket", newId: "one" }).ok
  );
  assert.deepEqual(e.snapshot(), before);
  assert(
    e.execute({ type: "socket", id: "gems", socket: "socket", newId: "one" }).ok
  );
  assert.equal(e.snapshot().items.find((i) => i.id === "gems")?.quantity, 4);
  assert(
    e.execute({
      type: "move",
      id: "gear",
      to: { container: "other", x: 0, y: 0 },
    }).ok
  );
  assert.equal(
    e.snapshot().items.find((i) => i.id === "one")?.position.container,
    "socket"
  );
  const full = e.snapshot();
  assert(!e.execute({ type: "unsocket", id: "one", container: "other" }).ok);
  assert.deepEqual(e.snapshot(), full);
  assert(e.execute({ type: "unsocket", id: "one", container: "bag" }).ok);
  assert.equal(e.snapshot().items.find((i) => i.id === "gems")?.quantity, 5);
  assert(!e.snapshot().items.some((i) => i.id === "one"));
});
test("business rejection and unsubscribe; returned data cannot mutate engine", () => {
  const e = new Inventory(
    [{ id: "a", width: 1, height: 1 }],
    [{ id: "b", kind: "grid", width: 1, height: 1 }],
    {
      validate: (commands) =>
        commands.some((c) => c.type === "remove")
          ? "Quest item locked"
          : undefined,
    }
  );
  let events = 0;
  const off = e.on("change", () => events++);
  const result = e.execute({
    type: "add",
    item: {
      id: "a",
      definition: "a",
      quantity: 1,
      position: { container: "b", x: 0, y: 0 },
    },
  });
  assert(result.ok);
  if (result.ok) result.snapshot.items[0].quantity = 999;
  assert.equal(e.snapshot().items[0].quantity, 1);
  off();
  assert(!e.execute({ type: "remove", id: "a" }).ok);
  assert.equal(events, 1);
});
test("load rejects corrupted state atomically and deletion cleans socket children and bindings", () => {
  const e = make();
  e.execute({
    type: "add",
    item: {
      id: "a",
      definition: "a",
      quantity: 1,
      position: { container: "bag", x: 0, y: 0 },
    },
  });
  e.execute({ type: "bind", key: "1", reference: { item: "a" } });
  const before = e.snapshot();
  for (const corrupt of [
    { ...before, schemaVersion: 99 },
    { ...before, items: [...before.items, ...before.items] },
    { ...before, shortcuts: { x: { item: "missing" } } },
  ]) {
    assert(!e.load(corrupt as any).ok);
    assert.deepEqual(e.snapshot(), before);
  }
  assert(e.execute({ type: "remove", id: "a" }).ok);
  assert.deepEqual(e.snapshot().shortcuts, {});
});

test("equipment placement backtracks only returning pieces in fragmented space", () => {
  const e = new Inventory(
    [
      { id: "new", width: 1, height: 1, slots: ["mainhand"], twoHanded: true },
      { id: "horizontal", width: 2, height: 1, slots: ["mainhand"] },
      { id: "vertical", width: 1, height: 2, slots: ["offhand"] },
      { id: "block", width: 1, height: 1 },
    ],
    [
      { id: "bag", kind: "grid", width: 3, height: 2 },
      { id: "main", kind: "slot", slot: "mainhand" },
      { id: "off", kind: "slot", slot: "offhand" },
    ]
  );
  assert(
    e.execute([
      {
        type: "add",
        item: {
          id: "new",
          definition: "new",
          quantity: 1,
          position: { container: "bag", x: 0, y: 1 },
        },
      },
      {
        type: "add",
        item: {
          id: "a",
          definition: "horizontal",
          quantity: 1,
          position: { container: "main" },
        },
      },
      {
        type: "add",
        item: {
          id: "b",
          definition: "vertical",
          quantity: 1,
          position: { container: "off" },
        },
      },
      ...([1, 2].map((x) => ({
        type: "add",
        item: {
          id: "block" + x,
          definition: "block",
          quantity: 1,
          position: { container: "bag", x, y: 1 },
        },
      })) as any),
    ]).ok
  );
  const before = e.snapshot();
  assert(e.execute({ type: "equip", id: "new", slot: "main" }).ok);
  assert.deepEqual(e.snapshot().items.find((i) => i.id === "a")?.position, {
    container: "bag",
    x: 1,
    y: 0,
    rotation: 0,
  });
  assert.deepEqual(e.snapshot().items.find((i) => i.id === "b")?.position, {
    container: "bag",
    x: 0,
    y: 0,
    rotation: 0,
  });
  for (const old of before.items.filter((i) => i.id.startsWith("block")))
    assert.deepEqual(
      e.snapshot().items.find((i) => i.id === old.id),
      old
    );
  assert.equal(e.occupant("off"), "new");
});

test("seeded random moves, splits, merges and cancellations preserve ownership and quantity", () => {
  let seed = 43521;
  const random = () => {
    seed = (Math.imul(seed, 1664525) + 1013904223) >>> 0;
    return seed / 4294967296;
  };
  const e = new Inventory(
    [{ id: "stack", width: 1, height: 2, rotatable: true, maxStack: 10 }],
    [
      { id: "a", kind: "grid", width: 8, height: 8 },
      { id: "b", kind: "grid", width: 8, height: 8 },
    ]
  );
  for (let n = 0; n < 8; n++)
    assert(
      e.execute({
        type: "add",
        item: {
          id: String(n),
          definition: "stack",
          quantity: 5,
          position: { container: "a", x: n, y: 0 },
        },
      }).ok
    );
  for (let n = 0; n < 1000; n++) {
    const before = e.snapshot(),
      item = before.items[Math.floor(random() * before.items.length)],
      target = before.items[Math.floor(random() * before.items.length)];
    const to = {
      container: random() < 0.5 ? "a" : "b",
      x: Math.floor(random() * 11) - 1,
      y: Math.floor(random() * 11) - 1,
      rotation: Math.floor(random() * 4),
    };
    const kind = Math.floor(random() * 3),
      command: any =
        kind === 0
          ? { type: "move", id: item.id, to }
          : kind === 1
          ? {
              type: "split",
              id: item.id,
              newId: "random-" + n,
              quantity: 1,
              to,
            }
          : { type: "merge", id: item.id, target: target.id };
    const preview = e.preview(command);
    assert.deepEqual(e.snapshot(), before);
    if (random() < 0.25) continue;
    const result = e.execute(
        command,
        preview.ok ? preview.revision : undefined
      ),
      after = e.snapshot();
    if (!result.ok) assert.deepEqual(after, before);
    else
      for (const old of before.items.filter(
        (i) => !result.affected.includes(i.id)
      ))
        assert.deepEqual(
          after.items.find((i) => i.id === old.id),
          old
        );
    assert.equal(
      after.items.reduce((sum, i) => sum + i.quantity, 0),
      40
    );
    assert.equal(
      new Set(after.items.map((i) => i.id)).size,
      after.items.length
    );
    const restored = new Inventory(
      [...e.definitions.values()],
      after.containers
    );
    assert(restored.load(after).ok);
  }
});

test("business operation validation sees complete public command exactly once", () => {
  let seen: string[] = [];
  const e = new Inventory(
    [{ id: "weapon", width: 1, height: 1, slots: ["mainhand"] }],
    [
      { id: "bag", kind: "grid", width: 2, height: 1 },
      { id: "main", kind: "slot", slot: "mainhand" },
    ],
    {
      validate: (commands, state) => {
        seen.push(commands[0].type);
        if (
          commands.some((c) => c.type === "move" && c.to.container === "main")
        )
          return "Only equip commands may equip";
        if (commands[0].type === "equip" && state.items.length !== 1)
          return "Missing item";
      },
    }
  );
  assert(
    e.execute({
      type: "add",
      item: {
        id: "w",
        definition: "weapon",
        quantity: 1,
        position: { container: "bag", x: 0, y: 0 },
      },
    }).ok
  );
  seen = [];
  assert(e.preview({ type: "equip", id: "w", slot: "main" }).ok);
  assert.deepEqual(seen, ["equip"]);
  seen = [];
  assert(e.equip("w", "main").ok);
  assert.deepEqual(seen, ["equip"]);
});
