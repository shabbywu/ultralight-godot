# ultralight-godot

**使用 HTML、CSS 和 JavaScript 构建 Godot 界面。**

[English](README.md) | 简体中文

ultralight-godot 通过 GDExtension 将 [Ultralight](https://ultralig.ht/) 集成到 Godot。
使用 `UltralightView` 显示网页内容，通过 GDScript 或 C# 与 JavaScript 交互，
并在 2D、3D 场景中使用支持透明背景的网页界面。

![在 Godot 中渲染 HTML 界面](https://github.com/user-attachments/assets/6216a3bb-6f5d-4d63-9aa7-102b0001135b)

[Showcase](https://shabbywu.github.io/ultralight-godot/zh-CN/) · [API 文档](https://shabbywu.github.io/ultralight-godot/zh-CN/api/) · [安装](#安装) · [示例](#示例) · [JavaScript 绑定](#javascript-绑定) · [自动发布](#自动发布)

## 功能

- 渲染 HTML，使用 CSS 编写界面样式，支持透明背景。
- 在 GDScript 或 C# 中执行 JavaScript、调用 JavaScript 函数。
- 向 JavaScript 暴露 Godot 对象和回调。
- 支持鼠标移动、点击、滚轮和键盘输入。
- 使用配套 mini-browser 远程检查器调试网页内容。
- 在 Godot 编辑器中直接查阅类文档。

## 环境要求

| 组件 | 支持版本 / 平台 |
| --- | --- |
| Godot | 4.3 及以上（扩展声明的最低兼容版本） |
| Ultralight SDK | 当前源码要求 **2.0.0-beta.2** |
| Windows | x86_64 |
| Linux | x86_64 |
| macOS | arm64（Apple Silicon） |

当前流水线生成的发布包已包含匹配的 SDK 运行库。历史版本可能使用不同的 SDK，
请查阅对应 Release 的说明，确保扩展与 SDK 版本匹配。

**Ultralight Free 版本将 View 动画与重绘限制为 60 FPS。**
关闭 Godot 帧率上限、关闭 VSync 或将 `max_render_fps` 设为 `0`，
均无法解除 SDK 版本本身的限制。

## 安装

1. 打开 [Releases 页面](https://github.com/shabbywu/ultralight-godot/releases)。
2. 下载目标版本的 `ultralight-view.<tag>.gdextension.7z`。
3. 将压缩包解压到 Godot 项目根目录。当前流水线生成的安装包目录结构如下：

   ```text
   your-project/
   ├── project.godot
   └── addons/
       └── ultralight-view/
           ├── ultralight-view.gdextension
           ├── csharp/
           └── bin/
               ├── win64/
               ├── x11/
               └── macos-arm64/
   ```

4. 在 Godot 中打开项目，添加 `UltralightView` 节点。
5. 参考[示例](#示例)加载网页内容并与游戏逻辑连接。

合并安装包包含三个平台的 Debug、Release 扩展及匹配的 Ultralight 运行库，
使用时无需单独下载 SDK。升级时请替换原有的 `addons/ultralight-view` 目录，避免混用版本。

如需使用远程检查器，请从同一个 Release 下载对应平台的 `mini-browser` 压缩包。
每个浏览器安装包也已包含所需的 SDK 运行库。

## 示例

| 示例 | 演示内容 |
| --- | --- |
| [加载 HTML 并绑定 C# 回调](samples/Sample%201%20-%20Load%20Html%20And%20Bind%20CSharp%20Callback) | 加载网页内容，通过 JavaScript 调用 C# |
| [Ashen Reliquary 背包](samples/Sample%202%20-%20Inventory%20System) | 使用网页技术构建背包界面 |
| [3D 界面](samples/Sample%203%20-%203D) | 在 3D 场景中集成 `UltralightView` |
| [GPU / CPU 对比](samples/Sample%204%20-%20GPU) | Canvas/SVG 动画、FPS 和渲染耗时 |

以下为实际 Godot 运行画面的循环 WebP 录制，点击示例名称查看使用说明。

### 1 · HTML 与 C# 回调

![按钮悬停及 JavaScript 调用 C#](docs/media/sample-1.webp)

### 2 · Ashen Reliquary 背包

![装备、镶嵌、快捷栏和后端生成物品](docs/media/sample-2.webp)

### 3 · 3D 网页界面

![3D 场景中的网页滚动与物理物体](docs/media/sample-3.webp)

### 4 · GPU / CPU 对比

![切换渲染后端与 SVG、Canvas 动画](docs/media/sample-4.webp)

## JavaScript 绑定

注册一次，刷新时自动恢复；namespace 由应用自行选择。

```gdscript
var api := UltralightAPI.create("game")

func _ready():
    api.register_function("save", save_game)
    api.register_object("inventory", inventory)
    api.attach($WebView)
    var loading = $WebView.load_url("file:///ui/index.html")
    if not loading.is_completed():
        await loading.completed
    var result = loading.get_result()
    if not result.ok:
        push_error(result.error.message)
```

```javascript
await game.save();
game.inventory.items.push(item);
```

新 API 提供主文档／框架信号、显式 Result／Task、C# async/await 和双向共享容器。
容器按需包装，修改会写回源数据；需要独立副本时使用 `evaluate_copy()` 或 `snapshot()`。
`UltralightBindingOptions` 可限制根对象成员、嵌套写入及按值转换。

参阅[使用指南与迁移说明](docs/api-guide.md)，了解 API 契约及与早期版本的差异。
历史 Release 的文档应以对应 tag 为准。
C# 项目需要 Godot .NET 4.4.1／.NET 8 或兼容环境，并编译安装包内的 `csharp/*.cs`。

添加 `UltralightInspector` 并设置 `target_view` 即可内嵌调试工具；[分屏场景](samples/inspector.tscn)可直接复用。
文本剪贴板、CSS 光标和 Control 焦点由 Godot 接入，无需 AppCore。
完整 IME 预编辑尚未提供。扩展运行库为 Ultralight、UltralightCore、WebCore；mini-browser 单独使用 AppCore。

## Showcase 与 API 文档

[在线展示](https://shabbywu.github.io/ultralight-godot/zh-CN/)收录四个示例的运行录制，
[API 文档](https://shabbywu.github.io/ultralight-godot/zh-CN/api/)包含插件类的属性、方法与信号。
API 内容由 `ultralight-view/doc_classes/` 的 Godot XML 文档生成。
中文正文和搜索摘要来自 `site/locales/api.zh-CN.json`，签名及默认参数与英文版共用 XML，构建时检查翻译是否齐全。

本地预览：

```sh
python3 scripts/build_site.py
python3 -m http.server 8000 --directory dist
```

`Showcase Pages` 工作流构建 PR，并在 `main` 分支部署站点。
仓库的 **Settings → Pages → Source** 需设为 **GitHub Actions**。

## 自动发布

发布 GitHub Release（包括预发布版本）后，会针对该 Release 的 tag 启动两个独立的工作流：

| 工作流 | 上传的附件 |
| --- | --- |
| [Build GDExtension](.github/workflows/build-gdextension.yaml) | `ultralight-view.<tag>.gdextension.7z`，包含所有支持平台的扩展和 SDK 运行库 |
| [Build Mini Browser](.github/workflows/build-mini-browser.yaml) | `mini-browser.linux.7z`、`mini-browser.win64.7z`、`mini-browser.macos-arm64.7z` |

1. 提交 workflow 修改，确保版本 tag 包含这些修改。
2. 为该 tag 创建 Release，并点击 **Publish release**。仅保存草稿不会触发构建。
3. 每个工作流分别构建 Windows x86_64、Linux x86_64 和 macOS arm64。
4. 各自的构建及已有验证步骤全部成功后，打包并将附件上传到该 Release。
   一个工作流失败不会阻止另一个工作流上传。

打包时会检查扩展、可执行文件及 SDK 运行库是否齐全，缺失则失败。
上传使用内置的 `GITHUB_TOKEN`，无需额外配置 secret。

如需重新构建已有 Release 的附件，可在 Actions 中手动运行对应工作流，
将 `release_tag` 填为目标 tag。工作流会检出该 tag，覆盖同名附件，并保留 Release 描述。
留空 `release_tag` 则只生成可下载的 Actions 构建产物，不上传到 Release。
