import json, re
from mod_tables import MOD_SOURCES, MOD_DESTINATIONS

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

print('total GUI params:', len(params))
from collections import Counter
print(Counter(p['section'] for p in params))

with open('parameters.json', 'w') as f:
    json.dump(params, f, indent=2)
