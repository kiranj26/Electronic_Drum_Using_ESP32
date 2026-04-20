#!/usr/bin/env python3
"""
ESP32 Digital Drum Kit — Wiring Diagram Generator
Generates docs/wiring/phase4b_wiring.svg

Run from repo root:
  python3 docs/wiring/generate_wiring.py

Update this file when hardware changes (new phase).
Current state: Phase 4b — SD Card + I2S Amp + Speaker + 7 Buttons
"""

import os

OUT = os.path.join(os.path.dirname(__file__), "phase4b_wiring.svg")

W, H = 1500, 920

# ── Color palette ──────────────────────────────────────────────
C = {
    "bg":          "#0d1117",   # GitHub dark bg
    "panel":       "#161b22",   # module fill
    "panel_esp":   "#1a2332",   # ESP32 fill
    "border":      "#30363d",   # module border
    "border_esp":  "#58a6ff",   # ESP32 border (accent)
    "text":        "#e6edf3",   # primary text
    "text_dim":    "#8b949e",   # secondary text
    "text_pin":    "#c9d1d9",   # pin labels
    "red":         "#ff6b6b",   # 3V3 / power
    "black":       "#666666",   # GND
    "yellow":      "#ffd93d",   # SPI bus
    "blue":        "#6bcbff",   # I2S bus
    "green":       "#6bd96b",   # GPIO buttons
    "orange":      "#ffaa5e",   # audio signal
    "white":       "#ffffff",
}

svg = []

def S(*parts):
    svg.extend(parts)

def rect(x, y, w, h, fill, stroke=None, sw=1.5, rx=8):
    stroke = stroke or C["border"]
    S(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" '
      f'fill="{fill}" stroke="{stroke}" stroke-width="{sw}" rx="{rx}"/>')

def text(x, y, s, size=12, color=None, anchor="middle", bold=False, italic=False):
    color = color or C["text"]
    weight = "bold" if bold else "normal"
    style_extra = "font-style:italic;" if italic else ""
    S(f'<text x="{x}" y="{y}" font-size="{size}" fill="{color}" '
      f'text-anchor="{anchor}" font-weight="{weight}" '
      f'style="font-family:\'Courier New\',monospace;{style_extra}">{s}</text>')

def wire(points, color, w=2.0):
    """Draw a wire as connected line segments. points = [(x,y), ...]"""
    if len(points) < 2:
        return
    d = f"M {points[0][0]} {points[0][1]}"
    for px, py in points[1:]:
        d += f" L {px} {py}"
    S(f'<path d="{d}" stroke="{color}" stroke-width="{w}" '
      f'fill="none" stroke-linecap="round" stroke-linejoin="round"/>')

def dot(x, y, color, r=3.5):
    S(f'<circle cx="{x}" cy="{y}" r="{r}" fill="{color}"/>')

def pin_row(x, y, label, gpio, side="left", color=None):
    """Draw one pin row on a module. Returns (wire_x, y) for connection point."""
    color = color or C["text_pin"]
    if side == "left":
        dot(x, y + 6, color)
        text(x + 10, y + 10, label, size=10, color=color, anchor="start")
        if gpio:
            text(x + 10, y + 22, gpio, size=9, color=C["text_dim"], anchor="start", italic=True)
        return (x, y + 6)
    else:
        dot(x, y + 6, color)
        text(x - 10, y + 10, label, size=10, color=color, anchor="end")
        if gpio:
            text(x - 10, y + 22, gpio, size=9, color=C["text_dim"], anchor="end", italic=True)
        return (x, y + 6)

