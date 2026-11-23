import json, re
from mod_tables import MOD_SOURCES, MOD_DESTINATIONS

# "LFO N Sync Setting" / "Delay N Sync Setting" are enums whose option list
# is spelled out ONLY on LFO1/Delay1's row, with "N>1" rows saying "(see
# LFO 1/Delay 1 Sync Setting)" -- the auto-extraction can't resolve that
# cross-reference, and even LFO1/Delay1's own inline list got truncated
# because it spans a PDF page break. Hand-transcribed from the manual's
# MIDI Implementation NRPN table (pages 81-84 in v1.2).
LFO_SYNC_SETTING_OPTIONS = [
    "32nd Triplet", "32nd", "16th Triplet", "16th", "8th Triplet", "8th",
    "Quarter Triplet", "Quarter", "Dotted Quarter", "Half", "3 Quarters",
    "4 Quarters", "6 Quarters", "8 Quarters", "16 Quarters", "32 Quarters",
]
DELAY_SYNC_SETTING_OPTIONS = [
    "64th", "32nd", "Dotted 32nd", "16th", "Dotted 16th", "8th", "Dotted 8th",
    "Quarter", "Dotted Quarter", "Half", "Dotted Half", "Whole",
]

entries = json.load(open('entries_final.json'))

SECTION_RULES = [
    (re.compile(r'^Osc 1'), 'OSC1'),
    (re.compile(r'^Osc 2'), 'OSC2'),
    (re.compile(r'^Osc 3'), 'OSC3'),
    (re.compile(r'^Osc 4'), 'OSC4'),
    (re.compile(r'^(Air|Girth|Hack|Decimate|Drive)$'), 'CHARACTER'),
    (re.compile(r'^(Feedback)'), 'FEEDBACK'),
    (re.compile(r'^(Voice Volume|Pan Spread|Distortion Amount)'), 'VCA_MISC'),
    (re.compile(r'^VCA Env'), 'VCA_ENV'),
    (re.compile(r'^Velocity to VCA'), 'VCA_ENV'),
    (re.compile(r'^LPF Env'), 'LPF_ENV'),
    (re.compile(r'^Velocity to LPF'), 'LPF_ENV'),
    (re.compile(r'^LPF'), 'LPF'),
    (re.compile(r'^HPF'), 'HPF'),
    (re.compile(r'^Env(elope)? ?3'), 'ENV3'),
    (re.compile(r'^Velocity to Env 3'), 'ENV3'),
    (re.compile(r'^Env(elope)? ?4'), 'ENV4'),
    (re.compile(r'^Velocity to Env 4'), 'ENV4'),
    (re.compile(r'^LFO 1'), 'LFO1'),
    (re.compile(r'^LFO 2'), 'LFO2'),
    (re.compile(r'^LFO 3'), 'LFO3'),
    (re.compile(r'^LFO 4'), 'LFO4'),
    (re.compile(r'^Delay 1'), 'DELAY1'),
    (re.compile(r'^Delay 2'), 'DELAY2'),
    (re.compile(r'^Delay 3'), 'DELAY3'),
    (re.compile(r'^Delay 4'), 'DELAY4'),
    (re.compile(r'^Mod \d+'), 'MODMATRIX'),
    (re.compile(r'^Unison'), 'UNISON'),
    (re.compile(r'^(Split Point|A/B Mode)'), 'GLOBAL'),
    (re.compile(r'^Arp'), 'ARP'),
    (re.compile(r'^Arpeggiator'), 'ARP'),
    (re.compile(r'^BPM'), 'ARP'),
    (re.compile(r'^Glide'), 'GLIDE'),
    (re.compile(r'^Pitchbend'), 'GLOBAL'),
    (re.compile(r'^Name Character'), 'NAME'),
]

def classify(desc):
    for rx, sec in SECTION_RULES:
        if rx.match(desc):
            return sec
    return 'OTHER'

def parse_enum(desc):
    desc = re.sub(r'\s*0 to 126 = -127 to -1\s*127 = 0\s*128 to 254 = 1 to 127\s*', '', desc).strip()
    # split "Name 0 = A 1 = B ..." into (clean_name, [options])
    m = re.match(r'^(.*?)\s+0\s*=\s*', desc)
    if not m:
        return desc.strip(), None
    name = m.group(1).strip()
    rest = desc[m.start(0):]
    opts = re.findall(r'(\d+)\s*=\s*([^\d]+?)(?=\d+\s*=|\(|$)', rest)
    options = [o[1].strip().rstrip('.') for o in sorted(opts, key=lambda x: int(x[0]))]
    return name, options if options else None

