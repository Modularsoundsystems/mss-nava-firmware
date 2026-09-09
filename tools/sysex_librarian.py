#!/usr/bin/env python3
"""NAVA SysEx librarian.

Converts between the raw external-EEPROM image and the .syx format the firmware
speaks, so patterns can be archived and pushed back after a chip erase.

    # turn the Phase 0 Intel-HEX EEPROM dump into a restorable .syx
    python tools/sysex_librarian.py hex2syx backup/nava_patterns.hex out.syx

    # turn a .syx captured from the unit back into a raw image
    python tools/sysex_librarian.py syx2bin dump.syx out.bin

    # emit the one-message request that makes the unit dump everything
    python tools/sysex_librarian.py request out.syx

    # list Windows MIDI outputs, then send a .syx paced to the DIN wire rate
    python tools/sysex_librarian.py ports
    python tools/sysex_librarian.py send restore.syx --port 1

Capturing a dump works with any librarian - MIDI-OX on Windows, SysEx Librarian
on macOS, amidi on Linux:

    amidi -p hw:1,0,0 -d -t 60 > dump.syx  # capture

**Restoring does not.** A restore is ~1,800 messages and 143 KB, and a USB MIDI
interface accepts them from the host far faster than its DIN output can clock
them out at 31250 baud - roughly fifteen times faster. The interface has to
buffer the difference, and a few kilobytes in it simply drops the rest. That is
the failure where a restore writes the first handful of patterns and stops: the
messages never reached the DIN cable, so nothing on the NAVA can report it.

`send` fixes it by pacing to the wire: no message leaves until the one before it
has had time to clock out, so the interface never queues more than one. It uses
Windows' own winmm through ctypes - no third-party modules, nothing to install.

Elsewhere, pace it in whatever sends the file. MIDI-OX: SysEx View → Configure →
low-level output buffers, set a delay after F7. Anything with no pacing control
will drop a restore this size.

No third-party modules required.

Wire format is documented in src/nava/SysEx.inc.
"""
import sys

SYSEX_ID   = 0x7D          # non-commercial / educational manufacturer ID
SYSEX_DEV  = 0x4E          # 'N'
CMD_DUMP_ALL = 0x01
CMD_PTRN     = 0x10
CMD_SETUP    = 0x11
CMD_END      = 0x7F

PAGE         = 64
MAX_PTRN     = 128
MAX_TRACK    = 16
PTRN_OFFSET  = 0

# ---- format v2, what the firmware reads today. Mirrors struct Pattern in
# src/nava/define.h and the map at the top of src/nava/EEprom.inc.
PTRN_SIZE    = 896
PTRN_PAGES   = PTRN_SIZE // PAGE                          # 14
TRACK_SIZE   = 512
TRACK_OFFSET = PTRN_SIZE * MAX_PTRN                       # 114688
BANK_OFFSET  = TRACK_OFFSET + TRACK_SIZE * MAX_TRACK      # 122880
SETUP_OFFSET = BANK_OFFSET + PAGE                         # 122944

# v2 record field offsets
V2_MAGIC, V2_FORMAT, V2_NAME  = 0, 2, 4
V2_LENGTH, V2_SCALE, V2_DIR   = 16, 17, 18
V2_SHUFFLE, V2_FLAM           = 20, 21
V2_EXTLEN, V2_GRPPOS          = 22, 23
V2_GRPLEN, V2_TOTALACC        = 24, 25
V2_INST, V2_FLAMMASK          = 32, 64
V2_VELOCITY, V2_EXTNOTE       = 96, 768

# ---- format v1, the source of a conversion only
V1_PTRN_SIZE    = 448
V1_TRACK_SIZE   = 1024
V1_TRACK_OFFSET = V1_PTRN_SIZE * MAX_PTRN                 # 57344
V1_SETUP_OFFSET = V1_TRACK_OFFSET + V1_TRACK_SIZE * MAX_TRACK   # 73728
V1_EXT_OFFSET   = 64
V1_VEL_OFFSET   = 192

# Instrument indices are shift-register bit positions, not a dense list.
# Only these five have the low/high button pair whose double-push arms flam,
# so only these can carry a flam bit - see InstHasFlam() in define.h.
BD, SD, LT, MT, HT = 8, 9, 10, 11, 2
FLAM_INSTS = (BD, SD, LT, MT, HT)


