using System;
using System.Collections.Generic;
using Godot;

namespace GPUExample;

public partial class Gui : Control {
    [Export] public bool EnableInspector { get; set; }
    [Export] public bool UseGpu { get; set; } = true;
    [Export(PropertyHint.Range, "24,1440,24")] public int CanvasObjects { get; set; } = 480;
    [Export] public bool UseSvg { get; set; } = true;
    [Export] public bool LimitFrameRate { get; set; }

    [Signal] public delegate void OnDomReadyEventHandler();
    [Signal] public delegate void OnWindowObjectReadyEventHandler();

    private static readonly Color Ink = new("e5edf7");
    private static readonly Color Muted = new("93a4bc");
    private static readonly Color Green = new("5ee3b7");
    private static readonly Color Orange = new("ffbc74");
    private static readonly int[] ObjectCounts = { 120, 240, 480, 960, 1440 };
    private SubViewport _viewport;
    private TextureRect _view, _preview;
    private Label _fpsLabel, _backend, _fpsValue, _frameValue, _cpuValue, _uploadValue, _uploadCaption;
    private Label _status, _description;
    private Label _pageFpsValue, _sdkLimit;
    private Button _gpuButton, _cpuButton, _svgButton, _canvasButton, _limitButton;
    private readonly List<Button> _loadButtons = new();
    private FrameGraph _graph;
    private double _elapsed, _cpuUsTotal;
    private int _samples;
    private long _lastUploadBytes;
    private bool _hasUploadBaseline;
    private double _lastPageFrames;
    private bool _hasPageBaseline;

    public override void _Ready() {
        _viewport = GetNode<SubViewport>("SubViewport");
        _view = GetNode<TextureRect>("SubViewport/UltralightView");
        _fpsLabel = GetNode<Label>("SubViewport/PerformanceOverlay/FPSLabel");
        BuildInterface();
        _view.Connect("on_dom_ready", Callable.From(() => {
            StartAnimation();
            _status.Text = "● 本地页面已就绪 · 可点击画面中的交互按钮";
            backing_OnDomReady?.Invoke();
        }));
        _view.Connect("on_window_object_ready", Callable.From(() => backing_OnWindowObjectReady?.Invoke()));
        ApplyFrameLimit();
        _view.Set("accelerated", UseGpu);
        _view.Call("init_view");
        if (EnableInspector)
            Engine.Singleton.GetSingleton("UltralightSingleton").Call("start_remove_inspector_server", "127.0.0.1", 19999);
        UpdateControls();
    }

    public override void _Process(double delta) {
        if (_view == null) return;
        _graph.AddFrame(delta * 1000);
        using var statistics = _view.Call("get_render_statistics").AsGodotDictionary();
        _cpuUsTotal += statistics["ultralight_render_us"].AsDouble();
        _samples++;
        _elapsed += delta;
        if (_elapsed < 0.25) return;
        var gpu = _view.Call("is_accelerated").AsBool();
        var backend = gpu ? "显卡模式 · GPU" : (UseGpu ? "软件模式 · CPU（自动回退）" : "软件模式 · CPU");
        var color = gpu ? Green : Orange;
        var cpuMs = _cpuUsTotal / _samples / 1000;
        var fps = Engine.GetFramesPerSecond();
        var pageFrames = _view.Call("execute_script", "typeof canvasBenchmarkStats === 'object' ? canvasBenchmarkStats.frames : 0").AsDouble();
        _pageFpsValue.Text = _hasPageBaseline && pageFrames >= _lastPageFrames
            ? $"{(pageFrames - _lastPageFrames) / _elapsed:0.0}" : "--";
        _lastPageFrames = pageFrames;
        _hasPageBaseline = true;
        var sdkEdition = statistics.ContainsKey("sdk_edition") ? statistics["sdk_edition"].AsString() : "1.4";
        var sdkMaxFps = statistics.ContainsKey("sdk_max_fps") ? statistics["sdk_max_fps"].AsInt64() : 0;
        _sdkLimit.Text = !statistics.ContainsKey("sdk_max_fps")
            ? $"网页 SDK：{sdkEdition} · 版本上限未报告"
            : sdkMaxFps > 0 ? $"网页 SDK：{sdkEdition} · 最高 {sdkMaxFps} FPS" : $"网页 SDK：{sdkEdition} · 无版本帧率上限";
        var bytes = statistics[gpu ? "bitmap_upload_bytes" : "cpu_page_upload_bytes"].AsInt64();
        var mib = _hasUploadBaseline ? Math.Max(0, bytes - _lastUploadBytes) / _elapsed / 1048576 : 0;
        _lastUploadBytes = bytes;
        _hasUploadBaseline = true;
        _backend.Text = backend;
        _backend.AddThemeColorOverride("font_color", color);
        _fpsValue.Text = $"{fps:0}";
        _frameValue.Text = $"{_elapsed / _samples * 1000:0.0} ms";
        _cpuValue.Text = $"{cpuMs:0.0} ms";
        _uploadValue.Text = $"{mib:0.0} MiB/s";
        _uploadCaption.Text = gpu ? "GPU 资源位图上传" : "CPU 整页像素上传";
        _fpsLabel.AddThemeColorOverride("font_color", color);
        _fpsLabel.Text = $"FPS {fps:0}  |  网页 {_pageFpsValue.Text} FPS  |  {backend}  |  {(UseSvg ? "SVG" : "Canvas")} {CanvasObjects}  |  {(LimitFrameRate ? "Godot 60 FPS" : "Godot 已解锁")}";
        _elapsed = 0;
        _cpuUsTotal = 0;
        _samples = 0;
    }

