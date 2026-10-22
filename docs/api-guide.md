# 页面、API 注册表与跨语言数据

普通 Godot 项目使用原生扩展；C# 项目额外编译安装包内的 `addons/ultralight-view/csharp/` 源码。无需 AppCore 或单独驱动循环。

## GDScript：注册一次，刷新后继续可用

```gdscript
extends Control

@onready var view: UltralightView = $WebView
var api := UltralightAPI.create("game")
var inventory := {"items": []}

func _ready():
    api.register_function("save", func(): return "saved")
    api.register_object("inventory", inventory)
    var attached = api.attach(view)
    if not attached.ok:
        push_error(attached.error.message)
        return
    var loading = view.load_html("<button onclick='this.textContent=game.save()'>保存</button>")
    if not loading.is_completed():
        await loading.completed
    var result = loading.get_result()
    if not result.ok:
        push_error(result.error.message)

func _exit_tree():
    api.close()
```

```javascript
await game.save();
game.inventory.items.push(item);
```

`namespace` 由调用方提供，必须是一个 ASCII JS 标识符。不解析 `app.game` 这样的点分路径，也不默认导出 `window.godot` 或整个 View。
`UltralightAPI.create()` 对无效名称返回 null；注册、附着操作返回 `UltralightResult`。
注册调用也应检查返回结果，失败时读取 error 并处理后再继续。上例需要场景中存在名为 WebView、尺寸有效的 UltralightView 节点。
同一注册表可以附着多个 View，一个 View 可以使用多个不同 namespace；附着和分离幂等。
已有全局名称或其他注册表占用相同 namespace 时报告冲突。

新文档会先安装当前全部注册项，再发 `window_ready`，然后执行页面脚本。刷新不需要应用重绑。
`unregister(name)` 撤销注册项；`detach(view)` 撤销该 View 的 namespace；`close()` 撤销全部附着并清除注册定义。
这些操作不会关闭 View。JS 已缓存的对象、函数和嵌套容器同样失效。

临时导出使用 `view.bind_function(name, callable, options)` / `view.bind_object(name, object, options)`，返回 `UltralightBinding`。
临时 bind_object 接收 Godot Object；导出根 Array／Dictionary 使用注册表的 register_object。
丢弃句柄不会自动解绑；使用 `binding.close()` 或 `view.unbind(name)`。关闭旧句柄不会误删后来替换的绑定。

## 加载、信号和状态

运行时使用 `load_url(url, timeout=30)`、`load_html(html, base_url="", timeout=30)`、`reload(timeout=30)`。
加载方法返回任务，默认等待主文档 DOM 就绪。内联 HTML 的 base_url 为相对 CSS、脚本和图片提供基址。
进入场景后有有效尺寸即自动初始化；容器尚未布局时，显式加载请求会等待有效尺寸。

编辑器初始配置使用 `source_type`（0：URL；1：HTML）、`initial_url`、`initial_html`、`initial_base_url`。
初始配置与运行时实际 URL 分开；读取 `current_url` 查看当前页面，`navigation_id` 区分不同主文档。
`document_ready`、`is_loading()`、`is_initialized()` 和 `get_last_load_error()` 可用于同步查询。
`rebuild()` 用于重新应用渲染后端配置，会销毁旧文档及其句柄；普通加载不需要调用它。

| 信号 | 含义 |
| --- | --- |
| `load_started(event)` | 主文档开始加载 |
| `window_ready(event)` | 主文档全局对象可用，注册表已安装 |
| `dom_ready(event)` | 主文档 DOM 就绪 |
| `frame_dom_ready(event)` | 任意框架 DOM 就绪，包含主框架 |
| `load_finished(event)` | 主文档加载完成，不代表业务异步工作完成 |
| `load_failed(failure)` | 主文档失败；字典包含 event、error、SDK domain/code |
| `console_message(message)` | 等级、消息、source_url、line、column、navigation_id |

`UltralightFrameEvent` 提供 navigation_id、frame_id、is_main_frame、url，字段只读。
主文档 DOM 回调先更新状态，再发 dom_ready；文档仍有效时再发 frame_dom_ready。监听两者通常会收到两次主框架通知。

`wait_ready(timeout=30)` 和 `wait_for_selector(selector, timeout=30)` 返回任务。选择器只判断元素存在。
已满足条件可以同步完成，统一按“检查状态，再 await completed”使用。无效选择器返回脚本错误。
等待、Promise 和任务都不另开循环；加载、更新及渲染由 Godot 帧循环推进。

## 结果、错误与任务

```gdscript
var result = $WebView.evaluate("document.title")
if result.ok:
    print(result.value)
else:
    print(result.error.code, result.error.message, result.error.stack)

var task = $WebView.evaluate_async("fetchData()", 10.0)
if not task.is_completed():
    await task.completed
var outcome = task.get_result()
```

