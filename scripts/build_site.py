"""Build the localized showcase and API reference using the standard library."""

from __future__ import annotations

import json
import re
import shutil
import textwrap
from dataclasses import dataclass
from html import escape
from pathlib import Path
from urllib.parse import quote
from xml.etree import ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "site"
OUTPUT = ROOT / "dist"
LOCALES = ("zh-CN", "en")
REPOSITORY = "https://github.com/shabbywu/ultralight-godot"
TOKEN = re.compile(r"\{\{([A-Za-z_]+)\}\}")
INTERNAL = {"on_update_frame", "on_resized", "update_frame"}


@dataclass(frozen=True)
class Project:
    """Localized sample metadata and its recorded media references."""

    id: str
    title: str
    sample: str
    kind: dict[str, str]
    description: dict[str, str]
    features: dict[str, list[str]]
    animation: str
    poster: str
    api: str
    width: int
    height: int


@dataclass(frozen=True)
class ApiClass:
    """A Godot class documentation source."""

    name: str
    inherits: str
    node: ET.Element


CLASSES = tuple(
    ApiClass(node.attrib["name"], node.attrib["inherits"], node)
    for node in (
        ET.parse(ROOT / "ultralight-view/doc_classes" / name).getroot()
        for name in ("UltralightView.xml", "UltralightSingleton.xml")
    )
)


def e(value: object) -> str:
    """Escape text and attribute values for HTML."""
    return escape(str(value), quote=True)


def text(node: ET.Element | None) -> str:
    """Read and dedent the text of an XML element."""
    return textwrap.dedent("".join(node.itertext())).strip() if node is not None else ""


def render(template: str, messages: dict[str, str], **raw: str) -> str:
    """Substitute escaped translations and explicitly rendered HTML fragments."""
    values = {key: e(value) for key, value in messages.items()} | raw
    return TOKEN.sub(lambda match: values[match[1]], template)


def json_script(value: object) -> str:
    """Serialize embedded JSON without allowing an HTML end tag."""
    return json.dumps(value, ensure_ascii=False).replace("<", "\\u003c")


def type_link(name: str) -> str:
    """Link plugin types locally and Godot types to their official reference."""
    if name in {cls.name for cls in CLASSES}:
        return f'<a href="{e(name)}.html">{e(name)}</a>'
    if name == "void":
        return "void"
    target = name.lower()
    url = f"https://docs.godotengine.org/en/stable/classes/class_{target}.html"
    return f'<a href="{url}">{e(name)}</a>'


def inline(value: str, class_name: str) -> str:
    """Render inline Godot documentation markup as safe HTML."""
    chunks = re.split(r"(\[code\].*?\[/code\])", value, flags=re.S)
    result: list[str] = []
    for chunk in chunks:
        if chunk.startswith("[code]"):
            result.append(f"<code>{e(chunk[6:-7])}</code>")
            continue
        chunk = e(chunk)
        for tag, html in (("b", "strong"), ("i", "em")):
            chunk = chunk.replace(f"[{tag}]", f"<{html}>").replace(
                f"[/{tag}]", f"</{html}>"
            )
        chunk = re.sub(r"\[param ([\w]+)\]", r"<code>\1</code>", chunk)

        def reference(match: re.Match[str]) -> str:
            kind, target = match.groups()
            owner, _, member = target.rpartition(".")
            owner = owner or class_name
            member = member or target
            section = {"method": "method", "member": "property", "signal": "signal"}[
                kind
            ]
            return f'<a href="{e(owner)}.html#{section}-{e(member)}"><code>{e(member)}</code></a>'

        chunk = re.sub(r"\[(method|member|signal) ([\w.]+)\]", reference, chunk)
        chunk = re.sub(
            r"\[([A-Z][\w]*|bool|int|float)\]",
            lambda match: f"<code>{type_link(match[1])}</code>",
            chunk,
        )
        result.append(chunk)
    return "".join(result)


def documentation(value: str, class_name: str) -> str:
    """Render paragraphs, lists and code blocks from the Godot XML descriptions."""
    parts = re.split(r"(\[codeblock\].*?\[/codeblock\])", value, flags=re.S)
    output: list[str] = []
    for part in parts:
        if part.startswith("[codeblock]"):
            output.append(code_block(part[11:-12].strip(), "GDScript"))
            continue
        for paragraph in re.split(r"\n\s*\n", part.strip()):
            if not paragraph:
                continue
            lines = paragraph.splitlines()
            first_item = next(
                (i for i, line in enumerate(lines) if line.strip().startswith("- ")),
                len(lines),
            )
            if first_item:
                output.append(
                    f"<p>{inline(' '.join(lines[:first_item]), class_name)}</p>"
                )
            if first_item < len(lines):
                output.append("<ul>")
                for line in lines[first_item:]:
                    output.append(
                        f"<li>{inline(line.strip().removeprefix('- '), class_name)}</li>"
                    )
                output.append("</ul>")
    return "".join(output)


