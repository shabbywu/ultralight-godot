using System.Collections.Immutable;

namespace AshenReliquary.Backend;

/// <summary>Inventory region categories.</summary>
public enum ContainerKind { Grid, Slot, Socket }
/// <summary>Stable request rejection codes.</summary>
public enum ErrorCode { Rejected, Stale, Busy, NoSpace }
/// <summary>Committed state and persistence notifications.</summary>
public enum ChangeKind { StateChanged, PersistenceChanged }

/// <summary>An immutable exact location; slots and sockets have no grid coordinates.</summary>
public sealed record Position(string Container, int? X = null, int? Y = null, int Rotation = 0);
/// <summary>An immutable backend-owned item definition.</summary>
public sealed record Definition(string Id, string Name, int Width, int Height, string Rarity, int Power,
    bool Rotatable = false, int MaxStack = 1, string? StackKey = null,
    ImmutableArray<string> Slots = default, bool TwoHanded = false);
/// <summary>An immutable region definition with kind-specific configuration.</summary>
public sealed record Container(string Id, ContainerKind Kind, int Width = 0, int Height = 0,
    string? Slot = null, string? Owner = null, ImmutableArray<string> Accepts = default);
/// <summary>An immutable instance value replaced by transaction primitives.</summary>
public sealed record Item(string Id, string Definition, int Quantity, Position Position);
/// <summary>A typed shortcut reference.</summary>
public abstract record Shortcut;
/// <summary>A shortcut referencing an existing item.</summary>
public sealed record ItemShortcut(string ItemId) : Shortcut;
/// <summary>A shortcut referencing a named business action.</summary>
public sealed record ActionShortcut(string Action) : Shortcut;
/// <summary>A complete graph; mutable collections are copied at every ownership boundary.</summary>
public sealed record Snapshot(int Revision, List<Container> Containers, List<Item> Items,
    Dictionary<string, Shortcut> Shortcuts, int SchemaVersion = 1)
{
    /// <summary>Copy mutable collections; contained domain values are immutable.</summary>
    public Snapshot Copy() => this with
    {
        Containers = [.. Containers],
        Items = [.. Items],
        Shortcuts = new Dictionary<string, Shortcut>(Shortcuts, StringComparer.Ordinal)
    };
}
/// <summary>Backend-calculated character attributes.</summary>
public sealed record Attributes(int Power, int Ward);
/// <summary>A closed result type for previews and mutations.</summary>
public abstract record Result;
/// <summary>An admitted candidate or committed transaction.</summary>
public sealed record Success(int Revision, Snapshot Snapshot, ImmutableArray<string> Affected,
    Attributes? Attributes = null) : Result;
/// <summary>A rejected request with unchanged authority.</summary>
public sealed record Failure(ErrorCode Code, string Message) : Result;
/// <summary>An immutable backend event.</summary>
public sealed record StateChanged(ChangeKind Kind, int Revision);
/// <summary>A detached projection for rendering.</summary>
public sealed record ReadModel(Snapshot Snapshot, Attributes Attributes, bool Dirty, string PersistenceError);
/// <summary>An optional equipment delta or a rejection explanation.</summary>
public sealed record Comparison(int Power = 0, int Ward = 0, string Message = "");
/// <summary>Immutable item and region configuration.</summary>
public sealed record Catalog(ImmutableArray<Definition> Definitions, ImmutableArray<Container> Containers);