同步求值返回 `UltralightResult`，普通 JS null 是成功的 Nil，JS undefined 使用 `UltralightUndefined`。
JS Promise 通过 `evaluate_async()` 等待，普通返回值也会完成同一种任务。
`UltralightError` 保存 code、message，以及可获得的 stack、source_url、line、column。
任务只完成一次，完成后继续保留结果。`cancel()` 仅取消本次观察/投递，不终止 JS Promise 或撤销已经发生的业务操作。
超时单位统一为秒，须有限、大于零且不超过 1,000,000,000。导航和重建使旧任务以 PageGone 结束，关闭使任务结束；文档失效后旧 JS 句柄不能再使用。

GDScript 向 JS 提供异步业务时，显式完成请求对象：

```gdscript
api.register_async("doubleLater", func(request, arguments):
    get_tree().create_timer(0.1).timeout.connect(func():
        if not request.is_completed():
            request.resolve(arguments[0] * 2)))
```

页面调用 `await game.doubleLater(21)` 得到 42。失败时使用 `request.reject("HostException", "说明")`；不要依赖 Callable 自动识别 GDScript 协程。

## C# async/await

```csharp
using Godot;
using System.Threading.Tasks;
using Ultralight;

// 在 Godot 主线程执行，例如 Control._Ready() 内；api 应保存为所有者字段。
var view = UltralightViewClient.Wrap(GetNode<Control>("WebView"));
var api = new UltralightApi("game");
api.RegisterAsync("addLater", async (args, cancellationToken) =>
{
    int total = args[0].AsInt32() + args[1].AsInt32();
    await Task.Delay(10, cancellationToken);
    return total;
});
api.Attach(view);

await view.LoadHtmlAsync("<script>globalThis.total = game.addLater(2, 3)</script>");
int total = await view.EvaluateAsync<int>("total");
```

注册同步函数使用 `Register(name, Callable)`；注册 Task/Task<T> 使用 `RegisterAsync`，ValueTask/ValueTask<T> 使用 `RegisterValueTask`。
JS 调用异步项得到 Promise。`RegisterObject()` 也会适配普通 C# 实例的公开属性、字段、同步/异步方法，无需 `[Export]`。
托管对象身份使用弱键缓存；已撤销且不再被应用或其他绑定持有的对象可以回收。
当前方法重载按参数数量匹配，有歧义时报告错误；需要 CancellationToken 的方法可在最后声明该参数。

C# 封装将原生结果映射成返回值或异常：脚本/加载错误使用 `UltralightException`，保留 Code、JavaScriptStack 和 SourceUrl；取消与超时使用相应 .NET 异常。
注册表实现 IDisposable；所有者退出时 Dispose，View 只保留附着所需的原生注册定义。
例如在 `_ExitTree()` 中调用保存的注册表字段的 `Dispose()`。入口使用 `async void _Ready()` 时，应在方法内部捕获加载／脚本异常，避免把异常交给未观察的异步调用。

后台任务完成只进入线程安全队列，数据转换和 JS 投递回到 Godot 主线程。导航、解绑和关闭会撤销旧投递，并取消提供给处理器的 CancellationToken。
取消为协作请求，不强行取消外部任务。不要在主线程用 Wait/Result 同步等待异步操作，也不要从后台线程访问 Godot 或 JS 句柄。
传入已取消的 CancellationToken 时，封装直接返回已取消的 Task，不执行 JS 或发起导航；操作已经发起后的取消不撤销既有副作用。

需要直接使用 Godot 对象已有的原生 API 时使用 `RegisterNativeObject()`。
旧后端方法接收具体 Godot Array/Dictionary 参数时，可以显式设置 `copyContainers: true`；背包样例演示了此用法。

## 双向共享容器

新 API 默认按引用共享可变容器，首次导出只创建访问到的代理；嵌套数据按需包装。

| 数据 | 对端表现 |
| --- | --- |
| Godot Array | JS Array Proxy，支持索引、length、push/pop/splice/map/迭代 |
| Godot Dictionary | JS 对象 Proxy，字符串键参与属性访问与枚举 |
| C# List / 字符串键 Dictionary | 相应实时代理；遵守源集合的类型、可写性及固定尺寸约束 |
| JS Array | GDScript 中的 UltralightJSArray；C# 可包装成 UltralightJsArray（IList<Variant>） |
| JS 普通对象 | UltralightJSObject；C# 可包装成 UltralightJsObject（IDictionary<string, Variant>） |

Godot/C# 的修改在 JS 下次读取时可见，JS 修改写回源数据。同一绑定、同一文档内保持容器身份，支持别名及循环引用。
不同绑定独立撤销；这不是响应式通知系统，UI 仍通过应用自己的事件或更新流程刷新。

