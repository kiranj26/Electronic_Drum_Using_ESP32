// ============================================================
// ESP32 Digital Drum Kit — Phase 0 Firmware
// UART command echo over USB Serial (115200 baud)
//
// How it works:
//   - Type a shortcut key in Serial Monitor and press Enter
//   - ESP32 echoes back the canonical drum command string
//   - Chrome web app reads that string and plays the sound
//
// Shortcut keys:
//   1 or k → KICK
//   2 or s → SNARE
//   3 or h → HIHAT_CLOSED
//   4 or H → HIHAT_OPEN
//   5 or t → TOM_LOW
//   6 or T → TOM_MID
//   7 or c → CRASH
//   8 or r → RIDE
// ============================================================

#define BAUD_RATE 115200

// Map of input characters to drum command strings
struct DrumMapping {
  char    key;
  const char* command;
};

static const DrumMapping DRUM_MAP[] = {
  { '1', "KICK"         },
  { 'k', "KICK"         },
  { '2', "SNARE"        },
  { 's', "SNARE"        },
  { '3', "HIHAT_CLOSED" },
  { 'h', "HIHAT_CLOSED" },
  { '4', "HIHAT_OPEN"   },
  { 'H', "HIHAT_OPEN"   },
  { '5', "TOM_LOW"      },
  { 't', "TOM_LOW"      },
  { '6', "TOM_MID"      },
  { 'T', "TOM_MID"      },
  { '7', "CRASH"        },
  { 'c', "CRASH"        },
  { '8', "RIDE"         },
  { 'r', "RIDE"         },
};

static const uint8_t DRUM_MAP_SIZE = sizeof(DRUM_MAP) / sizeof(DRUM_MAP[0]);

void setup() {
  Serial.begin(BAUD_RATE);
  while (!Serial) { ; }  // wait for Serial to be ready

  Serial.println("# ESP32 Drum Kit — Phase 0 ready");
  Serial.println("# Keys: 1=KICK 2=SNARE 3=HIHAT_CLOSED 4=HIHAT_OPEN");
  Serial.println("#       5=TOM_LOW 6=TOM_MID 7=CRASH 8=RIDE");
  Serial.println("# (also: k s h H t T c r)");
}

void loop() {
  if (!Serial.available()) return;

  String input = Serial.readStringUntil('\n');
  input.trim();

  if (input.length() == 0) return;

  // Check if the full command string was typed directly (e.g. from a script)
  if (is_valid_command(input)) {
    Serial.println(input);
    return;
  }

  // Single-character shortcut
  if (input.length() == 1) {
    char key = input.charAt(0);
    for (uint8_t i = 0; i < DRUM_MAP_SIZE; i++) {
      if (DRUM_MAP[i].key == key) {
        Serial.println(DRUM_MAP[i].command);
        return;
      }
    }
  }

  // Unknown input — send an error comment (lines starting with # are ignored by web app)
  Serial.print("# unknown: ");
  Serial.println(input);
}

bool is_valid_command(const String& s) {
  return s == "KICK"         ||
         s == "SNARE"        ||
         s == "HIHAT_CLOSED" ||
         s == "HIHAT_OPEN"   ||
         s == "TOM_LOW"      ||
         s == "TOM_MID"      ||
         s == "CRASH"        ||
         s == "RIDE";
}
