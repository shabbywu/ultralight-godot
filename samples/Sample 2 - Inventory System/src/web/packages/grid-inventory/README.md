# grid-inventory

TypeScript inventory engine and DOM view. The package exports TypeScript source
for use with a TypeScript-aware bundler.

```ts
import { Inventory } from 'grid-inventory/core';
import { InventoryView } from 'grid-inventory/dom';

const inventory = new Inventory(
  [{ id: 'sword', width: 1, height: 3, rotatable: true, slots: ['mainhand'] }],
  [{ id: 'bag', kind: 'grid', width: 12, height: 6 }],
);
const command = { type: 'add', item: {
  id: 'sword-1', definition: 'sword', quantity: 1,
  position: { container: 'bag', x: 0, y: 0, rotation: 0 },
}} as const;
const preview = inventory.preview(command);
if (preview.ok) inventory.execute(command, preview.revision);
```

`preview(command | commands[])` returns either `{ok:false, code, message}` or `{ok:true, revision, snapshot, affected}`. The revision is the state against which the operation was evaluated. Preview never commits or chooses new instance IDs. A successful preview describes the complete candidate state, including affected equipment and socket relationships. Supply its revision as the second argument of `execute` to reject an outdated operation. Execution always recalculates admission; a preview result itself is not an executable command.

`execute(command | commands[], expectedRevision?)` validates and commits the complete operation, emitting one `change` event. A failure leaves items, quantities, positions, bindings and revision unchanged. Returned snapshots are detached copies. `snapshot()` also returns a copy; `load(snapshot)` validates the entire versioned state before replacing it and notifying once.

Commands: `add`, `move`, `remove`, `split`, `merge`, `equip`, `socket`, `unsocket`, `bind`, and `consume`. The exported `Command` union documents their fields. Split requires a caller-supplied `newId`; socketing one gem from a stack likewise requires `newId`. Cancel the preview without executing to leave both quantity and identity unchanged.

Grid coordinates are integers. `rotation` is 0, 1, 2 or 3 (quarter turns). Definitions opt into rotation. `occupant(container, x, y)` reads the committed cell index; `occupant(slotId)` reads an equipment/socket occupant. A two-handed mainhand item indexes the offhand slot to the **same item ID**. It is never duplicated in `items`.

Equipment slots specify a `slot` category, matched against the definition's `slots`. The category `mainhand` and `offhand` implement two-hand reservation. `equip` returns all conflicting equipment to the incoming item's source grid, trying larger pieces first with stable ID ordering and backtracking only those pieces. It may reuse the incoming item's vacated cells. `move` to a grid always uses the exact coordinates. `findSpace` is an explicit helper for shortcut transfers/unloads.

Sockets are containers with `kind:'socket'`, an `owner` item ID and optional `accepts` definition IDs. Include them in a complete snapshot with their owners. Socketed gems retain independent item records; moving an owner does not move its gem's socket position. `unsocket` first merges into a compatible destination stack, then tries a vacant grid position. Neither failure nor preview removes a gem. Deleting an owner also deletes its socket containers and contents.

Definitions supply `maxStack` and optional `stackKey`. The constructor optionally accepts `stackKey(item,state)`, `accept(item,container,state)`, and `validate(commands,candidateState)` hooks. Hook functions are runtime configuration, never snapshot data. Merge leaves overflow at its original position and redirects bindings only when the source stack disappears. Consumption and deletion clear bindings to removed items.

`on(event, handler)` returns an unsubscribe function. The engine emits `change`; the DOM layer emits `hover`, `activate`, and `reject`. Callbacks should be synchronous and avoid mutating their inputs.

## DOM view

```ts
const view = new InventoryView(inventory, element, {
  cellSize: 40,
  gap: 2,
  render(item) { return document.createElement('div'); },
  command(item, position, container) {
    return {type:'move', id:item.id, to:position};
  },
}).mount();
// Later:
view.destroy();
```

Every container receives a `[data-container]` section, and every item a `[data-item]` element. Style these to arrange equipment, drawers and grids without changing inventory coordinates. Dimensions, cell gaps and CSS transforms affect display and hit tests. Layout does not repack on viewport changes.

The `command` hook selects business operations for drops, such as `equip` and `socket`. Compatible item drops merge stacks. `splitNextDrag(id,quantity,newId)` arms a quantity-selected split; it creates nothing until a valid drop. Pointer Events are used when available, with a mouse fallback. R rotates at the current anchor only when the candidate fits. Escape, pointer cancellation, loss of focus, state replacement and destruction cancel a drag.

`hover` has `enter`, `move`, and `leave` phases with `itemId`, `position`, `anchor` (DOMRect), `pointer` and `modifiers`. Leave retains the complete last context and is emitted when dragging begins, the item is removed, the pointer exits or the view is destroyed. `activate` conveys `itemId` and `kind:'equip'|'transfer'` for double click / Shift click. The view owns no attribute, comparison or consumption rules.

From this directory: `npm ci`, `npm run check`, `npm test`.

During dragging, R changes the held orientation whenever the definition allows rotation, including outside containers and over blocked cells. Drop preview validity is independent; release validates and commits the destination. Cancelling leaves the committed orientation unchanged.

Ctrl can be pressed or released anytime during dragging; the modifier on mouse/pointer release determines split versus whole-stack movement. Splitting defaults to half the stack, and toggling Ctrl preserves the adjusted split quantity. While holding the split, the wheel adjusts quantity within 1..quantity-1. The ghost displays the held quantity. Shift click remains quick transfer. No item instance or quantity change is committed until a valid drop; `splitNextDrag` remains available for custom controls.

`ViewOptions.dropTarget(item, hit)` can return `{element, command}` for targets outside the inventory root. The view previews that command, highlights the target bounds, and commits on release using the drag revision. Ashen uses this for quickbar bindings; binding references the original item even while Ctrl split mode is active. A completed drag suppresses the subsequent click to prevent accidental activation.

## Host-owned state

`InventoryView` accepts an `InventoryPort` containing `snapshot`, `preview`, `execute`,
`definitions`, `dimensions`, `occupant`, `on`, and `emit`. Ashen Reliquary implements
this port through C# backend methods and detached render snapshots. A host change
event refreshes the port and view. Command results do not replace the rendered state.
