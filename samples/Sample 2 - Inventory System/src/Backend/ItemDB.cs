using System.Collections.Immutable;

namespace AshenReliquary.Backend;

/// <summary>Immutable item database backed by the shared catalog, independent of Godot APIs.</summary>
public sealed class ItemDB
{
    private readonly Dictionary<string, Definition> _definitions;
    public Catalog Catalog { get; }
    public ImmutableArray<Definition> Definitions => Catalog.Definitions;
    public ImmutableArray<Container> Containers => Catalog.Containers;

    public ItemDB(Catalog catalog)
    {
        Inventory.Require(!catalog.Definitions.IsDefaultOrEmpty && !catalog.Containers.IsDefaultOrEmpty, "Empty catalog");
        Inventory.Require(catalog.Definitions.Select(d => d.Id).Distinct(StringComparer.Ordinal).Count() == catalog.Definitions.Length, "Duplicate item definition");
        Catalog = catalog;
        _definitions = catalog.Definitions.ToDictionary(d => d.Id, StringComparer.Ordinal);
    }
    public static ItemDB FromJson(string json) => new(SnapshotCodec.ReadCatalog(json));
    public Definition Get(string id) => _definitions.TryGetValue(id, out var definition) ? definition : throw new Rejected("Unknown definition: " + id);
}