    private void BuildInterface() {
        Theme = new Theme {
            DefaultFont = new SystemFont { FontNames = new[] { "SF Pro Display", "PingFang SC", "Noto Sans CJK SC", "sans-serif" } },
            DefaultFontSize = 16
        };
        _viewport.GetNode<Control>("PerformanceOverlay").Theme = Theme;
        var background = new ColorRect { Color = new Color("0c111b"), MouseFilter = MouseFilterEnum.Ignore };
        AddChild(background);
        background.SetAnchorsAndOffsetsPreset(LayoutPreset.FullRect);
        var margin = new MarginContainer();
        AddChild(margin);
        margin.SetAnchorsAndOffsetsPreset(LayoutPreset.FullRect);
        foreach (var side in new[] { "left", "right", "top", "bottom" })
            margin.AddThemeConstantOverride("margin_" + side, 24);
        var page = Column(margin, 18);
        var header = Row(page, 16);
        var heading = Column(header, 4);
        heading.SizeFlagsHorizontal = SizeFlags.ExpandFill;
        Text(heading, "ULTRALIGHT  /  RENDER LAB", 13, Green);
        Text(heading, "GPU 性能测试台", 30, Ink);
        var badge = Card(header, new Color("152330"), 14);
        _backend = Text(badge, "显卡模式 · GPU", 18, Green);
        var body = Row(page, 20);
        body.SizeFlagsVertical = SizeFlags.ExpandFill;
        var stage = Column(Card(body, new Color("121b29"), 16), 12);
        stage.GetParent<Control>().SizeFlagsHorizontal = SizeFlags.ExpandFill;
        var stageHeader = Row(stage, 12);
        var title = Text(stageHeader, "实时网页画面", 19, Ink);
        title.SizeFlagsHorizontal = SizeFlags.ExpandFill;
        Text(stageHeader, "1024 × 768  ·  固定渲染分辨率", 13, Muted);
        var aspect = new AspectRatioContainer {
            Ratio = 1024f / 768,
            StretchMode = AspectRatioContainer.StretchModeEnum.Fit,
            SizeFlagsHorizontal = SizeFlags.ExpandFill,
            SizeFlagsVertical = SizeFlags.ExpandFill,
            CustomMinimumSize = new Vector2(540, 405)
        };
        stage.AddChild(aspect);
        _preview = new TextureRect {
            Name = "Preview", Texture = _viewport.GetTexture(),
            ExpandMode = TextureRect.ExpandModeEnum.IgnoreSize,
            StretchMode = TextureRect.StretchModeEnum.Scale,
            MouseFilter = MouseFilterEnum.Stop,
            FocusMode = FocusModeEnum.Click
        };
        aspect.AddChild(_preview);
        _preview.GuiInput += ForwardInput;
        _preview.MouseExited += () => _viewport.NotifyMouseExited();
        _preview.MouseEntered += () => _viewport.NotifyMouseEntered();
        _status = Text(stage, "● 正在载入本地测试页面…", 13, Muted);
        Text(stage, "相同负载下切换后端比较。SVG 侧重几何绘制；Canvas 同时考验 JS、绘图与上传。", 13, Muted);
        var sidebar = Column(body, 12);
        sidebar.CustomMinimumSize = new Vector2(302, 0);
        var stats = Column(Card(sidebar, new Color("121b29"), 16), 4);
        Text(stats, "GODOT 实时性能", 13, Muted);
        var fpsRow = Row(stats, 10);
        _fpsValue = Text(fpsRow, "--", 42, Ink);
        var fpsUnits = Text(fpsRow, "FPS", 16, Muted);
        fpsUnits.SizeFlagsVertical = SizeFlags.ShrinkEnd;
        _graph = new FrameGraph { CustomMinimumSize = new Vector2(260, 48), MouseFilter = MouseFilterEnum.Ignore };
        stats.AddChild(_graph);
        Text(stats, "最近 120 帧 · 虚线为 16.7 ms", 12, Muted);
        _pageFpsValue = Metric(stats, "网页动画 FPS");
        _pageFpsValue.Name = "PageFPSValue";
        _sdkLimit = Text(stats, "正在读取网页 SDK 帧率上限…", 12, Muted);
        _sdkLimit.Name = "SDKFrameLimit";
        _frameValue = Metric(stats, "Godot 帧耗时");
        _cpuValue = Metric(stats, "网页绘制 CPU 耗时");
        var upload = Row(stats, 8);
        _uploadCaption = Text(upload, "GPU 资源位图上传", 13, Muted);
        _uploadCaption.SizeFlagsHorizontal = SizeFlags.ExpandFill;
        _uploadValue = Text(upload, "-- MiB/s", 15, Ink);
        Text(stats, "绘制耗时含命令准备，不含 GPU 执行。", 12, Muted);
        var controls = Column(Card(sidebar, new Color("121b29"), 16), 6);
        Text(controls, "测试配置", 18, Ink);
        Text(controls, "渲染后端  ·  G 切换", 13, Muted);
        var backendRow = Row(controls, 8);
        _gpuButton = Action(backendRow, "显卡 GPU", "GPUButton", () => SetBackend(true));
        _cpuButton = Action(backendRow, "软件 CPU", "CPUButton", () => SetBackend(false));
        Text(controls, "动画负载  ·  B 切换", 13, Muted);
        var workloads = Row(controls, 8);
        _svgButton = Action(workloads, "SVG 矢量", "SVGButton", () => SetWorkload(true));
        _canvasButton = Action(workloads, "Canvas", "CanvasButton", () => SetWorkload(false));
        Text(controls, "对象数量  ·  L 切换", 13, Muted);
        var loads = Row(controls, 6);
        foreach (var count in ObjectCounts) {
            var button = Action(loads, count.ToString(), "Load" + count, () => SetObjects(count));
            _loadButtons.Add(button);
        }
        _limitButton = Action(controls, "", "LimitButton", () => {
            LimitFrameRate = !LimitFrameRate;
            ApplyFrameLimit();
            UpdateControls();
        });
        var frameLimitNote = Text(controls, "Ultralight 2.0 免费版最高 60 FPS，无法解锁。\n此按钮仅解除 Godot 帧率限制。", 12, Muted);
        frameLimitNote.Name = "FrameLimitNote";
        frameLimitNote.AutowrapMode = TextServer.AutowrapMode.WordSmart;
        frameLimitNote.CustomMinimumSize = new Vector2(266, 0);
        _description = Text(controls, "", 13, Muted);
        _description.AutowrapMode = TextServer.AutowrapMode.WordSmart;
        _description.CustomMinimumSize = new Vector2(266, 36);
        var footer = Row(page, 12);
        var hints = Text(footer, "G  渲染模式     B  动画类型     L  对象数量     V  帧率限制", 13, Muted);
        hints.SizeFlagsHorizontal = SizeFlags.ExpandFill;
        Text(footer, "Sample 4  ·  本地测试 / 无网络依赖", 13, Muted);
    }

