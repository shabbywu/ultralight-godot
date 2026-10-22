using System.Collections.Immutable;
using System.Collections.ObjectModel;

namespace AshenReliquary.Backend;

/// <summary>A domain rejection discards the isolated candidate.</summary>
public sealed class Rejected(string message) : Exception(message);

/// <summary>Own and validate the authoritative graph; gameplay is composed by Game.</summary>
public sealed class Inventory
{
    private readonly Dictionary<string, Definition> _definitions;
    private Snapshot _state;
    public IReadOnlyDictionary<string, Definition> Definitions { get; }

    public Inventory(ImmutableArray<Definition> definitions, Snapshot snapshot)
    {
        Require(definitions.All(d => d.Width > 0 && d.Height > 0 && d.MaxStack > 0), "Invalid definitions");
        Require(definitions.Select(d => d.Id).Distinct(StringComparer.Ordinal).Count() == definitions.Length, "Duplicate definition");
        _definitions = definitions.ToDictionary(d => d.Id, StringComparer.Ordinal);
        Definitions = new ReadOnlyDictionary<string, Definition>(_definitions);
        Validate(snapshot);
        _state = snapshot.Copy();
    }

    public static void Require(bool condition, string message)
    {
        if (!condition) throw new Rejected(message);
    }
    public Snapshot Snapshot() => _state.Copy();
    public Definition Definition(string id) => _definitions.TryGetValue(id, out var d) ? d : throw new Rejected("Unknown definition");
    public static Item Item(Snapshot state, string id) => state.Items.Find(i => i.Id == id) ?? throw new Rejected("Unknown item: " + id);
    public static Container Container(Snapshot state, string id) => state.Containers.Find(c => c.Id == id) ?? throw new Rejected("Unknown container: " + id);
    public (int Width, int Height) Dimensions(Item item)
    {
        var d = Definition(item.Definition);
        return item.Position.Rotation % 2 == 0 ? (d.Width, d.Height) : (d.Height, d.Width);
    }

    /// <summary>Validate the entire graph and return occupied cells; null coordinates identify slots.</summary>
    public Dictionary<(string Container, int? X, int? Y), string> Validate(Snapshot state)
    {
        Require(state.SchemaVersion == 1 && state.Revision >= 0, "Invalid schema or revision");
        Require(state.Containers.Select(c => c.Id).Distinct(StringComparer.Ordinal).Count() == state.Containers.Count, "Duplicate container");
        Require(state.Items.Select(i => i.Id).Distinct(StringComparer.Ordinal).Count() == state.Items.Count, "Duplicate item ID");
        var items = state.Items.ToDictionary(i => i.Id, StringComparer.Ordinal);
        var cells = new Dictionary<(string, int?, int?), string>();
        foreach (var c in state.Containers)
        {
            Require(!string.IsNullOrEmpty(c.Id), "Empty container identity");
            Require(c.Kind == ContainerKind.Socket || c.Owner is null, "Invalid owner");
            if (c.Kind == ContainerKind.Grid) Require(c.Width > 0 && c.Height > 0, "Invalid grid dimensions");
            if (c.Kind == ContainerKind.Socket) Require(c.Owner is not null && items.ContainsKey(c.Owner), "Missing socket owner");
        }
        foreach (var item in state.Items)
        {
            Require(!string.IsNullOrEmpty(item.Id), "Empty item identity");
            var d = Definition(item.Definition);
            var c = Container(state, item.Position.Container);
            var p = item.Position;
            Require(item.Quantity >= 1 && item.Quantity <= d.MaxStack, "Invalid quantity");
            Require(p.Rotation >= 0 && p.Rotation <= 3 && (d.Rotatable || p.Rotation == 0), "Invalid rotation");
            Require(c.Accepts.IsDefault || c.Accepts.Contains(item.Definition), "Container rejected item");
            var keys = new List<(string, int?, int?)>();
            if (c.Kind == ContainerKind.Grid)
            {
                var (w, h) = Dimensions(item);
                Require(p.X is >= 0 && p.Y is >= 0 && (long)p.X.Value + w <= c.Width && (long)p.Y.Value + h <= c.Height, "Out of bounds");
                for (var y = p.Y!.Value; y < p.Y.Value + h; y++)
                    for (var x = p.X!.Value; x < p.X.Value + w; x++) keys.Add((c.Id, x, y));
            }
            else
            {
                Require(c.Kind != ContainerKind.Slot || (!d.Slots.IsDefault && d.Slots.Contains(c.Slot!)), "Invalid equipment slot");
                Require(c.Kind != ContainerKind.Socket || item.Quantity == 1, "Socket takes one gem");
                keys.Add((c.Id, null, null));
                if (d.TwoHanded && c.Slot == "mainhand")
                    keys.AddRange(state.Containers.Where(v => v.Slot == "offhand").Select(v => (v.Id, (int?)null, (int?)null)));
            }
            foreach (var key in keys)
            {
                Require(!cells.ContainsKey(key), "Occupied");
                cells.Add(key, item.Id);
            }
            var owner = c.Owner;
            var seen = new HashSet<string>(StringComparer.Ordinal) { item.Id };
            while (owner is not null)
            {
                Require(seen.Add(owner), "Socket cycle");
                owner = Container(state, Item(state, owner).Position.Container).Owner;
            }
        }
        foreach (var (key, reference) in state.Shortcuts)
        {
            Require(key is not ("__proto__" or "prototype" or "constructor"), "Invalid shortcut key");
            switch (reference)
            {
                case ItemShortcut r: Require(items.ContainsKey(r.ItemId), "Invalid shortcut reference"); break;
                case ActionShortcut r: Require(!string.IsNullOrEmpty(r.Action), "Empty shortcut action"); break;
                default: throw new Rejected("Unknown shortcut type");
            }
        }
        return cells;
    }