def code_block(code: str, language: str = "") -> str:
    """Render a selectable code sample with a progressive copy control."""
    return (
        f'<div class="code-block"><div class="code-caption"><span>{e(language)}</span>'
        '<button class="copy-code text-button" type="button" hidden>Copy</button></div>'
        f'<pre><code>{e(code)}</code></pre><span class="copy-result" role="status"></span></div>'
    )


def connect_code() -> str:
    """Provide equivalent scene setup examples for both scripting languages."""
    gdscript = """extends Control

@onready var view = $UltralightView

func _ready():
    view.on_window_object_ready.connect(func():
        view.bind_func("greet", func(name: String):
            return "Hello, " + name))
    view.htmlUrl = "file:///ui/index.html"
    await get_tree().process_frame
    view.init_view()"""
    csharp = """using Godot;

public partial class WebUI : Control
{
    public override async void _Ready()
    {
        var view = GetNode<Control>("UltralightView");
        view.Connect("on_window_object_ready", Callable.From(() =>
            view.Call("bind_func", "greet",
                Callable.From((string name) => "Hello, " + name))));
        view.Set("htmlUrl", "file:///ui/index.html");
        await ToSignal(GetTree(), SceneTree.SignalName.ProcessFrame);
        view.Call("init_view");
    }
}"""
    return (
        '<div data-code-tabs><div class="code-tabs" role="tablist" hidden>'
        '<button type="button" role="tab" aria-selected="true" aria-controls="gdscript-code" id="gdscript-tab">GDScript</button>'
        '<button type="button" role="tab" aria-selected="false" aria-controls="csharp-code" id="csharp-tab" tabindex="-1">C#</button></div>'
        f'<div id="gdscript-code" role="tabpanel" aria-labelledby="gdscript-tab">{code_block(gdscript, "GDScript")}</div>'
        f'<div id="csharp-code" role="tabpanel" aria-labelledby="csharp-tab">{code_block(csharp, "C#")}</div></div>'
    )


def sample_card(
    project: Project, locale: str, prefix: str, t: dict[str, str], index: int
) -> str:
    """Render an accessible animation and still preview with source links."""
    poster = e(prefix + project.poster)
    animation = e(prefix + "assets/media/" + project.animation)
    title = e(project.title)
    alt = e(project.description[locale])
    source = REPOSITORY + "/tree/main/samples/" + quote(project.sample, safe="")
    tags = "".join(f"<li>{e(tag)}</li>" for tag in project.features[locale])
    label = e(t["expand_label"].format(project=project.title))
    return f'''<article class="project" id="{e(project.id)}" aria-labelledby="{e(project.id)}-title">
<div class="project-heading"><h3 id="{e(project.id)}-title"><span class="project-number">{index:02}</span>{title}</h3><span class="kind">{e(project.kind[locale])}</span><div class="project-links"><a href="{source}">{e(t["source"])} ↗</a><a href="{source}#readme">{e(t["guide"])} ↗</a><a href="api/{e(project.api)}">{e(t["related_api"])} →</a></div></div>
<p class="project-description">{alt}</p><ul class="tags">{tags}</ul>
<div class="media-pair">
<figure class="demo motion-demo" data-animation="{animation}" data-poster="{poster}" data-title="{title}">
<figcaption><span class="media-title"><i class="media-dot" aria-hidden="true"></i>{e(t["animated"])}</span><span class="loop-label">↻ {e(t["loop_preview"])}</span></figcaption>
<img class="demo-image animated-image" src="{poster}" width="{project.width}" height="{project.height}" loading="lazy" decoding="async" alt="{alt}">
<div class="motion-controls"><button class="motion-toggle" type="button" aria-pressed="false" hidden><span class="motion-symbol" aria-hidden="true">▶</span><span class="motion-label">{e(t["play"])}</span></button><span class="motion-state">{e(t["paused"])}</span><noscript><a href="{animation}">{e(t["play"])} ↗</a></noscript></div></figure>
<figure class="demo still-demo"><figcaption><span class="media-title"><span class="image-icon" aria-hidden="true">▧</span>{e(t["still"])}</span><a class="expand" href="{poster}" data-lightbox data-title="{title}" data-alt="{alt}" aria-label="{label}">{e(t["expand"])} ↗</a></figcaption>
<a class="still-link" href="{poster}" data-lightbox data-title="{title}" data-alt="{alt}" aria-label="{label}"><img class="demo-image still-image" src="{poster}" width="{project.width}" height="{project.height}" loading="lazy" decoding="async" alt="{alt}"></a><div class="still-caption">{e(project.kind[locale])}</div></figure>
</div></article>'''