def encode7(src):
    """8-bit -> 7-bit. Each group of up to 7 bytes becomes a high-bit byte
    followed by the 7-bit remainders. 64 bytes -> 74."""
    out = bytearray()
    for i in range(0, len(src), 7):
        group = src[i:i + 7]
        msb = 0
        for k, b in enumerate(group):
            if b & 0x80:
                msb |= 1 << k
        out.append(msb)
        out.extend(b & 0x7F for b in group)
    return bytes(out)


def decode7(src):
    """7-bit -> 8-bit, the inverse of encode7()."""
    out = bytearray()
    i = 0
    while i < len(src):
        msb = src[i]
        i += 1
        for k in range(7):
            if i >= len(src):
                break
            v = src[i] & 0x7F
            i += 1
            if msb & (1 << k):
                v |= 0x80
            out.append(v)
    return bytes(out)


def message(cmd, a=None, b=None, payload=b""):
    m = bytearray([0xF0, SYSEX_ID, SYSEX_DEV, cmd])
    if a is not None:
        m.append(a)
    if b is not None:
        m.append(b)
    m.extend(payload)
    m.append(0xF7)
    return bytes(m)


def load_ihex(path):
    """Read an Intel-HEX file into a flat bytearray.

    Opened as binary and masked to 7 bits before parsing. Intel HEX is pure
    ASCII, so clearing bit 7 cannot lose information - and a USB-TTL capture at
    the wrong parity/framing injects exactly that: '0' arrives as 0xB0, CR as
    0x8D. Every record's checksum is then verified, which is what proves the
    repair was sound rather than merely plausible. Records that still fail are
    reported instead of being silently folded into the image.
    """
    raw = open(path, "rb").read()
    text = bytes(b & 0x7F for b in raw)
    lines = text.replace(b"\r\n", b"\n").replace(b"\r", b"\n").split(b"\n")

    mem, base, top = {}, 0, 0
    bad = 0
    for line in lines:
        line = line.strip()
        if not line.startswith(b":"):
            continue
        try:
            d = bytes.fromhex(line[1:].decode("ascii"))
        except Exception:
            bad += 1
            continue
        if len(d) < 5 or len(d) != d[0] + 5 or (sum(d) & 0xFF) != 0:
            bad += 1
            continue
        ln, addr, typ = d[0], (d[1] << 8) | d[2], d[3]
        data = d[4:4 + ln]
        if typ == 0x00:
            for i, byte in enumerate(data):
                a = base + addr + i
                mem[a] = byte
                top = max(top, a)
        elif typ == 0x04:
            base = ((data[0] << 8) | data[1]) << 16
        elif typ == 0x02:
            base = ((data[0] << 8) | data[1]) << 4
        elif typ == 0x01:
            break

    if bad:
        print(f"WARNING {bad} record(s) failed their checksum and were skipped "
              f"- the resulting image has holes")
    buf = bytearray(top + 1)
    for a, byte in mem.items():
        buf[a] = byte
    return buf


def cmd_hex2syx(args):
    if len(args) != 2:
        sys.exit("usage: hex2syx <eeprom.hex> <out.syx>")
    src, dst = args
    mem = load_ihex(src)
    need = SETUP_OFFSET + PAGE
    if len(mem) < need:
        sys.exit(f"image is {len(mem)} bytes, need at least {need} "
                 f"- is this a full 128 KB EEPROM dump?")

    out = bytearray()
    patterns = 0
    for n in range(MAX_PTRN):
        for p in range(PTRN_PAGES):
            off = PTRN_OFFSET + n * PTRN_SIZE + p * PAGE
            out += message(CMD_PTRN, n, p, encode7(mem[off:off + PAGE]))
        patterns += 1
    out += message(CMD_SETUP, 0, 0, encode7(mem[SETUP_OFFSET:SETUP_OFFSET + PAGE]))

    with open(dst, "wb") as fh:
        fh.write(out)
    print(f"wrote {dst}: {patterns} patterns + setup, "
          f"{patterns * PTRN_PAGES + 1} messages, {len(out)} bytes")
    print("NOTE: tracks are not included - the firmware's SysEx protocol covers "
          "patterns and the global setup only.")


