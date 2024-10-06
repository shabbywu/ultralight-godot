# ultralight-godot

Display Web-Content in Godot. You can use html/css/JavaScript as like godot native part.

## Installation

1. download prebuilt binaray from [github release page](https://github.com/shabbywu/ultralight-godot/releases/tag/v0.2.0)
2. unzip archive to your godot project rootdir, you will get a folder named addons.
3. launcher godot editor, add new Node `UltralightView` to display web-content in Godot.


## Features
- Render html in godot.
- Multiple platform support:
  - Windows(x86_64 is tested)
  - Macos(x86_64 is tested)
  - ❕Linux (build successfully but untested, **if anyone can test it, please let me know**)
- Use CSS to control styles, which is easier to use and controllable than Godot's native UI.
- JavaScript Interop with godot, just like native part of godot.
  - Execute JavaScript from csharp/gdscript.
  - Bind csharp/gdscript callback to ultralight JavaScript context.
  - Bind csharp/gdscript object inhereit from godot::Object to ultralight JavaScript context.
  - Call JavaScript function in csharp/gdscript.
- Built in docs, can check at godot editor.
- Supports transparent background, helpful for use as a UI system.
- Input
    - Mouse Input
    - Keyboard Input
    - Mouse Scroll Input
- Remote UI Inspector, aka, DevTools.

## Samples
- [Sample 1 - Load Html And Bind CSharp Callback](samples/Sample%201%20-%20Load%20Html%20And%20Bind%20CSharp%20Callback)
- [Sample 2 - Grid-based Inventory System](samples/Sample%202%20-%20Inventory%20System)
- [Sample 3 - 3D](samples/Sample%202%20-%203D)

## In Progress
- [ ] More Samples about how to use ultralight-godot as UI system.
- [ ] Tutorial...


## Screenshot

![image](https://github.com/user-attachments/assets/6216a3bb-6f5d-4d63-9aa7-102b0001135b)

https://github.com/user-attachments/assets/b193f148-a446-4772-89aa-aed81d1bd360
