#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <SPIFFS.h>

// ============================================================
// ESP32 Digital Drum Kit — Phase 2
// WiFi Access Point + WebSocket → iPhone Safari → Web Audio
//
// How it works:
//   1. ESP32 boots and creates a WiFi hotspot
//   2. iPhone connects to that hotspot
//   3. iPhone opens Safari → http://192.168.4.1
//   4. ESP32 serves index.html (bundled with base64 WAV samples)
//   5. iPhone web app connects back via WebSocket on port 81
//   6. Button press → ISR → WebSocket broadcast → phone plays sound
//
// First time setup:
//   1. Run: python3 web_app/phase2/bundle.py
//   2. Flash filesystem: pio run --target uploadfs
//   3. Flash firmware:   pio run --target upload
// ============================================================

// ── WiFi credentials ─────────────────────────────────────────
#define WIFI_SSID     "DrumKit-ESP32"
#define WIFI_PASSWORD "drumkit123"
#define WIFI_CHANNEL  6

// ── Server ports ──────────────────────────────────────────────
#define HTTP_PORT      80
#define WS_PORT        81

// ── Button config ─────────────────────────────────────────────
#define DEBOUNCE_MS   10
#define NUM_BUTTONS    7

// ── Servers ───────────────────────────────────────────────────
WebServer        http_server(HTTP_PORT);
WebSocketsServer ws_server(WS_PORT);

// ── Button definitions ────────────────────────────────────────
struct Button {
  const uint8_t  pin;
  const char*    command;
};

static const Button BUTTONS[NUM_BUTTONS] = {
  {  4, "KICK"         },  // GPIO4
  {  5, "SNARE"        },  // GPIO5
  { 12, "HIHAT_CLOSED" },  // GPIO12
  { 13, "HIHAT_OPEN"   },  // GPIO13
  { 14, "TOM_LOW"      },  // GPIO14
  { 15, "TOM_MID"      },  // GPIO15
  { 18, "CRASH"        },  // GPIO18
};

// ── ISR state ────────────────────────────────────────────────
volatile bool     trigger_flags[NUM_BUTTONS]    = { false };
volatile uint32_t last_trigger_ms[NUM_BUTTONS]  = { 0 };

// ── ISRs ──────────────────────────────────────────────────────
#define MAKE_ISR(idx)                                         \
  void IRAM_ATTR isr_btn_##idx() {                            \
    uint32_t now = millis();                                  \
    if (now - last_trigger_ms[idx] >= DEBOUNCE_MS) {         \
      last_trigger_ms[idx] = now;                             \
      trigger_flags[idx] = true;                              \
    }                                                         \
  }

MAKE_ISR(0) MAKE_ISR(1) MAKE_ISR(2) MAKE_ISR(3)
MAKE_ISR(4) MAKE_ISR(5) MAKE_ISR(6)

static void (*ISR_HANDLERS[NUM_BUTTONS])() = {
  isr_btn_0, isr_btn_1, isr_btn_2, isr_btn_3,
  isr_btn_4, isr_btn_5, isr_btn_6,
};

// ── WebSocket event handler ───────────────────────────────────
void on_ws_event(uint8_t client_id, WStype_t type,
                 uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.printf("# WebSocket client %u connected\n", client_id);
      break;
    case WStype_DISCONNECTED:
      Serial.printf("# WebSocket client %u disconnected\n", client_id);
      break;
    default:
      break;
  }
}

// ── HTTP handler — serve index.html from SPIFFS ───────────────
void handle_root() {
  File f = SPIFFS.open("/index.html", "r");
  if (!f) {
    http_server.send(500, "text/plain",
      "index.html not found. Run bundle.py then uploadfs.");
    Serial.println("# ERROR: /index.html not in SPIFFS");
    return;
  }
  http_server.streamFile(f, "text/html");
  f.close();
}

// ── Setup ─────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }
  Serial.println("# ESP32 Drum Kit — Phase 2 booting...");

  // Init SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("# ERROR: SPIFFS mount failed");
    return;
  }
  Serial.println("# SPIFFS mounted");

  // Start WiFi AP
  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
  Serial.printf("# WiFi AP started: SSID=%s  IP=%s\n",
    WIFI_SSID, WiFi.softAPIP().toString().c_str());

  // Start HTTP server
  http_server.on("/", HTTP_GET, handle_root);
  http_server.begin();
  Serial.printf("# HTTP server started on port %d\n", HTTP_PORT);

  // Start WebSocket server
  ws_server.begin();
  ws_server.onEvent(on_ws_event);
  Serial.printf("# WebSocket server started on port %d\n", WS_PORT);

  // Init buttons
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    pinMode(BUTTONS[i].pin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTONS[i].pin),
                    ISR_HANDLERS[i], FALLING);
  }
  Serial.println("# Buttons ready");
  Serial.println("# ---");
  Serial.printf("# Connect iPhone to WiFi: %s\n", WIFI_SSID);
  Serial.printf("# Password: %s\n", WIFI_PASSWORD);
  Serial.println("# Then open Safari: http://192.168.4.1");
}

// ── Main loop ─────────────────────────────────────────────────
void loop() {
  ws_server.loop();
  http_server.handleClient();

  // Check button trigger flags and broadcast over WebSocket
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    if (trigger_flags[i]) {
      trigger_flags[i] = false;
      ws_server.broadcastTXT(BUTTONS[i].command);
      Serial.printf("▶ %s\n", BUTTONS[i].command);
    }
  }
}
