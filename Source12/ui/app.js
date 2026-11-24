// Seer12 layout. Builds the four pages and wires every display to its params.
(function () {
  const { el, knob, valueBox, select, toggle, segmented, hbar } = UI;
  const css = getComputedStyle(document.documentElement);
  const C = k => css.getPropertyValue("--" + k).trim();

  // ---------- helpers ----------
  function card(title, acc, opts = {}) {
    const c = el("div", "card" + (opts.cls ? " " + opts.cls : ""));
    c.style.setProperty("--acc", C(acc));
    if (title !== null) {
      const h = el("div", "c-hdr");
      h.append(el("div", "c-title", title));
      c.append(h);
      c.hdr = h;
    }
    return c;
  }
  function ctrls(...kids) { const d = el("div", "ctrls"); d.append(...kids); return d; }
  function well(cls) { const w = el("div", "well " + (cls || "")); const s = document.createElementNS("http://www.w3.org/2000/svg", "svg"); w.append(s); w.svg = s; return w; }
  function watch(ids, fn) { ids.forEach(id => Seer.on(id, fn)); redrawers.push(fn); fn(); }
  const N = id => Seer.norm(id), V = id => Seer.get(id);

  // Card with internal tabs (ENV / LFO): returns {card, bodies[]}
  function tabbedCard(title, acc, names) {
    const c = card(title, acc);
    const tabs = el("div", "c-tabs"), bodies = [];
    names.forEach((n, i) => {
      const t = el("button", "c-tab" + (i ? "" : " on"), n);
      const b = el("div", "tabbody" + (i ? "" : " on"));
      t.addEventListener("click", () => {
        tabs.querySelectorAll(".c-tab").forEach(x => x.classList.remove("on"));
        bodies.forEach(x => x.classList.remove("on"));
        t.classList.add("on"); b.classList.add("on");
        b.dispatchEvent(new Event("shown"));
      });
      tabs.append(t); bodies.push(b);
    });
    c.hdr.append(tabs);
    bodies.forEach(b => c.append(b));
    return { card: c, bodies };
  }

  // ---------- header ----------
  const app = document.getElementById("app");
  const hdr = el("div", "hdr");
  const logo = el("div", "logo", `<div class="logo-mark"></div>SEER<b>12</b>`);
  const tabs = el("div", "tabs");
  // Preset strip: shows the hardware's current patch; while you touch a
  // control it shows that parameter instead, then falls back to the patch.
  const lcd = el("div", "lcd", `<button class="lcd-arrow" title="Previous program">\u2039</button>` +
    `<div class="lcd-body"><span class="lcd-sec"></span><span class="lcd-name"></span><span class="lcd-val"></span></div>` +
    `<button class="lcd-arrow" title="Next program">\u203a</button>`);
  const [prevBtn, nextBtn] = lcd.querySelectorAll(".lcd-arrow");
  prevBtn.addEventListener("click", () => Seer.command("program", { delta: -1 }));
  nextBtn.addEventListener("click", () => Seer.command("program", { delta: 1 }));
  const syncBtn = el("button", "hbtn sync", `<i></i>SYNC`);
  syncBtn.title = "Pull the full preset from the hardware";
  syncBtn.addEventListener("click", () => { syncBtn.classList.add("busy"); Seer.command("sync"); setTimeout(() => syncBtn.classList.remove("busy"), 1500); });
  const gearBtn = el("button", "hbtn gear", `<svg viewBox="0 0 24 24" width="16" height="16"><path fill="none" stroke="currentColor" stroke-width="1.7" d="M12 15.2a3.2 3.2 0 1 0 0-6.4 3.2 3.2 0 0 0 0 6.4Z"/><path fill="none" stroke="currentColor" stroke-width="1.7" stroke-linejoin="round" d="M19.4 13.5a7.6 7.6 0 0 0 0-3l2-1.6-2-3.4-2.4 1a7.7 7.7 0 0 0-2.6-1.5L14 2.5h-4l-.4 2.5A7.7 7.7 0 0 0 7 6.5l-2.4-1-2 3.4 2 1.6a7.6 7.6 0 0 0 0 3l-2 1.6 2 3.4 2.4-1a7.7 7.7 0 0 0 2.6 1.5l.4 2.5h4l.4-2.5a7.7 7.7 0 0 0 2.6-1.5l2.4 1 2-3.4-2-1.6Z"/></svg>`);
  gearBtn.title = "Settings: MIDI link, monitor, SysEx tools";
  gearBtn.addEventListener("click", () => { gearBtn.classList.toggle("on"); Seer.command("settings"); });
  hdr.append(logo, tabs, lcd, syncBtn, gearBtn);
  if (!Seer.isPlugin) hdr.append(el("div", "badge", "DEMO"));
  app.append(hdr);

  const secTitle = Object.fromEntries(window.SEER_SECTIONS.map(s => [s.id, s.title]));
  const lcdSec = lcd.querySelector(".lcd-sec"), lcdName = lcd.querySelector(".lcd-name"), lcdVal = lcd.querySelector(".lcd-val");
  let lcdTimer = null;
  function showPatch() {
    const s = Seer.status;
    lcd.classList.remove("param");
    lcdSec.textContent = "PROGRAM";
    lcdName.textContent = s.patch || "No preset synced";
    lcdVal.textContent = "";
    syncBtn.title = (s.link ? s.link + " \u2014 " : "") + "pull the full preset from the hardware";
  }
  Seer.onStatus(() => { if (!lcd.classList.contains("param")) showPatch(); });
  UI.onTouch(id => {
    const p = Seer.param(id);
    lcd.classList.add("param");
    lcdSec.textContent = (secTitle[p.section] || "").toUpperCase();
    lcdName.textContent = p.name;
    lcdVal.textContent = UI.fmt(id, Seer.get(id));
    clearTimeout(lcdTimer);
    lcdTimer = setTimeout(showPatch, 2200);
  });

  const pages = {};
  // Graphics read their element's pixel size, so redraw after a page/tab is shown.
  const redrawers = [];
  function redrawAll() { requestAnimationFrame(() => redrawers.forEach(fn => fn())); }
  [["VOICE", "p-voice"], ["MATRIX", "p-matrix"], ["FX", "p-fx"], ["ARP", "p-arp"]].forEach(([name, cls], i) => {
    const pg = el("div", "page " + cls + (i ? "" : " on"));
    const t = el("button", "tab" + (i ? "" : " on"), name === "ARP" ? "ARP · GLOBAL" : name);
    t.addEventListener("click", () => {
      Object.values(pages).forEach(p => { p.page.classList.remove("on"); p.tab.classList.remove("on"); });
      pg.classList.add("on"); t.classList.add("on");
      redrawAll();
    });
    tabs.append(t); app.append(pg);
    pages[name] = { page: pg, tab: t };
  });

  // =====================================================================
  // VOICE
  // =====================================================================
  const voice = pages.VOICE.page;
  const left = el("div", "col"), right = el("div", "col");
  voice.append(left, right);

  // ---- oscillators ----
  for (let n = 1; n <= 4; n++) {
    const o = id => `osc_${n}_${id}`;
    const c = card(null, "osc", { cls: "osc" });
    const tag = el("div", "osc-tag", `OSC <b>${n}</b>`);

    const disp = el("div", "osc-disp");
    const w = well();
    const shapeSel = select(o("shape"), null, { cls: "overlay" });
    w.append(shapeSel);
    disp.append(w);
    const waves = el("div", "osc-waves");
    waves.append(select(o("wave_left"), null, { prefix: "L" }), select(o("wave_right"), null, { prefix: "R" }));
    disp.append(waves);
    watch([o("shape"), o("shape_mod"), o("level"), o("wave_left"), o("wave_right")], () =>
      {
        const sh = V(o("shape")), wt = sh >= 5 && sh <= 16;
        waves.classList.toggle("inactive", !wt);
        G.osc(w.svg, { shape: sh, mod: N(o("shape_mod")), modRaw: V(o("shape_mod")), level: N(o("level")), waveL: V(o("wave_left")), waveR: V(o("wave_right")), color: C("trace") });
      });

    const k = el("div", "osc-ctrls");
    const tune = el("div", "stack");
    tune.append(valueBox(o("pitch"), "Pitch"), valueBox(o("fine_tune"), "Fine"));
    const ok = { size: 42 };
    k.append(tune, el("div", "vsep"),
      knob(o("level"), "Level", ok), knob(o("shape_mod"), "Shape mod", ok),
      knob(o("fm"), "FM", ok), knob(o("am"), "AM", ok), knob(o("slop"), "Slop", ok), knob(o("glide_amount"), "Glide", ok));
    if (n === 1) k.append(knob("osc_1_sub_octave", "Sub oct", ok));
    const tg = el("div", "osc-tog");
    tg.append(toggle(o("sync"), "SYNC"), toggle(o("key_follow"), "KEY TRK"), toggle(o("wave_reset"), "RESET"));
    k.append(el("div", "vsep"), tg);

    c.append(tag, disp, k);
    left.append(c);
  }

  // ---- character / feedback / output ----
  const r1 = el("div", "row");
  const ch = card("CHARACTER", "flt");
  const chk = ctrls(knob("air", "Air"), knob("girth", "Girth"), knob("hack", "Hack"), knob("decimate", "Decimate"), knob("drive", "Drive"));
  chk.classList.add("spread"); chk.style.padding = "0 14px"; ch.append(chk);
  const fb = card("FEEDBACK", "flt");
  fb.append(ctrls(knob("feedback_amount", "Amount"), knob("feedback_tuning", "Tuning")));
  const out = card("OUTPUT", "flt");
  out.append(ctrls(knob("voice_volume", "Volume"), knob("pan_spread", "Pan sprd"), knob("distortion_amount", "Distort")));
  ch.style.flex = "1"; out.style.flex = "none";
  r1.append(ch, fb, out);
  left.append(r1);

  // ---- filters ----
  const r2 = el("div", "row");
  function filterCard(title, type, ids) {
    const c = card(title, "flt"); c.style.flex = "1";
    const body = el("div", "flt");
    const w = well();
    const k = ctrls(knob(ids.f, "Cutoff", { size: 48 }), knob(ids.r, "Reso", { size: 48 }), knob(ids.k, "Key amt", { size: 40 }));
    if (ids.pole) { const t = toggle(ids.pole, "24 dB"); t.style.marginLeft = "auto"; c.hdr.append(t); }
    k.classList.add("spread"); k.style.flex = "1"; k.style.flexWrap = "nowrap";
    body.append(w, k); c.append(body);
    watch([ids.f, ids.r].concat(ids.pole ? [ids.pole] : []), () =>
      G.filter(w.svg, { type, freq: N(ids.f), res: N(ids.r), poles: ids.pole ? (V(ids.pole) ? 4 : 2) : 2, color: C("trace") }));
    return c;
  }
  r2.append(
    filterCard("LOW-PASS", "lp", { f: "lpf_frequency", r: "lpf_resonance", k: "lpf_key_amount", pole: "lpf_2_4_pole" }),
    filterCard("HIGH-PASS", "hp", { f: "hpf_frequency", r: "hpf_resonance", k: "hpf_key_amount" }));
  left.append(r2);

  // ---- envelopes ----
  const ENVS = [
    { tab: "FILTER", amt: "lpf_envelope_amt", vel: "velocity_to_lpf_env_amt", p: "lpf_env_" },
    { tab: "AMP", amt: "vca_envelope_amt", vel: "velocity_to_vca_env_amt", p: "vca_env_" },
    { tab: "ENV 3", amt: "envelope_3_amt", vel: "velocity_to_env_3_amt", p: "envelope_3_", dest: "envelope_3_destination" },
    { tab: "ENV 4", amt: "envelope_4_amt", vel: "velocity_to_env_4_amt", p: "envelope_4_", dest: "envelope_4_destination" },
  ];
  const envT = tabbedCard("ENVELOPE", "mod", ENVS.map(e => e.tab));
  ENVS.forEach((e, i) => {
    const b = envT.bodies[i], w = well("env-disp");
    const id = s => e.p + s;
    const ks = { size: 38 };
    const k = ctrls(knob(e.amt, "Amount", ks), knob(e.vel, "Velocity", ks), el("div", "vsep"),
      knob(id("delay"), "Delay", ks), knob(id("attack"), "Attack", ks), knob(id("decay"), "Decay", ks), knob(id("sustain"), "Sustain", ks), knob(id("release"), "Release", ks));
    k.classList.add("spread");
    const foot = ctrls(toggle(id("repeat"), "REPEAT"));
    foot.style.marginTop = "8px"; foot.style.alignItems = "center";
    if (e.dest) { const s = select(e.dest, null); s.style.flex = "1"; foot.append(el("div", "k-label", "DEST"), s); }
    b.append(w, k, foot);
    b.addEventListener("shown", () => draw());
    const draw = () => G.env(w.svg, { d: N(id("delay")), a: N(id("attack")), dec: N(id("decay")), s: N(id("sustain")), r: N(id("release")), repeat: V(id("repeat")), color: C("trace") });
    watch([id("delay"), id("attack"), id("decay"), id("sustain"), id("release"), id("repeat")], draw);
  });
  right.append(envT.card);

  // ---- LFOs ----
  const lfoT = tabbedCard("LFO", "lfo", ["LFO 1", "LFO 2", "LFO 3", "LFO 4"]);
  for (let n = 1; n <= 4; n++) {
    const id = s => `lfo_${n}_${s}`, b = lfoT.bodies[n - 1], w = well("lfo-disp");
    const top = ctrls(select(id("shape"), "Shape"), toggle(id("sync"), "SYNC"), select(id("sync_setting"), "Sync rate"), toggle(id("wave_reset"), "RESET"));
    top.style.alignItems = "flex-start"; top.style.marginBottom = "6px";
    top.children[0].style.width = "130px"; top.children[2].style.width = "120px";
    const k = ctrls(knob(id("frequency"), "Rate"), knob(id("amount"), "Amount"), knob(id("slew_rate"), "Slew"), knob(id("phase"), "Phase"));
    const dest = select(id("destination"), "Destination"); dest.style.flex = "1"; dest.style.marginLeft = "8px"; dest.style.alignSelf = "center";
    k.append(dest);
    b.append(w, top, k);
    const lfoDraw = () => {
      const syncOn = !!V(id("sync"));
      b.querySelector(".knob").style.opacity = syncOn ? 0.35 : 1;
      // Draw by label, not index: LFO 1's shape param only has 5 choices
      // (index 4 = random) while LFO 2-4 use the hardware's 0-7 numbering.
      const lbl = UI.fmt(id("shape"), V(id("shape"))).toLowerCase();
      const shapeIdx = ["triangle", "rev", "sawtooth", "square", "pulse 1", "pulse 2", "pulse 3", "random"].findIndex(k => lbl.startsWith(k));
      G.lfo(w.svg, { shape: shapeIdx < 0 ? V(id("shape")) : shapeIdx, rate: syncOn ? N(id("sync_setting")) : N(id("frequency")), amount: N(id("amount")),
        slew: N(id("slew_rate")), phase: N(id("phase")), color: C("trace") });
    };
    watch([id("shape"), id("frequency"), id("amount"), id("slew_rate"), id("phase"), id("sync"), id("sync_setting")], lfoDraw);
    b.addEventListener("shown", lfoDraw);
  }
  right.append(lfoT.card);

  // ---- unison / glide ----
  const r3 = el("div", "row");
  const un = card("UNISON", "osc"); un.style.flex = "1.4";
  const unC = ctrls(knob("unison_detune", "Detune"));
  const unS = el("div", "stack"); unS.append(toggle("unison", "ON"), toggle("unison_mode", "MODE"));
  const ka = select("unison_key_assign", "Key assign"); ka.style.flex = "1"; ka.style.alignSelf = "center";
  unC.append(unS, ka); unC.style.alignItems = "center"; un.append(unC);
  const gl = card("GLIDE", "osc"); gl.style.flex = "1";
  const glC = ctrls(toggle("glide", "ON")); glC.style.alignItems = "center";
  const gm = select("glide_mode", "Mode"); gm.style.flex = "1"; glC.append(gm); gl.append(glC);
  r3.append(un, gl);
  right.append(r3);

  // =====================================================================
  // MATRIX
  // =====================================================================
  const mx = pages.MATRIX.page;
  const mxC = card("MOD MATRIX", "mod");
  mxC.append(el("div", "mx-legend", "<span>#</span><span>SOURCE</span><span>AMOUNT</span><span></span><span>DESTINATION</span>"));
  const summary = el("div", "mx-summary");
  function refreshSummary() {
    const act = [];
    for (let s = 1; s <= 16; s++) if (V(`mod_${s}_source`) > 0 && V(`mod_${s}_amount`) !== 127) act.push(s);
    summary.innerHTML = act.length ? act.map(s => `<div class="mx-sum-row"><b>${String(s).padStart(2, "0")}</b>` +
      `<span>${UI.fmt(`mod_${s}_source`, V(`mod_${s}_source`))}</span><i>${UI.fmt(`mod_${s}_amount`, V(`mod_${s}_amount`))}</i>` +
      `<span>${UI.fmt(`mod_${s}_destination`, V(`mod_${s}_destination`))}</span></div>`).join("")
      : `<div class="note">No active slots. A slot is active when it has a source and a non-zero amount.</div>`;
  }
  for (let s = 1; s <= 16; s++) {
    const row = el("div", "mx-row");
    row.append(el("div", "mx-num", String(s).padStart(2, "0")), select(`mod_${s}_source`, null), hbar(`mod_${s}_amount`),
      el("div", "mx-arrow", "→"), select(`mod_${s}_destination`, null));
    watch([`mod_${s}_source`, `mod_${s}_amount`, `mod_${s}_destination`], () => {
      row.classList.toggle("active", V(`mod_${s}_source`) > 0 && V(`mod_${s}_amount`) !== 127);
      refreshSummary();
    });
    mxC.append(row);
  }
  mx.append(mxC);
  const mxRight = el("div", "col");
  const direct = card("DIRECT ROUTING", "lfo");
  [["LFO 1", "lfo_1_destination"], ["LFO 2", "lfo_2_destination"], ["LFO 3", "lfo_3_destination"], ["LFO 4", "lfo_4_destination"],
   ["ENV 3", "envelope_3_destination"], ["ENV 4", "envelope_4_destination"]].forEach(([n, id]) => {
    const r = el("div", "direct-row"); r.append(el("span", "", n), select(id, null)); direct.append(r);
  });
  direct.append(el("div", "note", "Each LFO and the two auxiliary envelopes also have their own destination, in addition to the 16 matrix slots."));
  const sumC = card("ACTIVE ROUTES", "mod"); sumC.style.flex = "1"; sumC.append(summary);
  mxRight.append(direct, sumC);
  mx.append(mxRight);

  // =====================================================================
  // FX
  // =====================================================================
  const fx = pages.FX.page;
  for (let n = 1; n <= 4; n++) {
    const id = s => `delay_${n}_${s}`;
    const c = card(`DELAY ${n}`, "fx", { cls: "flexcard" }), w = well("dl-disp");
    const row1 = ctrls(knob(id("time"), "Time", { size: 52 }), knob(id("amount"), "Amount", { size: 52 }), knob(id("feedback"), "Feedback", { size: 52 }));
    row1.classList.add("spread");
    const row2 = ctrls(knob(id("lowpass_filter"), "Low-pass"), knob(id("highpass_filter"), "High-pass"), knob(id("pan"), "Pan"));
    row2.classList.add("spread"); row2.style.marginTop = "6px";
    const row3 = ctrls(toggle(id("sync"), "SYNC"), toggle(id("filter_mode"), "FILTER MODE"));
    row3.style.margin = "10px 0 8px";
    const sync = select(id("sync_setting"), "Sync rate");
    c.append(w, row1, row2, row3, sync);
    watch([id("time"), id("amount"), id("feedback"), id("sync"), id("sync_setting")], () => {
      const syncOn = !!V(id("sync"));
      row1.children[0].style.opacity = syncOn ? 0.35 : 1;
      G.delay(w.svg, { time: syncOn ? N(id("sync_setting")) : N(id("time")), amount: N(id("amount")), feedback: N(id("feedback")), color: C("trace") });
    });
    fx.append(c);
  }
  const fxCh = card("CHARACTER · DRIVE", "flt", { cls: "span4" });
  const fxk = ctrls(knob("air", "Air", { size: 52 }), knob("girth", "Girth", { size: 52 }), knob("hack", "Hack", { size: 52 }), knob("decimate", "Decimate", { size: 52 }),
    knob("drive", "Drive", { size: 52 }), el("div", "vsep"), knob("distortion_amount", "Distortion", { size: 52 }), knob("feedback_amount", "Feedback", { size: 52 }), knob("feedback_tuning", "Fb tuning", { size: 52 }));
  fxk.style.gap = "26px"; fxk.style.justifyContent = "center"; fxk.style.paddingTop = "18px";
  fxCh.append(fxk); fx.append(fxCh);

  // =====================================================================
  // ARP / GLOBAL
  // =====================================================================
  const ap = pages.ARP.page;
  const arpC = card("ARPEGGIATOR", "arp", { cls: "flexcard" });
  const aw = well("arp-disp");
  const fr1 = el("div", "field-row");
  fr1.append(toggle("arpeggiator", "ARP ON"), segmented("arpeggiator_mode", "Mode"), segmented("arpeggiator_range", "Range", ["1 OCT", "2 OCT", "3 OCT"]),
    segmented("arpeggiator_repeats", "Repeats", ["×1", "×2", "×3", "×4"]));
  const fr2 = el("div", "field-row");
  const cd = select("arp_clock_divide", "Clock divide"); cd.style.width = "220px";
  fr2.append(cd, toggle("arp_auto_latch", "AUTO LATCH"), toggle("arp_lock_on_off", "LOCK"));
  fr2.style.alignItems = "center";
  arpC.append(aw, fr1, fr2);
  watch(["arpeggiator", "arpeggiator_mode", "arpeggiator_range", "arpeggiator_repeats"], () =>
    G.arp(aw.svg, { on: V("arpeggiator"), mode: V("arpeggiator_mode"), range: V("arpeggiator_range"), repeats: V("arpeggiator_repeats"), color: C("trace") }));
  ap.append(arpC);

  const glob = card("GLOBAL · LAYERS", "osc");
  const g1 = el("div", "field-row"); g1.append(segmented("a_b_mode", "Layer mode", ["NORMAL", "SPLIT", "STACK"]));
  g1.firstChild.style.flex = "1";
  const g2 = el("div", "field-row"); g2.append(valueBox("split_point", "Split point"), valueBox("pitchbend_range_up", "Bend up"), valueBox("pitchbend_range_down", "Bend down"));
  g2.style.justifyContent = "space-between";
  glob.append(g1, g2);
  const perf = card("PERFORMANCE", "osc");
  const p1 = el("div", "field-row");
  p1.append(toggle("unison", "UNISON"), toggle("unison_mode", "MODE"), knob("unison_detune", "Detune"));
  const ka2 = select("unison_key_assign", "Key assign"); ka2.style.flex = "1"; p1.append(ka2);
  p1.style.alignItems = "center";
  const p2 = el("div", "field-row"); p2.style.alignItems = "center";
  const gm2 = select("glide_mode", "Glide mode"); gm2.style.flex = "1";
  p2.append(toggle("glide", "GLIDE"), gm2);
  perf.append(p1, p2);
  const apRight = el("div", "col"); apRight.append(glob, perf);
  ap.append(apRight);

  // =====================================================================
  // scaling: the UI is designed at 1440x880 and scaled to fit the window
  // =====================================================================
  function fit() {
    const s = Math.min(window.innerWidth / 1440, window.innerHeight / 880);
    window.SEER_SCALE = s;
    app.style.transform = `translate(${(window.innerWidth - 1440 * s) / 2}px, ${(window.innerHeight - 880 * s) / 2}px) scale(${s})`;
  }
  window.addEventListener("resize", fit); fit();
  redrawAll();

  // Shortcut: Ctrl+1..4 switches pages.
  window.addEventListener("keydown", e => {
    if ((e.ctrlKey || e.metaKey) && e.key >= "1" && e.key <= "4") { Object.values(pages)[+e.key - 1].tab.click(); e.preventDefault(); }
  });
})();
