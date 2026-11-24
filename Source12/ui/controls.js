// Seer12 UI controls: knob, value box, select, pill toggle, segmented, h-bar.
// Every control is bound to one parameter id and stays in sync both ways.
(function () {
  const NS = "http://www.w3.org/2000/svg";
  const NOTE = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"];

  const BIPOLAR = /^(lpf_envelope_amt|envelope_\d_amt|mod_\d+_amount)$/;
  const isBipolar = id => BIPOLAR.test(id);

  // ---- value formatting -------------------------------------------------
  function fmt(id, v) {
    const p = Seer.param(id);
    if (p.type === "toggle") return v ? "On" : "Off";
    if (window.SEER_LIST_FOR(id)) return window.SEER_LABEL(id, v);
    if (/^osc_\d_pitch$/.test(id)) return NOTE[v % 12] + Math.floor(v / 12);
    if (/fine_tune$/.test(id)) { const c = v - 50; return (c > 0 ? "+" : "") + c + "c"; }
    if (isBipolar(id)) { const b = v - 127; return (b > 0 ? "+" : "") + b; }
    if (/(^|_)pan(_spread)?$/.test(id) && p.max === 127) return v === 64 ? "C" : v < 64 ? "L" + (64 - v) : "R" + (v - 64);
    if (id === "split_point") return NOTE[v % 12] + (Math.floor(v / 12) - 1);
    if (id === "arpeggiator_range") return (v + 1) + " oct";
    if (id === "arpeggiator_repeats") return "×" + (v + 1);
    if (id === "bpm") return v + " bpm";
    if (/^pitchbend_range/.test(id)) return v + " st";
    if (id === "feedback_tuning") return v + " st";
    return String(v);
  }

  // ---- hover / last-touched display hook --------------------------------
  const touchListeners = [];
  function touched(id) { touchListeners.forEach(fn => fn(id)); }

  function el(tag, cls, html) {
    const e = document.createElement(tag);
    if (cls) e.className = cls;
    if (html !== undefined) e.innerHTML = html;
    return e;
  }

  function hoverable(node, id) {
    node.addEventListener("pointerenter", () => touched(id));
    Seer.on(id, () => touched(id));
  }

  // ---- drag helper (vertical drag, shift = fine) ------------------------
  function dragNorm(node, id, opts = {}) {
    let startY = 0, startN = 0, dragging = false;
    const range = opts.horizontal ? 180 : 200;
    node.addEventListener("pointerdown", e => {
      if (e.button !== 0) return;
      dragging = true; startY = opts.horizontal ? e.clientX : e.clientY; startN = Seer.norm(id);
      node.setPointerCapture(e.pointerId); node.classList.add("drag");
      Seer.begin(id); e.preventDefault();
    });
    node.addEventListener("pointermove", e => {
      if (!dragging) return;
      const pos = opts.horizontal ? e.clientX : e.clientY;
      const delta = opts.horizontal ? pos - startY : startY - pos;
      const scale = window.SEER_SCALE || 1;
      const n = Math.min(1, Math.max(0, startN + delta / scale / (e.shiftKey ? range * 5 : range)));
      Seer.setNorm(id, n);
    });
    const up = () => { if (!dragging) return; dragging = false; node.classList.remove("drag"); Seer.end(id); };
    node.addEventListener("pointerup", up);
    node.addEventListener("pointercancel", up);
    node.addEventListener("dblclick", () => {
      const p = Seer.param(id);
      Seer.begin(id); Seer.set(id, isBipolar(id) ? Math.round((p.min + p.max) / 2) : p.min); Seer.end(id);
    });
    node.addEventListener("wheel", e => {
      e.preventDefault();
      Seer.begin(id); Seer.set(id, Seer.get(id) + (e.deltaY < 0 ? 1 : -1)); Seer.end(id);
    }, { passive: false });
  }

  // ---- Knob -------------------------------------------------------------
  const A0 = -135, A1 = 135;
  function polar(cx, cy, r, a) { const t = (a - 90) * Math.PI / 180; return [cx + r * Math.cos(t), cy + r * Math.sin(t)]; }
  function arc(cx, cy, r, s, e) {
    if (Math.abs(e - s) < 0.01) return "";
    const [x0, y0] = polar(cx, cy, r, Math.min(s, e)), [x1, y1] = polar(cx, cy, r, Math.max(s, e));
    return `M${x0.toFixed(2)} ${y0.toFixed(2)}A${r} ${r} 0 ${Math.abs(e - s) > 180 ? 1 : 0} 1 ${x1.toFixed(2)} ${y1.toFixed(2)}`;
  }

  function knob(id, label, opts = {}) {
    const size = opts.size || 44;
    const wrap = el("div", "knob" + (opts.cls ? " " + opts.cls : ""));
    wrap.style.setProperty("--k", size + "px");
    const c = size / 2, r = c - 3.5, cap = r - 6;
    const svg = document.createElementNS(NS, "svg");
    svg.setAttribute("viewBox", `0 0 ${size} ${size}`);
    svg.setAttribute("width", size); svg.setAttribute("height", size);
    svg.innerHTML =
      `<path class="k-track" d="${arc(c, c, r, A0, A1)}"/>` +
      `<path class="k-val"/>` +
      `<circle class="k-cap" cx="${c}" cy="${c}" r="${cap}"/>` +
      `<circle class="k-hi" cx="${c}" cy="${c - cap * 0.18}" r="${cap * 0.82}"/>` +
      `<line class="k-ptr" x1="${c}" y1="${c}" x2="${c}" y2="${c - cap + 2}"/>`;
    const val = el("div", "k-value");
    const lab = el("div", "k-label", label);
    wrap.append(svg, val, lab);
    const vPath = svg.querySelector(".k-val"), ptr = svg.querySelector(".k-ptr");
    const bip = isBipolar(id);
    function draw() {
      const n = Seer.norm(id), a = A0 + n * (A1 - A0);
      vPath.setAttribute("d", bip ? arc(c, c, r, 0, a) : arc(c, c, r, A0, a));
      const [x, y] = polar(c, c, cap - 2.5, a), [x0, y0] = polar(c, c, cap * 0.35, a);
      ptr.setAttribute("x1", x0); ptr.setAttribute("y1", y0); ptr.setAttribute("x2", x); ptr.setAttribute("y2", y);
      val.textContent = fmt(id, Seer.get(id));
    }
    Seer.on(id, draw); draw();
    dragNorm(svg, id); hoverable(wrap, id);
    return wrap;
  }

  // ---- Value box (Vital-style draggable number) -------------------------
  function valueBox(id, label, opts = {}) {
    const wrap = el("div", "vbox" + (opts.cls ? " " + opts.cls : ""));
    const box = el("div", "vb-field");
    const lab = el("div", "k-label", label);
    wrap.append(box, lab);
    const draw = () => { box.textContent = fmt(id, Seer.get(id)); };
    Seer.on(id, draw); draw();
    dragNorm(box, id); hoverable(wrap, id);
    return wrap;
  }

  // ---- Select (custom popup list) ---------------------------------------
  let openMenu = null;
  document.addEventListener("pointerdown", e => {
    if (openMenu && !openMenu.contains(e.target)) { openMenu.remove(); openMenu = null; }
  });

  function select(id, label, opts = {}) {
    const p = Seer.param(id);
    const wrap = el("div", "sel" + (opts.cls ? " " + opts.cls : ""));
    const btn = el("button", "sel-btn");
    const txt = el("span", "sel-txt");
    btn.append(el("span", "sel-arrow l", "‹"), txt, el("span", "sel-arrow r", "›"));
    wrap.append(btn);
    if (label) wrap.append(el("div", "k-label", label));
    const draw = () => { txt.innerHTML = (opts.prefix ? `<em>${opts.prefix}</em>` : "") + fmt(id, Seer.get(id)); };
    Seer.on(id, draw); draw();
    const step = d => { Seer.begin(id); Seer.set(id, Seer.get(id) + d); Seer.end(id); };
    btn.querySelector(".l").addEventListener("click", e => { e.stopPropagation(); step(-1); });
    btn.querySelector(".r").addEventListener("click", e => { e.stopPropagation(); step(1); });
    btn.addEventListener("click", () => {
      if (openMenu) { openMenu.remove(); openMenu = null; }
      const menu = el("div", "menu");
      const pick = v => { Seer.begin(id); Seer.set(id, v); Seer.end(id); menu.remove(); openMenu = null; };
      const item = v => {
        const it = el("div", "menu-it" + (v === Seer.get(id) ? " on" : ""), fmt(id, v));
        it.addEventListener("click", () => pick(v));
        return it;
      };
      const lst = window.SEER_LIST_FOR(id);
      if (lst && lst.categories) {
        // Grouped menu (mod sources / destinations): one column block per category.
        menu.classList.add("grouped");
        const groups = new Map();
        for (let v = p.min; v <= p.max; v++) {
          const cat = lst.categories[v - p.min] || "General";
          if (!groups.has(cat)) groups.set(cat, []);
          groups.get(cat).push(v);
        }
        groups.forEach((vals, cat) => {
          const g = el("div", "menu-group");
          g.append(el("div", "menu-cat", cat));
          vals.forEach(v => g.append(item(v)));
          menu.append(g);
        });
      } else {
        const n = p.max - p.min + 1;
        if (n > 14) menu.classList.add("cols");
        for (let v = p.min; v <= p.max; v++) menu.append(item(v));
      }
      document.getElementById("app").append(menu);
      const s = window.SEER_SCALE || 1, br = btn.getBoundingClientRect(), ar = document.getElementById("app").getBoundingClientRect();
      let left = (br.left - ar.left) / s, top = (br.bottom - ar.top) / s + 4;
      menu.style.left = left + "px"; menu.style.top = top + "px";
      const mh = menu.offsetHeight, mw = menu.offsetWidth;
      if (top + mh > 870) menu.style.top = Math.max(8, (br.top - ar.top) / s - mh - 4) + "px";
      if (left + mw > 1430) menu.style.left = (1430 - mw) + "px";
      openMenu = menu;
    });
    btn.addEventListener("wheel", e => { e.preventDefault(); step(e.deltaY < 0 ? -1 : 1); }, { passive: false });
    hoverable(wrap, id);
    return wrap;
  }

  // ---- Pill toggle ------------------------------------------------------
  function toggle(id, label, opts = {}) {
    const b = el("button", "pill" + (opts.cls ? " " + opts.cls : ""), `<i></i><span>${label}</span>`);
    const draw = () => b.classList.toggle("on", !!Seer.get(id));
    Seer.on(id, draw); draw();
    b.addEventListener("click", () => { Seer.begin(id); Seer.set(id, Seer.get(id) ? 0 : 1); Seer.end(id); });
    hoverable(b, id);
    return b;
  }

  // ---- Segmented control ------------------------------------------------
  function segmented(id, label, names, opts = {}) {
    const p = Seer.param(id);
    const wrap = el("div", "seg-wrap" + (opts.cls ? " " + opts.cls : ""));
    const seg = el("div", "seg");
    const items = [];
    for (let v = p.min; v <= p.max; v++) {
      const it = el("button", "seg-it", names ? names[v - p.min] : fmt(id, v));
      it.addEventListener("click", () => { Seer.begin(id); Seer.set(id, v); Seer.end(id); });
      items.push(it); seg.append(it);
    }
    wrap.append(seg);
    if (label) wrap.append(el("div", "k-label", label));
    const draw = () => items.forEach((it, i) => it.classList.toggle("on", i + p.min === Seer.get(id)));
    Seer.on(id, draw); draw();
    hoverable(wrap, id);
    return wrap;
  }

  // ---- Horizontal bar (bipolar amount, used in the mod matrix) ----------
  function hbar(id, opts = {}) {
    const wrap = el("div", "hbar" + (opts.cls ? " " + opts.cls : ""));
    const fill = el("div", "hb-fill"), mid = el("div", "hb-mid"), txt = el("div", "hb-txt");
    wrap.append(mid, fill, txt);
    const bip = isBipolar(id);
    const draw = () => {
      const n = Seer.norm(id);
      if (bip) {
        const l = Math.min(n, 0.5), r = Math.max(n, 0.5);
        fill.style.left = (l * 100) + "%"; fill.style.width = ((r - l) * 100) + "%";
      } else { fill.style.left = "0"; fill.style.width = (n * 100) + "%"; }
      txt.textContent = fmt(id, Seer.get(id));
    };
    Seer.on(id, draw); draw();
    dragNorm(wrap, id, { horizontal: true }); hoverable(wrap, id);
    return wrap;
  }

  window.UI = { el, knob, valueBox, select, toggle, segmented, hbar, fmt, isBipolar, onTouch: fn => touchListeners.push(fn) };
})();
