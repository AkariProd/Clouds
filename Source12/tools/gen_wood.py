"""Generate ui/wood.png -- a procedural walnut texture for the side cheeks.

Procedural (no downloaded/licensed image): growth rings + warped figure +
elongated pores, coloured like oiled walnut. Grain runs vertically, since the
cheeks are tall thin strips. Output is 2x the CSS size (16x880 -> 32x1760).

Run from Source12/:  python tools/gen_wood.py [seed]
"""
import sys, pathlib
import numpy as np
from scipy.ndimage import gaussian_filter
from PIL import Image

seed = int(sys.argv[1]) if len(sys.argv) > 1 else 12
rng = np.random.default_rng(seed)
root = pathlib.Path(__file__).resolve().parent.parent

SS = 3                    # supersampling factor (rendered big, then downscaled)
W, H = 140 * SS, 1760 * SS  # plank; a slice is cut out at the end
OUT_W, OUT_H = 32, 1760


def field(sx, sy, amp=1.0):
    """Smooth noise field, anisotropic (sy along the grain)."""
    f = gaussian_filter(rng.standard_normal((H, W)), sigma=(sy * SS, sx * SS), mode="wrap")
    f /= (np.abs(f).max() + 1e-9)
    return f * amp


v, u = np.mgrid[0:H, 0:W].astype(np.float64)

# Figure: low-frequency warping of the rings, stretched along the grain.
warp = field(18, 140, 9.0) + field(6, 40, 2.2) + field(2.5, 12, 0.5)

# Growth rings (slightly wavy, irregular spacing).
spacing = (9.0 + field(30, 300, 2.5)) * SS
r = (u + warp * 3.2 * SS) / spacing
ring = r - np.floor(r)
latewood = np.clip((ring - 0.55) / 0.45, 0, 1) ** 2.2 * (1 - np.clip((ring - 0.97) / 0.03, 0, 1) * 0.6)  # dark band, soft edges
earlywood_soft = np.exp(-((ring - 0.15) / 0.12) ** 2) * 0.25

# Pores: short dark streaks along the grain.
pores = gaussian_filter(rng.standard_normal((H, W)), sigma=(5.0 * SS, 0.5 * SS), mode="wrap")
pores /= np.abs(pores).max()
pores = np.clip((pores - 0.25) * 2.6, 0, 1) * (0.5 + latewood * 0.8)

# Fine fibre texture and broad colour drift.
fibre = gaussian_filter(rng.standard_normal((H, W)), sigma=(14 * SS, 0.6 * SS), mode="wrap")
fibre /= np.abs(fibre).max()
drift = field(60, 500, 1.0)

dark = np.clip(0.42 * latewood - earlywood_soft * 0.6 + 0.35 * pores + 0.10 * fibre + 0.12 * drift + 0.30, 0, 1)

light_col = np.array([128, 78, 42], dtype=np.float64)    # oiled walnut, light
dark_col = np.array([38, 19, 9], dtype=np.float64)       # latewood / pores
mid_col = np.array([86, 49, 24], dtype=np.float64)

t = dark[..., None]
col = np.where(t < 0.5, light_col + (mid_col - light_col) * (t / 0.5),
               mid_col + (dark_col - mid_col) * ((t - 0.5) / 0.5))

# Subtle chatoyance: lighter streaks that wander along the grain.
sheen = np.clip(field(4, 80, 1.0), 0, 1) ** 2 * 18
col = np.clip(col + sheen[..., None], 0, 255)

x0 = int(W * 0.37)
big = Image.fromarray(col[:, x0:x0 + OUT_W * SS].astype(np.uint8), "RGB")
img = big.resize((OUT_W, OUT_H), Image.LANCZOS)
out = root / "ui" / "wood.png"
img.save(out, optimize=True)
print("wrote", out, img.size, out.stat().st_size, "bytes")