def split_messages(blob):
    """Yield the inner bytes of each F0...F7 message."""
    i = 0
    while True:
        try:
            start = blob.index(0xF0, i)
            end = blob.index(0xF7, start + 1)
        except ValueError:
            return
        yield blob[start + 1:end]
        i = end + 1


def v1_record_to_v2(src, n):
    """One 448-byte v1 pattern record -> one 896-byte v2 record.

    Must agree byte for byte with MigrateOnePattern() in src/nava/EEprom.inc,
    which does the same job on the unit. Two implementations of one mapping is
    a real risk, so the field offsets above are named rather than inline and
    both sides carry the same layout comment.

    v1 record:  0..31 inst[16] LE, 32..39 setup, 40..63 unused,
                64..191 extNote[128], 192..447 velocity[16][16]
    """
    rec = bytearray(PTRN_SIZE)

    rec[0] = ord('N')
    rec[1] = ord('V')
    rec[V2_FORMAT] = 2
    rec[V2_NAME:V2_NAME + 12] = ("P%03d" % (n + 1)).ljust(12).encode("ascii")

    rec[V2_LENGTH]   = src[32]
    rec[V2_SCALE]    = src[33]
    rec[V2_DIR]      = 0            # FORWARD; v1 never persisted a direction
    rec[V2_SHUFFLE]  = src[34]
    rec[V2_FLAM]     = src[35]
    rec[V2_EXTLEN]   = src[36]
    rec[V2_GRPPOS]   = src[37]
    rec[V2_GRPLEN]   = src[38]
    rec[V2_TOTALACC] = src[39]

    # inst[] is already little-endian pairs in v1, so this is a straight copy.
    rec[V2_INST:V2_INST + 32] = src[0:32]

    # Velocity and flam stop sharing a byte here. In v1, bit 7 of the velocity
    # meant "flam", so the byte was a 0..127 level and a boolean at once.
    for i in range(16):
        for j in range(16):
            v = src[V1_VEL_OFFSET + i * 16 + j]
            rec[V2_VELOCITY + i * 16 + j] = v & 0x7F
            if (v & 0x80) and i in FLAM_INSTS:
                rec[V2_FLAMMASK + i * 2 + (j >> 3)] |= 1 << (j & 7)

    rec[V2_EXTNOTE:V2_EXTNOTE + 128] = src[V1_EXT_OFFSET:V1_EXT_OFFSET + 128]
    return rec


def cmd_v1tov2(args):
    if len(args) != 2:
        sys.exit("usage: v1tov2 <v1_eeprom.hex> <out.syx>")
    src, dst = args
    mem = load_ihex(src)
    need = V1_SETUP_OFFSET + PAGE
    if len(mem) < need:
        sys.exit("image is %d bytes, need at least %d - is this a full v1 "
                 "128 KB EEPROM dump?" % (len(mem), need))

    out = bytearray()
    nonempty = 0
    for n in range(MAX_PTRN):
        off = n * V1_PTRN_SIZE
        v1 = mem[off:off + V1_PTRN_SIZE]
        if any(v1[0:32]):
            nonempty += 1
        rec = v1_record_to_v2(v1, n)
        for p in range(PTRN_PAGES):
            out += message(CMD_PTRN, n, p, encode7(rec[p * PAGE:(p + 1) * PAGE]))

    setup = bytearray(PAGE)
    setup[0:6] = mem[V1_SETUP_OFFSET:V1_SETUP_OFFSET + 6]
    setup[6] = 0                    # hhPriority: OH wins, matching v1 behaviour
    out += message(CMD_SETUP, 0, 0, encode7(setup))

    with open(dst, "wb") as fh:
        fh.write(out)
    print("wrote %s: %d patterns (%d with trigs) + setup, %d messages, %d bytes"
          % (dst, MAX_PTRN, nonempty, MAX_PTRN * PTRN_PAGES + 1, len(out)))
    print("NOTE: tracks are not included - the SysEx protocol covers patterns "
          "and the global setup only. The unit rebuilds its bank header on the "
          "next boot from pattern 0's magic, so no separate step is needed.")


