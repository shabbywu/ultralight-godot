struct PendingOperation {
    Ref<UltralightTask> task;
    uint64_t generation;
    std::chrono::steady_clock::time_point deadline;
    String selector;
    bool finish_loading = false;
    int kind = 0;
};
std::vector<PendingOperation> operations;
std::map<String, Ref<UltralightAPI>> apis;
uint64_t navigation_id = 0;
bool loading = false, document_ready = false, load_finished = false, host_navigation = false, closed = false;
String current_url;
int64_t source_type = 0;
bool inline_source = false;
String initial_url, initial_html, initial_base_url;

Ref<UltralightError> last_load_error;
std::thread::id main_thread = std::this_thread::get_id();

Ref<UltralightFrameEvent> frameEvent(uint64_t frame, bool main, const ultralight::String &url) {
    Ref<UltralightFrameEvent> e;
    e.instantiate();
    e->navigation_id = navigation_id;
    e->frame_id = frame;
    e->main_frame = main;
    auto text = url.utf8();
    e->url = String::utf8(text.data(), text.length());
    return e;
}
void expireOperations(const String &code, const String &message) {
    auto pending = std::exchange(operations, {});
    for (auto &item : pending)
        item.task->reject(code, message);
}
bool beginNavigation() {
    auto id = ObjectID(get_instance_id());
    auto generation = ++navigation_id;
    loading = true;
    document_ready = false;
    load_finished = false;
    last_load_error.unref();
    auto old = std::exchange(bridge, {});
    auto pending = std::exchange(operations, {});
    if (old)
        old->invalidate();
    for (auto &item : pending)
        item.task->reject("PageGone", "Document replaced");
    return ObjectDB::get_instance(id) == this && navigation_id == generation;
}
Ref<UltralightTask> watch(double timeout, const String &selector = "", bool finished = false, int kind = 0) {
    Ref<UltralightTask> task;
    task.instantiate();
    if (std::this_thread::get_id() != main_thread) {
        task->reject("WrongThread", "Use the Godot main thread");
        return task;
    }
    if (closed) {
        task->reject("Closed", "View closed");
        return task;
    }
    if (!std::isfinite(timeout) || timeout <= 0 || timeout > 1000000000.0) {
        task->reject("InvalidArgument", "Timeout must be finite and positive");
        return task;
    }
    if (kind == 0 && (finished ? load_finished : document_ready)) {
        task->resolve();
        return task;
    }
    if (kind == 1 && document_ready) {
        auto result = evaluate("document.querySelector(" + godot::JSON::stringify(selector) + ") !== null");
        if (!result->is_ok()) {
            task->finish(result);
            return task;
        }
        if (bool(result->value)) {
            task->resolve(true);
            return task;
        }
    }
    operations.push_back({task, navigation_id,
                          std::chrono::steady_clock::now() + std::chrono::milliseconds(int64_t(timeout * 1000)),
                          selector, finished, kind});
    return task;
}