    private static VBoxContainer Column(Node parent, int spacing) {
        var box = new VBoxContainer(); parent.AddChild(box);
        box.AddThemeConstantOverride("separation", spacing); return box;
    }
    private static HBoxContainer Row(Node parent, int spacing) {
        var box = new HBoxContainer(); parent.AddChild(box);
        box.AddThemeConstantOverride("separation", spacing); return box;
    }
    private static Label Text(Node parent, string text, int size, Color color) {
        var label = new Label { Text = text, MouseFilter = MouseFilterEnum.Ignore };
        label.AddThemeFontSizeOverride("font_size", size);
        label.AddThemeColorOverride("font_color", color); parent.AddChild(label); return label;
    }
    private static PanelContainer Card(Node parent, Color color, int padding) {
        var card = new PanelContainer();
        card.AddThemeStyleboxOverride("panel", new StyleBoxFlat {
            BgColor = color, ContentMarginLeft = padding, ContentMarginRight = padding,
            ContentMarginTop = padding, ContentMarginBottom = padding,
            CornerRadiusTopLeft = 12, CornerRadiusTopRight = 12,
            CornerRadiusBottomLeft = 12, CornerRadiusBottomRight = 12
        });
        parent.AddChild(card); return card;
    }
    private static Label Metric(Node parent, string caption) {
        var row = Row(parent, 8);
        var label = Text(row, caption, 13, Muted); label.SizeFlagsHorizontal = SizeFlags.ExpandFill;
        return Text(row, "-- ms", 15, Ink);
    }
    private static Button Action(Node parent, string text, string name, Action action) {
        var button = new Button { Name = name, Text = text, ToggleMode = true, FocusMode = FocusModeEnum.None, SizeFlagsHorizontal = SizeFlags.ExpandFill, CustomMinimumSize = new Vector2(0, 38) };
        foreach (var state in new[] { "normal", "hover", "pressed" }) {
            button.AddThemeStyleboxOverride(state, new StyleBoxFlat {
                BgColor = new Color(state == "pressed" ? "245f50" : state == "hover" ? "28394f" : "1d2a3d"),
                CornerRadiusTopLeft = 7, CornerRadiusTopRight = 7,
                CornerRadiusBottomLeft = 7, CornerRadiusBottomRight = 7,
                ContentMarginLeft = 8, ContentMarginRight = 8
            });
        }
        button.AddThemeColorOverride("font_color", Ink);
        button.AddThemeColorOverride("font_pressed_color", Green);
        parent.AddChild(button); button.Pressed += action; return button;
    }