def search_index() -> list[dict[str, str]]:
    """Index documented classes, methods, properties and signals for client search."""
    entries: list[dict[str, str]] = []
    for cls in CLASSES:
        entries.append(
            {
                "name": cls.name,
                "owner": "",
                "kind": "class",
                "summary": text(cls.node.find("brief_description")),
                "href": cls.name + ".html",
            }
        )
        for selector, kind in (
            ("members/member", "property"),
            ("methods/method", "method"),
            ("signals/signal", "signal"),
        ):
            for node in cls.node.findall(selector):
                name = node.attrib["name"]
                entries.append(
                    {
                        "name": name,
                        "owner": cls.name,
                        "kind": kind,
                        "summary": text(node)
                        if kind == "property"
                        else text(node.find("description")),
                        "href": f"{cls.name}.html#{kind}-{name}",
                    }
                )
    return entries


def sidebar(cls_name: str, t: dict[str, str]) -> str:
    """Render document navigation and a progressively enhanced search box."""
    links = (
        '<a href="./"'
        + (' aria-current="page"' if not cls_name else "")
        + f">{e(t['overview'])}</a>"
    )
    for cls in CLASSES:
        current = ' aria-current="page"' if cls.name == cls_name else ""
        links += f'<a href="{cls.name}.html"{current}>{cls.name}</a>'
    return f'''<aside class="api-sidebar"><div class="api-search" hidden><label for="api-search">{e(t["nav_api"])}</label><input id="api-search" type="search" placeholder="{e(t["search"])}" autocomplete="off" aria-controls="search-results"><div id="search-results" class="search-results" role="region" aria-live="polite" hidden></div></div><details class="api-menu" open><summary>{e(t["classes"])}</summary><nav aria-label="{e(t["nav_api"])}">{links}</nav></details></aside>'''


def api_overview(t: dict[str, str]) -> str:
    """Introduce the reference with lifecycle, interop and rendering guidance."""
    cards = "".join(
        f'<a class="api-class-card" href="{cls.name}.html"><span class="step">CLASS / {e(cls.inherits)}</span><h2>{cls.name} →</h2><p>{e(text(cls.node.find("brief_description")))}</p></a>'
        for cls in CLASSES
    )
    sections = "".join(
        f'<section class="api-guide" id="{key}"><h2>{e(t[key + "_title"])}</h2><p>{e(t[key + "_text"])}</p></section>'
        for key in ("lifecycle", "bridge", "render")
    )
    return f"""<main class="shell api-layout" id="content">{sidebar("", t)}<article class="api-content"><p class="eyebrow">REFERENCE / GODOT GDEXTENSION</p><h1>{e(t["api_title"])}</h1><p class="lead">{e(t["api_description"])}</p><p class="api-note">{e(t["api_source_note"])}</p><div class="api-class-grid">{cards}</div>{sections}<p class="api-callout">{e(t["fps_text"])}</p></article></main>"""


def signature(node: ET.Element, *, link: bool = False) -> str:
    """Format a method signature with parameter defaults and linked types."""
    name = node.attrib["name"]
    arguments = []
    for param in node.findall("param"):
        default = (
            f" = {e(param.attrib['default'])}" if "default" in param.attrib else ""
        )
        arguments.append(
            f"{e(param.attrib['name'])}: {type_link(param.attrib['type'])}{default}"
        )
    label = f'<a href="#method-{e(name)}">{e(name)}</a>' if link else e(name)
    returns = node.find("return")
    result = type_link(returns.attrib["type"] if returns is not None else "void")
    qualifiers = (
        f' <span class="qualifier">{e(node.attrib["qualifiers"])}</span>'
        if "qualifiers" in node.attrib
        else ""
    )
    return f"<code>{label}({', '.join(arguments)}) → {result}{qualifiers}</code>"


