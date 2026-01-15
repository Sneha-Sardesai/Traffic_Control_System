//Code for the Modern System:
// Non-blocking traffic controller with sensor override + debounce/debug

#define A_RED     8
#define A_YELLOW  9
#define A_GREEN   10

#define B_RED     7
#define B_YELLOW  6
#define B_GREEN   5

#define A_IR      11
#define B_IR      4

// durations (ms)
const unsigned long GREEN_DURATION      = 3000;
const unsigned long YELLOW_DURATION     = 1000;
const unsigned long SENSOR_DEBOUNCE_MS  =75;    // debounce for IR
const unsigned long MIN_GREEN_HOLD_MS   = 800;   // avoid flapping between greens

// If your IR module pulls LOW when it detects something set to LOW.
// If it pulls HIGH on detection, change to HIGH.
const int A_SENSOR_ACTIVE = LOW;
const int B_SENSOR_ACTIVE = LOW;

enum State { A_GREEN_STATE = 0, A_YELLOW_STATE = 1, B_GREEN_STATE = 2, B_YELLOW_STATE = 3 };
unsigned long stateDurations[4] = { GREEN_DURATION, YELLOW_DURATION, GREEN_DURATION, YELLOW_DURATION };

int currentState = A_GREEN_STATE;
unsigned long lastChange = 0;

// override mode:
bool overrideActive = false; // true while a sensor-requested green is active
int overrideWho = 0; // 1 = A, 2 = B
unsigned long lastImmediateSet = 0; // track when we set immediate green (respect MIN_GREEN_HOLD_MS)

// --- debouncing / stable sensor tracking ---
int A_RawLast;
unsigned long A_RawLastTime = 0;
bool A_Stable = false;

int B_RawLast;
unsigned long B_RawLastTime = 0;
bool B_Stable = false;

void setup() {
Serial.begin(115200);

pinMode(A_RED, OUTPUT);
pinMode(A_YELLOW, OUTPUT);
pinMode(A_GREEN, OUTPUT);

pinMode(B_RED, OUTPUT);
pinMode(B_YELLOW, OUTPUT);
pinMode(B_GREEN, OUTPUT);

  // Most IR modules use active-LOW => use INPUT_PULLUP.
  // If your module is active-HIGH, change to pinMode(..., INPUT).
pinMode(A_IR, INPUT_PULLUP);
pinMode(B_IR, INPUT_PULLUP);

setAllLightsOff();
applyState(A_GREEN_STATE);
currentState = A_GREEN_STATE;
lastChange = millis();

  // initialize debouncing baselines
A_RawLast = digitalRead(A_IR);
A_Stable = (A_RawLast == A_SENSOR_ACTIVE);
A_RawLastTime = millis();

B_RawLast = digitalRead(B_IR);
B_Stable = (B_RawLast == B_SENSOR_ACTIVE);
B_RawLastTime = millis();

Serial.println("Traffic controller started");
Serial.print("Initial sensors T:"); Serial.print(A_Stable ? "ON":"OFF");
Serial.print(" S:"); Serial.println(B_Stable ? "ON":"OFF");
}

