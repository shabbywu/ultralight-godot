using Godot.Collections;

namespace InventorySystem;

using Godot;

public partial class Inventory: GodotObject
{
    public Array Layout { get; set; }
    public Callable AddItemCallback { get; set; }
    
    public Inventory()
    {
        Layout = new Array();
    }
    
    public void AddItem(Dictionary item)
    {
        AddItemCallback.Call(item);
    }
}