def api_class(cls: ApiClass, t: dict[str, str]) -> str:
    """Render every documented property, method and signal for one class."""
    name = cls.name
    properties = cls.node.findall("members/member")
    methods = cls.node.findall("methods/method")
    signals = cls.node.findall("signals/signal")
    summary = ""
    if properties:
        rows = "".join(
            f'<tr><td><code><a href="#property-{e(node.attrib["name"])}">{e(node.attrib["name"])}</a></code></td><td><code>{type_link(node.attrib["type"])}</code></td><td><code>{e(node.attrib.get("default", "—"))}</code></td></tr>'
            for node in properties
        )
        summary = f'<section id="properties"><h2>{e(t["properties"])}</h2><div class="table-wrap"><table><thead><tr><th>{e(t["member"])}</th><th>{e(t["type"])}</th><th>{e(t["default"])}</th></tr></thead><tbody>{rows}</tbody></table></div></section>'
    regular = [node for node in methods if node.attrib["name"] not in INTERNAL]
    method_summary = "".join(
        f"<li>{signature(node, link=True)}</li>" for node in regular
    )
    summary += f'<section id="methods"><h2>{e(t["methods"])}</h2><ul class="method-summary">{method_summary}</ul></section>'
    property_details = "".join(
        f'<section class="api-member" id="property-{e(node.attrib["name"])}"><h3><a href="#property-{e(node.attrib["name"])}">{e(node.attrib["name"])}</a><span class="member-kind">{e(node.attrib["type"])}</span></h3>{documentation(text(node), name)}<p class="accessors">{e(t["default"])}: <code>{e(node.attrib.get("default", "—"))}</code> · {e(t["accessors"])}: <a href="#method-{e(node.attrib["setter"])}"><code>{e(node.attrib["setter"])}()</code></a> / <a href="#method-{e(node.attrib["getter"])}"><code>{e(node.attrib["getter"])}()</code></a></p></section>'
        for node in properties
    )

    def method_detail(node: ET.Element) -> str:
        method_name = e(node.attrib["name"])
        return f'<section class="api-member" id="method-{method_name}"><h3><a href="#method-{method_name}">{method_name}</a></h3><div class="signature">{signature(node)}</div>{documentation(text(node.find("description")), name)}</section>'

    method_details = "".join(method_detail(node) for node in regular)
    signal_details = "".join(
        f'<section class="api-member" id="signal-{e(node.attrib["name"])}"><h3><a href="#signal-{e(node.attrib["name"])}">{e(node.attrib["name"])}</a></h3>{documentation(text(node.find("description")), name)}</section>'
        for node in signals
    )
    if signals:
        signal_details = f'<section id="signals"><h2>{e(t["signals"])}</h2>{signal_details}</section>'
    internal = "".join(
        method_detail(node) for node in methods if node.attrib["name"] in INTERNAL
    )
    if internal:
        internal = f'<details class="api-internal"><summary>{e(t["internal"])}</summary>{internal}</details>'
    toc = "".join(
        f'<a href="#{key}">{e(t[key])}</a>'
        for key, nodes in (
            ("properties", properties),
            ("methods", methods),
            ("signals", signals),
        )
        if nodes
    )
    source = f"{REPOSITORY}/blob/main/ultralight-view/doc_classes/{name}.xml"
    return f'''<main class="shell api-layout" id="content">{sidebar(name, t)}<article class="api-content"><p class="eyebrow">API / CLASS REFERENCE</p><h1>{name}</h1><p class="inherits">{e(t["inherits"])}: <code>{type_link(cls.inherits)}</code></p><p class="lead">{e(text(cls.node.find("brief_description")))}</p><div class="api-description">{documentation(text(cls.node.find("description")), name)}</div><nav class="api-toc" aria-label="{e(t["on_this_page"])}">{toc}</nav>{summary}{property_details}{method_details}{signal_details}{internal}<p class="api-source"><a href="{source}">{e(t["read_xml"])} ↗</a></p></article></main>'''


def write_page(
    relative: str,
    locale: str,
    t: dict[str, str],
    title: str,
    description: str,
    content: str,
    *,
    section: str = "",
    entry: bool = False,
) -> None:
    """Write a page with links relative to its final deployment location."""
    target = OUTPUT / relative
    target.parent.mkdir(parents=True, exist_ok=True)
    prefix = "../" * len(Path(relative).parent.parts)
    home = prefix + locale + "/"
    links = "".join(
        f'<a href="{prefix}{code}/{section}" lang="{code}" hreflang="{code}" data-language="{code}"'
        + (' aria-current="page"' if code == locale else "")
        + f">{label}</a>"
        for code, label in (("zh-CN", "中文"), ("en", "EN"))
    )
    search = (
        f'<script type="application/json" id="api-index">{json_script(search_index())}</script>'
        if section.startswith("api/")
        else ""
    )
    page = render(
        (SOURCE / "layout.html").read_text(),
        t,
        LANG=locale,
        ENTRY=str(entry).lower(),
        TITLE=e(title),
        DESCRIPTION=e(description),
        PREFIX=prefix,
        HOME=home,
        API=home + "api/",
        LICENSES=home + "licenses.html",
        ZH_URL=prefix + "zh-CN/" + section,
        EN_URL=prefix + "en/" + section,
        LOCALE_NAV=links,
        CONTENT=content,
        MESSAGES=json_script(t),
        SEARCH_INDEX=search,
    )
    target.write_text(page, encoding="utf-8")


