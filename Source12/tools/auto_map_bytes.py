#!/usr/bin/env python3
"""Fully automated Prophet 12 parameter byte-offset mapper.

Talks directly to the hardware over MIDI (no GUI/plugin involved), using
raw python-rtmidi -- mido's wrapper silently drops SysEx replies on this
setup even with its ignore-types filter disabled, while plain rtmidi
receives them fine.

  1. For each parameter, sends an NRPN message setting it to a new value.
  2. Requests a Program Edit Buffer Dump via SysEx.
  3. Waits for the reply, unpacks it, diffs against the previous dump.
  4. Records (param name -> byte offset) when exactly one byte changed.

Safe to re-run/resume: skips params already in the offset map, and saves
after every single parameter so a crash/interrupt loses at most one step.
"""
import json
import time
import sys
from pathlib import Path

import rtmidi

OUTPORT_NAME = "Prophet 12 Keyboard"
INPORT_NAME = "Prophet 12 Keyboard"
PARAMS_PATH = Path("C:/Users/produ/AppData/Local/Temp/p12/parameters.json")
MAP_PATH = Path("C:/Users/produ/dev/skyjad/Source12/tools/byte_offset_map.json")
LOG_PATH = Path("C:/Users/produ/AppData/Local/Temp/p12/mapper_log.txt")

DSI_ID = 0x01
P12_ID = 0x2A
CMD_REQUEST_EDIT_BUFFER = 0x06
CMD_PROGRAM_DATA = 0x02
CMD_EDIT_BUFFER_DATA = 0x03


def log(msg):
    line = f"[{time.strftime('%H:%M:%S')}] {msg}"
    print(line, flush=True)
    with open(LOG_PATH, "a") as f:
        f.write(line + "\n")


def open_port_containing(cls, name_substr):
    dev = cls()
    ports = dev.get_ports()
    matches = [i for i, p in enumerate(ports) if name_substr in p]
    if not matches:
        raise RuntimeError(f"no port containing {name_substr!r} in {ports}")
    dev.open_port(matches[0])
    return dev


def unpack_prophet_data(packed) -> bytes:
    out = bytearray()
    i = 0
    n = len(packed)
    while i < n:
        msb_bits = packed[i]
        i += 1
        for bit in range(7):
            if i >= n:
                break
            lo7 = packed[i]
            i += 1
            msb = (msb_bits >> bit) & 1
            out.append((msb << 7) | lo7)
    return bytes(out)


def send_nrpn(outport, nrpn, value, channel=0):
    outport.send_message([0xB0 | channel, 99, (nrpn >> 7) & 0x7F])
    outport.send_message([0xB0 | channel, 98, nrpn & 0x7F])
    clamped = max(0, min(16383, value))
    outport.send_message([0xB0 | channel, 6, (clamped >> 7) & 0x7F])
    outport.send_message([0xB0 | channel, 38, clamped & 0x7F])


def request_dump_and_wait(outport, inport, timeout=5.0):
    outport.send_message([0xF0, DSI_ID, P12_ID, CMD_REQUEST_EDIT_BUFFER, 0xF7])
    deadline = time.time() + timeout
    buf = bytearray()
    collecting = False
    while time.time() < deadline:
        msg = inport.get_message()
        if msg is None:
            time.sleep(0.01)
            continue
        data = msg[0]
        if not collecting:
            if len(data) >= 4 and data[0] == 0xF0 and data[1] == DSI_ID and data[2] == P12_ID \
                    and data[3] in (CMD_PROGRAM_DATA, CMD_EDIT_BUFFER_DATA):
                buf.extend(data)
                collecting = True
            else:
                continue  # some other sysex / stray message -- ignore
        else:
            # WinMM delivers one ~1024-byte MIDIHDR buffer per get_message()
            # call for a long sysex, not the whole message at once --
            # subsequent chunks arrive as plain continuation bytes.
            buf.extend(data)

        if collecting and buf[-1] == 0xF7:
            cmd = buf[3]
            if cmd == CMD_EDIT_BUFFER_DATA:
                return unpack_prophet_data(buf[4:-1])
            elif cmd == CMD_PROGRAM_DATA:
                return unpack_prophet_data(buf[6:-1])
    return None


def main():
    params = json.load(open(PARAMS_PATH))
    offset_map = json.loads(MAP_PATH.read_text()) if MAP_PATH.exists() else {}

    todo = [p for p in params if p['name'] not in offset_map]
    if len(sys.argv) > 1:
        todo = todo[: int(sys.argv[1])]
    log(f"{len(params)} total params, {len(todo)} remaining to map (this run)")

    outport = open_port_containing(rtmidi.MidiOut, OUTPORT_NAME)
    inport = open_port_containing(rtmidi.MidiIn, INPORT_NAME)
    inport.ignore_types(sysex=False, timing=True, active_sense=True)

    try:
        while inport.get_message() is not None:
            pass  # drain stale input
        baseline = request_dump_and_wait(outport, inport)
        if baseline is None:
            log("ERROR: no response to initial dump request -- check MIDI Sysex Enable on the P12")
            sys.exit(1)
        log(f"baseline captured, {len(baseline)} bytes")

        prev = baseline
        for i, p in enumerate(todo):
            name = p['name']
            nrpn = p['nrpnA']
            lo, hi = p['min'], p['max']
            if lo == hi:
                continue  # degenerate range

            # Many params may already happen to sit at whichever single
            # value we'd try first (leftover from earlier testing / the
            # current patch) -- try up to 3 distinct candidates before
            # giving up, instead of a single alternating guess.
            mid = (lo + hi) // 2
            candidates = [v for v in (hi, lo, mid) if v != None]
            candidates = list(dict.fromkeys(candidates))  # dedupe, keep order

            result_off = None
            for new_val in candidates:
                send_nrpn(outport, nrpn, new_val)
                time.sleep(0.08)
                dump = request_dump_and_wait(outport, inport)
                if dump is None:
                    log(f"  {name}: NO DUMP REPLY -- skipping")
                    break
                if len(dump) != len(prev):
                    log(f"  {name}: length mismatch {len(prev)} vs {len(dump)} -- skipping")
                    prev = dump
                    break

                diffs = [(j, prev[j], dump[j]) for j in range(len(dump)) if prev[j] != dump[j]]
                prev = dump
                if len(diffs) == 1:
                    off, old, new = diffs[0]
                    offset_map[name] = {"byte_offset": off, "nrpnA": nrpn, "min": lo, "max": hi,
                                         "last_old": old, "last_new": new}
                    log(f"  [{i+1}/{len(todo)}] {name}: byte {off} ({old} -> {new})  OK (tried {new_val})")
                    result_off = off
                    break
                elif len(diffs) == 0:
                    continue  # try next candidate value
                else:
                    log(f"  [{i+1}/{len(todo)}] {name}: {len(diffs)} bytes changed -- {diffs[:6]} -- SKIPPED (ambiguous, tried {new_val})")
                    break

            if result_off is None and name not in offset_map:
                log(f"  [{i+1}/{len(todo)}] {name}: no candidate value produced a clean 1-byte diff -- SKIPPED")

            MAP_PATH.write_text(json.dumps(offset_map, indent=2, sort_keys=True))
    finally:
        del outport
        del inport

    log(f"done. {len(offset_map)}/{len(params)} parameters mapped -> {MAP_PATH}")


if __name__ == "__main__":
    main()