```gdscript
var result = $WebView.evaluate("globalThis.items=[1,2];items")
var items = result.value
items.append(3)
print(items.get_value(0).value)
var snapshot = items.snapshot() # Result，value 为独立 Array
```

```csharp
using var items = await view.EvaluateArrayAsync("globalThis.items=[1,2];items");
items.Add(3);
items.Insert(0, 9);
var snapshot = items.Snapshot();
```

JS 句柄提供 get_value/set_value、keys、size、append、remove、invoke/invoke_method、snapshot 和 close/is_closed。
`size()` 失败返回 -1，0 表示有效的空集合。`append()` 只接受数组；`remove()` 遇到不可配置的属性返回失败结果。
`remove` 遵循 JS delete 语义；IList.RemoveAt 使用 splice 移位。句柄关闭会影响共享同一句柄的别名。
函数方法调用需要接收对象时使用 invoke_method，避免丢失 this。
JS 函数也可通过 `evaluate()` 转成 Godot Callable，其返回值使用共享转换；需要明确的 Result 错误时，使用 `evaluate_handle()` 保留函数再调用 `invoke()`。旧 execute_script 返回的 Callable 继续使用按值转换。

Godot Array 始终稠密：扩大长度或跳跃赋值用 Undefined 填充，delete 保留位置，splice 删除并移位。
typed Array、不支持空位的托管集合、固定尺寸数组和只读数据会拒绝不符合源约束的操作。
容器不支持冻结、修改原型或重定义属性描述符；未知对象类型不会自动变成空 Dictionary。

`UltralightBindingOptions` 可设置：

- `members`：根对象成员名单，枚举不调用 getter；嵌套对象按自身公开成员规则导出。
- `read_only`：阻止属性和嵌套容器写入，不推断方法本身是否有副作用。
- `copy_containers`：该注册项的容器使用按值转换，适合已有的类型化 Godot 后端边界。

namespace 内保留 `$bridge`，不能用作注册项名称。
`game.$bridge.copy(value)` 创建独立的 JS 容器快照；`game.$bridge.get/set(dictionary, key, ...)` 保留 Godot Dictionary 的键类型，避免把数字键强制转换为字符串。
普通属性枚举只覆盖 String/StringName 键，源字典的其他键仍保留。CLR Dictionary 首版要求字符串键。

快照使用 `evaluate_copy()`、JS 句柄 snapshot 或 JS 中的 namespace.$bridge.copy。
只复制自有可枚举属性，循环或超过 64 层的数据会报错。共享代理支持循环不意味着 JSON 或快照支持循环。
重复全量计算时先复制所需子树可能更快；共享模式每次访问都需要跨语言调用。
性能脚本为 `tests/api/containers.gd`，分别测量构造、首次暴露、少量读取、全量遍历和快照。

## 剪贴板、光标、调试

文本剪贴板通过 Godot DisplayServer 接入 SDK；CSS cursor 映射为 Control 光标，Control 焦点同步到页面。
`input_text(text)` 发送已经提交的文本，`scroll_pixels(delta)` 发送像素滚动；3D 转发坐标仍使用 View 本地像素。
已提交文本不代表完整 IME 预编辑和候选框定位支持。

在场景中添加 `UltralightInspector`，设置 `target_view` 即可打开内嵌 Inspector。页面资源来自匹配 SDK，随扩展内嵌，无需拷贝 Inspector HTML/CSS/JS。
Inspector 随目标重建或释放更新关联。可复用 [分屏调试场景](../samples/inspector.tscn)。远程调试入口为 `start_remote_inspector_server(address, port)`。

## 迁移和支持边界

- 使用新信号 window_ready/dom_ready/frame_dom_ready；旧 on_* 信号仅作为过渡入口。
- 使用注册表取代每次 window ready 时 bind；临时绑定返回可撤销句柄。
- 使用显式加载和 evaluate/evaluate_async/evaluate_copy；旧 execute_script 保留原来的按值、Nil/日志行为，用于迁移现有项目。
- `bind_object` 的第三参数现在是 BindingOptions。旧布尔成员查询入口临时保留为 bind_object_legacy，用于既有回归场景。
- 默认共享后，接收具体 Array/Dictionary 的旧方法应显式选择 copy_containers，或改用共享句柄接口。
- Godot String 自身会截断内嵌 NUL；此限制不会因改用共享容器自动消失。需要保留原始 JS 字符串时使用保留句柄及其 `utf8_bytes()` 接口。
- 当前不支持 Session、完整 IME、编辑器 Dock、任意 CLR 值类型和 JS Map/Set/Date 等特殊对象适配。

中文类参考见本地生成站点的 `zh-CN/api/`。API 名称、签名和默认参数来自 Godot XML；中文说明与搜索摘要由 `site/locales/api.zh-CN.json` 提供，构建时会检查条目是否齐全。
