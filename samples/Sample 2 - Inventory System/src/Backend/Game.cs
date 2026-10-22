using System.Collections.Immutable;

namespace AshenReliquary.Backend;

/// <summary>Authoritative gameplay composed from typed inventory primitives.</summary>
public sealed class Game
{
    private readonly Inventory _inventory;
    private readonly ItemDB _items;
    private readonly ISnapshotStore _store;
    private readonly Func<ImmutableArray<Definition>, Definition> _choose;
    private readonly Func<string> _nextId;
    private bool _publishing;
    private bool _dirty;
    private string _persistenceError = "";
    public event Action<StateChanged>? Changed;
    public event Action<Exception>? ObserverFailed;
    public ImmutableArray<Definition> Definitions => _items.Definitions;

    public Game(ItemDB items, ISnapshotStore store, Func<ImmutableArray<Definition>, Definition>? choose = null, Func<string>? nextId = null)
    {
        _items = items; _store = store;
        _choose = choose ?? (definitions => definitions[Random.Shared.Next(definitions.Length)]);
        _nextId = nextId ?? (() => Guid.NewGuid().ToString("N"));
        var saved = store.Load();
        _inventory = new Inventory(items.Definitions, saved ?? InitialSnapshot(items.Catalog));
        _dirty = saved is null;
    }
    public static Snapshot InitialSnapshot(Catalog catalog)
    {
        var items = catalog.Definitions.Take(18).Select((d, n) => new Item($"seed-{n}", d.Id, d.MaxStack > 1 ? 8 : 1,
            new Position(n < 10 ? "ground" : "bag", (n % 4) * 3, ((n < 10 ? n : n - 10) / 4) * 4))).ToList();
        items.Add(new Item("chest", "gothic-breastplate", 1, new Position("bag", 0, 1)));
        items.Add(new Item("gem", "double-cross", 6, new Position("bag", 11, 0)));
        return new Snapshot(0, [.. catalog.Containers, new Container("saint-socket", ContainerKind.Socket, Owner: "seed-0", Accepts: ["double-cross"])], items, new(StringComparer.Ordinal));
    }
    public Snapshot Snapshot() => _inventory.Snapshot();
    public Attributes Attributes(Snapshot state)
    {
        var equipped = state.Items.Where(i => Inventory.Container(state, i.Position.Container).Kind == ContainerKind.Slot).ToArray();
        var owners = equipped.Select(i => i.Id).ToHashSet(StringComparer.Ordinal);
        var gems = state.Items.Count(i => Inventory.Container(state, i.Position.Container).Owner is { } owner && owners.Contains(owner));
        return new Attributes(10 + equipped.Sum(i => _inventory.Definition(i.Definition).Power) + gems * 5, 5 + equipped.Length * 3);
    }
    public ReadModel Read() { var snapshot = Snapshot(); return new(snapshot, Attributes(snapshot), _dirty, _persistenceError); }
    private void Notify(ChangeKind kind)
    {
        _publishing = true;
        try
        {
            var value = new StateChanged(kind, Snapshot().Revision);
            foreach (var callback in Changed?.GetInvocationList() ?? [])
                try { ((Action<StateChanged>)callback)(value); } catch (Exception error) { ObserverFailed?.Invoke(error); }
        }
        finally { _publishing = false; }
    }
    private bool Persist()
    {
        try { _store.Save(Snapshot()); _dirty = false; _persistenceError = ""; return true; }
        catch (Exception error) { _dirty = true; _persistenceError = error.Message; return false; }
    }
    public bool Save()
    {
        if (_publishing) return false;
        var result = Persist(); Notify(ChangeKind.PersistenceChanged); return result;
    }
    private Result Preview(Action<InventoryTransaction> action)
    {
        var sequence = 0;
        var result = _inventory.Preview(action, () => $"preview-{Snapshot().Revision}-{sequence++}");
        return result is Success success ? success with { Attributes = Attributes(success.Snapshot) } : result;
    }
    private Result Commit(Result result)
    {
        if (result is not Success success) return result;
        _inventory.Commit(success.Snapshot); _dirty = true; Persist(); Notify(ChangeKind.StateChanged);
        return success with { Revision = Snapshot().Revision };
    }
    private Result Execute(Action<InventoryTransaction> action, int? expectedRevision)
    {
        if (_publishing) return new Failure(ErrorCode.Busy, "Cannot mutate during notification");
        if (expectedRevision is not null && expectedRevision != Snapshot().Revision) return new Failure(ErrorCode.Stale, "State changed");
        var admitted = Preview(action);
        return admitted is Failure ? admitted : Commit(_inventory.Preview(action, _nextId));
    }
    private void Equip(InventoryTransaction tx, string id, string slotId)
    {
        var item = tx.Item(id); var dest = tx.Container(slotId); var source = tx.Container(item.Position.Container);
        var d = _inventory.Definition(item.Definition);
        Inventory.Require(source.Kind == ContainerKind.Grid, "Equip from a grid");
        Inventory.Require(dest.Kind == ContainerKind.Slot && !d.Slots.IsDefault && d.Slots.Contains(dest.Slot!), "Invalid equipment slot");
        var conflicts = tx.State.Items.Where(i => tx.Container(i.Position.Container).Kind == ContainerKind.Slot &&
            (i.Position.Container == slotId || (d.TwoHanded && tx.Container(i.Position.Container).Slot == "offhand") ||
            (dest.Slot == "offhand" && _inventory.Definition(i.Definition).TwoHanded)))
            .OrderByDescending(i => _inventory.Definition(i.Definition).Width * _inventory.Definition(i.Definition).Height)
            .ThenBy(i => i.Id, StringComparer.Ordinal).ToArray();
        tx.MoveItem(id, new Position(slotId));
        bool Place(int index)
        {
            if (index == conflicts.Length) { _inventory.Validate(tx.State); return true; }
            var old = conflicts[index];
            foreach (var position in tx.Positions(old.Id, source.Id))
            {
                tx.MoveItem(old.Id, position);
                var partial = new InventoryTransaction(_inventory, tx.State.Copy(), _nextId);
                foreach (var pending in conflicts.Skip(index + 1)) partial.RemoveItem(pending.Id);
                try { _inventory.Validate(partial.State); if (Place(index + 1)) return true; } catch (Rejected) { }
            }
            return false;
        }
        Inventory.Require(Place(0), "Cannot return all equipment to source grid");
    }
    private void EquipBest(InventoryTransaction tx, string id)
    {
        var item = tx.Item(id);
        if (tx.Container(item.Position.Container).Kind != ContainerKind.Grid) { tx.PlaceInGrid(id, "bag"); return; }
        var d = _inventory.Definition(item.Definition);
        var occupied = _inventory.Validate(tx.State);
        var slots = tx.State.Containers.Where(c => c.Kind == ContainerKind.Slot && !d.Slots.IsDefault && d.Slots.Contains(c.Slot!))
            .OrderBy(c => occupied.ContainsKey((c.Id, null, null)));
        foreach (var slot in slots)
        {
            var trial = new InventoryTransaction(_inventory, tx.State.Copy(), _nextId);
            try { Equip(trial, id, slot.Id); tx.State = trial.State; return; } catch (Rejected) { }
        }
        throw new Rejected("No compatible equipment slot with space");
    }
    private static void Transfer(InventoryTransaction tx, string id) => tx.PlaceInGrid(id, tx.Item(id).Position.Container == "bag" ? "ground" : "bag");
    private static void Socket(InventoryTransaction tx, string id, string socketId)
    {
        Inventory.Require(tx.Container(socketId).Kind == ContainerKind.Socket, "Invalid socket");
        if (tx.Item(id).Quantity > 1) tx.SplitStack(id, 1, new Position(socketId)); else tx.MoveItem(id, new Position(socketId));
    }
    private void Unsocket(InventoryTransaction tx, string id, string containerId)
    {
        var item = tx.Item(id);
        Inventory.Require(tx.Container(item.Position.Container).Kind == ContainerKind.Socket, "Item is not socketed");
        Inventory.Require(tx.Container(containerId).Kind == ContainerKind.Grid, "Destination is not a grid");
        var target = tx.State.Items.Find(i => i.Id != id && i.Position.Container == containerId && i.Definition == item.Definition && i.Quantity < _inventory.Definition(i.Definition).MaxStack);
        if (target is not null) tx.MergeStacks(id, target.Id); else tx.PlaceInGrid(id, containerId);
    }
    private static void Activate(InventoryTransaction tx, string key)
    {
        if (!tx.State.Shortcuts.TryGetValue(key, out var reference) || reference is not ItemShortcut item)
            throw new Rejected("Shortcut has no item");
        Inventory.Require(tx.Item(item.ItemId).Definition == "ember-flask", "Item cannot be activated");
        tx.ConsumeItem(item.ItemId, 1);
    }
    public Result CreateRelic()
    {
        if (_publishing) return new Failure(ErrorCode.Busy, "Cannot mutate during notification");
        var definition = _choose(Definitions); var ground = Inventory.Container(Snapshot(), "ground"); var id = _nextId();
        for (var y = 0; y < ground.Height; y++) for (var x = 0; x < ground.Width; x++)
            {
                var item = new Item(id, definition.Id, 1, new Position("ground", x, y));
                var result = _inventory.Preview(tx => tx.AddItem(item), _nextId);
                if (result is Success) return Commit(result);
            }
        return new Failure(ErrorCode.NoSpace, "No space on Ground");
    }
    public Comparison? Comparison(string id)
    {
        var state = Snapshot();
        try
        {
            var item = Inventory.Item(state, id); var d = _inventory.Definition(item.Definition);
            var slot = state.Containers.Find(c => c.Kind == ContainerKind.Slot && !d.Slots.IsDefault && d.Slots.Contains(c.Slot!));
            if (slot is null) return null;
            var result = PreviewEquipItem(id, slot.Id);
            if (result is Failure failure) return new Comparison(Message: failure.Message);
            var before = Attributes(state); var after = ((Success)result).Attributes!;
            return new Comparison(after.Power - before.Power, after.Ward - before.Ward);
        }
        catch (Rejected) { return null; }
    }
    public Result MoveItem(string id, string container, int x, int y, int rotation, int? expectedRevision = null) => Execute(tx => tx.MoveItem(id, new Position(container, x, y, rotation)), expectedRevision);
    public Result PreviewMoveItem(string id, string container, int x, int y, int rotation) => Preview(tx => tx.MoveItem(id, new Position(container, x, y, rotation)));

