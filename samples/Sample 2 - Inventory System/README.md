# Ashen Reliquary

![Sample 2 preview](../../docs/media/sample-2.webp)

A Godot .NET inventory sample rendered with Ultralight. Supports equipment slots,
stack splitting and merging, gem sockets, shortcuts and saved inventories.

## Run

Requirements: Godot **4.7.2 .NET**, .NET SDK **8 or newer**, Node.js **20 or newer**,
and the Ultralight View **v0.5.0** extension package.

1. Extract the extension package into `src/`. Its manifest must be at
   `src/addons/ultralight-view/ultralight-view.gdextension`.
2. Build the frontend:

   ```sh
   cd src/web
   npm ci
   npm run build
   ```

3. Open `src/project.godot` with Godot .NET, build the C# project, and run the scene.

Rebuild the frontend after editing `src/web/`. Vite writes runtime assets into
`src/ui/`.

## Controls

- Drag to move an item; press **R** while dragging to rotate it.
- Double click to equip an item or return it to the bag.
- **Shift click** to transfer between the bag and ground.
- Hold **Ctrl** while dragging to split a stack; use the wheel to adjust quantity.
- Drag a gem into a socket, or an item onto a shortcut slot.
- **New relic** generates an item. **Reload UI** reloads the interface and keeps
  the inventory. **Save / Retry** saves the current inventory.

Inventories are saved automatically to `user://inventory-v1.json` after changes.
If a save fails, the current inventory remains available and saving can be retried.

## Source

- `src/Backend/`: item catalog, inventory transactions, gameplay and persistence.
- `src/GameBridge.cs` and `src/WireCodec.cs`: JavaScript API and detached state copies.
- `src/Main.cs`: scene lifecycle and change notifications.
- `src/web/`: Vue interface and the local `grid-inventory` package.

C# owns the inventory state. The interface sends requests and refreshes its copy
when the backend emits a change event.

For exports, include `ui/**` and `Backend/catalog.json` as non-resource files in
the Godot export preset.

See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for source and license notices.
