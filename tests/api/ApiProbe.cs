using Godot;
using System;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;
using Ultralight;

public partial class ApiProbe : Node
{
    [Signal] public delegate void FinishedEventHandler(bool success);
    public sealed class Model
    {
        public string Label { get; set; } = "hello";
        public List<int> Items { get; } = new() { 1, 2 };
        public Dictionary<string, object> Options { get; } = new() { ["enabled"] = true };
        public int GetterReads { get; private set; }
        public string Expensive { get { GetterReads++; return "value"; } }
        public async Task<int> IncrementAsync(int value, CancellationToken token)
        {
            await Task.Delay(10, token).ConfigureAwait(false);
            return value + 1;
        }
        public ValueTask<int> ImmediateAsync(int value) => ValueTask.FromResult(value + 2);
        public Task<DateTime> UnsupportedAsync() => Task.FromResult(DateTime.UtcNow);
        public string Fail() => throw new InvalidOperationException("managed failure");
    }
    private static void Check(bool value, string message) { if (!value) throw new Exception(message); }
    public async void Run()
    {
        Control? native = null;
        try
        {
            native = ClassDB.Instantiate("UltralightView").AsGodotObject() as Control ?? throw new Exception("View construction");
            native.Size = new Vector2(640, 480); AddChild(native);
            var view = UltralightViewClient.Wrap(native);
            using var api = new UltralightApi("app");
            var model = new Model();
            api.RegisterObject("model", model);
            api.RegisterAsync("sum", async (args, token) =>
            {
                int sum = args[0].AsInt32() + args[1].AsInt32();
                await Task.Delay(10, token).ConfigureAwait(false);
                return sum;
            });
            api.RegisterValueTask("fast", (args, token) => ValueTask.FromResult<Variant>(99));
            bool pinged = false;
            api.RegisterAsync("ping", async (args, token) => { await Task.Delay(1, token); pinged = true; });
            api.RegisterValueTask("noop", (args, token) => ValueTask.CompletedTask);
            api.Attach(view);
            await view.LoadHtmlAsync("<script>globalThis.first=app.model.Label</script>");
            Check(await view.EvaluateAsync<string>("first") == "hello", "first script access");
            using (var alreadyCancelled = new CancellationTokenSource())
            {
                alreadyCancelled.Cancel();
                long generation = native.Get("navigation_id").AsInt64();
                try { await view.EvaluateAsync("globalThis.cancelledRan=true", alreadyCancelled.Token); throw new Exception("pre-cancelled evaluation succeeded"); }
                catch (OperationCanceledException) { }
                try { await view.LoadHtmlAsync("<h1>cancelled</h1>", cancellationToken: alreadyCancelled.Token); throw new Exception("pre-cancelled navigation succeeded"); }
                catch (OperationCanceledException) { }
                Check(native.Get("navigation_id").AsInt64() == generation, "pre-cancelled load does not navigate");
                Check(view.Evaluate("typeof cancelledRan==='undefined'").AsBool(), "pre-cancelled evaluation has no side effects");
            }
            Check(await view.EvaluateAsync<int>("app.sum(3,4)") == 7, "managed async to JS");
            Check(await view.EvaluateAsync<int>("app.fast()") == 99, "managed ValueTask");
            await view.EvaluateAsync("app.ping()");
            Check(pinged, "non-generic Task handler");
            await view.EvaluateAsync("app.noop()");
            Check(await view.EvaluateAsync<int>("app.model.IncrementAsync(8)") == 9, "object Task method");
            Check(await view.EvaluateAsync<int>("app.model.ImmediateAsync(8)") == 10, "object ValueTask method");
            Check(await view.EvaluateAsync<bool>("app.model.UnsupportedAsync().then(()=>false,e=>String(e).includes('Unsupported'))"), "unsupported async result rejects instead of hanging");
            view.Evaluate("app.model.Items.push(3);app.model.Items[0]=12;app.model.Options.enabled=false");
            Check(model.Items.Count == 3 && model.Items[0] == 12, "managed list shared writes");
            Check((bool)model.Options["enabled"] == false, "managed dictionary writes");
            model.Items[1] = 55;
            Check(view.Evaluate("app.model.Items[1]").AsInt32() == 55, "managed source writes visible");
            Check(view.Evaluate("app.model.Items===app.model.Items").AsBool(), "managed list identity");
            view.Evaluate("app.model.Items.pop();app.model.Items.splice(0,1)");
            Check(model.Items.Count == 1 && model.Items[0] == 55, "managed pop and splice removals");
            view.Evaluate("Object.keys(app.model)");
            Check(model.GetterReads == 0, "enumeration must not invoke getters");
            Check(view.Evaluate("(()=>{try{app.model.Fail();return false}catch(e){return String(e).includes('managed failure')}})()").AsBool(), "managed exception becomes JS Error");
            using (var items = new UltralightJsArray(view.Evaluate("globalThis.live=[1,2];live").AsGodotObject()))
            {
                items.Add(3); items.Insert(1, 7); items.RemoveAt(0);
                Check(view.Evaluate("live.join(',')").AsString() == "7,2,3", "JS IList adapter");
                var snapshot = items.Snapshot(); items[0] = 9;
                Check(snapshot[0].AsInt32() == 7, "JS IList snapshot");
            }
            using (var options = new UltralightJsObject(view.Evaluate("globalThis.options={a:1};options").AsGodotObject()))
            {
                options["b"] = 2; options.Remove("a");
                Check(view.Evaluate("options.b===2 && !('a' in options)").AsBool(), "JS IDictionary adapter");
            }
            using var cancellation = new CancellationTokenSource();
            var pending = view.EvaluateAsync("new Promise(()=>{})", cancellation.Token);
            cancellation.Cancel();
            try { await pending; throw new Exception("cancellation did not propagate"); } catch (OperationCanceledException) { }
            var stale = view.EvaluateAsync("new Promise(()=>{})");
            await view.LoadHtmlAsync("<script>globalThis.restored=app.model.Label</script>");
            try { await stale; throw new Exception("stale await completed"); } catch (UltralightException e) when (e.Code == "PageGone") { }
            Check(view.Evaluate("restored").AsString() == "hello", "registry restored");
            GD.Print("MANAGED_API PASS Task ValueTask managed objects collections cancellation");
            EmitSignal(SignalName.Finished, true);
        }
        catch (Exception error)
        {
            GD.PushError(error.ToString());
            EmitSignal(SignalName.Finished, false);
        }
        finally { native?.QueueFree(); }
    }
}
