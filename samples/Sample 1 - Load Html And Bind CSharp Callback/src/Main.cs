using Godot;
using System;
using Godot.NativeInterop;

namespace demo;

public partial class Main : Node2D
{
	public int A = 1;
	
	private TextureRect View { get; set; }
	
	// Called when the node enters the scene tree for the first time.
	public override void _Ready()
	{
		View = GetNode<TextureRect>("View");
		View.Connect("on_window_object_ready", new Callable(this, "OnWindowObjectReady"));
		
		// Load html, code base on https://www.w3schools.com/css/tryit.asp?filename=trycss_buttons_hover
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

<button class=""button button1"" onclick=""logInfo('invoke simple godot::Callbale')"">Green</button>
<button class=""button button2"" onclick=""logInfo(Math.random())"">Blue</button>
<button class=""button button3"" onclick=""logInfo(['convert js array to godot::Array', 1,2,3])"">Red</button>
<button class=""button button4"" onclick=""logInfo({{case: 'convert js object to godot::Dictionary', complex: {{array: []}}}})"">Gray</button>
<button class=""button button5"" onclick=""main.LogInfo('access godot::Object function and property: ' + main.A++)"">Black</button>

</body>
</html>
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

	public void OnWindowObjectReady()
	{
		// bind function to window.logInfo
		View.Call("bind_func", "logInfo", new Callable(this, "LogInfo"));
		// bind object to window.main
		View.Call("bind_object", "main", this);
	}
}
