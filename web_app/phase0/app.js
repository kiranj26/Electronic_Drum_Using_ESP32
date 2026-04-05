// ============================================================
// ESP32 Digital Drum Kit — Phase 0 Web App
// Web Serial API + Web Audio API
// Chrome only (Web Serial not supported in Firefox/Safari)
// ============================================================

const BAUD_RATE = 115200;

// Maps ESP32 command strings to pad IDs
const COMMAND_TO_PAD = {
  KICK:         'kick',
  SNARE:        'snare',
  HIHAT_CLOSED: 'hihat_closed',
  HIHAT_OPEN:   'hihat_open',
  TOM_LOW:      'tom_low',
  TOM_MID:      'tom_mid',
  CRASH:        'crash',
  RIDE:         'ride',
};

// Sample filenames — place WAV files in ./samples/
const SAMPLE_FILES = {
  kick:         'samples/kick.wav',
  snare:        'samples/snare.wav',
  hihat_closed: 'samples/hihat_closed.wav',
  hihat_open:   'samples/hihat_open.wav',
  tom_low:      'samples/tom_low.wav',
  tom_mid:      'samples/tom_mid.wav',
  crash:        'samples/crash.wav',
  ride:         'samples/ride.wav',
};

// ── State ────────────────────────────────────────────────────
let audioCtx    = null;
let audioBuffers = {};   // padId → AudioBuffer
let port        = null;
let reader      = null;
let lineBuffer  = '';

// ── Audio ────────────────────────────────────────────────────
async function init_audio() {
  audioCtx = new AudioContext();
  const results = await Promise.allSettled(
    Object.entries(SAMPLE_FILES).map(async ([id, path]) => {
      try {
        const res = await fetch(path);
        if (!res.ok) throw new Error(`HTTP ${res.status}`);
        const arrayBuf = await res.arrayBuffer();
        audioBuffers[id] = await audioCtx.decodeAudioData(arrayBuf);
        set_pad_loaded(id, true);
      } catch (err) {
        console.warn(`Could not load sample "${id}" from ${path}:`, err.message);
        set_pad_loaded(id, false);
      }
    })
  );
  return results;
}

function play_pad(padId) {
  if (!audioCtx) return;
  if (audioCtx.state === 'suspended') audioCtx.resume();

  const buf = audioBuffers[padId];
  if (!buf) {
    log(`No sample loaded for: ${padId}`);
    return;
  }

  const src = audioCtx.createBufferSource();
  src.buffer = buf;
  src.connect(audioCtx.destination);
  src.start(0);

  flash_pad(padId);
}

// ── Web Serial ───────────────────────────────────────────────
async function connect_serial() {
  if (!('serial' in navigator)) {
    alert('Web Serial API not available.\nPlease use Chrome or Edge.');
    return;
  }

  try {
    port = await navigator.serial.requestPort();
    await port.open({ baudRate: BAUD_RATE });

    set_connected(true);
    log('Connected to ESP32');
    read_serial_loop();
  } catch (err) {
    if (err.name !== 'NotFoundError') {
      log(`Connection error: ${err.message}`);
    }
  }
}

async function disconnect_serial() {
  if (reader) {
    await reader.cancel();
    reader = null;
  }
  if (port) {
    await port.close();
    port = null;
  }
  set_connected(false);
  log('Disconnected');
}

async function read_serial_loop() {
  const decoder = new TextDecoderStream();
  port.readable.pipeTo(decoder.writable);
  reader = decoder.readable.getReader();

  try {
    while (true) {
      const { value, done } = await reader.read();
      if (done) break;
      lineBuffer += value;
      process_line_buffer();
    }
  } catch (err) {
    if (err.name !== 'AbortError') {
      log(`Serial read error: ${err.message}`);
    }
  } finally {
    reader = null;
    set_connected(false);
  }
}

function process_line_buffer() {
  const lines = lineBuffer.split('\n');
  lineBuffer = lines.pop();  // keep incomplete last chunk

  for (const raw of lines) {
    const line = raw.trim();
    if (!line || line.startsWith('#')) {
      if (line.startsWith('#')) log(line);
      continue;
    }
    handle_command(line);
  }
}

function handle_command(cmd) {
  const padId = COMMAND_TO_PAD[cmd];
  if (padId) {
    play_pad(padId);
    log(`▶ ${cmd}`);
  } else {
    log(`? unknown command: ${cmd}`);
  }
}

// ── UI helpers ───────────────────────────────────────────────
function flash_pad(padId) {
  const el = document.getElementById(`pad-${padId}`);
  if (!el) return;
  el.classList.add('active');
  setTimeout(() => el.classList.remove('active'), 120);
}

function set_pad_loaded(padId, loaded) {
  const el = document.getElementById(`pad-${padId}`);
  if (!el) return;
  el.classList.toggle('no-sample', !loaded);
  if (!loaded) el.title = 'Sample not loaded — place WAV in samples/';
}

function set_connected(connected) {
  document.getElementById('btn-connect').textContent    = connected ? 'Disconnect' : 'Connect to ESP32';
  document.getElementById('status-dot').className      = connected ? 'dot connected' : 'dot';
  document.getElementById('status-label').textContent  = connected ? 'Connected' : 'Not connected';
}

function log(msg) {
  const el = document.getElementById('log');
  const line = document.createElement('div');
  line.textContent = `${timestamp()} ${msg}`;
  el.appendChild(line);
  el.scrollTop = el.scrollHeight;
  // Keep log from growing forever
  while (el.children.length > 80) el.removeChild(el.firstChild);
}

function timestamp() {
  const d = new Date();
  return `[${String(d.getHours()).padStart(2,'0')}:${String(d.getMinutes()).padStart(2,'0')}:${String(d.getSeconds()).padStart(2,'0')}]`;
}

// ── Init ─────────────────────────────────────────────────────
window.addEventListener('DOMContentLoaded', async () => {
  await init_audio();
  log('Audio engine ready');

  document.getElementById('btn-connect').addEventListener('click', async () => {
    if (port) {
      await disconnect_serial();
    } else {
      await connect_serial();
    }
  });

  // On-screen pad clicks (works without ESP32)
  document.querySelectorAll('.pad').forEach(el => {
    el.addEventListener('click', () => {
      const padId = el.dataset.pad;
      play_pad(padId);
      log(`▶ ${padId.toUpperCase()} (click)`);
    });
  });

  // Keyboard shortcuts matching firmware (1-8 and k s h H t T c r)
  const KEY_MAP = {
    '1': 'kick',   'k': 'kick',
    '2': 'snare',  's': 'snare',
    '3': 'hihat_closed', 'h': 'hihat_closed',
    '4': 'hihat_open',   'H': 'hihat_open',
    '5': 'tom_low',      't': 'tom_low',
    '6': 'tom_mid',      'T': 'tom_mid',
    '7': 'crash',        'c': 'crash',
    '8': 'ride',         'r': 'ride',
  };
  document.addEventListener('keydown', e => {
    if (e.target.tagName === 'INPUT') return;
    const padId = KEY_MAP[e.key];
    if (padId) {
      play_pad(padId);
      log(`▶ ${padId.toUpperCase()} (key: ${e.key})`);
    }
  });
});
