# Introduction

![Sample 1 preview](../../docs/media/sample-1.webp)

Load HTML with async/await and expose callbacks through a persistent, caller-named API registry.

## Usage
1. Install a build matching this source into `src/addons/ultralight-view/`, including its `csharp/` helpers.
2. Run with Godot .NET 4.4.1 and .NET 8 (or a compatible environment).
3. The page calls `game.logInfo()` and `game.main.LogInfo()`; `UltralightApi` restores these registrations after navigation.

See the [API guide](../../docs/api-guide.md) for shared containers, async callbacks and migration.

## Features
- [x] Rendering interactive html in godot.
- [x] Bind csharp callback to ultralight javascript context.
- [x] Bind godot::Object to ultralight javascript context.
