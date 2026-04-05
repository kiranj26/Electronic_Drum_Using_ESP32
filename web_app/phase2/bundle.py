#!/usr/bin/env python3
"""
bundle.py — Phase 2 web app bundler

Reads WAV samples from web_app/phase0/samples/,
base64-encodes them, and injects them into template.html.
Outputs the final self-contained index.html to:
  firmware/phase2/data/index.html   ← flashed to ESP32 SPIFFS

Usage (run from repo root):
  python3 web_app/phase2/bundle.py

Then flash filesystem to ESP32:
  cd firmware/phase2
  pio run --target uploadfs
"""

import base64
import os
import sys

# ── Paths ─────────────────────────────────────────────────────
REPO_ROOT   = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SAMPLES_DIR = os.path.join(REPO_ROOT, "web_app", "phase0", "samples")
TEMPLATE    = os.path.join(REPO_ROOT, "web_app", "phase2", "template.html")
OUTPUT      = os.path.join(REPO_ROOT, "firmware", "phase2", "data", "index.html")

# ── Sample file map: placeholder → filename ──────────────────
SAMPLE_MAP = {
    "%%KICK%%":         "kick.wav",
    "%%SNARE%%":        "snare.wav",
    "%%HIHAT_CLOSED%%": "hihat_closed.wav",
    "%%HIHAT_OPEN%%":   "hihat_open.wav",
    "%%TOM_LOW%%":      "tom_low.wav",
    "%%TOM_MID%%":      "tom_mid.wav",
    "%%CRASH%%":        "crash.wav",
}

def encode_wav(filepath):
    with open(filepath, "rb") as f:
        return base64.b64encode(f.read()).decode("ascii")

def main():
    print(f"Samples dir : {SAMPLES_DIR}")
    print(f"Template    : {TEMPLATE}")
    print(f"Output      : {OUTPUT}")
    print()

    # Read template
    if not os.path.exists(TEMPLATE):
        print(f"ERROR: template.html not found at {TEMPLATE}")
        sys.exit(1)

    with open(TEMPLATE, "r", encoding="utf-8") as f:
        html = f.read()

    # Encode and inject each sample
    total_bytes = 0
    for placeholder, filename in SAMPLE_MAP.items():
        filepath = os.path.join(SAMPLES_DIR, filename)
        if not os.path.exists(filepath):
            print(f"  MISSING: {filename} — pad will be disabled in app")
            html = html.replace(placeholder, "")
            continue

        b64 = encode_wav(filepath)
        size_kb = len(b64) * 3 / 4 / 1024
        total_bytes += len(b64)
        html = html.replace(placeholder, b64)
        print(f"  ✓  {filename:<24} {size_kb:.0f} KB")

    total_kb = total_bytes * 3 / 4 / 1024
    output_kb = len(html.encode("utf-8")) / 1024
    print()
    print(f"Total sample data : {total_kb:.0f} KB")
    print(f"Output HTML size  : {output_kb:.0f} KB")

    # Check SPIFFS limit (~1.5MB usable with default partition)
    if output_kb > 1400:
        print("WARNING: output may be too large for SPIFFS (limit ~1.4MB)")
        print("         Consider reducing sample quality or count")

    # Write output
    os.makedirs(os.path.dirname(OUTPUT), exist_ok=True)
    with open(OUTPUT, "w", encoding="utf-8") as f:
        f.write(html)

    print(f"\nDone → {OUTPUT}")
    print()
    print("Next steps:")
    print("  1.  cd firmware/phase2")
    print("  2.  pio run --target uploadfs   ← uploads index.html to ESP32 SPIFFS")
    print("  3.  pio run --target upload     ← flashes firmware")
    print("  4.  On iPhone: connect to WiFi 'DrumKit-ESP32' (password: drumkit123)")
    print("  5.  Open Safari → http://192.168.4.1")

if __name__ == "__main__":
    main()
