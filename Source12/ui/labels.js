// Display labels for parameter values.
// Enum params carry their own option names (and categories) in params.js,
// generated from Parameters.cpp -- those are always used as-is.
// A few params are plain ints in the plugin but are lists on the hardware
// (OSC 2-4 shape/waves, OSC 1 Wave R, LFO 2-4 shape); they borrow the names
// below. Only what is SHOWN changes -- the value sent is the raw integer.
(function () {
  const byId = Object.fromEntries(window.SEER_PARAMS.map(p => [p.id, p]));
  const opts = id => (byId[id] && byId[id].options) || [];

  // LFO shape values 0-7, from the P12 Module manual's NRPN table.
  const LFO_SHAPES = ["Triangle", "Rev saw", "Sawtooth", "Square", "Pulse 1", "Pulse 2", "Pulse 3", "Random"];

  const borrowed = [
    [/^osc_\d_shape$/, () => opts("osc_1_shape")],
    [/^osc_\d_wave_(left|right)$/, () => opts("osc_1_wave_left")],
    [/^lfo_\d_shape$/, () => LFO_SHAPES],
  ];

  window.SEER_LIST_FOR = function (id) {
    const p = byId[id];
    if (p && p.options) return { list: p.options, categories: p.categories || null };
    for (const [re, get] of borrowed)
      if (re.test(id)) return { list: get(), categories: null };
    return null;
  };

  window.SEER_LABEL = function (id, value) {
    const l = window.SEER_LIST_FOR(id);
    const v = Math.round(value);
    if (!l) return String(v);
    return l.list[v] !== undefined ? l.list[v] : "#" + (v + 1);
  };
})();