def cmd_syx2bin(args):
    if len(args) != 2:
        sys.exit("usage: syx2bin <dump.syx> <out.bin>")
    src, dst = args
    with open(src, "rb") as fh:
        blob = fh.read()

    mem = bytearray(SETUP_OFFSET + PAGE)
    seen, setup = set(), False
    for body in split_messages(blob):
        if len(body) < 3 or body[0] != SYSEX_ID or body[1] != SYSEX_DEV:
            continue
        cmd = body[2]
        if cmd == CMD_PTRN and len(body) >= 5:
            n, p = body[3], body[4]
            data = decode7(body[5:])[:PAGE]
            if n < MAX_PTRN and p < PTRN_PAGES and len(data) == PAGE:
                off = PTRN_OFFSET + n * PTRN_SIZE + p * PAGE
                mem[off:off + PAGE] = data
                seen.add(n)
        elif cmd == CMD_SETUP and len(body) >= 5:
            data = decode7(body[5:])[:PAGE]
            if len(data) == PAGE:
                mem[SETUP_OFFSET:SETUP_OFFSET + PAGE] = data
                setup = True

    with open(dst, "wb") as fh:
        fh.write(mem)
    print(f"wrote {dst}: {len(seen)}/{MAX_PTRN} patterns, "
          f"setup {'yes' if setup else 'MISSING'}")
    if len(seen) != MAX_PTRN:
        missing = sorted(set(range(MAX_PTRN)) - seen)
        print(f"WARNING incomplete dump, missing patterns: {missing[:16]}"
              f"{' ...' if len(missing) > 16 else ''}")


def cmd_request(args):
    if len(args) != 1:
        sys.exit("usage: request <out.syx>")
    with open(args[0], "wb") as fh:
        fh.write(message(CMD_DUMP_ALL))
    print(f"wrote {args[0]}: dump-all request "
          f"(unit replies with ~143 KB over about 46 s)")


# ---------------------------------------------------------------------------
# Paced sending, Windows, through winmm
#
# A restore is ~1,800 messages. A USB MIDI interface takes them from the host at
# USB speed and clocks them out of its DIN socket at 31250 baud - about fifteen
# times slower - so it has to buffer the difference. A few kilobytes in, a cheap
# one stops buffering and starts dropping, which is a restore that writes the
# first few patterns and silently abandons the rest.
#
# The cure is to never give the interface a queue: send one message, wait the
# time that message needs on the wire, send the next. That caps the transfer at
# the wire rate, which is the floor anyway - about 46 seconds for a full bank.
#
# winmm through ctypes rather than a MIDI package, so this stays installable by
# copying the file.
# ---------------------------------------------------------------------------

MIDI_BYTES_PER_SEC = 3125.0        # 31250 baud, 10 bits a byte
MHDR_DONE          = 0x00000001


def _winmm():
    import ctypes
    if not hasattr(ctypes, "windll"):
        sys.exit("send/ports need Windows. Elsewhere, pace the transfer in your\n"
                 "own librarian - see the note at the top of this file.")
    return ctypes.windll.winmm, ctypes


def _check(mm, rc, what):
    if rc:
        import ctypes
        buf = ctypes.create_unicode_buffer(256)
        mm.midiOutGetErrorTextW(rc, buf, 256)
        sys.exit(f"{what} failed ({rc}): {buf.value}")


def cmd_ports(args):
    mm, ctypes = _winmm()

    class CAPS(ctypes.Structure):
        _fields_ = [("wMid", ctypes.c_ushort), ("wPid", ctypes.c_ushort),
                    ("vDriverVersion", ctypes.c_uint), ("szPname", ctypes.c_wchar * 32),
                    ("wTechnology", ctypes.c_ushort), ("wVoices", ctypes.c_ushort),
                    ("wNotes", ctypes.c_ushort), ("wChannelMask", ctypes.c_ushort),
                    ("dwSupport", ctypes.c_uint)]

    n = mm.midiOutGetNumDevs()
    if not n:
        sys.exit("no MIDI outputs found")
    print("MIDI outputs:")
    for i in range(n):
        caps = CAPS()
        mm.midiOutGetDevCapsW(i, ctypes.byref(caps), ctypes.sizeof(caps))
        print(f"  {i}  {caps.szPname}")
    print("\nPass the number to `send --port N`.")


