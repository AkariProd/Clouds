import pdfplumber, json, re

FOOTER_RE = re.compile(r'^\d+\s+MIDI Implementation\s+Dave Smith Instruments$|^Dave Smith Instruments$|^Prophet 12 Module Operation Manual.*$|^NRPN NRPN Value Description$|^Layer A Layer B$')

ROW_RE = re.compile(r'^(\d+(?:-\d+)?)\s+(\d+(?:-\d+)?|X)\s+(?:([\d\-]+)\s+)?(.+)$')

with pdfplumber.open('manual.pdf') as pdf:
    entries = []
    for pi in range(75, 91):  # 75..90 inclusive = program parameter table
        text = pdf.pages[pi].extract_text()
        for line in text.split('\n'):
            line = line.strip()
            if not line:
                continue
            if FOOTER_RE.match(line):
                continue
            m = ROW_RE.match(line)
            if m:
                nrpnA, nrpnB, value, desc = m.groups()
                entries.append({'nrpnA': nrpnA, 'nrpnB': nrpnB, 'value': value or '', 'desc': desc})
            else:
                # continuation (enum option line or wrapped description)
                if entries:
                    entries[-1]['desc'] += ' ' + line
                # else: pre-table boilerplate, ignore

    # drop leading junk rows that aren't part of the program-parameter table
    # (first real entry is NRPN Layer A = 0, "Osc 1 Pitch")
    first_idx = next(i for i, e in enumerate(entries) if e['nrpnA'] == '0' and e['desc'].startswith('Osc 1 Pitch'))
    entries = entries[first_idx:]

    # trim trailing ASCII-table bleed-over from the last real row
    last = entries[-1]
    cut = last['desc'].find('The following table')
    if cut != -1:
        last['desc'] = last['desc'][:cut].strip()

    print('total rows', len(entries))
    for e in entries[:6]:
        print(e)
    print('...')
    for e in entries[-10:]:
        print(e)

    with open('entries_final.json', 'w') as f:
        json.dump(entries, f, indent=2)