def module_box(x, y, w, h, title, subtitle=None, color=C["panel"], border=C["border"]):
    rect(x, y, w, h, color, border, sw=1.5)
    text(x + w // 2, y + 22, title, size=13, bold=True, color=C["text"])
    if subtitle:
        text(x + w // 2, y + 38, subtitle, size=10, color=C["text_dim"])

# ═══════════════════════════════════════════════════════════════
# Layout constants
# ═══════════════════════════════════════════════════════════════

# ESP32 DevKit
E_X, E_Y, E_W, E_H = 600, 80, 210, 580

# SD Card module
SD_X, SD_Y, SD_W, SD_H = 120, 80, 190, 220

# MAX98357A amp
AMP_X, AMP_Y, AMP_W, AMP_H = 1100, 80, 190, 240

# Speaker
SPK_X, SPK_Y, SPK_R = 1200, 430, 55

# Buttons row
BTN_Y = 740
BTN_W, BTN_H = 70, 50
BUTTONS = [
    (130,  BTN_Y, "KICK",         "GPIO 4",  C["red"]),
    (230,  BTN_Y, "SNARE",        "GPIO 33", C["white"]),
    (330,  BTN_Y, "HH CLO",       "GPIO 12", C["yellow"]),
    (430,  BTN_Y, "HH OPN",       "GPIO 13", C["yellow"]),
    (530,  BTN_Y, "TOM L",        "GPIO 14", C["blue"]),
    (630,  BTN_Y, "TOM M",        "GPIO 15", C["blue"]),
    (730,  BTN_Y, "CRASH",        "GPIO 32", C["orange"]),
]

# ═══════════════════════════════════════════════════════════════
# Build SVG
# ═══════════════════════════════════════════════════════════════

S(f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" '
  f'viewBox="0 0 {W} {H}">')

# Background
rect(0, 0, W, H, C["bg"], C["bg"], rx=0)

# ── Title bar ─────────────────────────────────────────────────
rect(0, 0, W, 54, C["panel_esp"], C["border_esp"], sw=0, rx=0)
text(W // 2, 24, "ESP32 Digital Drum Kit", size=18, bold=True, color=C["border_esp"])
text(W // 2, 44, "Phase 4b — Wiring Diagram  |  SD Card + I2S Amp + Speaker + 7 Buttons",
     size=11, color=C["text_dim"])

# ── SD Card module ────────────────────────────────────────────
module_box(SD_X, SD_Y, SD_W, SD_H, "MicroSD Breakout", "Adafruit SPI Module")

sd_pins = [
    ("VCC",  "3V3",    C["red"],    60),
    ("GND",  "GND",    C["black"],  85),
    ("CLK",  "GPIO 18",C["yellow"], 110),
    ("MOSI", "GPIO 23",C["yellow"], 135),
    ("MISO", "GPIO 19",C["yellow"], 160),
    ("CS",   "GPIO 5", C["yellow"], 185),
]
sd_conn = {}  # label → (x, y)
for label, gpio, color, dy in sd_pins:
    px = SD_X + SD_W
    py = SD_Y + dy - SD_Y  # relative
    dot(SD_X + SD_W, SD_Y + dy, color)
    text(SD_X + SD_W - 8, SD_Y + dy + 4,  label, size=10, color=color,   anchor="end")
    text(SD_X + SD_W - 8, SD_Y + dy + 15, gpio,  size=9,  color=C["text_dim"], anchor="end", italic=True)
    sd_conn[label] = (SD_X + SD_W, SD_Y + dy)

# ── ESP32 ─────────────────────────────────────────────────────
rect(E_X, E_Y, E_W, E_H, C["panel_esp"], C["border_esp"], sw=2)
text(E_X + E_W // 2, E_Y + 22, "ESP32", size=15, bold=True, color=C["border_esp"])
text(E_X + E_W // 2, E_Y + 40, "DevKit V1", size=10, color=C["text_dim"])

# Chip graphic (inner rectangle)
rect(E_X + 30, E_Y + 55, E_W - 60, E_H - 110, "#0d1117", C["border"], sw=1, rx=4)
text(E_X + E_W // 2, E_Y + E_H // 2 - 10, "ESP32", size=11, color=C["border"], bold=True)
text(E_X + E_W // 2, E_Y + E_H // 2 + 8,  "WROOM-32", size=9,  color=C["border"])

# Left-side pins  (connect out to left = SD card side)
left_pins = [
    ("3V3",  "power",   C["red"],    70),
    ("GND",  "power",   C["black"],  100),
    ("D5",   "SD CS",   C["yellow"], 160),
    ("D18",  "SD SCK",  C["yellow"], 190),
    ("D19",  "SD MISO", C["yellow"], 220),
    ("D23",  "SD MOSI", C["yellow"], 250),
    ("D4",   "KICK",    C["green"],  340),
    ("D12",  "HH CLO",  C["green"],  370),
    ("D13",  "HH OPN",  C["green"],  400),
    ("D14",  "TOM L",   C["green"],  430),
    ("D15",  "TOM M",   C["green"],  460),
    ("D22",  "I2S DIN", C["blue"],   530),
]
esp_left = {}
for label, func, color, dy in left_pins:
    px = E_X
    py = E_Y + dy
    dot(px, py, color)
    text(px + 8, py + 4,  label, size=10, color=color,        anchor="start")
    text(px + 8, py + 15, func,  size=9,  color=C["text_dim"], anchor="start", italic=True)
    esp_left[label] = (px, py)

# Right-side pins (connect out to right = amp side)
right_pins = [
    ("3V3",  "power",    C["red"],    70),
    ("GND",  "power",    C["black"],  100),
    ("D25",  "I2S LRC",  C["blue"],   160),
    ("D26",  "I2S BCLK", C["blue"],   190),
    ("D33",  "SNARE",    C["green"],  310),
    ("D32",  "CRASH",    C["green"],  340),
]
esp_right = {}
for label, func, color, dy in right_pins:
    px = E_X + E_W
    py = E_Y + dy
    dot(px, py, color)
    text(px - 8, py + 4,  label, size=10, color=color,        anchor="end")
    text(px - 8, py + 15, func,  size=9,  color=C["text_dim"], anchor="end", italic=True)
    esp_right[label] = (px, py)

# ── MAX98357A amp ─────────────────────────────────────────────
module_box(AMP_X, AMP_Y, AMP_W, AMP_H, "MAX98357A", "I2S Class D Amp")

amp_pins = [
    ("VIN",  "3V3",    C["red"],    60),
    ("GND",  "GND",    C["black"],  85),
    ("DIN",  "GPIO 22",C["blue"],   115),
    ("BCLK", "GPIO 26",C["blue"],   140),
    ("LRC",  "GPIO 25",C["blue"],   165),
    ("GAIN", "→ GND",  C["black"],  190),
    ("SD",   "→ 3V3",  C["red"],    215),
]
amp_conn = {}
for label, gpio, color, dy in amp_pins:
    px = AMP_X
    py = AMP_Y + dy
    dot(px, py, color)
    text(px + 8, py + 4,  label, size=10, color=color,        anchor="start")
    text(px + 8, py + 15, gpio,  size=9,  color=C["text_dim"], anchor="start", italic=True)
    amp_conn[label] = (px, py)

# Speaker output pins on amp (right side)
dot(AMP_X + AMP_W, AMP_Y + 90,  C["orange"])
text(AMP_X + AMP_W - 8, AMP_Y + 94,  "OUT+", size=10, color=C["orange"], anchor="end")
dot(AMP_X + AMP_W, AMP_Y + 115, C["orange"])
text(AMP_X + AMP_W - 8, AMP_Y + 119, "OUT−", size=10, color=C["orange"], anchor="end")

# ── Speaker ───────────────────────────────────────────────────
cx, cy = SPK_X, SPK_Y
# Outer circle
S(f'<circle cx="{cx}" cy="{cy}" r="{SPK_R}" fill="{C["panel"]}" '
  f'stroke="{C["orange"]}" stroke-width="2"/>')
# Inner circles (cone rings)
for r in [40, 28, 16]:
    S(f'<circle cx="{cx}" cy="{cy}" r="{r}" fill="none" '
      f'stroke="{C["border"]}" stroke-width="1"/>')
# Center dot
S(f'<circle cx="{cx}" cy="{cy}" r="5" fill="{C["orange"]}"/>')
text(cx, cy + SPK_R + 16, "Speaker", size=11, bold=True, color=C["orange"])
text(cx, cy + SPK_R + 30, "3W 4Ω", size=10, color=C["text_dim"])

# Audio wires: amp OUT → speaker
wire([(AMP_X + AMP_W, AMP_Y + 90),
      (AMP_X + AMP_W + 40, AMP_Y + 90),
      (AMP_X + AMP_W + 40, cy - 18),
      (cx - SPK_R, cy - 18)], C["orange"], w=2)
wire([(AMP_X + AMP_W, AMP_Y + 115),
      (AMP_X + AMP_W + 55, AMP_Y + 115),
      (AMP_X + AMP_W + 55, cy + 18),
      (cx - SPK_R, cy + 18)], C["orange"], w=2)
text(AMP_X + AMP_W + 22, AMP_Y + 82, "OUT+", size=9, color=C["orange"])
text(AMP_X + AMP_W + 22, AMP_Y + 127, "OUT−", size=9, color=C["orange"])

# ── Wires: ESP32 ↔ SD Card ────────────────────────────────────
spi_map = {
    "3V3":  ("D5",  "VCC",  C["red"]),     # power (separate)
    "GND":  ("GND", "GND",  C["black"]),   # ground
    "CLK":  ("D18", "CLK",  C["yellow"]),
    "MOSI": ("D23", "MOSI", C["yellow"]),
    "MISO": ("D19", "MISO", C["yellow"]),
    "CS":   ("D5",  "CS",   C["yellow"]),
}
# VCC: ESP32 3V3 (left) → SD VCC
vcc_ex, vcc_ey = esp_left["3V3"]
vcc_sx, vcc_sy = sd_conn["VCC"]
wire([(vcc_ex, vcc_ey), (vcc_sx - 60, vcc_ey), (vcc_sx - 60, vcc_sy), (vcc_sx, vcc_sy)],
     C["red"], w=2)

# GND: ESP32 GND (left) → SD GND
gnd_ex, gnd_ey = esp_left["GND"]
gnd_sx, gnd_sy = sd_conn["GND"]
wire([(gnd_ex, gnd_ey), (gnd_sx - 40, gnd_ey), (gnd_sx - 40, gnd_sy), (gnd_sx, gnd_sy)],
     C["black"], w=2)

# SPI: CS, SCK, MOSI, MISO
for esp_pin, sd_pin, color in [
    ("D5",  "CS",   C["yellow"]),
    ("D18", "CLK",  C["yellow"]),
    ("D19", "MISO", C["yellow"]),
    ("D23", "MOSI", C["yellow"]),
]:
    ex, ey = esp_left[esp_pin]
    sx, sy = sd_conn[sd_pin]
    mid_x = (ex + sx) // 2
    wire([(ex, ey), (mid_x, ey), (mid_x, sy), (sx, sy)], color, w=1.8)

# SPI bus label
rect(340, 170, 80, 20, C["panel"], C["yellow"], sw=1, rx=4)
text(380, 184, "SPI Bus", size=10, color=C["yellow"], bold=True)

# ── Wires: ESP32 ↔ MAX98357A ──────────────────────────────────
# 3V3 power to amp
wire([(esp_right["3V3"][0], esp_right["3V3"][1]),
      (esp_right["3V3"][0] + 60, esp_right["3V3"][1]),
      (esp_right["3V3"][0] + 60, amp_conn["VIN"][1]),
      (amp_conn["VIN"][0], amp_conn["VIN"][1])],
     C["red"], w=2)

# GND to amp
wire([(esp_right["GND"][0], esp_right["GND"][1]),
      (esp_right["GND"][0] + 40, esp_right["GND"][1]),
      (esp_right["GND"][0] + 40, amp_conn["GND"][1]),
      (amp_conn["GND"][0], amp_conn["GND"][1])],
     C["black"], w=2)

# I2S: DIN, BCLK, LRC
i2s_map = [
    ("D22", "left",  "DIN",  C["blue"]),
    ("D26", "right", "BCLK", C["blue"]),
    ("D25", "right", "LRC",  C["blue"]),
]
for esp_pin, esp_side, amp_pin, color in i2s_map:
    if esp_side == "right":
        ex, ey = esp_right[esp_pin]
        ax, ay = amp_conn[amp_pin]
        mid_x = (ex + ax) // 2
        wire([(ex, ey), (mid_x, ey), (mid_x, ay), (ax, ay)], color, w=1.8)
    else:
        # DIN goes from left side of ESP32, routes around bottom
        ex, ey = esp_left[esp_pin]
        ax, ay = amp_conn[amp_pin]
        wire([(ex, ey),
              (ex - 30, ey),
              (ex - 30, E_Y + E_H + 30),
              (ax - 60,  E_Y + E_H + 30),
              (ax - 60,  ay),
              (ax, ay)], color, w=1.8)

# I2S bus label
rect(870, 270, 80, 20, C["panel"], C["blue"], sw=1, rx=4)
text(910, 284, "I2S Bus", size=10, color=C["blue"], bold=True)

# GAIN → GND annotation
gain_x, gain_y = amp_conn["GAIN"]
wire([(gain_x, gain_y), (gain_x - 30, gain_y), (gain_x - 30, gain_y + 30)], C["black"], w=1.5)
dot(gain_x - 30, gain_y + 30, C["black"])
text(gain_x - 38, gain_y + 44, "GND", size=9, color=C["black"])
text(gain_x - 38, gain_y + 55, "+15dB", size=8, color=C["text_dim"], italic=True)

# ── Buttons ────────────────────────────────────────────────────
for bx, by, blabel, bgpio, bcolor in BUTTONS:
    # Button box
    rect(bx, by, BTN_W, BTN_H, C["panel"], bcolor, sw=1.5, rx=6)
    text(bx + BTN_W // 2, by + 18, blabel, size=10, color=bcolor, bold=True)
    text(bx + BTN_W // 2, by + 32, bgpio,  size=9,  color=C["text_dim"])

    # GND connection (short line down)
    dot(bx + BTN_W // 2, by + BTN_H, C["black"], r=3)

    # GPIO wire up to ESP32
    gpio_num = int(bgpio.split()[1])
    esp_pin = f"D{gpio_num}"

    # Find connection point on ESP32
    if esp_pin in esp_left:
        tx, ty = esp_left[esp_pin]
        btn_top_x = bx + BTN_W // 2
        btn_top_y = by
        # Route: button top → up → left → ESP32 pin
        wire([(btn_top_x, btn_top_y),
              (btn_top_x, ty + 10),
              (tx, ty + 10),
              (tx, ty)],
             bcolor, w=1.5)
    elif esp_pin in esp_right:
        tx, ty = esp_right[esp_pin]
        btn_top_x = bx + BTN_W // 2
        btn_top_y = by
        wire([(btn_top_x, btn_top_y),
              (btn_top_x, ty + 10),
              (tx, ty + 10),
              (tx, ty)],
             bcolor, w=1.5)

# Shared GND rail for buttons
gnd_rail_y = BTN_Y + BTN_H + 20
rect(100, gnd_rail_y, 730, 6, C["black"], C["black"], sw=0, rx=3)
text(835, gnd_rail_y + 8, "GND (shared)", size=10, color=C["black"])
for bx, by, _, _, _ in BUTTONS:
    wire([(bx + BTN_W // 2, by + BTN_H),
          (bx + BTN_W // 2, gnd_rail_y + 3)], C["black"], w=1.5)

# ── Legend ─────────────────────────────────────────────────────
leg_x, leg_y = 1060, 560
rect(leg_x, leg_y, 380, 200, C["panel"], C["border"], sw=1, rx=8)
text(leg_x + 190, leg_y + 20, "Wire Legend", size=12, bold=True, color=C["text"])

legend_items = [
    (C["red"],    "3V3 Power"),
    (C["black"],  "GND"),
    (C["yellow"], "SPI Bus (SD Card)"),
    (C["blue"],   "I2S Bus (Audio)"),
    (C["green"],  "GPIO (Buttons)"),
    (C["orange"], "Audio Output"),
]
for i, (color, label) in enumerate(legend_items):
    lx = leg_x + 20
    ly = leg_y + 45 + i * 26
    S(f'<line x1="{lx}" y1="{ly}" x2="{lx + 30}" y2="{ly}" '
      f'stroke="{color}" stroke-width="2.5" stroke-linecap="round"/>')
    dot(lx, ly, color, r=3)
    dot(lx + 30, ly, color, r=3)
    text(lx + 40, ly + 4, label, size=11, color=C["text_dim"], anchor="start")

# ── Pin reference table ────────────────────────────────────────
tbl_x, tbl_y = 60, 830
text(tbl_x, tbl_y, "GPIO Reference:", size=11, bold=True, color=C["text_dim"], anchor="start")
pin_refs = [
    "D4=KICK", "D12=HH_CLO", "D13=HH_OPN", "D14=TOM_L", "D15=TOM_M",
    "D32=CRASH", "D33=SNARE", "D5=SD_CS", "D18=SD_SCK",
    "D19=SD_MISO", "D23=SD_MOSI", "D22=I2S_DIN", "D25=I2S_LRC", "D26=I2S_BCLK",
]
cols = 7
for i, ref in enumerate(pin_refs):
    col = i % cols
    row = i // cols
    text(tbl_x + col * 200, tbl_y + 20 + row * 18, ref,
         size=10, color=C["text_dim"], anchor="start", italic=True)

# ── Footer ─────────────────────────────────────────────────────
text(W // 2, H - 12,
     "github.com/kiranj26/Electronic_Drum_Using_ESP32  |  Phase 4b  |  Auto-generated by docs/wiring/generate_wiring.py",
     size=10, color=C["border"], italic=True)

S('</svg>')

with open(OUT, "w") as f:
    f.write("\n".join(svg))

print(f"Generated: {OUT}")
print(f"  Canvas:     {W} x {H}")
print(f"  Components: ESP32, MicroSD Breakout, MAX98357A, Speaker, 7x Buttons")