    public Result RemoveItem(string id, int? expectedRevision = null) => Execute(tx => tx.RemoveItem(id), expectedRevision);
    public Result PreviewRemoveItem(string id) => Preview(tx => tx.RemoveItem(id));

    public Result SplitStack(string id, int quantity, string container, int x, int y, int rotation, int? expectedRevision = null) => Execute(tx => tx.SplitStack(id, quantity, new Position(container, x, y, rotation)), expectedRevision);
    public Result PreviewSplitStack(string id, int quantity, string container, int x, int y, int rotation) => Preview(tx => tx.SplitStack(id, quantity, new Position(container, x, y, rotation)));

    public Result MergeStacks(string id, string target, int? expectedRevision = null) => Execute(tx => tx.MergeStacks(id, target), expectedRevision);
    public Result PreviewMergeStacks(string id, string target) => Preview(tx => tx.MergeStacks(id, target));

    public Result EquipItem(string id, string slot, int? expectedRevision = null) => Execute(tx => Equip(tx, id, slot), expectedRevision);
    public Result PreviewEquipItem(string id, string slot) => Preview(tx => Equip(tx, id, slot));

    public Result EquipBestSlot(string id, int? expectedRevision = null) => Execute(tx => EquipBest(tx, id), expectedRevision);
    public Result PreviewEquipBestSlot(string id) => Preview(tx => EquipBest(tx, id));

    public Result TransferItem(string id, int? expectedRevision = null) => Execute(tx => Transfer(tx, id), expectedRevision);
    public Result PreviewTransferItem(string id) => Preview(tx => Transfer(tx, id));

    public Result SocketGem(string id, string socket, int? expectedRevision = null) => Execute(tx => Socket(tx, id, socket), expectedRevision);
    public Result PreviewSocketGem(string id, string socket) => Preview(tx => Socket(tx, id, socket));

    public Result UnsocketGem(string id, string container, int? expectedRevision = null) => Execute(tx => Unsocket(tx, id, container), expectedRevision);
    public Result PreviewUnsocketGem(string id, string container) => Preview(tx => Unsocket(tx, id, container));

    public Result BindItem(string key, string id, int? expectedRevision = null) => Execute(tx => tx.BindItem(key, id), expectedRevision);
    public Result PreviewBindItem(string key, string id) => Preview(tx => tx.BindItem(key, id));

    public Result ActivateShortcut(string key, int? expectedRevision = null) => Execute(tx => Activate(tx, key), expectedRevision);
    public Result PreviewActivateShortcut(string key) => Preview(tx => Activate(tx, key));

}
