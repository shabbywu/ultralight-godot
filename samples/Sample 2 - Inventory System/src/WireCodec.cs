using AshenReliquary.Backend;
using Godot;
using GArray = Godot.Collections.Array;
using GDict = Godot.Collections.Dictionary;
using Container = AshenReliquary.Backend.Container;
using Item = AshenReliquary.Backend.Item;

namespace AshenReliquary;

/// <summary>Fresh Godot Variant containers only at the JavaScript boundary; no domain objects escape.</summary>
public static class WireCodec
{
    private static GArray Values<T>(IEnumerable<T> source, Func<T, Variant> encode)
    {
        var values = new GArray();
        foreach (var item in source) values.Add(encode(item));
        return values;
    }
    public static GArray Definitions(IEnumerable<Definition> definitions) => Values(definitions, d => new GDict
    {
        ["id"] = d.Id,
        ["name"] = d.Name,
        ["width"] = d.Width,
        ["height"] = d.Height,
        ["rarity"] = d.Rarity,
        ["power"] = d.Power,
        ["rotatable"] = d.Rotatable,
        ["maxStack"] = d.MaxStack,
        ["twoHanded"] = d.TwoHanded,
        ["slots"] = d.Slots.IsDefault ? new GArray() : Values(d.Slots, slot => slot)
    });
    private static GDict Position(Position p)
    {
        var result = new GDict { ["container"] = p.Container, ["rotation"] = p.Rotation };
        if (p.X is { } x) result["x"] = x;
        if (p.Y is { } y) result["y"] = y;
        return result;
    }
    private static GDict Container(Container c)
    {
        var result = new GDict { ["id"] = c.Id, ["kind"] = c.Kind.ToString().ToLowerInvariant() };
        if (c.Kind == ContainerKind.Grid) { result["width"] = c.Width; result["height"] = c.Height; }
        if (c.Slot is not null) result["slot"] = c.Slot;
        if (c.Owner is not null) result["owner"] = c.Owner;
        if (!c.Accepts.IsDefault) result["accepts"] = Values(c.Accepts, id => id);
        return result;
    }
    private static GDict Item(Item i) => new() { ["id"] = i.Id, ["definition"] = i.Definition, ["quantity"] = i.Quantity, ["position"] = Position(i.Position) };
    public static GDict Snapshot(Snapshot s)
    {
        var shortcuts = new GDict();
        foreach (var (key, value) in s.Shortcuts)
            shortcuts[key] = value switch
            {
                ItemShortcut item => new GDict { ["item"] = item.ItemId },
                ActionShortcut action => new GDict { ["action"] = action.Action },
                _ => throw new InvalidOperationException("Unknown shortcut")
            };
        return new GDict
        {
            ["schemaVersion"] = s.SchemaVersion,
            ["revision"] = s.Revision,
            ["containers"] = Values(s.Containers, c => Container(c)),
            ["items"] = Values(s.Items, i => Item(i)),
            ["shortcuts"] = shortcuts
        };
    }
    private static GDict Attributes(Attributes a) => new() { ["power"] = a.Power, ["ward"] = a.Ward };
    public static GDict Read(ReadModel model) => new()
    {
        ["snapshot"] = Snapshot(model.Snapshot),
        ["attributes"] = Attributes(model.Attributes),
        ["dirty"] = model.Dirty,
        ["persistenceError"] = model.PersistenceError
    };
    public static GDict Result(Result result)
    {
        if (result is Failure failure) return new GDict
        {
            ["ok"] = false,
            ["code"] = failure.Code switch
            {
                ErrorCode.Stale => "STALE",
                ErrorCode.Busy => "BUSY",
                ErrorCode.NoSpace => "NO_SPACE",
                _ => "REJECTED"
            },
            ["message"] = failure.Message
        };
        var success = (Success)result;
        var value = new GDict
        {
            ["ok"] = true,
            ["revision"] = success.Revision,
            ["snapshot"] = Snapshot(success.Snapshot),
            ["affected"] = Values(success.Affected, id => id)
        };
        if (success.Attributes is { } attributes) value["attributes"] = Attributes(attributes);
        return value;
    }
    public static Variant Comparison(Comparison? value)
    {
        if (value is null) return default(Variant);
        if (value.Message.Length > 0) return new GDict { ["message"] = value.Message };
        return new GDict { ["power"] = value.Power, ["ward"] = value.Ward };
    }
    public static GDict Event(StateChanged value) => new() { ["type"] = value.Kind == ChangeKind.StateChanged ? "stateChanged" : "persistenceChanged", ["revision"] = value.Revision };
}
