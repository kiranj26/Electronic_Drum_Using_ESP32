#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <SPIFFS.h>

// ============================================================
// ESP32 Digital Drum Kit — Phase 3
// FreeRTOS dual-core task split — fixes Phase 2 latency
//
// Phase 2 problem:
//   Everything ran in loop() on Core 1. Button checks competed
//   with webSocket.loop() and server.handleClient(). Under load
//   (WiFi busy) button presses could be delayed or missed.
//
// Phase 3 fix:
//   Core 0 — WiFiTask:  handles all WebSocket + HTTP traffic
//   Core 1 — InputTask: checks button flags, broadcasts commands
//   ISRs fire on any core (IRAM_ATTR guarantees fast execution)
//
// Result: button input never waits for WiFi processing.
//         WiFi never waits for button scanning.
//
// Everything else identical to Phase 2:
//   Same wiring, same GPIO pins, same web app, same commands.
//
// Setup (same as Phase 2):
//   1. python3 web_app/phase2/bundle.py
//   2. pio run --target uploadfs
//   3. pio run --target upload
// ============================================================

// ── WiFi credentials ─────────────────────────────────────────
#define WIFI_SSID     "DrumKit-ESP32"
#define WIFI_PASSWORD "drumkit123"
#define WIFI_CHANNEL  6

// ── Server ports ─────────────────────────────────────────────
#define HTTP_PORT     80
#define WS_PORT       81

// ── Button config ─────────────────────────────────────────────
#define DEBOUNCE_MS   10
#define NUM_BUTTONS   7

// ── FreeRTOS task config ──────────────────────────────────────
#define WIFI_TASK_CORE      0
#define INPUT_TASK_CORE     1
#define WIFI_TASK_PRIORITY  19
#define INPUT_TASK_PRIORITY 20    // input slightly higher — never miss a hit
#define WIFI_TASK_STACK     8192
#define INPUT_TASK_STACK    2048

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

// ── ISR state ─────────────────────────────────────────────────
// volatile: written in ISR (any core), read in InputTask (Core 1)
volatile bool     trigger_flags[NUM_BUTTONS]   = { false };
volatile uint32_t last_trigger_ms[NUM_BUTTONS] = { 0 };

// ── ISRs ──────────────────────────────────────────────────────
// IRAM_ATTR: ISR lives in IRAM — executes from any core even
// when flash cache is busy serving WiFi packets on Core 0
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
      Serial.printf("# WS client %u connected\n", client_id);
      break;
    case WStype_DISCONNECTED:
      Serial.printf("# WS client %u disconnected\n", client_id);
      break;
    default:
      break;
  }
}

// ── HTTP handler ──────────────────────────────────────────────
void handle_root() {
  File f = SPIFFS.open("/index.html", "r");
  if (!f) {
    http_server.send(500, "text/plain",
      "index.html not found. Run bundle.py then uploadfs.");
    return;
  }
  http_server.streamFile(f, "text/html");
  f.close();
}

// ── Core 0 — WiFi Task ────────────────────────────────────────
// Handles all network I/O: WebSocket polling + HTTP requests.
// Completely isolated from button input on Core 1.
void wifi_task(void* pv) {
  Serial.println("# WiFiTask  → Core 0");
  while (true) {
    ws_server.loop();
    http_server.handleClient();
    vTaskDelay(1);  // yield 1 tick (~1ms) — prevents watchdog reset
  }
}

// ── Core 1 — Input Task ───────────────────────────────────────
// Reads button trigger flags set by ISRs and broadcasts commands.
// Completely isolated from WiFi processing on Core 0.
void input_task(void* pv) {
  Serial.println("# InputTask → Core 1");
  while (true) {
    for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
      if (trigger_flags[i]) {
        trigger_flags[i] = false;                // clear flag first
        ws_server.broadcastTXT(BUTTONS[i].command);
        Serial.printf("▶ %s\n", BUTTONS[i].command);
      }
    }
    vTaskDelay(1);  // yield — keeps input loop at ~1ms resolution
  }
}

// ── Setup ─────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }
  Serial.println("# ESP32 Drum Kit — Phase 3 booting...");
  Serial.println("# FreeRTOS dual-core: WiFi=Core0  Input=Core1");

  // Init SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("# ERROR: SPIFFS mount failed");
    return;
  }
  Serial.println("# SPIFFS mounted");

  // Start WiFi AP
  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
  Serial.printf("# WiFi AP: SSID=%s  IP=%s\n",
    WIFI_SSID, WiFi.softAPIP().toString().c_str());

  // Start HTTP server
  http_server.on("/", HTTP_GET, handle_root);
  http_server.begin();
  Serial.printf("# HTTP server on port %d\n", HTTP_PORT);

  // Start WebSocket server
  ws_server.begin();
  ws_server.onEvent(on_ws_event);
  Serial.printf("# WebSocket server on port %d\n", WS_PORT);

  // Init buttons
  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    pinMode(BUTTONS[i].pin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTONS[i].pin),
                    ISR_HANDLERS[i], FALLING);
  }
  Serial.println("# Buttons ready");

  // Launch FreeRTOS tasks pinned to specific cores
  xTaskCreatePinnedToCore(
    wifi_task,          // task function
    "WiFiTask",         // name (for debugging)
    WIFI_TASK_STACK,    // stack size in bytes
    NULL,               // parameters
    WIFI_TASK_PRIORITY, // priority
    NULL,               // task handle (not needed)
    WIFI_TASK_CORE      // Core 0
  );

  xTaskCreatePinnedToCore(
    input_task,
    "InputTask",
    INPUT_TASK_STACK,
    NULL,
    INPUT_TASK_PRIORITY,
    NULL,
    INPUT_TASK_CORE     // Core 1
  );

  Serial.println("# Tasks launched");
  Serial.println("# ---");
  Serial.printf("# Connect iPhone to WiFi: %s\n", WIFI_SSID);
  Serial.printf("# Password: %s\n", WIFI_PASSWORD);
  Serial.println("# Then open Safari: http://192.168.4.1");
}

// ── Loop — intentionally empty ────────────────────────────────
// All work is done in FreeRTOS tasks above.
// loop() runs on Core 1 at idle priority — sleeping it prevents
// interference with InputTask.
void loop() {
  vTaskDelay(portMAX_DELAY);
}
