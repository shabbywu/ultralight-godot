using AshenReliquary.Backend;
using Godot;
using GArray = Godot.Collections.Array;
using GDict = Godot.Collections.Dictionary;

namespace AshenReliquary;

/// <summary>Explicit scalar application API. Snake-case member names implement the shared frontend contract.</summary>
public partial class GameBridge : RefCounted
{
    private Game _game = null!;
    public void Initialize(Game game) => _game = game;
    public GArray definitions => WireCodec.Definitions(_game.Definitions);
    public string host_name() => "Godot / Ultralight";
    public GDict read() => WireCodec.Read(_game.Read());
    public GDict create_relic() => WireCodec.Result(_game.CreateRelic());
    public bool save() => _game.Save();
    public Variant comparison(string itemId) => WireCodec.Comparison(_game.Comparison(itemId));
    public GDict move_item(string id, string container, int x, int y, int rotation, int expectedRevision) => WireCodec.Result(_game.MoveItem(id, container, x, y, rotation, expectedRevision));
    public GDict preview_move_item(string id, string container, int x, int y, int rotation) => WireCodec.Result(_game.PreviewMoveItem(id, container, x, y, rotation));

    public GDict remove_item(string id, int expectedRevision) => WireCodec.Result(_game.RemoveItem(id, expectedRevision));
    public GDict preview_remove_item(string id) => WireCodec.Result(_game.PreviewRemoveItem(id));

    public GDict split_stack(string id, int quantity, string container, int x, int y, int rotation, int expectedRevision) => WireCodec.Result(_game.SplitStack(id, quantity, container, x, y, rotation, expectedRevision));
    public GDict preview_split_stack(string id, int quantity, string container, int x, int y, int rotation) => WireCodec.Result(_game.PreviewSplitStack(id, quantity, container, x, y, rotation));

    public GDict merge_stacks(string id, string target, int expectedRevision) => WireCodec.Result(_game.MergeStacks(id, target, expectedRevision));
    public GDict preview_merge_stacks(string id, string target) => WireCodec.Result(_game.PreviewMergeStacks(id, target));

    public GDict equip_item(string id, string slot, int expectedRevision) => WireCodec.Result(_game.EquipItem(id, slot, expectedRevision));
    public GDict preview_equip_item(string id, string slot) => WireCodec.Result(_game.PreviewEquipItem(id, slot));

    public GDict equip_best_slot(string id, int expectedRevision) => WireCodec.Result(_game.EquipBestSlot(id, expectedRevision));
    public GDict preview_equip_best_slot(string id) => WireCodec.Result(_game.PreviewEquipBestSlot(id));

    public GDict transfer_item(string id, int expectedRevision) => WireCodec.Result(_game.TransferItem(id, expectedRevision));
    public GDict preview_transfer_item(string id) => WireCodec.Result(_game.PreviewTransferItem(id));

    public GDict socket_gem(string id, string socket, int expectedRevision) => WireCodec.Result(_game.SocketGem(id, socket, expectedRevision));
    public GDict preview_socket_gem(string id, string socket) => WireCodec.Result(_game.PreviewSocketGem(id, socket));

    public GDict unsocket_gem(string id, string container, int expectedRevision) => WireCodec.Result(_game.UnsocketGem(id, container, expectedRevision));
    public GDict preview_unsocket_gem(string id, string container) => WireCodec.Result(_game.PreviewUnsocketGem(id, container));

    public GDict bind_item(string key, string id, int expectedRevision) => WireCodec.Result(_game.BindItem(key, id, expectedRevision));
    public GDict preview_bind_item(string key, string id) => WireCodec.Result(_game.PreviewBindItem(key, id));

    public GDict activate_shortcut(string key, int expectedRevision) => WireCodec.Result(_game.ActivateShortcut(key, expectedRevision));
    public GDict preview_activate_shortcut(string key) => WireCodec.Result(_game.PreviewActivateShortcut(key));

}