    /// <summary>Compose atomic primitives on a private candidate, then validate once.</summary>
    public Result Preview(Action<InventoryTransaction> action, Func<string> allocateId)
    {
        var tx = new InventoryTransaction(this, Snapshot(), allocateId);
        try
        {
            action(tx);
            Validate(tx.State);
            tx.State = tx.State with { Revision = checked(_state.Revision + 1) };
            var before = _state.Items.ToDictionary(i => i.Id, StringComparer.Ordinal);
            var after = tx.State.Items.ToDictionary(i => i.Id, StringComparer.Ordinal);
            var affected = before.Keys.Union(after.Keys, StringComparer.Ordinal).Where(id => before.GetValueOrDefault(id) != after.GetValueOrDefault(id))
                .Order(StringComparer.Ordinal).ToImmutableArray();
            return new Success(_state.Revision, tx.State, affected);
        }
        catch (Rejected error) { return new Failure(ErrorCode.Rejected, error.Message); }
    }
    public void Commit(Snapshot candidate) { Validate(candidate); _state = candidate.Copy(); }
}

/// <summary>Atomic inventory primitives operating only on a detached candidate.</summary>
public sealed class InventoryTransaction(Inventory inventory, Snapshot state, Func<string> allocateId)
{
    public Inventory Owner { get; } = inventory;
    public Snapshot State { get; set; } = state;
    public Item Item(string id) => Inventory.Item(State, id);
    public Container Container(string id) => Inventory.Container(State, id);
    public void AddItem(Item item) => State.Items.Add(item);
    private void Replace(Item item) => State.Items[State.Items.FindIndex(i => i.Id == item.Id)] = item;
    public void MoveItem(string id, Position destination) => Replace(Item(id) with { Position = destination });

    public void RemoveItem(string id)
    {
        Item(id);
        var removed = new HashSet<string>(StringComparer.Ordinal) { id };
        while (true)
        {
            var children = State.Items.Where(i => Container(i.Position.Container).Owner is { } owner && removed.Contains(owner)).Select(i => i.Id).ToArray();
            var previous = removed.Count;
            removed.UnionWith(children);
            if (removed.Count == previous) break;
        }
        State.Items.RemoveAll(i => removed.Contains(i.Id));
        State.Containers.RemoveAll(c => c.Owner is not null && removed.Contains(c.Owner));
        foreach (var key in State.Shortcuts.Where(pair => pair.Value is ItemShortcut r && removed.Contains(r.ItemId)).Select(p => p.Key).ToArray())
            State.Shortcuts.Remove(key);
    }
    public void SplitStack(string id, int quantity, Position destination)
    {
        var item = Item(id);
        Inventory.Require(quantity > 0 && quantity < item.Quantity, "Invalid split");
        Replace(item with { Quantity = item.Quantity - quantity });
        AddItem(new Item(allocateId(), item.Definition, quantity, destination));
    }
    public void MergeStacks(string id, string targetId)
    {
        var item = Item(id); var target = Item(targetId);
        var a = Owner.Definition(item.Definition); var b = Owner.Definition(target.Definition);
        Inventory.Require(id != targetId && (a.StackKey ?? a.Id) == (b.StackKey ?? b.Id), "Incompatible stack");
        var count = Math.Min(item.Quantity, b.MaxStack - target.Quantity);
        Inventory.Require(count > 0, "Stack full");
        Replace(target with { Quantity = target.Quantity + count });
        if (item.Quantity > count) Replace(item with { Quantity = item.Quantity - count });
        else
        {
            State.Items.Remove(item);
            foreach (var key in State.Shortcuts.Where(p => p.Value is ItemShortcut r && r.ItemId == id).Select(p => p.Key).ToArray())
                State.Shortcuts[key] = new ItemShortcut(targetId);
        }
    }
    public void ConsumeItem(string id, int quantity)
    {
        var item = Item(id);
        Inventory.Require(quantity > 0 && quantity <= item.Quantity, "Invalid consumption");
        if (item.Quantity == quantity) RemoveItem(id);
        else Replace(item with { Quantity = item.Quantity - quantity });
    }
    public void BindItem(string key, string id) { Item(id); State.Shortcuts[key] = new ItemShortcut(id); }
    public IEnumerable<Position> Positions(string id, string containerId)
    {
        var c = Container(containerId);
        Inventory.Require(c.Kind == ContainerKind.Grid, "Destination is not a grid");
        var rotations = Owner.Definition(Item(id).Definition).Rotatable ? new[] { 0, 1 } : new[] { 0 };
        for (var y = 0; y < c.Height; y++) for (var x = 0; x < c.Width; x++) foreach (var rotation in rotations)
                    yield return new Position(containerId, x, y, rotation);
    }
    public void PlaceInGrid(string id, string containerId)
    {
        var previous = Item(id).Position;
        foreach (var position in Positions(id, containerId))
        {
            MoveItem(id, position);
            try { Owner.Validate(State); return; } catch (Rejected) { }
        }
        MoveItem(id, previous);
        throw new Rejected("No space");
    }
}
