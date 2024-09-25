using System;
using System.Collections.Generic;
using System.Linq;

namespace InventorySystem;

public class ItemDB
{
    public class ItemDef
    {
        public String Name { get; set; }
        public int Width { get; set; }
        public int Height { get; set; }
        public String Image { get; set; }
    }

    private static Dictionary<String, ItemDef> Defs = new Dictionary<string, ItemDef>();

    static ItemDB()
    {
        Defs.Add("FrostBite", new ItemDef()
        {
            Name = "FrostBite",
            Width = 1,
            Height = 3,
            Image = "file:///webfe/src/assets/images/FrostBite.webp"
        });
        
        Defs.Add("LuckyPiggybank", new ItemDef()
        {
            Name = "LuckyPiggybank",
            Width = 2,
            Height = 1,
            Image = "file:///webfe/src/assets/images/LuckyPiggybank.webp"
        });
        
        Defs.Add("LunarArmor", new ItemDef()
        {
            Name = "LunarArmor",
            Width = 2,
            Height = 3,
            Image = "file:///webfe/src/assets/images/LunarArmor.webp"
        });
        
        Defs.Add("QQIcon", new ItemDef()
        {
            Name = "QQIcon",
            Width = 1,
            Height = 1,
            Image = "file:///webfe/src/assets/images/QQIcon.webp"
        });
    }

    public static Godot.Collections.Dictionary GenerateRandomItem()
    {
        var def = Defs.ElementAt(Random.Shared.Next(0, Defs.Count)).Value;
        Godot.Collections.Dictionary o = new()
        {
            {"w", def.Width},
            {"h", def.Height},
            {"content", $"<img src=\"{def.Image}\"></img>"}
        };
        return o;
    }
}