void loop() {
  unsigned long now = millis();

  // --- read raw pins and debounce ---
  int A_Raw = digitalRead(A_IR);
  if (A_Raw != A_RawLast) {
A_RawLast = A_Raw;
A_RawLastTime = now;
  } else if (now - A_RawLastTime>= SENSOR_DEBOUNCE_MS) {
    bool newStable = (A_Raw == A_SENSOR_ACTIVE);
    if (newStable != A_Stable) {
A_Stable = newStable;
Serial.print("A stable -> "); Serial.println(A_Stable ? "ON":"OFF");
    }
  }

  int A_Raw = digitalRead(B_IR);
  if (B_Raw != B_RawLast) {
B_RawLast = B_Raw;
B_RawLastTime = now;
  } else if (now - B_RawLastTime>= SENSOR_DEBOUNCE_MS) {
    bool newStable = (B_Raw == B_SENSOR_ACTIVE);
    if (newStable != B_Stable) {
B_Stable = newStable;
Serial.print("B stable -> "); Serial.println(B_Stable ? "ON":"OFF");
    }
  }

  // occasional raw debug (every 500ms)
  static unsigned long lastDbg = 0;
  if (now - lastDbg> 500) {
Serial.print("RAW T:"); Serial.print(A_Raw);
Serial.print(" ("); Serial.print(A_Stable ? "STABLE ON":"STABLE OFF"); Serial.print(")");
Serial.print("  S:"); Serial.print(B_Raw);
Serial.print(" ("); Serial.print(B_Stable ? "STABLE ON":"STABLE OFF"); Serial.println(")");
lastDbg = now;
  }

  // --- priority/override logic (use stable sensor values) ---
  // If a sensor requests, give it immediate green (respect MIN_GREEN_HOLD_MS to avoid flapping)
  if (A_Stable&& !B_Stable) {
    if (!overrideActive || overrideWho != 1) {
      // if we recently set another immediate green, prevent flipping too fast
      if (now - lastImmediateSet>= MIN_GREEN_HOLD_MS) {
overrideActive = true;
overrideWho = 1;
set_A_GreenImmediate();
lastImmediateSet = now;
Serial.println("Override: A GREEN (stable)");
      }
    }
    return;
  } else if (B_Stable&& !A_Stable) {
    if (!overrideActive || overrideWho != 2) {
      if (now - lastImmediateSet>= MIN_GREEN_HOLD_MS) {
overrideActive = true;
overrideWho = 2;
set_B_GreenImmediate();
lastImmediateSet = now;
Serial.println("Override: B GREEN (stable)");
      }
    }
    return;
  } else if (A_Stable&&B_Stable) {
    // both active: prioritize A (change if you prefer alternate policy)
    if (!overrideActive || overrideWho != 1) {
      if (now - lastImmediateSet>= MIN_GREEN_HOLD_MS) {
overrideActive = true;
overrideWho = 1;
set_A_GreenImmediate();
lastImmediateSet = now;
Serial.println("Override: BOTH active, giving Tanvi priority");
      }
    }
    return;
  }

  // No sensors stable
  if (overrideActive) {
    // sensors cleared -> finish override by going to that side's yellow then continue normal cycle
overrideActive = false;
    if (overrideWho == 1) {
applyState(A_YELLOW_STATE);
currentState = A_YELLOW_STATE;
lastChange = now;
Serial.println("Override ended -> A YELLOW");
    } else if (overrideWho == 2) {
applyState(B_YELLOW_STATE);
currentState = B_YELLOW_STATE;
lastChange = now;
Serial.println("Override ended -> B YELLOW");
    }
overrideWho = 0;
    return;
  }

  // Normal state machine (non-blocking)
  if (now - lastChange>= stateDurations[currentState]) {
currentState = (currentState + 1) % 4;
applyState(currentState);
lastChange = now;

    switch (currentState) {
      case A_GREEN_STATE: Serial.println("State -> A GREEN"); break;
      case A_YELLOW_STATE: Serial.println("State -> A YELLOW"); break;
      case B_GREEN_STATE: Serial.println("State -> B GREEN"); break;
      case B_YELLOW_STATE: Serial.println("State -> B YELLOW"); break;
    }
  }
}

// Helpers
void setAllLightsOff() {
digitalWrite(A_GREEN, LOW);
digitalWrite(A_YELLOW, LOW);
digitalWrite(A_RED, LOW);

digitalWrite(B_GREEN, LOW);
digitalWrite(B_YELLOW, LOW);
digitalWrite(B_RED, LOW);
}

void applyState(int st) {
setAllLightsOff();
  if (st == A_GREEN_STATE) {
digitalWrite(A_GREEN, HIGH);
digitalWrite(B_RED, HIGH);
  } else if (st == A_YELLOW_STATE) {
digitalWrite(A_YELLOW, HIGH);
digitalWrite(B_RED, HIGH);
  } else if (st == B_GREEN_STATE) {
digitalWrite(B_GREEN, HIGH);
digitalWrite(A_RED, HIGH);
  } else if (st == B_YELLOW_STATE) {
digitalWrite(B_YELLOW, HIGH);
digitalWrite(A_RED, HIGH);
  }
}

void set_A_GreenImmediate() {
setAllLightsOff();
digitalWrite(A_GREEN, HIGH);
digitalWrite(B_RED, HIGH);
currentState = A_GREEN_STATE;
lastChange = millis();
}

void set_B_GreenImmediate() {
setAllLightsOff();
digitalWrite(B_GREEN, HIGH);
digitalWrite(A_RED, HIGH);
currentState = B_GREEN_STATE;
lastChange = millis();
}
