#!/usr/bin/env python3
"""Builds Source12/tools/byte_offset_map.json by diffing a sequence of
Seer12 SysEx captures (Documents/Seer12Captures/NNNN_<tag>.bin) against the
one immediately before it. Each capture is expected to differ from its
predecessor by exactly one changed byte -- the parameter the user just
touched on the hardware, tagged with that parameter's id/name.

Usage:
    python3 diff_captures.py "/c/Users/produ/Documents/Seer12Captures"

Captures whose tag is "untagged", or that are the very first "baseline"
capture, are skipped as a diff source but still participate as the
"previous" capture for the next one.
"""
import json
import sys
from pathlib import Path


def main():
    capture_dir = Path(sys.argv[1] if len(sys.argv) > 1 else ".")
    files = sorted(capture_dir.glob("*.bin"), key=lambda f: int(f.stem.split("_", 1)[0]))
    if len(files) < 2:
        print("Need at least 2 captures.")
        return

    map_path = Path(__file__).parent / "byte_offset_map.json"
    offset_map = json.loads(map_path.read_text()) if map_path.exists() else {}

    prev_data = files[0].read_bytes()
    prev_tag = files[0].stem.split("_", 1)[1] if "_" in files[0].stem else "baseline"
    print(f"baseline: {files[0].name} ({len(prev_data)} bytes)")

    for f in files[1:]:
        data = f.read_bytes()
        tag = f.stem.split("_", 1)[1] if "_" in f.stem else "untagged"
        if len(data) != len(prev_data):
            print(f"  {f.name}: SKIP (length mismatch {len(prev_data)} vs {len(data)})")
            prev_data, prev_tag = data, tag
            continue
        diffs = [(i, prev_data[i], data[i]) for i in range(len(data)) if prev_data[i] != data[i]]
        if len(diffs) == 0:
            print(f"  {f.name} ({tag}): no change vs previous -- capture may not have registered")
        elif len(diffs) == 1:
            off, old, new = diffs[0]
            print(f"  {f.name} ({tag}): byte {off}: {old} -> {new}")
            if tag not in ("untagged", "baseline"):
                offset_map[tag] = {"byte_offset": off, "last_old": old, "last_new": new}
        else:
            print(f"  {f.name} ({tag}): {len(diffs)} bytes changed (expected 1) -- {diffs[:10]}")
        prev_data, prev_tag = data, tag

    map_path.write_text(json.dumps(offset_map, indent=2, sort_keys=True))
    print(f"\n{len(offset_map)} parameters mapped so far -> {map_path}")


if __name__ == "__main__":
    main()
