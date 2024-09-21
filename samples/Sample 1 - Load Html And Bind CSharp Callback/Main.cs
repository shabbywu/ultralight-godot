using Godot;
using System;
using Godot.NativeInterop;

namespace demo;

public partial class Main : Node2D
{
    [Export] public int A = 1;
    
	private TextureRect View { get; set; }
	
	// Called when the node enters the scene tree for the first time.
	public override void _Ready()
	{
		View = GetNode<TextureRect>("View");
        View.Connect("on_dom_ready", new Callable(this, "OnDomReady"));
        
		View.Set("html", $@"
<!DOCTYPE html>
<html>
<head>
<style>
.button {{
  background-color: #04AA6D; /* Green */
  border: none;
  color: white;
  padding: 16px 32px;
  text-align: center;
  text-decoration: none;
  display: inline-block;
  font-size: 16px;
  margin: 4px 2px;
  transition-duration: 0.4s;
  cursor: pointer;
}}

.button1 {{
  background-color: white; 
  color: black; 
  border: 2px solid #04AA6D;
}}

.button1:hover {{
  background-color: #04AA6D;
  color: white;
}}

.button2 {{
  background-color: white; 
  color: black; 
  border: 2px solid #008CBA;
}}

.button2:hover {{
  background-color: #008CBA;
  color: white;
}}

.button3 {{
  background-color: white; 
  color: black; 
  border: 2px solid #f44336;
}}

.button3:hover {{
  background-color: #f44336;
  color: white;
}}

.button4 {{
  background-color: white;
  color: black;
  border: 2px solid #e7e7e7;
}}

.button4:hover {{background-color: #e7e7e7;}}

.button5 {{
  background-color: white;
  color: black;
  border: 2px solid #555555;
}}

.button5:hover {{
  background-color: #555555;
  color: white;
}}
</style>
</head>
<body>

<h2>Hoverable Buttons</h2>

<p>Use the :hover selector to change the style of the button when you move the mouse over it.</p>
<p><strong>Tip:</strong> Use the transition-duration property to determine the speed of the ""hover"" effect:</p>

<button class=""button button1"" onclick=""logInfo('click Green')"">Green</button>
<button class=""button button2"" onclick=""logInfo(455)"">Blue</button>
<button class=""button button3"" onclick=""logInfo([1,2,3])"">Red</button>
<button class=""button button4"" onclick=""logInfo({{a: 1, b: 2}})"">Gray</button>
<button class=""button button5"" onclick=""main.LogInfo(main.A++)"">Black</button>

</body>
</html>
");
        
        View.Call("bind_func", "logInfo", new Callable(this, "LogInfo"));
        View.Call("execute_script", @"
logInfo(""test"");
var a = {
    a: 1,
    b: [1,2,3],
    c: { c1: 2}
};
logInfo(a);
");

    }

	// Called every frame. 'delta' is the elapsed time since the previous frame.
	public override void _Process(double delta)
	{

	}
    
	public void LogInfo(Variant message)
	{
		GD.Print(message);
	}

    public void OnDomReady()
    {
        View.Call("bind_func", "logInfo", new Callable(this, "LogInfo"));
        View.Call("bind_object", "main", this);
    }
}
