// Seer12 <-> plugin bridge.
//
// Inside the plugin, JUCE 8's WebBrowserComponent injects window.__JUCE__.
// We use only its two low-level primitives:
//   window.__JUCE__.backend.emitEvent(id, payload)      JS  -> C++
//   window.__JUCE__.backend.addEventListener(id, fn)    C++ -> JS
// Events (see PluginEditor.cpp):
//   JS -> C++  "seer:ready"   {}                     ask for a full snapshot
//              "seer:set"     {id, value}            value = real (unnormalised) value
//              "seer:gesture" {id, begin: bool}      host automation gestures
//              "seer:program" {delta: -1|1}          previous / next program on the hardware
//              "seer:sync"    {}                     pull a full edit-buffer dump (Sync Now)
//              "seer:settings" {}                    toggle the native settings drawer
//   C++ -> JS  "seer:params"  {id: value, ...}       changed values (or full snapshot)
//              "seer:status"  {patch, link, synced, seq}   preset name / MIDI link / last sync
//
// Opened in a normal browser (no __JUCE__), it runs in DEMO mode with local
// state so the UI can be designed and tested without building the plugin.
(function () {
  const P = window.SEER_PARAMS;
  const byId = Object.fromEntries(P.map(p => [p.id, p]));
  const values = {};
  const listeners = {};
  const anyListeners = [];
  const statusListeners = [];

  const juce = () => window.__JUCE__ && window.__JUCE__.backend;
  const isPlugin = !!juce();

  function clamp(p, v) { return Math.min(p.max, Math.max(p.min, Math.round(v))); }

  function notify(id, v, fromHost) {
    (listeners[id] || []).forEach(fn => fn(v, fromHost));
    anyListeners.forEach(fn => fn(id, v, fromHost));
  }

  function apply(id, v, fromHost) {
    const p = byId[id];
    if (!p) return;
    v = clamp(p, v);
    if (values[id] === v) return;
    values[id] = v;
    notify(id, v, fromHost);
  }

  // Sensible-looking defaults for demo mode (and before the first snapshot).
  function demoDefault(p) {
    const r = p.max - p.min;
    if (/^mod_\d+_source$/.test(p.id)) return { mod_1_source: 3, mod_2_source: 7, mod_3_source: 12 }[p.id] || 0;
    if (/^mod_\d+_amount$/.test(p.id)) return { mod_1_amount: 170, mod_2_amount: 90, mod_3_amount: 220 }[p.id] || 127;
    if (/^mod_\d+_destination$/.test(p.id)) return { mod_1_destination: 10, mod_2_destination: 3, mod_3_destination: 41 }[p.id] || 0;
    if (/_sustain$/.test(p.id)) return Math.round(p.min + r * 0.6);
    if (/_attack$/.test(p.id)) return Math.round(r * 0.12);
    if (/_decay$/.test(p.id)) return Math.round(r * 0.35);
    if (/_release$/.test(p.id)) return Math.round(r * 0.4);
    if (/_delay$/.test(p.id)) return 0;
    if (/_pitch$/.test(p.id)) return 48;
    if (/fine_tune$/.test(p.id)) return 50;
    if (/^osc_[1-3]_level$/.test(p.id)) return { osc_1_level: 110, osc_2_level: 96, osc_3_level: 80 }[p.id];
    if (p.id === "osc_3_wave_right") return 9;
    if (/^osc_\d_shape$/.test(p.id)) return { osc_1_shape: 1, osc_2_shape: 2, osc_3_shape: 7, osc_4_shape: 0 }[p.id];
    if (/shape_mod$/.test(p.id)) return 64;
    if (p.id === "lpf_frequency") return 92;
    if (p.id === "lpf_resonance") return 48;
    if (p.id === "lpf_2_4_pole") return 1;
    if (p.id === "hpf_frequency") return 14;
    if (p.id === "voice_volume") return 100;
    if (/^lfo_\d_frequency$/.test(p.id)) return 90;
    if (/^lfo_\d_amount$/.test(p.id)) return 70;
    if (/^lfo_\d_shape$/.test(p.id)) return { lfo_1_shape: 0, lfo_2_shape: 2, lfo_3_shape: 4, lfo_4_shape: 7 }[p.id];
    if (/(amt|amount)$/.test(p.id) && r >= 254) return Math.round(r / 2);  // bipolar centre
    if (/pan$|pan_spread/.test(p.id)) return 64;
    if (/^delay_\d_(amount|feedback)$/.test(p.id)) return 50;
    if (/^delay_\d_time$/.test(p.id)) return 120;
    if (/^delay_\d_lowpass_filter$/.test(p.id)) return 100;
    if (p.id === "arpeggiator") return 1;
    if (p.id === "arpeggiator_range") return 1;
    return p.min;
  }
  P.forEach(p => { values[p.id] = demoDefault(p); });

  window.Seer = {
    isPlugin,
    params: P,
    param: id => byId[id],
    get: id => values[id],
    norm: id => { const p = byId[id]; return (values[id] - p.min) / ((p.max - p.min) || 1); },
    set(id, v) {
      const p = byId[id];
      if (!p) return;
      v = clamp(p, v);
      if (values[id] === v) return;
      apply(id, v, false);
      if (isPlugin) juce().emitEvent("seer:set", { id, value: v });
    },
    setNorm(id, n) { const p = byId[id]; this.set(id, p.min + n * (p.max - p.min)); },
    begin(id) { if (isPlugin) juce().emitEvent("seer:gesture", { id, begin: true }); },
    end(id) { if (isPlugin) juce().emitEvent("seer:gesture", { id, begin: false }); },
    on(id, fn) { (listeners[id] = listeners[id] || []).push(fn); },
    onAny(fn) { anyListeners.push(fn); },
    listenerCount: id => (listeners[id] || []).length,  // used by tests
    // Hardware / host commands (no-ops in demo mode).
    command(name, payload) { if (isPlugin) juce().emitEvent("seer:" + name, payload || {}); },
    status: { patch: isPlugin ? "" : "Demo patch", link: isPlugin ? "" : "Demo mode (no plugin)", synced: 0, seq: 0 },
    onStatus(fn) { statusListeners.push(fn); fn(this.status); },
  };

  if (isPlugin) {
    juce().addEventListener("seer:params", obj => {
      for (const id in obj) apply(id, Number(obj[id]), true);
    });
    juce().addEventListener("seer:status", s => {
      Object.assign(window.Seer.status, s);
      statusListeners.forEach(fn => fn(window.Seer.status));
    });
    juce().emitEvent("seer:ready", {});
  }
})();