    private void UpdateControls() {
        _gpuButton.SetPressedNoSignal(UseGpu); _cpuButton.SetPressedNoSignal(!UseGpu);
        _svgButton.SetPressedNoSignal(UseSvg); _canvasButton.SetPressedNoSignal(!UseSvg);
        foreach (var button in _loadButtons) button.SetPressedNoSignal(button.Text == CanvasObjects.ToString());
        _limitButton.SetPressedNoSignal(!LimitFrameRate);
        _limitButton.Text = LimitFrameRate ? "Godot 60 FPS  ·  V 解锁" : "Godot 已解锁  ·  V 限帧";
        _description.Text = UseSvg ? "SVG：曲线路径、描边、变换与透明叠加。" : "Canvas：渐变、阴影、裁剪、曲线与透明混合。";
    }
    private void ResetMeasurements() {
        _elapsed = 0; _cpuUsTotal = 0; _samples = 0; _hasUploadBaseline = false; _hasPageBaseline = false;
        _graph.ClearFrames();
        _fpsValue.Text = "--"; _frameValue.Text = "-- ms"; _cpuValue.Text = "-- ms"; _uploadValue.Text = "-- MiB/s";
        _pageFpsValue.Text = "--";
    }
    private void SetBackend(bool gpu) {
        if (UseGpu != gpu) {
            UseGpu = gpu; ResetMeasurements();
            _status.Text = "● 正在切换渲染后端…";
            _view.Set("accelerated", UseGpu); _view.Call("init_view");
        }
        UpdateControls();
    }
    private void SetWorkload(bool svg) {
        UseSvg = svg; StartAnimation(); ResetMeasurements(); UpdateControls();
    }
    private void SetObjects(int count) {
        CanvasObjects = count; StartAnimation(); ResetMeasurements(); UpdateControls();
    }
    private void ApplyFrameLimit() {
        Engine.MaxFps = LimitFrameRate ? 60 : 0;
        DisplayServer.WindowSetVsyncMode(LimitFrameRate ? DisplayServer.VSyncMode.Enabled : DisplayServer.VSyncMode.Disabled);
        if (_view.HasMethod("set_max_render_fps"))
            _view.Set("max_render_fps", LimitFrameRate ? 60 : 0);
        ResetMeasurements();
    }
    private void StartAnimation() {
        CanvasObjects = Math.Clamp(CanvasObjects, 24, 1440);
        _view.Call("execute_script", $"if (typeof startCanvasBenchmark === 'function') {{ startCanvasBenchmark({CanvasObjects}); setBenchmarkMode('{(UseSvg ? "svg" : "canvas")}'); }}");
    }
    public override void _Input(InputEvent @event) {
        if (@event is not InputEventKey key || !key.Pressed || key.Echo || key.CtrlPressed || key.MetaPressed || key.AltPressed) return;
        var code = key.PhysicalKeycode == Key.None ? key.Keycode : key.PhysicalKeycode;
        switch (code) {
            case Key.G: SetBackend(!UseGpu); break;
            case Key.B: SetWorkload(!UseSvg); break;
            case Key.L: SetObjects(ObjectCounts[(Array.IndexOf(ObjectCounts, CanvasObjects) + 1) % ObjectCounts.Length]); break;
            case Key.V: LimitFrameRate = !LimitFrameRate; ApplyFrameLimit(); UpdateControls(); break;
            default: return;
        }
        GetViewport().SetInputAsHandled();
    }
    private void ForwardInput(InputEvent input) {
        // The preview may scale with the window, while the workload remains 1024 x 768.
        var forwarded = (InputEvent)input.Duplicate();
        if (forwarded is InputEventMouse mouse) {
            var scale = new Vector2(_viewport.Size.X / _preview.Size.X, _viewport.Size.Y / _preview.Size.Y);
            mouse.Position *= scale; mouse.GlobalPosition = mouse.Position;
            if (mouse is InputEventMouseMotion motion) { motion.Relative *= scale; motion.Velocity *= scale; }
        }
        _viewport.PushInput(forwarded, true);
        _preview.AcceptEvent();
    }
}
