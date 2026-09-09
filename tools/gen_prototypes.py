#!/usr/bin/env python3
"""Regenerate src/nava/prototypes.h after adding or changing a function.

The Arduino IDE used to synthesise these automatically from the .ino files.
PlatformIO does not, and this project deliberately builds as ONE translation
unit (src/main.cpp includes every .inc), so forward declarations are required.

    python3 tools/gen_prototypes.py
"""
import os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
NAVA = os.path.join(ROOT, "src", "nava")

SIG = re.compile(
    r'^((?:unsigned\s+|signed\s+|static\s+|const\s+)*'
    r'(?:void|byte|boolean|bool|char|int|long|word|float|double)\s*\**\s*)'
    r'([A-Za-z_]\w*)\s*\(([^;{]*)\)\s*(?:\{|$)')

def main():
    protos, seen, depth = [], set(), 0
    for f in sorted(os.listdir(NAVA), key=str.lower):
        if not f.endswith(".inc"):
            continue
        for line in open(os.path.join(NAVA, f), encoding="utf-8", errors="replace"):
            # skip /* ... */ blocks - Button.inc carries ~130 lines of dead
            # commented-out code including a second MuteButtonGet()
            depth += line.count("/*") - line.count("*/")
            if depth > 0:
                continue
            line = line.split("//")[0].rstrip()
            m = SIG.match(line)
            if not m:
                continue
            ret, name, args = m.group(1).strip(), m.group(2), m.group(3).strip()
            if name in ("setup", "loop", "ISR"):
                continue
            key = f"{name}({args})"
            if key in seen:
                continue
            seen.add(key)
            protos.append(f"{ret} {name}({args});")

    out = os.path.join(NAVA, "prototypes.h")
    with open(out, "w") as fh:
        fh.write("//-------------------------------------------------\n"
                 "//                  NAVA v1.x\n"
                 "//   Forward declarations - GENERATED, do not hand-edit.\n"
                 "//   Regenerate: python3 tools/gen_prototypes.py\n"
                 "//-------------------------------------------------\n\n"
                 "#ifndef prototypes_h\n#define prototypes_h\n\n")
        fh.write("\n".join(sorted(protos, key=str.lower)))
        fh.write("\n\n#endif//prototypes_h\n")
    print(f"wrote {out}  ({len(protos)} prototypes)")

if __name__ == "__main__":
    sys.exit(main())
