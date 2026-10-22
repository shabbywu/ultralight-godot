import { Inventory, Item, Position, Command, Container } from "./core";
// A renderer can use a local engine or an authoritative host adapter.
export type InventoryPort = Pick<Inventory,
  "snapshot" | "preview" | "execute" | "definitions" | "dimensions" | "occupant" | "on" | "emit">;

export type Hover = {
  phase: "enter" | "move" | "leave";
  itemId: string;
  position: Position;
  anchor: DOMRect;
  pointer: { x: number; y: number };
  modifiers: { shift: boolean; alt: boolean; ctrl: boolean; meta: boolean };
};
export type ViewOptions = {
  cellSize?: number;
  gap?: number;
  render: (item: Item) => HTMLElement;
  command?: (item: Item, to: Position, container: Container) => Command;
  dropTarget?: (item: Item, hit: Element | null) => { element: HTMLElement; command: Command } | undefined;
};
let splitSequence = 0;
type Drag = {
  id: string;
  rotation: number;
  dx: number;
  dy: number;
  scale: number;
  revision: number;
  ghost: HTMLElement;
  preview: HTMLElement;
  to?: Position;
  command?: Command;
  split?: { quantity: number; newId: string };
  splitDraft?: { quantity: number; newId: string };
  preparedSplit?: { quantity: number; newId: string };
  ctrlDown: boolean;
  last?: MouseEvent;
  startX: number;
  startY: number;
  moved: boolean;
  pointerId?: number;
  capture?:HTMLElement;
};
export class InventoryView {
  private cleanup: (() => void)[] = [];
  private shift = false;
  private hover?: Hover;
  private focusFrame?: number;
  private drag?: Drag;
  private split?: { id: string; quantity: number; newId: string };
  constructor(
    private engine: InventoryPort,
    private root: HTMLElement,
    private options: ViewOptions
  ) {}
  /** Select quantity before the next drag. Nothing enters the inventory until drop. */
  splitNextDrag(id: string, quantity: number, newId: string) {
    this.cancel();
    this.split = { id, quantity, newId };
  }
  private endHover() {
    if (this.hover) {
      this.engine.emit("hover", { ...this.hover, phase: "leave" });
      this.hover = undefined;
    }
  }
  private cancel(reason = "cancel") {
    if (this.drag) this.engine.emit("cancel", { reason, itemId: this.drag.id });
    if (this.focusFrame !== undefined) cancelAnimationFrame(this.focusFrame);
    this.focusFrame = undefined;
    const pointerId = this.drag?.pointerId;
    const capture=this.drag?.capture;
    if(pointerId!==undefined&&capture?.hasPointerCapture?.(pointerId))capture.releasePointerCapture(pointerId);
    this.drag?.ghost.remove();
    this.drag?.preview.remove();
    this.drag = undefined;
    this.split = undefined;
    this.shift = false;
    this.endHover();
  }
  private render = () => {
    this.endHover();
    const fragment = document.createDocumentFragment();
    const s = this.engine.snapshot(),
      cell = this.options.cellSize || 44,
      gap = this.options.gap ?? 2;
    for (const c of s.containers) {
      const region = document.createElement("section");
      region.dataset.container = c.id;
      region.dataset.kind = c.kind;
      const reserved = this.engine.occupant(c.id);
      if (
        c.kind === "slot" &&
        reserved &&
        !s.items.some((i) => i.id === reserved && i.position.container === c.id)
      )
        region.dataset.reservedBy = reserved;
      region.className = "inventory-container";
      region.style.cssText = `position:relative;box-sizing:content-box;width:${
        (c.width || 2) * (cell + gap) - gap
      }px;height:${
        (c.height || 2) * (cell + gap) - gap
      }px;background-color:#191714;background-image:linear-gradient(#66543844 1px,transparent 1px),linear-gradient(90deg,#66543844 1px,transparent 1px);background-size:${
        cell + gap
      }px ${cell + gap}px;border:1px solid #786343`;
      region.setAttribute("aria-label", c.id);
      const title = document.createElement("label");
      title.textContent = c.id;
      title.style.cssText = "position:absolute;top:-22px";
      region.append(title);
      fragment.append(region);
      for (const item of s.items.filter((i) => i.position.container === c.id)) {
        const el = this.options.render(item),
          [w, h] = this.engine.dimensions(item);
        el.dataset.item = item.id;
        el.style.cssText += `;position:absolute;box-sizing:border-box;left:${
          (item.position.x || 0) * (cell + gap)
        }px;top:${(item.position.y || 0) * (cell + gap)}px;width:${
          c.kind === "grid" ? w * (cell + gap) - gap : 2 * cell + gap
        }px;height:${
          c.kind === "grid" ? h * (cell + gap) - gap : 2 * cell + gap
        }px;touch-action:none;cursor:grab`;
        region.append(el);
      }
    }
    this.root.replaceChildren(fragment);
  };
  mount() {
    this.destroy();
    const tabIndex = this.root.getAttribute("tabindex");
    if (tabIndex === null) this.root.tabIndex = -1;
    this.cleanup.push(() => {
      if (tabIndex === null) this.root.removeAttribute("tabindex");
      else this.root.setAttribute("tabindex", tabIndex);
    });
    this.render();
    this.cleanup.push(
      this.engine.on("change", () => {
        this.cancel();
        this.render();
      })
    );
    const listen = (
      target: EventTarget,
      event: string,
      handler: EventListener,
      capture: boolean | AddEventListenerOptions = false
    ) => {
      target.addEventListener(event, handler, capture);
      this.cleanup.push(() => target.removeEventListener(event, handler, capture));
    };
    const refreshGhost = (d: Drag) => {
      const item = this.engine.snapshot().items.find((i) => i.id === d.id)!;
      const position = { ...item.position, rotation: d.rotation };
      const held = { ...item, position, quantity: d.split?.quantity ?? item.quantity };
      const content = this.options.render(held);
      d.ghost.replaceChildren(...Array.from(content.childNodes));
      const [w, h] = this.engine.dimensions(held),
        gap = this.options.gap ?? 2,
        step = (this.options.cellSize || 44) + gap;
      d.ghost.style.width = (w * step - gap) * d.scale + "px";
      d.ghost.style.height = (h * step - gap) * d.scale + "px";
    };
    const down = (e: MouseEvent) => {
      if (e.button !== 0) return;
      const el = (e.target as HTMLElement).closest<HTMLElement>("[data-item]");
      if (!el) return;
      const id = el.dataset.item!,
        item = this.engine.snapshot().items.find((i) => i.id === id)!;
      const preparedSplit = this.split?.id === id ? this.split : undefined;
      const splitDraft = e.ctrlKey && item.quantity > 1
        ? { quantity: Math.floor(item.quantity / 2), newId: "split-" + Date.now() + "-" + ++splitSequence }
        : undefined;
      const split = preparedSplit || splitDraft;
      if (!split && (e.shiftKey || this.shift)) {
        this.engine.emit("activate", { itemId: id, kind: "transfer" });
        return;
      }
      this.endHover();
      this.root.focus({ preventScroll: true });
      const rect = el.getBoundingClientRect(),
        ghost = el.cloneNode(true) as HTMLElement,
        preview = document.createElement("div");
      ghost.removeAttribute("data-item");
      ghost.style.cssText += `;position:fixed;pointer-events:none;z-index:10000;opacity:.7;left:${rect.left}px;top:${rect.top}px;width:${rect.width}px;height:${rect.height}px`;
      document.body.append(ghost);
      preview.style.cssText =
        "position:fixed;pointer-events:none;z-index:9999;box-sizing:border-box";
      document.body.append(preview);
      const scale = rect.width / el.offsetWidth;
      this.drag = {
        id,
        rotation: item.position.rotation || 0,
        dx: (e.clientX - rect.left) / scale,
        dy: (e.clientY - rect.top) / scale,
        scale,
        revision: this.engine.snapshot().revision,
        ghost,
        preview,
        startX: e.clientX,
        startY: e.clientY,
        moved: false,
        pointerId: (e as PointerEvent).pointerId,
        capture:el,
        split,
        splitDraft,
        preparedSplit,
        ctrlDown: e.ctrlKey,
      };
      this.split = undefined;
      if (split) refreshGhost(this.drag);
      const pointerId = (e as PointerEvent).pointerId;
      if (pointerId !== undefined && el.setPointerCapture)
        el.setPointerCapture(pointerId);
      let focused = document.hasFocus();
      const checkFocus = () => {
        if (!this.drag) return;
        const next = document.hasFocus();
        if (focused && !next) {
          this.cancel("focus");
          return;
        }
        focused = next;
        this.focusFrame = requestAnimationFrame(checkFocus);
      };
      this.focusFrame = requestAnimationFrame(checkFocus);
      move(e);
      e.preventDefault();
    };
    const move = (e: MouseEvent, ctrlDown = e.ctrlKey) => {
      const d = this.drag;
      if (!d) {
        const el = (e.target as HTMLElement).closest?.<HTMLElement>(
          "[data-item]"
        );
        if (el && this.root.contains(el)) {
          const id = el.dataset.item!,
            item = this.engine.snapshot().items.find((i) => i.id === id);
          if (!item) return;
          if (this.hover?.itemId !== id) this.endHover();
          const context: Hover = {
            phase: this.hover ? "move" : "enter",
            itemId: id,
            position: item.position,
            anchor: el.getBoundingClientRect(),
            pointer: { x: e.clientX, y: e.clientY },
            modifiers: {
              shift: e.shiftKey || this.shift,
              alt: e.altKey,
              ctrl: e.ctrlKey,
              meta: e.metaKey,
            },
          };
          this.hover = context;
          this.engine.emit("hover", context);
        } else this.endHover();
        return;
      }
      d.ctrlDown = ctrlDown;
      if (ctrlDown && !d.splitDraft) {
        const item = this.engine.snapshot().items.find((i) => i.id === d.id)!;
        if (item.quantity > 1) d.splitDraft = {
          quantity: Math.floor(item.quantity / 2),
          newId: "split-" + Date.now() + "-" + ++splitSequence,
        };
      }
      const split = d.preparedSplit || (ctrlDown ? d.splitDraft : undefined);
      if (split !== d.split) {
        d.split = split;
        refreshGhost(d);
      }
      if (Math.hypot(e.clientX - d.startX, e.clientY - d.startY) > 3)
        d.moved = true;
      d.last = e;
      d.ghost.style.left = e.clientX - d.dx * d.scale + "px";
      d.ghost.style.top = e.clientY - d.dy * d.scale + "px";
      d.to = undefined;
      d.command = undefined;
      d.preview.style.display = "none";
      const hit = document.elementFromPoint(e.clientX, e.clientY);
      const held = this.engine.snapshot().items.find((i) => i.id === d.id)!;
      const external = this.options.dropTarget?.(held, hit);
      if (external) {
        d.command = external.command;
        const result = this.engine.preview(external.command);
        const rect = external.element.getBoundingClientRect();
        d.preview.style.cssText = `position:fixed;pointer-events:none;z-index:9999;box-sizing:border-box;display:block;left:${rect.left}px;top:${rect.top}px;width:${rect.width}px;height:${rect.height}px;border:2px solid ${result.ok ? "#a5be6a" : "#d56455"};background:${result.ok ? "#a5be6a22" : "#d5645522"}`;
        return;
      }
      const region = hit?.closest<HTMLElement>("[data-container]");
      if (!region || !this.root.contains(region)) return;
      const rect = region.getBoundingClientRect(),
        state = this.engine.snapshot(),
        c = state.containers.find((c) => c.id === region.dataset.container)!,
        step = (this.options.cellSize || 44) + (this.options.gap ?? 2),
        scale = rect.width / region.offsetWidth;
      d.scale = scale;
      const to: Position = {
          container: c.id,
          x:
            c.kind === "grid"
              ? Math.floor(
                  (e.clientX - rect.left - region.clientLeft * scale) /
                    scale /
                    step
                ) - Math.floor(d.dx / step)
              : undefined,
          y:
            c.kind === "grid"
              ? Math.floor(
                  (e.clientY - rect.top - region.clientTop * scale) /
                    scale /
                    step
                ) - Math.floor(d.dy / step)
              : undefined,
          rotation: d.rotation,
        },
        item = state.items.find((i) => i.id === d.id)!;
      const target = hit?.closest<HTMLElement>("[data-item]")?.dataset.item;
      let command: Command = d.split
        ? { type: "split", id: d.id, ...d.split, to }
        : this.options.command?.(item, to, c) || { type: "move", id: d.id, to };
      if (!d.split && target && target !== d.id) {
        const merge: Command = { type: "merge", id: d.id, target };
        if (this.engine.preview(merge).ok) command = merge;
      }
      d.to = to;
      d.command = command;
      const result = this.engine.preview(command),
        [w, h] = this.engine.dimensions({ ...item, position: to });
      d.ghost.style.width = (w * step - (this.options.gap ?? 2)) * scale + "px";
      d.ghost.style.height =
        (h * step - (this.options.gap ?? 2)) * scale + "px";
      d.ghost.style.left = e.clientX - d.dx * scale + "px";
      d.ghost.style.top = e.clientY - d.dy * scale + "px";
      d.preview.style.cssText += `;display:block;left:${
        rect.left + (to.x || 0) * step * scale
      }px;top:${rect.top + (to.y || 0) * step * scale}px;width:${
        (c.kind === "grid"
          ? w * step - (this.options.gap ?? 2)
          : region.offsetWidth) * scale
      }px;height:${
        (c.kind === "grid"
          ? h * step - (this.options.gap ?? 2)
          : region.offsetHeight) * scale
      }px;border:2px solid ${result.ok ? "#a5be6a" : "#d56455"};background:${
        result.ok ? "#a5be6a22" : "#d5645522"
      }`;
    };
    let suppressClick = false;
    const up = (e: MouseEvent) => {
      if (!this.drag) return;
      move(e);
      const d = this.drag;
      if (d.moved) suppressClick = true;
      if (d.command && d.moved) {
        const result = this.engine.execute(d.command, d.revision);
        if (!result.ok) this.engine.emit("reject", result);
      }
      this.cancel();
    };
    const pointer = typeof PointerEvent !== "undefined";
    // The release binds/moves only; it must not also activate a drop target.
    listen(document, pointer ? "pointerdown" : "mousedown", () => { suppressClick = false; }, true);
    listen(document, "click", ((e: MouseEvent) => {
      if (!suppressClick) return;
      suppressClick = false;
      e.preventDefault();
      e.stopImmediatePropagation();
    }) as EventListener, true);
    listen(
      this.root,
      pointer ? "pointerdown" : "mousedown",
      down as EventListener
    );
    listen(
      document,
      pointer ? "pointermove" : "mousemove",
      move as EventListener
    );
    listen(document, pointer ? "pointerup" : "mouseup", up as EventListener);
    listen(window, "blur", () => this.cancel("blur"));
    listen(document, "pointercancel", () => this.cancel("pointercancel"));
    listen(this.root, "mouseleave", () => {
      if (!this.drag) this.endHover();
    });
    listen(this.root, "dblclick", ((e: MouseEvent) => {
      if (e.ctrlKey || e.shiftKey) return;
      const el = (e.target as HTMLElement).closest<HTMLElement>("[data-item]");
      if (el)
        this.engine.emit("activate", {
          itemId: el.dataset.item,
          kind: "equip",
        });
    }) as EventListener);
    listen(document, "keydown", ((e: KeyboardEvent) => {
      const key = (e.key || "").toLowerCase();
      if (key === "shift" || e.keyCode === 16) this.shift = true;
      if ((key === "control" || e.code === "ControlLeft" || e.code === "ControlRight" || e.keyCode === 17) && this.drag?.last) {
        move(this.drag.last, true);
      }
      if (key === "escape" || e.code === "Escape" || e.keyCode === 27) this.cancel();
      if ((key === "r" || e.code === "KeyR" || e.keyCode === 82) && this.drag) {
        const d = this.drag;
        const item = this.engine.snapshot().items.find((i) => i.id === d.id)!;
        if (!this.engine.definitions.get(item.definition)?.rotatable) return;
        e.preventDefault();
        d.rotation = (d.rotation + 1) % 4;
        d.moved = true;
        // Held orientation is independent of the candidate drop location.
        refreshGhost(d);
        if (d.last) move(d.last, d.ctrlDown);
      }
    }) as EventListener, true);
    listen(document, "wheel", ((e: WheelEvent) => {
      const d = this.drag;
      if (!d?.split || !e.deltaY) return;
      e.preventDefault();
      const item = this.engine.snapshot().items.find((i) => i.id === d.id)!;
      d.split.quantity = Math.max(1, Math.min(item.quantity - 1,
        d.split.quantity + (e.deltaY < 0 ? 1 : -1)));
      refreshGhost(d);
      if (d.last) move(d.last, d.ctrlDown);
    }) as EventListener, { passive: false });
    listen(document, "keyup", ((e: KeyboardEvent) => {
      if (e.key === "Shift" || e.keyCode === 16) this.shift = false;
      if ((e.key === "Control" || e.code === "ControlLeft" || e.code === "ControlRight" || e.keyCode === 17) && this.drag?.last) {
        move(this.drag.last, false);
      }
    }) as EventListener, true);
    return this;
  }
  destroy() {
    this.cancel();
    this.cleanup.splice(0).forEach((fn) => fn());
    this.root.replaceChildren();
  }
}