def cmd_send(args):
    if not args:
        sys.exit("usage: send <file.syx> --port N [--delay MS] [--wire-factor F]")

    path, port, extra_ms, factor = args[0], None, 0.0, 1.15
    i = 1
    while i < len(args):
        if args[i] == "--port" and i + 1 < len(args):
            port = int(args[i + 1]); i += 2
        elif args[i] == "--delay" and i + 1 < len(args):
            extra_ms = float(args[i + 1]); i += 2
        elif args[i] == "--wire-factor" and i + 1 < len(args):
            factor = float(args[i + 1]); i += 2
        else:
            sys.exit(f"unknown argument: {args[i]}")
    if port is None:
        sys.exit("--port is required; run `ports` to list them")

    with open(path, "rb") as fh:
        blob = fh.read()

    msgs = []
    j = 0
    while True:
        s = blob.find(b"\xF0", j)
        if s < 0:
            break
        e = blob.find(b"\xF7", s)
        if e < 0:
            break
        msgs.append(blob[s:e + 1])
        j = e + 1
    if not msgs:
        sys.exit(f"{path}: no SysEx messages found")

    import ctypes, time
    mm, _ = _winmm()

    class MIDIHDR(ctypes.Structure):
        _fields_ = [("lpData", ctypes.c_char_p), ("dwBufferLength", ctypes.c_uint),
                    ("dwBytesRecorded", ctypes.c_uint), ("dwUser", ctypes.c_void_p),
                    ("dwFlags", ctypes.c_uint), ("lpNext", ctypes.c_void_p),
                    ("reserved", ctypes.c_void_p), ("dwOffset", ctypes.c_uint),
                    ("dwReserved", ctypes.c_void_p * 8)]

    handle = ctypes.c_void_p()
    _check(mm, mm.midiOutOpen(ctypes.byref(handle), port, 0, 0, 0), "midiOutOpen")

    total = sum(len(m) for m in msgs)
    print(f"{path}: {len(msgs)} messages, {total} bytes")
    print(f"pacing to the wire{f' plus {extra_ms:g} ms' if extra_ms else ''} "
          f"- about {total / MIDI_BYTES_PER_SEC * factor + len(msgs) * extra_ms / 1000:.0f} s")

    try:
        for n, m in enumerate(msgs):
            buf = ctypes.create_string_buffer(m, len(m))
            hdr = MIDIHDR()
            hdr.lpData = ctypes.cast(buf, ctypes.c_char_p)
            hdr.dwBufferLength = hdr.dwBytesRecorded = len(m)
            hdr.dwFlags = 0

            _check(mm, mm.midiOutPrepareHeader(handle, ctypes.byref(hdr),
                                               ctypes.sizeof(hdr)), "midiOutPrepareHeader")
            _check(mm, mm.midiOutLongMsg(handle, ctypes.byref(hdr),
                                         ctypes.sizeof(hdr)), "midiOutLongMsg")

            # The driver owns the buffer until it sets MHDR_DONE; unpreparing
            # before that is what corrupts a long transfer on some drivers.
            while not (hdr.dwFlags & MHDR_DONE):
                time.sleep(0.001)
            mm.midiOutUnprepareHeader(handle, ctypes.byref(hdr), ctypes.sizeof(hdr))

            # MHDR_DONE means the driver has taken it, not that it has left the
            # DIN socket - so the wait is ours to do, and it is the whole point.
            time.sleep(len(m) / MIDI_BYTES_PER_SEC * factor + extra_ms / 1000.0)

            if (n + 1) % 100 == 0 or n + 1 == len(msgs):
                print(f"  {n + 1}/{len(msgs)}", end="\r", flush=True)
    finally:
        mm.midiOutClose(handle)

    print(f"\nsent {len(msgs)} messages.")
    print("The unit reloads the current pattern once the line has been quiet "
          "for 500 ms.")


COMMANDS = {"hex2syx": cmd_hex2syx, "syx2bin": cmd_syx2bin,
            "request": cmd_request, "v1tov2": cmd_v1tov2,
            "ports": cmd_ports, "send": cmd_send}


def main():
    if len(sys.argv) < 2 or sys.argv[1] not in COMMANDS:
        sys.exit(__doc__)
    COMMANDS[sys.argv[1]](sys.argv[2:])


if __name__ == "__main__":
    main()