public:
std::shared_ptr<UltralightRenderer::Listeners> inspectorListeners() const {
    return inspector_listeners;
}
ultralight::RefPtr<ultralight::View> inspectorView() const {
    return inspector_view;
}
ultralight::RefPtr<ultralight::View> nativeView() const {
    return view;
}
void set_source_type(int64_t value) {
    if (value == 0 || value == 1)
        source_type = value;
}
int64_t get_source_type() const {
    return source_type;
}
void set_initial_url(const String &value) {
    initial_url = value;
}
String get_initial_url() const {
    return initial_url;
}
void set_initial_html(const String &value) {
    initial_html = value;
}
String get_initial_html() const {
    return initial_html;
}
void set_initial_base_url(const String &value) {
    initial_base_url = value;
}
String get_initial_base_url() const {
    return initial_base_url;
}
Dictionary get_bridge_statistics() const {
    return bridge ? bridge->statistics() : Dictionary();
}
void unbind(const String &name) {
    if (bridge && bridge->active())
        bridge->removeScoped("", name);
}
bool is_initialized() const {
    return view.get() != nullptr;
}
bool is_loading() const {
    return loading;
}
bool is_document_ready() const {
    return document_ready;
}
int64_t get_navigation_id() const {
    return navigation_id;
}
String get_current_url() const {
    return current_url;
}
Ref<UltralightError> get_last_load_error() const {
    return last_load_error;
}
bool is_closed() const {
    return closed;
}
void forgetAPIs() {
    auto attached = std::exchange(apis, {});
    auto id = ObjectID(get_instance_id());
    for (auto &[name, api] : attached)
        api->forget_view(id);
}
void close() {
    closed = true;
    ++navigation_id;
    loading = document_ready = load_finished = false;
    forgetAPIs();
    resetView();
}
Ref<UltralightTask> load_url(const String &url, double timeout = 30.0) {
    if (std::this_thread::get_id() != main_thread)
        return watch(timeout);
    if (closed || !std::isfinite(timeout) || timeout <= 0 || timeout > 1000000000.0)
        return watch(timeout);
    if (!beginNavigation()) {
        Ref<UltralightTask> task;
        task.instantiate();
        task->reject("PageGone", "Navigation superseded or View released");
        return task;
    }
    host_navigation = true;
    inline_source = false;
    html = "";
    htmlUrl = url;
    current_url = url;
    auto task = watch(timeout);
    if (view.get()) {
        auto u = url.utf32();
        view->LoadURL(ultralight::String32(u.get_data(), u.length()));
    } else if (is_inside_tree() && get_size().x > 0 && get_size().y > 0)
        initView();
    return task;
}
String html_base_url;
Ref<UltralightTask> load_html(const String &content, const String &base_url = "", double timeout = 30.0) {
    if (std::this_thread::get_id() != main_thread)
        return watch(timeout);
    if (closed || !std::isfinite(timeout) || timeout <= 0 || timeout > 1000000000.0)
        return watch(timeout);
    if (!beginNavigation()) {
        Ref<UltralightTask> task;
        task.instantiate();
        task->reject("PageGone", "Navigation superseded or View released");
        return task;
    }
    host_navigation = true;
    inline_source = true;
    html = content;
    htmlUrl = "";
    html_base_url = base_url;
    current_url = base_url;
    auto task = watch(timeout);
    if (view.get()) {
        auto h = content.utf32(), u = base_url.utf32();
        view->LoadHTML(ultralight::String32(h.get_data(), h.length()), ultralight::String32(u.get_data(), u.length()));
    } else if (is_inside_tree() && get_size().x > 0 && get_size().y > 0)
        initView();
    return task;
}
Ref<UltralightTask> reload(double timeout = 30.0) {
    String content = html, base = html_base_url, url = current_url.is_empty() ? htmlUrl : current_url;
    if (inline_source)
        return load_html(content, base, timeout);
    return load_url(url, timeout);
}
Ref<UltralightTask> wait_ready(double timeout = 30.0) {
    return watch(timeout);
}
Ref<UltralightTask> wait_for_selector(const String &selector, double timeout = 30.0) {
    return watch(timeout, selector, false, 1);
}
Ref<UltralightResult> evaluate(const String &source) {
    if (std::this_thread::get_id() != main_thread)
        return UltralightResult::failure("WrongThread", "Use the Godot main thread");
    if (closed)
        return UltralightResult::failure("Closed", "View closed");
    if (!view.get())
        return UltralightResult::failure("NotReady", "View has no document");
    try {
        auto lock = view->LockJSContext();
        return UltralightResult::success(currentBridge(lock->ctx())->evaluateShared(source));
    }
    catch (const std::exception &e) {
        return UltralightResult::failure(e);
    }
}
Ref<UltralightResult> evaluate_copy(const String &source) {
    if (std::this_thread::get_id() != main_thread)
        return UltralightResult::failure("WrongThread", "Use the Godot main thread");
    if (!view.get() || closed)
        return UltralightResult::failure(closed ? "Closed" : "NotReady", "View has no document");
    try {
        auto lock = view->LockJSContext();
        return UltralightResult::success(currentBridge(lock->ctx())->evaluateCopy(source));
    }
    catch (const std::exception &e) {
        return UltralightResult::failure(e);
    }
}
Ref<UltralightResult> evaluate_handle(const String &source) {
    if (std::this_thread::get_id() != main_thread)
        return UltralightResult::failure("WrongThread", "Use the Godot main thread");
    if (!view.get() || closed)
        return UltralightResult::failure(closed ? "Closed" : "NotReady", "View has no document");
    try {
        auto lock = view->LockJSContext();
        return UltralightResult::success(currentBridge(lock->ctx())->evaluateHandle(source));
    }
    catch (const std::exception &e) {
        return UltralightResult::failure(e);
    }
}
Ref<UltralightTask> evaluate_async(const String &source, double timeout = 30.0) {
    auto task = watch(timeout, "", false, 2);
    if (task->is_completed())
        return task;
    if (!view.get()) {
        task->reject("NotReady", "View has no document");
        return task;
    }
    try {
        auto lock = view->LockJSContext();
        currentBridge(lock->ctx())->evaluateAsync(source, task);
    }
    catch (const std::exception &e) {
        task->finish(UltralightResult::failure(e));
    }
    return task;
}
Ref<UltralightResult> _attach_api(const Ref<UltralightAPI> &api) {
    if (closed)
        return UltralightResult::failure("Closed", "View closed");
    auto ns = api->get_namespace();
    if (apis.contains(ns))
        return apis[ns] == api ? UltralightResult::success()
                               : UltralightResult::failure("NamespaceConflict", "Namespace already attached");
    apis[ns] = api;
    auto current = bridge;
    auto id = ObjectID(get_instance_id());
    if (current && current->active()) {
        bool namespace_created = false;
        try {
            current->ensureNamespace(ns);
            namespace_created = true;
            auto definitions = api->definitions();
            auto keys = definitions.keys();
            for (int64_t i = 0; i < keys.size(); ++i) {
                auto r = _refresh_api(api, keys[i]);
                if (!r->is_ok())
                    throw std::runtime_error(godot_utf8(r->error->message));
            }
        }
        catch (const std::exception &e) {
            auto failure = namespace_created ? UltralightResult::failure(e)
                                             : UltralightResult::failure("NamespaceConflict", String::utf8(e.what()));
            // Roll back every installed entry, not only the host registry map.
            // Cancellation during cleanup may synchronously close this View.
            if (ObjectDB::get_instance(id) == this)
                apis.erase(ns);
            if (namespace_created && current->active()) {
                try {
                    current->detachNamespace(ns);
                }
                catch (const std::exception &) {
                }
            }
            return failure;
        }
    }
    return UltralightResult::success();
}
Ref<UltralightResult> _refresh_api(const Ref<UltralightAPI> &api, const String &name) {
    if (!bridge || !bridge->active())
        return UltralightResult::success();
    try {
        auto definitions = api->definitions();
        auto ns = api->get_namespace();
        if (!definitions.has(name))
            bridge->removeScoped(ns, name);
        else {
            Dictionary entry = definitions[name];
            bridge->bindScoped(ns, name, entry["value"], entry["async"], entry["members"], entry["copy"],
                               entry["read_only"]);
        }
        return UltralightResult::success();
    }
    catch (const std::exception &e) {
        return UltralightResult::failure(e);
    }
}
void _detach_api(const Ref<UltralightAPI> &api) {
    auto ns = api->get_namespace();
    if (!apis.contains(ns) || apis[ns] != api)
        return;
    auto previous = bridge;
    apis.erase(ns);
    if (previous && previous->active()) {
        try {
            previous->detachNamespace(ns);
        }
        catch (const std::exception &) {
        }
    }
}
Ref<UltralightBinding> bind_function(const String &name, const Callable &function,
                                     const Ref<UltralightBindingOptions> &options = {}) {
    if (!view.get() || closed)
        return {};
    try {
        auto lock = view->LockJSContext();
        return currentBridge(lock->ctx())->bindTemporary(name, function, options);
    }
    catch (const std::exception &e) {
        UtilityFunctions::push_error(String::utf8(e.what()));
        return {};
    }
}
Ref<UltralightBinding> bind_object(const String &name, Object *object,
                                   const Ref<UltralightBindingOptions> &options = {}) {
    if (!view.get() || closed)
        return {};
    try {
        auto lock = view->LockJSContext();
        return currentBridge(lock->ctx())->bindTemporary(name, object, options);
    }
    catch (const std::exception &e) {
        UtilityFunctions::push_error(String::utf8(e.what()));
        return {};
    }
}
void installAPIs() {
    auto snapshot = apis;
    for (auto &[ns, api] : snapshot) {
        bridge->ensureNamespace(ns);
        auto definitions = api->definitions();
        auto keys = definitions.keys();
        for (int64_t i = 0; i < keys.size(); ++i) {
            Dictionary entry = definitions[keys[i]];
            bridge->bindScoped(ns, keys[i], entry["value"], entry["async"], entry["members"], entry["copy"],
                               entry["read_only"]);
        }
    }
}
void input_text(const String &text) {
    if (!view.get())
        return;
    for (int64_t i = 0; i < text.length(); ++i) {
        KeyEvent event;
        event.type = KeyEvent::kType_Char;
        auto character = String::chr(text[i]).utf32();
        event.text = ultralight::String32(character.get_data(), character.length());
        view->FireKeyEvent(event);
    }
}
void scroll_pixels(const Vector2 &delta) {
    if (!view.get())
        return;
    ScrollEvent event;
    event.type = ScrollEvent::kType_ScrollByPixel;
    event.delta_x = delta.x;
    event.delta_y = delta.y;
    view->FireScrollEvent(event);
}
void syncFocus() {
    if (view.get()) {
        if (has_focus())
            view->Focus();
        else
            view->Unfocus();
    }
}
virtual void _process(double) override {
    if (closed)
        return;
    auto id = ObjectID(get_instance_id());
    if (!view.get() && (!lazy || !operations.empty()) && get_size().x > 0 && get_size().y > 0)
        initView();
    if (ObjectDB::get_instance(id) != this || closed)
        return;
    auto pending = std::exchange(operations, {});
    for (auto &item : pending) {
        if (ObjectDB::get_instance(id) != this) {
            item.task->reject("Closed", "View closed");
            continue;
        }
        if (item.task->is_completed())
            continue;
        if (closed)
            item.task->reject("Closed", "View closed");
        else if (item.generation != navigation_id)
            item.task->reject("PageGone", "Document replaced");
        else if (last_load_error.is_valid())
            item.task->reject(last_load_error->code, last_load_error->message);
        else if (std::chrono::steady_clock::now() >= item.deadline)
            item.task->reject("Timeout", "Operation timed out");
        else if (item.kind == 2)
            operations.push_back(std::move(item));
        else if (item.kind == 1 && document_ready) {
            auto result = evaluate("document.querySelector(" + godot::JSON::stringify(item.selector) + ") !== null");
            if (!result->is_ok())
                item.task->finish(result);
            else if (bool(result->value))
                item.task->resolve(true);
            else
                operations.push_back(std::move(item));
        } else if (item.kind == 0 && (item.finish_loading ? load_finished : document_ready))
            item.task->resolve();
        else
            operations.push_back(std::move(item));
    }
}
