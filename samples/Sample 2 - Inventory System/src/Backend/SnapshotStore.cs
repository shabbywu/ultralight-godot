using System.Text.Json;
using System.Text.Json.Serialization;

namespace AshenReliquary.Backend;

/// <summary>Persistence port independent of the scene tree and JavaScript.</summary>
public interface ISnapshotStore
{
    Snapshot? Load();
    void Save(Snapshot snapshot);
}

/// <summary>Strict JSON conversion used only for files and catalog configuration.</summary>
public static class SnapshotCodec
{
    private sealed record ShortcutDocument(string? Item = null, string? Action = null);
    private sealed record SnapshotDocument(int SchemaVersion, int Revision, List<Container> Containers,
        List<Item> Items, Dictionary<string, ShortcutDocument> Shortcuts);
    private static readonly JsonSerializerOptions Options = CreateOptions();
    private static JsonSerializerOptions CreateOptions()
    {
        var options = new JsonSerializerOptions
        {
            PropertyNamingPolicy = JsonNamingPolicy.CamelCase,
            UnmappedMemberHandling = JsonUnmappedMemberHandling.Disallow,
            DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingDefault,
            WriteIndented = true
        };
        options.Converters.Add(new JsonStringEnumConverter(JsonNamingPolicy.CamelCase, allowIntegerValues: false));
        return options;
    }
    public static Catalog ReadCatalog(string json)
    {
        var catalog = JsonSerializer.Deserialize<Catalog>(json, Options) ?? throw new JsonException("Missing catalog");
        if (catalog.Definitions.IsDefaultOrEmpty || catalog.Containers.IsDefaultOrEmpty) throw new JsonException("Empty catalog");
        return catalog;
    }
    public static string Encode(Snapshot snapshot) => JsonSerializer.Serialize(new SnapshotDocument(
        snapshot.SchemaVersion, snapshot.Revision, snapshot.Containers, snapshot.Items,
        snapshot.Shortcuts.ToDictionary(p => p.Key, p => p.Value switch
        {
            ItemShortcut r => new ShortcutDocument(Item: r.ItemId),
            ActionShortcut r => new ShortcutDocument(Action: r.Action),
            _ => throw new JsonException("Unknown shortcut")
        }, StringComparer.Ordinal)), Options);
    public static Snapshot Decode(string json)
    {
        var value = JsonSerializer.Deserialize<SnapshotDocument>(json, Options) ?? throw new JsonException("Missing snapshot");
        if (value.SchemaVersion != 1 || value.Revision < 0 || value.Containers is null || value.Items is null || value.Shortcuts is null)
            throw new JsonException("Invalid schema or snapshot records");
        if (value.Containers.Any(c => c is null || string.IsNullOrEmpty(c.Id)) ||
            value.Items.Any(i => i is null || string.IsNullOrEmpty(i.Id) || string.IsNullOrEmpty(i.Definition) || i.Position is null || string.IsNullOrEmpty(i.Position.Container)))
            throw new JsonException("Invalid identity or position");
        var shortcuts = new Dictionary<string, Shortcut>(StringComparer.Ordinal);
        foreach (var (key, reference) in value.Shortcuts)
        {
            if (reference is null || (reference.Item is null) == (reference.Action is null)) throw new JsonException("Invalid shortcut reference");
            shortcuts.Add(key, reference.Item is { Length: > 0 } item ? new ItemShortcut(item)
                : reference.Action is { Length: > 0 } action ? new ActionShortcut(action) : throw new JsonException("Empty shortcut reference"));
        }
        return new Snapshot(value.Revision, value.Containers, value.Items, shortcuts, value.SchemaVersion);
    }
}

/// <summary>Optional file persistence with detached ownership and atomic replacement.</summary>
public sealed class SnapshotStore(string? path = null) : ISnapshotStore
{
    private Snapshot? _saved = path is not null && File.Exists(path) ? SnapshotCodec.Decode(File.ReadAllText(path)) : null;
    public Snapshot? Load() => _saved?.Copy();
    public void Save(Snapshot snapshot)
    {
        var candidate = snapshot.Copy();
        if (path is not null)
        {
            Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(path))!);
            var temporary = path + ".tmp";
            File.WriteAllText(temporary, SnapshotCodec.Encode(candidate));
            File.Move(temporary, path, overwrite: true);
        }
        _saved = candidate;
    }
}