params = []
skip_groups = {'Arp Note', 'Arp Velocity'}

for e in entries:
    desc = e['desc'].strip()
    desc = re.sub(r'\s+', ' ', desc)
    nrpnA = e['nrpnA']
    nrpnB = e['nrpnB']
    value_range = e['value']

    # skip per-step arrays (ranges like "301-333") and name chars (not real-time knobs)
    if '-' in nrpnA and re.match(r'^\d+-\d+$', nrpnA):
        continue  # Arp Note / Arp Velocity step arrays
    if desc.startswith('Name Character'):
        continue

    name, options = parse_enum(desc)
    # strip trailing "(see X)" references -> keep as note, reuse options from referenced param name if present
    see_ref = None
    m = re.search(r'\(see ([^)]+)\)', name)
    if m:
        see_ref = m.group(1).strip()
        name = re.sub(r'\s*\(see [^)]+\)', '', name).strip()

    if value_range and '-' in value_range:
        try:
            lo, hi = value_range.split('-')
            lo, hi = int(lo), int(hi)
        except ValueError:
            lo, hi = 0, 127
    else:
        lo, hi = (0, 1) if options else (0, 127)

    ptype = 'enum' if options else ('toggle' if (lo, hi) == (0, 1) else 'knob')

    params.append({
        'nrpnA': int(nrpnA),
        'nrpnB': int(nrpnB) if nrpnB.isdigit() else None,
        'name': name,
        'section': classify(name),
        'min': lo,
        'max': hi,
        'type': ptype,
        'options': options,
        'categories': None,
        'seeRef': see_ref,
    })

# Override: "Mod N Source", "Mod N Destination", "LFO N Destination" and
# "Envelope 3/4 Destination" are plain knobs in the main table (the manual
# only says "(see table of ... sources/destinations and values)" with no
# inline enum) -- replace with the real dropdown list from Appendix A/B,
# with per-option categories so the GUI renders a tree-grouped dropdown
# instead of one 28- or 99-item flat list.
for p in params:
    if p['name'].endswith(' Source') and p['name'].startswith('Mod '):
        table = MOD_SOURCES
    elif p['name'].endswith(' Destination'):
        table = MOD_DESTINATIONS
    else:
        continue
    p['type'] = 'enum'
    p['options'] = [label for label, _cat in table]
    p['categories'] = [cat for _label, cat in table]
    p['min'] = 0
    p['max'] = len(table) - 1

# Same idea for the Sync Setting enums (see comment above the option lists).
for p in params:
    if re.match(r'^LFO \d+ Sync Setting$', p['name']):
        options = LFO_SYNC_SETTING_OPTIONS
    elif re.match(r'^Delay \d+ Sync Setting$', p['name']):
        options = DELAY_SYNC_SETTING_OPTIONS
    else:
        continue
    p['type'] = 'enum'
    p['options'] = options
    p['categories'] = None
    p['min'] = 0
    p['max'] = len(options) - 1

# The manual's printed NRPN numbers for "Delay N Pan" (427-430) are off by
# two -- confirmed empirically against real hardware (SysEx dump byte-diff
# testing, see Source12/tools/auto_map_bytes.py): NRPN 427/428 produce no
# response at all, while 429/430/431/432 do, landing at the expected
# interleaved-by-channel byte offsets that match every other Delay N field
# (e.g. Delay Time sits at bytes 96/97/98/99 for channels 1-4; Pan sits at
# 164/165/166/167 for NRPN 429/430/431/432, i.e. channel N -> NRPN 428+N).
for p in params:
    m = re.match(r'^Delay (\d) Pan$', p['name'])
    if not m:
        continue
    chan = int(m.group(1))
    p['nrpnA'] = 428 + chan
    if p['nrpnB'] is not None:
        p['nrpnB'] = 940 + chan

# BPM's printed NRPN (288) collides with A/B Mode's (also printed as 288) --
# confirmed empirically that 288 really is A/B Mode (changes the expected
# split/stack byte); BPM's true number is unknown (a brute-force scan of
# the unclaimed 296-300 gap right after it in the manual's Arp section
# didn't turn up a convincing match either). Shipping BPM at the wrong,
# colliding number would make its knob silently corrupt A/B Mode instead,
# which is worse than not having a BPM control at all -- drop it until the
# real number is found.
params = [p for p in params if p['name'] != 'BPM']

print('total GUI params:', len(params))
from collections import Counter
print(Counter(p['section'] for p in params))

with open('parameters.json', 'w') as f:
    json.dump(params, f, indent=2)
