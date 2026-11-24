// Seer12 live graphics. Every display is an SVG redrawn from parameter values.
// Seer12 makes no audio, so these are previews computed from the settings
// (idealised shapes), not measurements of the hardware's output.
(function () {
  let gid = 0;
  const f1 = n => n.toFixed(1);

  // Use the element's real pixel size so nothing is stretched; fall back to
  // a default while hidden (e.g. inactive tab) -- it is redrawn when shown.
  function size(svg, w, h) {
    const W = svg.clientWidth || w, H = svg.clientHeight || h;
    return [Math.round(W), Math.round(H)];
  }
  function frame(svg, w, h) {
    svg.setAttribute("viewBox", `0 0 ${w} ${h}`);
    svg.setAttribute("preserveAspectRatio", "none");
    if (!svg.dataset.g) svg.dataset.g = "g" + (++gid);
    return svg.dataset.g;
  }

  function grad(id, col, top = 0.35, bot = 0.0) {
    return `<defs><linearGradient id="${id}" x1="0" y1="0" x2="0" y2="1">` +
      `<stop offset="0" stop-color="${col}" stop-opacity="${top}"/>` +
      `<stop offset="1" stop-color="${col}" stop-opacity="${bot}"/></linearGradient></defs>`;
  }

  function grid(w, h, cols, rows) {
    let s = "";
    for (let i = 1; i < cols; i++) s += `<line class="g-grid" x1="${f1(w * i / cols)}" y1="0" x2="${f1(w * i / cols)}" y2="${h}"/>`;
    for (let i = 1; i < rows; i++) s += `<line class="g-grid" x1="0" y1="${f1(h * i / rows)}" x2="${w}" y2="${f1(h * i / rows)}"/>`;
    return s;
  }

  // Deterministic pseudo-random (for wavetable / noise previews).
  function rng(seed) { let s = seed * 9301 + 49297; return () => ((s = (s * 9301 + 49297) % 233280) / 233280); }

  // ---- oscillator waveform --------------------------------------------
  // shape: 0 off, 1 saw, 2 pulse, 3 tri, 4 sine, 5..16 wavetables, 17..19 noise
  function waveSample(shape, mod, ph, harmonics) {
    switch (shape) {
      case 1: return 1 - 2 * ph;
      case 2: { const pw = 0.5 + (mod - 0.5) * 0.9; return ph < pw ? 0.9 : -0.9; }
      case 3: return ph < 0.5 ? 4 * ph - 1 : 3 - 4 * ph;
      case 4: return Math.sin(ph * 2 * Math.PI);
      default:
        if (harmonics) {
          let y = 0, n = 0;
          harmonics.forEach((a, k) => { y += a * Math.sin(2 * Math.PI * (k + 1) * ph + k * 0.7); n += Math.abs(a); });
          return y / (n || 1) * 1.5;
        }
        return 0;
    }
  }

  // The 12 P12 wavetables. Sequential never published the wave data, so each
  // one is an additive recipe built to match its name / character:
  // [amplitude, phase] per harmonic, starting at the fundamental.
  const WAVETABLES = [
    /* 0 Tines   */ [[1, 0], [0.15, 0], [0, 0], [0.05, 0], [0, 0], [0, 0], [0.32, 0.5], [0, 0], [0, 0], [0, 0], [0, 0], [0, 0], [0, 0], [0.22, 1.2]],
    /* 1 Mellow  */ [[1, 0], [0.22, 1.6], [0.1, 0], [0.04, 1.6]],
    /* 2 Church  */ [[0.8, 0], [1, 0], [0.55, 0], [0.7, 0], [0, 0], [0.35, 0], [0, 0], [0.5, 0]],
    /* 3 Muted   */ [[1, 0], [0, 0], [0.42, 3.14], [0, 0], [0.12, 0], [0, 0], [0.04, 3.14]],
    /* 4 Nasal   */ [[0.35, 0], [0.5, 0.4], [0.85, 0.9], [1, 1.3], [0.75, 1.9], [0.4, 2.4], [0.2, 2.9]],
    /* 5 Boing   */ [[1, 0], [0.9, 1.57], [0.45, 0.2], [0.2, 2.6], [0.15, 0.8]],
    /* 6 Gothic  */ [[1, 0], [0, 0], [-0.55, 0], [0, 0], [0.42, 0], [0, 0], [-0.33, 0], [0, 0], [0.26, 0], [0, 0], [-0.2, 0]],
    /* 7 Ahhh    */ [[0.45, 0], [0.4, 0], [0.35, 0], [0.5, 0], [0.75, 0], [0.95, 0], [0.8, 0], [0.45, 0], [0.25, 0], [0.3, 0], [0.4, 0], [0.2, 0]],
    /* 8 Shrill  */ [[0.35, 0], [0.1, 0], [0.05, 0], [0, 0], [0, 0], [0.1, 0], [0.25, 0], [0.5, 0], [0.8, 0], [1, 0], [0.85, 0], [0.55, 0], [0.3, 0]],
    /* 9 Ohhhh   */ [[1, 0], [0.85, 0], [0.55, 0], [0.18, 0], [0.06, 0]],
    /* 10 Buzzzz */ [[1, 0], [0.71, 0], [0.58, 0], [0.5, 0], [0.45, 0], [0.41, 0], [0.38, 0], [0.35, 0], [0.33, 0], [0.32, 0], [0.3, 0], [0.29, 0], [0.28, 0], [0.27, 0], [0.26, 0], [0.25, 0], [0.24, 0], [0.24, 0]],
    /* 11 Meh    */ [[0.5, 0], [0.7, 0.3], [0.9, 0.6], [0.6, 0.9], [0.25, 0], [0.15, 0], [0.2, 0], [0.35, 1.1], [0.45, 1.4], [0.3, 1.7]],
  ];

  function tableSample(idx, ph) {
    const h = WAVETABLES[Math.max(0, Math.min(11, idx))];
    let y = 0, norm = 0;
    for (let k = 0; k < h.length; k++) {
      const [a, p] = h[k];
      if (!a) continue;
      y += a * Math.sin(2 * Math.PI * (k + 1) * ph + p);
      norm += Math.abs(a);
    }
    return norm ? y / norm : 0;
  }

  function osc(svg, o) {
    const [W, H] = size(svg, 240, 96), m = H / 2, g = frame(svg, W, H);
    const col = o.color;
    let body = grad(g, col, 0.16, 0.0) + `<line class="g-axis" x1="0" y1="${m}" x2="${W}" y2="${m}"/>`;
    const off = o.shape === 0 || o.level === 0;
    if (svg.parentNode) svg.parentNode.classList.toggle("is-off", off);
    if (off) { svg.innerHTML = body; return; }
    const amp = (H / 2 - 8) * (0.25 + 0.75 * o.level);
    let pts = [];
    if (o.shape >= 17) {
      const r = rng(o.shape), N = 160; let last = 0;
      for (let i = 0; i <= N; i++) {
        let v = r() * 2 - 1;
        if (o.shape === 17) v = last = last * 0.82 + v * 0.35;          // red: smoother
        if (o.shape === 19) { const t = v; v = t - last; last = t; v *= 0.6; } // violet: spikier
        pts.push([i / N * W, m - v * amp]);
      }
    } else {
      // Wavetables: Shape selects the base wave; Shape Mod is centred -- turning it
      // left crossfades toward Wave L, right toward Wave R (fully there at the ends).
      const N = 240, raw = [];
      const b = (o.modRaw !== undefined ? o.modRaw - 64 : (o.mod - 0.5) * 128) / 64;
      for (let i = 0; i <= N; i++) {
        const x = i / N, ph = (x * 2) % 1;
        let y;
        if (o.shape >= 5) {
          const base = tableSample(o.shape - 5, ph);
          const side = b < 0 ? tableSample(o.waveL || 0, ph) : tableSample(o.waveR || 0, ph);
          const t = Math.min(1, Math.abs(b));
          y = base * (1 - t) + side * t;
        } else y = waveSample(o.shape, o.mod, ph, null);
        raw.push(y);
      }
      const peak = o.shape >= 5 ? Math.max(1e-3, ...raw.map(Math.abs)) : 1;
      raw.forEach((y, i) => pts.push([i / N * W, m - (y / peak) * amp]));
    }
    const d = pts.map((p, i) => (i ? "L" : "M") + f1(p[0]) + " " + f1(p[1])).join("");
    body += `<path d="${d}L${W} ${m}L0 ${m}Z" fill="url(#${g})"/>`;
    body += `<path class="g-line" d="${d}" stroke="${col}"/>`;
    svg.innerHTML = body;
  }

  // ---- envelope (DADSR) ------------------------------------------------
  function env(svg, e) {
    const [W, H] = size(svg, 460, 176), top = 14, bot = H - 12, g = frame(svg, W, H);
    const col = e.color;
    const seg = W / 4.6;
    const x0 = 8, x1 = x0 + e.d * seg * 0.8, x2 = x1 + Math.max(2, e.a * seg), x3 = x2 + Math.max(2, e.dec * seg);
    const sy = bot - (bot - top) * e.s, x4 = x3 + seg * 0.75, x5 = x4 + Math.max(2, e.r * seg);
    const d = `M${x0} ${bot}L${f1(x1)} ${bot}` +
      `C${f1(x1 + (x2 - x1) * 0.35)} ${f1(top + (bot - top) * 0.2)} ${f1(x2 - (x2 - x1) * 0.1)} ${top} ${f1(x2)} ${top}` +
      `C${f1(x2 + (x3 - x2) * 0.25)} ${f1(sy)} ${f1(x2 + (x3 - x2) * 0.6)} ${f1(sy)} ${f1(x3)} ${f1(sy)}` +
      `L${f1(x4)} ${f1(sy)}` +
      `C${f1(x4 + (x5 - x4) * 0.25)} ${f1(bot)} ${f1(x4 + (x5 - x4) * 0.6)} ${bot} ${f1(x5)} ${bot}`;
    let s = grad(g, col, 0.18, 0.0) + grid(W, H, 8, 4);
    s += `<line class="g-axis" x1="0" y1="${bot}" x2="${W}" y2="${bot}"/>`;
    s += `<path d="${d}L${f1(x5)} ${bot}Z" fill="url(#${g})"/>`;
    s += `<path class="g-line" d="${d}" stroke="${col}"/>`;
    if (e.d > 0.01) s += `<circle class="g-node" cx="${f1(x1)}" cy="${bot}" r="4" stroke="${col}"/>`;
    [[x2, top], [x3, sy], [x4, sy], [x5, bot]].forEach(([x, y]) =>
      s += `<circle class="g-node" cx="${f1(x)}" cy="${f1(y)}" r="4.5" stroke="${col}"/>`);
    if (e.repeat) s += `<path class="g-loop" d="M${f1(x1)} ${bot + 6}Q${f1((x1 + x4) / 2)} ${bot + 14} ${f1(x4)} ${bot + 6}" stroke="${col}"/>`;
    svg.innerHTML = s;
  }

  // ---- LFO --------------------------------------------------------------
  function lfoSample(shape, ph, r) {
    switch (shape) {
      case 0: return ph < 0.5 ? 4 * ph - 1 : 3 - 4 * ph;
      case 1: return 2 * ph - 1;
      case 2: return 1 - 2 * ph;
      case 3: return ph < 0.5 ? 1 : -1;
      case 4: return ph < 0.3 ? 1 : -1;     // pulse widths are illustrative
      case 5: return ph < 0.18 ? 1 : -1;
      case 6: return ph < 0.08 ? 1 : -1;
      default: return r[Math.floor(ph * 8) % 8];
    }
  }

  function lfo(svg, l) {
    const [W, H] = size(svg, 460, 128), m = H / 2, g = frame(svg, W, H);
    const col = l.color, cycles = 1 + Math.round(l.rate * 3), amp = (H / 2 - 10) * (0.2 + 0.8 * l.amount);
    const r = []; const rr = rng(l.shape * 7 + 1); for (let i = 0; i < 8; i++) r.push(rr() * 2 - 1);
    const N = 360, slew = l.slew; let prev = null, pts = [];
    for (let i = 0; i <= N; i++) {
      const x = i / N, ph = ((x * cycles) + l.phase) % 1;
      let v = lfoSample(l.shape, ph, r);
      if (prev !== null && slew > 0) v = prev + (v - prev) * (1 - Math.pow(slew, 0.35) * 0.97);
      prev = v; pts.push([x * W, m - v * amp]);
    }
    const d = pts.map((p, i) => (i ? "L" : "M") + f1(p[0]) + " " + f1(p[1])).join("");
    svg.innerHTML = grad(g, col, 0.14, 0.0) + grid(W, H, 8, 4) +
      `<line class="g-axis" x1="0" y1="${m}" x2="${W}" y2="${m}"/>` +
      `<path d="${d}L${W} ${m}L0 ${m}Z" fill="url(#${g})"/>` +
      `<path class="g-line" d="${d}" stroke="${col}"/>`;
  }

  // ---- filter response --------------------------------------------------
  function filter(svg, f) {
    const [W, H] = size(svg, 210, 80), g = frame(svg, W, H);
    const col = f.color, dbTop = 18, dbBot = -42;
    const fc = Math.pow(10, 1.3 + f.freq * 3.0);   // ~20 Hz .. 20 kHz
    const Q = 0.6 + f.res * 9;
    const N = 200; let pts = [];
    for (let i = 0; i <= N; i++) {
      const fr = Math.pow(10, 1.3 + (i / N) * 3.0), w = fr / fc;
      const hp = f.type === "hp" ? w ** 4 : 1;
      const resonant = hp / ((1 - w * w) ** 2 + (w / Q) ** 2);       // 2-pole with resonance
      const butter = hp / ((1 - w * w) ** 2 + (w / 0.707) ** 2);     // extra 2 poles for 24 dB
      let db = 10 * Math.log10(resonant) + (f.poles === 4 ? 10 * Math.log10(butter) : 0);
      db = Math.max(dbBot - 6, Math.min(dbTop, db));
      pts.push([i / N * W, (dbTop - db) / (dbTop - dbBot) * H]);
    }
    const d = pts.map((p, i) => (i ? "L" : "M") + f1(p[0]) + " " + f1(p[1])).join("");
    const zero = dbTop / (dbTop - dbBot) * H;
    svg.innerHTML = grad(g, col, 0.16, 0.0) + grid(W, H, 6, 3) +
      `<line class="g-axis" x1="0" y1="${f1(zero)}" x2="${W}" y2="${f1(zero)}"/>` +
      `<path d="${d}L${W} ${H}L0 ${H}Z" fill="url(#${g})"/>` +
      `<path class="g-line" d="${d}" stroke="${col}"/>`;
  }

  // ---- delay taps -------------------------------------------------------
  function delay(svg, dl) {
    const [W, H] = size(svg, 320, 300), g = frame(svg, W, H);
    const col = dl.color, base = H - 6;
    let s = grad(g, col, 0.9, 0.25) + `<line class="g-axis" x1="0" y1="${base}" x2="${W}" y2="${base}"/>`;
    s += `<rect x="10" y="10" width="6" height="${base - 10}" rx="3" class="g-dry"/>`;
    const spacing = W * (0.05 + dl.time * 0.22);
    let lvl = dl.amount; const tops = [];
    for (let x = 10 + spacing, i = 0; x < W - 4 && i < 24 && lvl > 0.02; x += spacing, i++) {
      const h = (base - 10) * lvl;
      s += `<rect x="${f1(x)}" y="${f1(base - h)}" width="6" height="${f1(h)}" rx="3" fill="url(#${g})"/>`;
      tops.push([x + 3, base - h]);
      lvl *= Math.max(0.05, dl.feedback * 0.95);
    }
    if (tops.length > 1) s += `<path class="g-loop" stroke="${col}" d="${tops.map((p, i) => (i ? "L" : "M") + f1(p[0]) + " " + f1(p[1])).join("")}"/>`;
    svg.innerHTML = s;
  }

  // ---- arp pattern preview ---------------------------------------------
  function arp(svg, a) {
    const [W, H] = size(svg, 980, 600), g = frame(svg, W, H);
    const notes = [0, 4, 7], oct = a.range + 1, seq = [];
    for (let o = 0; o < oct; o++) notes.forEach(n => seq.push(n + o * 12));
    let pat;
    const r = rng(5);
    switch (a.mode) {
      case 0: pat = seq; break;
      case 1: pat = seq.slice().reverse(); break;
      case 2: pat = seq.concat(seq.slice(1, -1).reverse()); break;
      case 3: pat = seq; break;
      default: pat = seq.map(() => seq[Math.floor(r() * seq.length)]);
    }
    const steps = []; pat.forEach(n => { for (let k = 0; k <= a.repeats; k++) steps.push(n); });
    const total = Math.min(32, steps.length * 2), maxN = 12 * oct;
    let s = grid(W, H, 16, 4);
    const sw = W / total;
    for (let i = 0; i < total; i++) {
      const n = steps[i % steps.length], y = H - 24 - (n / maxN) * (H - 60);
      s += `<rect x="${f1(i * sw + 3)}" y="${f1(y - 7)}" width="${f1(sw - 6)}" height="14" rx="5" fill="${a.on ? a.color : "var(--dim2)"}" opacity="${a.on ? 0.95 - (i % steps.length) * 0.01 : 0.6}"/>`;
    }
    svg.innerHTML = s;
  }

  window.G = { osc, env, lfo, filter, delay, arp };
})();
