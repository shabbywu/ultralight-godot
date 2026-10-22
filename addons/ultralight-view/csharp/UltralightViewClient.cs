#nullable enable
using Godot;
using System;
using System.Threading;
using System.Threading.Tasks;

namespace Ultralight;

public sealed class UltralightException : Exception
{
    public string Code { get; }
    public string JavaScriptStack { get; }
    public string SourceUrl { get; }
    internal UltralightException(GodotObject error) : base(error.Get("message").AsString())
    {
        Code = error.Get("code").AsString();
        JavaScriptStack = error.Get("stack").AsString();
        SourceUrl = error.Get("source_url").AsString();
    }
}

/// <summary>
/// Main-thread facade for an UltralightView GDExtension node. Native results
/// become values or exceptions; cancellation requests may originate on any thread.
/// </summary>
public sealed class UltralightViewClient
{
    public Control Native { get; }
    private UltralightViewClient(Control view)
    {
        if (!view.IsClass("UltralightView")) throw new ArgumentException("Expected UltralightView", nameof(view));
        Native = view;
        UltralightDispatcher.Install();
    }
    public static UltralightViewClient Wrap(Control view) => new(view);
    internal static Variant Unwrap(GodotObject result)
    {
        if (!result.Get("ok").AsBool())
        {
            var error = result.Get("error").AsGodotObject();
            string code = error.Get("code").AsString();
            if (code == "Cancelled") throw new OperationCanceledException();
            if (code == "Timeout") throw new TimeoutException(error.Get("message").AsString());
            throw new UltralightException(error);
        }
        return result.Get("value");
    }
    public void Rebuild() => Native.Call("rebuild");
    public Variant Evaluate(string source) => Unwrap(Native.Call("evaluate", source).AsGodotObject());
    public Variant EvaluateCopy(string source) => Unwrap(Native.Call("evaluate_copy", source).AsGodotObject());
    public Task<Variant> EvaluateAsync(string source, CancellationToken cancellationToken = default, double timeout = 30)
        => Start("evaluate_async", cancellationToken, source, timeout);
    public async Task<T> EvaluateAsync<T>(string source, CancellationToken cancellationToken = default, double timeout = 30)
        => (T)ManagedConversion.FromVariant(await EvaluateAsync(source, cancellationToken, timeout), typeof(T))!;
    public async Task<UltralightJsArray> EvaluateArrayAsync(string source, CancellationToken cancellationToken = default)
        => new((await EvaluateAsync(source, cancellationToken)).AsGodotObject());
    public async Task<UltralightJsObject> EvaluateObjectAsync(string source, CancellationToken cancellationToken = default)
        => new((await EvaluateAsync(source, cancellationToken)).AsGodotObject());
    public Task<Variant> LoadUrlAsync(string url, CancellationToken cancellationToken = default, double timeout = 30)
        => Start("load_url", cancellationToken, url, timeout);
    public Task<Variant> LoadHtmlAsync(string html, string baseUrl = "", CancellationToken cancellationToken = default, double timeout = 30)
        => Start("load_html", cancellationToken, html, baseUrl, timeout);
    public Task<Variant> WaitForSelectorAsync(string selector, CancellationToken cancellationToken = default, double timeout = 30)
        => Start("wait_for_selector", cancellationToken, selector, timeout);
    private Task<Variant> Start(string method, CancellationToken token, params Variant[] arguments)
    {
        // An already-cancelled request must not execute JS or replace a page.
        if (token.IsCancellationRequested) return Task.FromCanceled<Variant>(token);
        return Await(Native.Call(method, arguments).AsGodotObject(), token);
    }
    internal static async Task<Variant> Await(GodotObject operation, CancellationToken cancellationToken)
    {
        var completion = new TaskCompletionSource<Variant>(TaskCreationOptions.RunContinuationsAsynchronously);
        var callback = Callable.From(() =>
        {
            try { completion.TrySetResult(Unwrap(operation.Call("get_result").AsGodotObject())); }
            catch (OperationCanceledException) { completion.TrySetCanceled(); }
            catch (Exception error) { completion.TrySetException(error); }
        });
        operation.Connect("completed", callback);
        // Cancellation can originate on any thread. Native calls and signal
        // disconnection always return to the Godot dispatcher.
        using var registration = cancellationToken.Register(() => UltralightDispatcher.Post(() =>
        {
            if (GodotObject.IsInstanceValid(operation)) operation.Call("cancel");
        }));
        // Connecting before checking also covers operations completed inline.
        if (operation.Call("is_completed").AsBool()) callback.Call();
        try { return await completion.Task; }
        finally
        {
            UltralightDispatcher.Post(() =>
            {
                if (GodotObject.IsInstanceValid(operation) && operation.IsConnected("completed", callback))
                    operation.Disconnect("completed", callback);
            });
        }
    }
}
