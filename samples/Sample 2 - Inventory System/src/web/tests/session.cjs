const { buildSync } = require("esbuild");
const assert = require("node:assert/strict");
const path = require("node:path");
const os = require("node:os");
const fs = require("node:fs");
const dir = fs.mkdtempSync(path.join(os.tmpdir(), "inventory-view-"));
try {
  const output = path.join(dir, "session.cjs");
  buildSync({ entryPoints: [path.resolve(__dirname, "../src/game/session.js")],
    outfile: output, bundle: true, platform: "node", format: "cjs" });
  const { createReliquary } = require(output);
  const detached = (value) => structuredClone(value);
  let state = { snapshot: { schemaVersion: 1, revision: 0,
    containers: [{id:"bag", kind:"grid", width:12, height:6}],
    items: [{id:"gem", definition:"gem", quantity:3, position:{container:"bag",x:0,y:0}}], shortcuts: {} },
    attributes: { power:10, ward:5 }, dirty:false, persistenceError:"" };
  let listener, dispatched, removed;
  const backend = {
    definitions: [{id:"gem",width:1,height:1}],
    read: () => state,
    subscribe(fn) { listener = fn; return 1; },
    unsubscribe(token) { removed = token; },
    remove_item(itemId, revision) { dispatched={itemId,revision}; return {ok:true}; },
    preview: () => ({ok:true, revision:state.snapshot.revision, snapshot:detached(state.snapshot)}),
    create_relic: () => ({ok:true}),
    comparison: () => null,
    save: () => true,
  };
  const session = createReliquary({backend, toValue:detached,
    subscribe(fn) {listener=fn; return () => {removed=1;};} });
  const snapshot = session.snapshot();
  snapshot.items[0].quantity = -2;
  assert.equal(session.snapshot().items[0].quantity, 3);
  assert.equal(state.snapshot.items[0].quantity, 3);
  assert.equal(session.engine.load, undefined);
  assert.throws(() => session.engine.emit("change", snapshot), /read-only/);
  let changes=0;
  session.engine.on("change", (snapshot) => {changes++; snapshot.items.length=0;});
  session.engine.execute({type:"remove", id:"gem"});
  assert.deepEqual(dispatched, {itemId:"gem",revision:0});
  assert.equal(changes, 0, "command result does not synthesize a committed state");
  assert.equal(session.snapshot().items.length, 1);
  state.snapshot.revision=1;
  state.snapshot.items.push({id:"server-new",definition:"gem",quantity:1,position:{container:"bag",x:2,y:0}});
  listener({type:"stateChanged",revision:1});
  assert.equal(changes, 1);
  assert.equal(session.snapshot().items.length, 2, "subscriber mutation cannot corrupt the cached projection");
  state.snapshot.items[0].quantity=5;
  assert.equal(session.snapshot().items[0].quantity, 3, "no live backend containers retained");
  listener({type:"stateChanged",revision:2});
  assert.equal(session.snapshot().items[0].quantity, 5);
  state.persistenceError="Disk full"; state.dirty=true;
  listener({type:"persistenceChanged",revision:2});
  assert.equal(session.persistenceError,"Disk full");
  assert.equal(changes,2,"save notifications do not fabricate inventory transactions");
  session.dispose();
  assert.equal(removed,1);
  assert.equal(session.engine.execute({type:"remove",id:"gem"}).code,"CLOSED");
  console.log("Detached rendering, backend events and command-only writes: PASS");
} finally { fs.rmSync(dir,{recursive:true}); }
