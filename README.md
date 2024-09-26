# ultralight-godot
Display Web-Content in Godot. You can use html/css/javascript as like godot native part.

## Features
- [x] Render html in godot.
- [x] Support mainstream OS: Windows(x86_64 is tested)/Macos(x86_64 is tested)/Linux (untested)
- [x] Use CSS to control styles, which is easier to use and controllable than Godot's native UI
- [x] Execute Javascript from csharp/gdscript.
- [x] Bind csharp/gdscript callback to ultralight javascript context.
- [x] Bind csharp/gdscript object inhereit from godot::Object to ultralight javascript context.
- [x] Call javascript function in csharp/gdscript. (NOT godot wasm)
- [x] Built in docs, can check at godot editor.
- [x] Supports transparent background, helpful for use as a UI system.

## Samples
- [Sample 1 - Load Html And Bind CSharp Callback](samples/Sample%201%20-%20Load%20Html%20And%20Bind%20CSharp%20Callback)
- [Sample 2 - Grid-based Inventory System](samples/Sample%202%20-%20Inventory%20System)

## Installation
1. download prebuilt binaray from [github release page](https://github.com/shabbywu/ultralight-godot/releases/tag/v0.2.0)
2. unzip archive to your godot project rootdir, you will get a folder named addons.
3. launcher godot editor, add new Node `UltralightView` to display web-content in Godot.

## Screenshot

![image](https://github.com/user-attachments/assets/6216a3bb-6f5d-4d63-9aa7-102b0001135b)

https://github.com/user-attachments/assets/b193f148-a446-4772-89aa-aed81d1bd360