def build() -> None:
    """Generate both locales, the default entry, shared assets and XML reference pages."""
    if OUTPUT.exists():
        shutil.rmtree(OUTPUT)
    OUTPUT.mkdir()
    shutil.copytree(SOURCE / "assets", OUTPUT / "assets")
    media = OUTPUT / "assets/media"
    media.mkdir()
    projects = [
        Project(**item) for item in json.loads((SOURCE / "projects.json").read_text())
    ]
    for project in projects:
        shutil.copy2(ROOT / "docs/media" / project.animation, media / project.animation)
        if not (SOURCE / project.poster).is_file():
            raise FileNotFoundError(project.poster)
    catalogs = {
        locale: json.loads((SOURCE / "locales" / f"{locale}.json").read_text())
        for locale in LOCALES
    }
    if set(catalogs["en"]) != set(catalogs["zh-CN"]):
        raise ValueError("Locale keys differ")
    for locale in LOCALES:
        t = catalogs[locale]
        for entry in (False, True) if locale == "zh-CN" else (False,):
            prefix = "" if entry else "../"
            cards = "".join(
                sample_card(project, locale, prefix, t, index)
                for index, project in enumerate(projects, 1)
            )
            # Entry-page links must also lead to the localized API reference.
            if entry:
                cards = cards.replace('href="api/', f'href="{locale}/api/')
            home = render(
                (SOURCE / "index.html").read_text(),
                t,
                API=prefix + locale + "/api/",
                PROJECTS=cards,
                PROJECT_INDEX="".join(
                    f'<a href="#{e(project.id)}">{index:02}<span>↘</span></a>'
                    for index, project in enumerate(projects, 1)
                ),
                CONNECT_CODE=connect_code(),
                HTML_CODE=code_block(
                    "<!doctype html>\n<html lang=\"en\">\n<body>\n  <button onclick=\"document.querySelector('p').textContent = greet('Godot')\">Say hello</button>\n  <p></p>\n</body>\n</html>",
                    "HTML",
                ),
            )
            write_page(
                "index.html" if entry else f"{locale}/index.html",
                locale,
                t,
                t["page_title"],
                t["page_description"],
                home,
                entry=entry,
            )
        write_page(
            f"{locale}/api/index.html",
            locale,
            t,
            t["api_title"] + " · ultralight-godot",
            t["api_description"],
            api_overview(t),
            section="api/",
        )
        for cls in CLASSES:
            write_page(
                f"{locale}/api/{cls.name}.html",
                locale,
                t,
                cls.name + " · ultralight-godot API",
                text(cls.node.find("brief_description")),
                api_class(cls, t),
                section=f"api/{cls.name}.html",
            )
        licenses = f'''<main class="shell license-main" id="content"><p class="eyebrow">CREDITS / LICENSES</p><h1>{e(t["license_title"])}</h1><p>{e(t["license_description"])}</p><div class="license-list"><article class="license-item"><h2>ultralight-godot</h2><a href="{REPOSITORY}/blob/main/LICENSE">{e(t["code_license"])} ↗</a></article><article class="license-item"><h2>Ashen Reliquary</h2><a href="{REPOSITORY}/blob/main/samples/Sample%202%20-%20Inventory%20System/THIRD_PARTY_NOTICES.md">{e(t["art_license"])} ↗</a></article><article class="license-item"><h2>Ultralight / Godot</h2><a href="https://ultralig.ht/">Ultralight ↗</a> · <a href="https://godotengine.org/license/">Godot ↗</a></article></div></main>'''
        write_page(
            f"{locale}/licenses.html",
            locale,
            t,
            t["license_title"],
            t["license_description"],
            licenses,
            section="licenses.html",
        )
    (OUTPUT / ".nojekyll").touch()
    print(
        f"Built {len(projects)} samples and {len(CLASSES)} API classes in {len(LOCALES)} locales → {OUTPUT}"
    )


if __name__ == "__main__":
    build()
