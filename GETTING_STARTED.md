# Getting Started — Phase 0

This guide takes you from zero to hearing drum sounds in your browser, triggered by your ESP32 over USB. No extra hardware needed.

---

## Part 1 — Install PlatformIO in VS Code

> Do this once. Skip if you already have PlatformIO.

1. Open **VS Code**
2. Click the **Extensions** icon on the left sidebar (looks like 4 squares)
3. Search for **PlatformIO IDE**
4. Click **Install** — it's the one by PlatformIO
5. Wait for it to finish. VS Code will ask you to **reload** — do it.
6. After reload you'll see a **alien head icon** (🪐) in the left sidebar. That's PlatformIO.

---

## Part 2 — Open the Firmware Project

1. In VS Code, go to **File → Open Folder**
2. Navigate to:
   ```
   Electronic_Drum_Using_ESP32/firmware/phase0/
   ```
   Open **that folder specifically** — not the root repo folder.
3. VS Code will detect the `platformio.ini` file and set up the project automatically.
   - You'll see a progress bar at the bottom. Wait for it to finish (first time takes 1–2 min, it's downloading the ESP32 toolchain).

---

## Part 3 — Connect the ESP32

1. Plug your **ESP32-M1 DevKit into your Mac** using a USB cable
2. Open **Terminal** in VS Code (menu: **Terminal → New Terminal**)
3. Run this to confirm your Mac can see it:
   ```
   ls /dev/cu.*
   ```
   You should see something like `/dev/cu.usbserial-0001` or `/dev/cu.SLAB_USBtoUART` in the list.
   - If you see nothing: your USB cable might be charge-only (no data). Try a different cable.

---

## Part 4 — Flash the Firmware

1. Look at the **blue bar at the very bottom** of VS Code (the status bar)
2. Click the **→ Upload** button (right arrow icon) in that bottom bar
   - Or press: `Cmd + Shift + P` → type `PlatformIO: Upload` → Enter
3. VS Code will compile the code and flash it to the ESP32.
   - You'll see a progress log in the terminal. It ends with `SUCCESS`.
   - If you see `[ERROR] No serial port found`: your ESP32 isn't detected — check the USB cable.

---

## Part 5 — Test the Firmware with Serial Monitor

1. In the bottom blue bar, click the **plug icon** (Serial Monitor)
   - Or press: `Cmd + Shift + P` → type `PlatformIO: Serial Monitor` → Enter
2. You should immediately see:
   ```
   # ESP32 Drum Kit — Phase 0 ready
   # Keys: 1=KICK  2=SNARE  3=HIHAT_CLOSED  4=HIHAT_OPEN
   #       5=TOM_LOW  6=TOM_MID  7=CRASH  8=RIDE
   ```
3. Type `1` and press **Enter**. You should see:
   ```
   KICK
   ```
4. That means the ESP32 is working. It's sending `KICK` over the USB cable.

> If nothing appears: press the **EN/RST button** on the ESP32 to reboot it.

---

## Part 6 — Get Drum Samples

The web app needs 8 WAV files. Get them free from the internet:

1. Go to **freesound.org** (free, no account needed to download with account)
   - Or use any drum sample pack you have — any WAV works
2. Download one WAV for each: kick, snare, hi-hat closed, hi-hat open, low tom, mid tom, crash, ride
3. Convert them to the right format using **Audacity** (free):
   - Open the WAV in Audacity
   - **Tracks → Stereo to Mono** (if it's stereo)
   - **Tracks → Resample** → set to **22050 Hz**
   - **File → Export → Export as WAV** → 16-bit PCM
4. Rename and place the files here (exact names matter):
   ```
   Electronic_Drum_Using_ESP32/web_app/phase0/samples/kick.wav
   Electronic_Drum_Using_ESP32/web_app/phase0/samples/snare.wav
   Electronic_Drum_Using_ESP32/web_app/phase0/samples/hihat_closed.wav
   Electronic_Drum_Using_ESP32/web_app/phase0/samples/hihat_open.wav
   Electronic_Drum_Using_ESP32/web_app/phase0/samples/tom_low.wav
   Electronic_Drum_Using_ESP32/web_app/phase0/samples/tom_mid.wav
   Electronic_Drum_Using_ESP32/web_app/phase0/samples/crash.wav
   Electronic_Drum_Using_ESP32/web_app/phase0/samples/ride.wav
   ```

---

## Part 7 — Open the Web App

The web app is a plain HTML file. But Chrome blocks audio from files opened directly from disk — you need to serve it with a simple local server.

1. Open a **new Terminal** (not the PlatformIO one)
2. Navigate to the web app folder:
   ```bash
   cd path/to/Electronic_Drum_Using_ESP32/web_app/phase0
   ```
3. Start a local server with Python (already on your Mac):
   ```bash
   python3 -m http.server 8080
   ```
4. Open **Chrome** and go to:
   ```
   http://localhost:8080
   ```
5. You'll see the drum pad grid.

> Must be **Chrome**. Web Serial API does not work in Firefox or Safari.

---

## Part 8 — Connect Everything Together

1. Make sure the ESP32 is still plugged in and the firmware is flashed (Part 4 done)
2. **Close the PlatformIO Serial Monitor** — Chrome and Serial Monitor cannot use the same port at the same time
3. In Chrome at `http://localhost:8080`, click **Connect to ESP32**
4. A popup appears listing serial ports — select your ESP32 port (e.g. `/dev/cu.usbserial-0001`)
5. Click **Connect**
6. The status dot turns **green**

---

## Part 9 — Play Drums

**From the web app (no ESP32 needed for this):**
- Click any pad with your mouse
- Or press keyboard keys: `1` `2` `3` `4` `5` `6` `7` `8`

**From the ESP32 (end-to-end test):**
- Open PlatformIO Serial Monitor, type `1`, press Enter
- You should hear the kick drum in your browser

That's Phase 0 — fully working.

---

## Troubleshooting

| Problem | Fix |
|---------|-----|
| `No serial port found` during upload | Try a different USB cable (charge-only cables won't work) |
| Serial Monitor shows nothing | Press the EN/RST button on the ESP32 to reboot |
| Chrome shows no serial ports | Make sure Serial Monitor is closed first |
| Pads are greyed out in browser | WAV files missing or wrong filename — check `samples/` folder |
| No sound in browser | Click anywhere on the page first (browser requires a user gesture to start audio) |
| `python3 -m http.server` not found | Install Python from python.org |

---

## Key Shortcuts Reference

| Key | Drum |
|-----|------|
| `1` or `k` | Kick |
| `2` or `s` | Snare |
| `3` or `h` | Hi-Hat Closed |
| `4` or `H` | Hi-Hat Open |
| `5` or `t` | Tom Low |
| `6` or `T` | Tom Mid |
| `7` or `c` | Crash |
| `8` or `r` | Ride |
