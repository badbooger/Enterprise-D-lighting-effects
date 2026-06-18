/*
  Engine Room ESP — Xiao ESP32-C3
  11 LED groups + ESP-NOW + OTA
*/

#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <ESPmDNS.h>
#include <NetworkUdp.h>
#include <ArduinoOTA.h>
#include <Preferences.h>
#include <math.h>

void RevertWiFi();
void StartWiFi(bool saveCredentials = false);
void handleLedCommand(int grp, int cmd, int val);
float readBatteryVolts();

const char* mdnsName      = "EngRoom";
uint32_t    last_ota_time = 0;
bool        servicesStarted = false;
bool        wifiActive      = false;
uint32_t    wifiDropMs      = 0;
#define WIFI_REVERT_MS 90000UL
uint8_t     wifiChannel   = 1;

uint8_t broadcastAddress[]  = {0x20, 0x6e, 0xf1, 0xa9, 0xa1, 0x14};  // DataPad — model install
uint8_t bridgeAddress[]     = {0xe0, 0x72, 0xa1, 0xd7, 0x37, 0x14};  // Bridge — model install
uint8_t dataPad2Address[]   = {0x30, 0xed, 0xa0, 0xac, 0xa1, 0xdc};  // DataPad 2.8" pocket remote

// ── LED command constants (must match Bridge and DataPad exactly) ─────────────
#define LED_ON             1
#define LED_OFF            2
#define LED_DIM            3
#define LED_BLINK          4
#define LED_ENGINE         5
#define LED_ELEC_SHORT     6
#define LED_STARTUP        7
#define LED_SHUTDOWN       8
#define LED_SET_BLINK_MS  20
#define LED_SET_WIN_MS    21
#define LED_SET_SPC_MS    22
#define LED_SET_AUTO_MS   23
#define LED_SET_CONN_MS   24
#define LED_SYNC_MODE     25   // assembled/separated mode (ledValue 1=assembled 0=separated)
#define LED_EFFECTS_SIMPLE 26
#define LED_ASSEMBLY_MODE  27
#define LED_WARP           28  // nacelle warp speed (ledValue 0=normal, 1-10=warp speed)
#define LED_BAT_LEVEL      31  // EngRoom → DataPad: battery mV as int
#define LED_ALL_OFF       99

// Special group constants
#define GRP_ALL_WINDOWS   18   // Bridge compat — used as "all EngRoom windows" here
#define GRP_BOTH_ENGINES  19   // Bridge engines — ignored by EngRoom LED_ENGINE handler
#define GRP_BOTH_NAC      20   // EngRoom nacelles
#define GRP_ALL           -1

// ── Shared message struct ─────────────────────────────────────────────────────
typedef struct struct_message {
  int      status, boardInd;
  char     password1[65];
  char     ssid1[33];
  int      startWifi, wifiStatus;
  uint32_t ipAddress;
  int      ledGroup, ledCmd, ledValue;
} struct_message;

struct_message      myData;
esp_now_peer_info_t peerInfo;

volatile int  startWifi1        = 0;
volatile bool saveWifi1         = false;
volatile bool pendingStateReply = false;

// Incoming command flag — set in callback, dispatched in loop
volatile bool newLedCmd  = false;
int pendingGroup = 0, pendingCmd = 0, pendingValue = 0;

// ── NVS ───────────────────────────────────────────────────────────────────────
Preferences prefs;

// ── Pin and group constants ───────────────────────────────────────────────────
#define NUM_WINDOWS 3
const int WIN_PINS[] = {20, 7, 21};  // neck (all), eng top, eng bot

#define PIN_NAV        5
#define PIN_DEFLECTOR  6
#define PIN_IMPULSE   10
#define PIN_NAC_R      3
#define PIN_NAC_L      8
#define PIN_PHOTON     4
#define PIN_BAT_ADC    2

// Group index constants (ledGroup values from DataPad)
#define IDX_NECK       0   // GPIO 20 (neck back + both battle bridges wired together)
#define IDX_ENG_TOP    1   // GPIO 7
#define IDX_ENG_BOT    2   // GPIO 21
#define IDX_NAV        5   // GPIO 5
#define IDX_DEFLECTOR  6   // GPIO 6
#define IDX_IMPULSE    7   // GPIO 10
#define IDX_NAC_RIGHT  8   // GPIO 9
#define IDX_NAC_LEFT   9   // GPIO 8
#define IDX_PHOTON    10   // GPIO 4

// All pins in order — used only by assembly mode walk
const int ALL_PINS[]  = {4, 5, 6, 7, 8, 9, 10, 20, 21};
#define NUM_ALL_PINS 9

#define LEDC_FREQ 5000
#define LEDC_BITS 8

// ── NVS timing ────────────────────────────────────────────────────────────────
uint32_t blinkIntervalMs  = 500;
uint32_t winStartupMs     = 300;
uint32_t spcStartupMs     = 800;
uint32_t autoStartDelayMs = 30000;
uint32_t connLostMs       = 300000;

// ── Auto-start ────────────────────────────────────────────────────────────────
uint32_t bootMs               = 0;
bool     bootIndicatorActive  = false;
uint32_t bootIndicatorStartMs = 0;
uint32_t bootIndicatorBlinkMs = 0;
bool     bootIndicatorPhase   = false;
bool     espNowReceived = false;
bool     autoStartFired = false;

// ── Connection monitor ────────────────────────────────────────────────────────
uint32_t lastContactMs  = 0;
bool     connectionLost  = false;
bool     intentionallyOff = false;  // set on shutdown/all-off; suppresses connection-loss auto-restart
const uint16_t CONN_LOST_PATTERN[] = {120, 80, 120, 80, 120, 1800};
#define CONN_PATTERN_LEN 6
uint8_t  connPatternIdx = 0;
uint32_t connPatternMs  = 0;

// ── Effect state machine ──────────────────────────────────────────────────────
enum Effect { EFF_IDLE, EFF_STARTUP, EFF_SHUTDOWN, EFF_ELEC_SHORT };
Effect currentEffect = EFF_IDLE;

struct SeqStep { int idx[3]; int count; };
const SeqStep STARTUP_SEQ[] = {
  {{IDX_NECK,    -1,           -1},            1},
  {{IDX_ENG_TOP, IDX_ENG_BOT, -1},            2},
  {{IDX_NAV,       -1,           -1},           1},
  {{IDX_DEFLECTOR, -1,           -1},           1},
  {{IDX_IMPULSE,   -1,           -1},           1},
  {{IDX_NAC_RIGHT, IDX_NAC_LEFT, -1},           2},
};
#define SEQ_TOTAL_STEPS 6
int      seqStep   = 0;
uint32_t lastSeqMs = 0;

// ── Nav blink ─────────────────────────────────────────────────────────────────
bool     blinkActive = false;
bool     blinkState  = false;
uint32_t lastBlinkMs = 0;

// ── Nacelle — ramp up, sine pulse, ramp down ─────────────────────────────────
#define NACELLE_RAMP_MS 5000
bool     nacelleActive       = false;
bool     nacelleRamping      = false;
bool     nacelleRampingDown  = false;
bool     nacSync             = true;
bool     nacelleWarpMode     = false;
uint8_t  nacelleWarpSpeed    = 0;
uint8_t  nacelleWarpPhase    = 0;      // 0=dim flash, 1=fast ramp, 2=pulse
uint32_t nacelleWarpPhaseMs  = 0;
uint32_t nacelleRampStartMs  = 0;
uint32_t nacelleRampDoneMs   = 0;
uint8_t  nacelleRampDownStart = 0;
uint8_t  nacelleCurrentDuty  = 0;

// ── Deflector glow ────────────────────────────────────────────────────────────
bool deflectorActive = false;

// ── Impulse pulse ─────────────────────────────────────────────────────────────
bool impulseActive = false;

// ── Photon torpedo ────────────────────────────────────────────────────────────
#define PHOTON_CHARGE_MS  750
#define PHOTON_FIRE_MS    200
bool     phtCharging = false;
bool     phtFiring   = false;
uint32_t phtStartMs  = 0;

// ── Effects simple mode (NVS-backed) ─────────────────────────────────────────
bool effectsSimple = false;

// ── Assembled/Separated sync mode ────────────────────────────────────────────
bool assembledMode = true;   // false = run independently, no Bridge sync messages

// ── Assembly mode (NVS-backed) ────────────────────────────────────────────────
bool     asmMode    = false;
uint32_t lastAsmMs  = 0;
int      asmPhase   = 0;
int      asmWalkIdx = 0;
uint32_t asmStartMs = 0;

// ── Window on/off state ───────────────────────────────────────────────────────
bool windowOn[NUM_WINDOWS] = {};

// ── Damage effect ─────────────────────────────────────────────────────────────
struct WinShortState { bool active, flickerMode, on; uint32_t nextMs; };
WinShortState winShort[NUM_WINDOWS];
uint32_t nextNacRDmgMs = 0;
uint32_t nextNacLDmgMs = 0;

bool     preDmgWindowOn[NUM_WINDOWS];
bool     preDmgNacelleActive, preDmgNacelleRamping;
bool     preDmgDeflectorActive, preDmgImpulseActive, preDmgBlinkActive;
uint32_t damageTimeoutMs = 120000;
uint32_t damageStartMs   = 0;


float readBatteryVolts() {
  return analogReadMilliVolts(PIN_BAT_ADC) * 3.83f / 1000.0f;
}


// ═══════════════════════════════════════════════════════════════════════════════
// HELPERS
// ═══════════════════════════════════════════════════════════════════════════════

// Write to a pin correctly regardless of whether it is PWM-attached
void pinWrite(int pin, bool on) {
  if (pin == PIN_NAV || pin == PIN_DEFLECTOR || pin == PIN_IMPULSE ||
      pin == PIN_NAC_R || pin == PIN_NAC_L   || pin == PIN_PHOTON) {
    ledcWrite(pin, on ? 255 : 0);
  } else {
    digitalWrite(pin, on ? HIGH : LOW);
  }
}

void allOff() {
  for (int i = 0; i < NUM_WINDOWS; i++) {
    if (winShort[i].active && winShort[i].flickerMode) {
      ledcDetach(WIN_PINS[i]);
      pinMode(WIN_PINS[i], OUTPUT);
    }
    digitalWrite(WIN_PINS[i], LOW);
    windowOn[i] = false;
  }
  ledcWrite(PIN_NAV,       0);
  ledcWrite(PIN_DEFLECTOR, 0);
  ledcWrite(PIN_IMPULSE,   0);
  ledcWrite(PIN_NAC_R,     0);
  ledcWrite(PIN_NAC_L,     0);
  ledcWrite(PIN_PHOTON,    0);
  blinkActive        = false;
  blinkState         = false;
  nacelleActive      = false;
  nacelleRamping     = false;
  nacelleRampingDown = false;
  nacelleCurrentDuty = 0;
  deflectorActive    = false;
  impulseActive      = false;
  phtCharging        = false;
  phtFiring          = false;
  nacSync            = true;
  nacelleWarpMode    = false;
  nacelleWarpSpeed   = 0;
  nacelleWarpPhase   = 0;
  nacelleWarpPhaseMs = 0;
  currentEffect      = EFF_IDLE;
}


// ═══════════════════════════════════════════════════════════════════════════════
// EFFECT UPDATES
// ═══════════════════════════════════════════════════════════════════════════════

void updateBlink() {
  if (!blinkActive) return;
  uint32_t now = millis();
  if (connectionLost) {
    if (now - connPatternMs >= CONN_LOST_PATTERN[connPatternIdx]) {
      connPatternMs  = now;
      connPatternIdx = (connPatternIdx + 1) % CONN_PATTERN_LEN;
      ledcWrite(PIN_NAV, (connPatternIdx % 2 == 0) ? 255 : 0);
    }
  } else {
    if (now - lastBlinkMs >= blinkIntervalMs) {
      lastBlinkMs = now;
      blinkState  = !blinkState;
      ledcWrite(PIN_NAV, blinkState ? 255 : 0);
    }
  }
}

void updateNacellePulse() {
  if (currentEffect == EFF_ELEC_SHORT) return;
  uint32_t now = millis();

  if (nacelleRampingDown) {
    uint32_t elapsed = now - nacelleRampStartMs;
    if (elapsed >= NACELLE_RAMP_MS) {
      ledcWrite(PIN_NAC_R, 0); ledcWrite(PIN_NAC_L, 0);
      nacelleCurrentDuty = 0; nacelleRampingDown = false; nacelleActive = false;
      return;
    }
    uint8_t duty = (uint8_t)(nacelleRampDownStart * (1.0f - elapsed / (float)NACELLE_RAMP_MS));
    ledcWrite(PIN_NAC_R, duty); ledcWrite(PIN_NAC_L, duty);
    nacelleCurrentDuty = duty;
    return;
  }

  if (nacelleRamping) {
    uint32_t elapsed = now - nacelleRampStartMs;
    if (elapsed >= NACELLE_RAMP_MS) {
      ledcWrite(PIN_NAC_R, 255); ledcWrite(PIN_NAC_L, 255);
      nacelleCurrentDuty = 255; nacelleRamping = false;
      nacelleActive = true; nacelleRampDoneMs = now;
      return;
    }
    uint8_t duty = (uint8_t)(255.0f * elapsed / NACELLE_RAMP_MS);
    ledcWrite(PIN_NAC_R, duty); ledcWrite(PIN_NAC_L, duty);
    nacelleCurrentDuty = duty;
    return;
  }

  if (!nacelleActive) return;

  static uint32_t lastNacPulseMs = 0;
  if (now - lastNacPulseMs < 20) return;
  lastNacPulseMs = now;

  if (effectsSimple) {
    ledcWrite(PIN_NAC_R, 255); ledcWrite(PIN_NAC_L, 255);
    nacelleCurrentDuty = 255; return;
  }

  if (nacelleWarpMode) {
    switch (nacelleWarpPhase) {

      case 0:  // snap to 25% — brief dim flash
        ledcWrite(PIN_NAC_R, 64); ledcWrite(PIN_NAC_L, 64);
        nacelleCurrentDuty = 64;
        if (now - nacelleWarpPhaseMs >= 3000) {
          nacelleWarpPhase   = 1;
          nacelleWarpPhaseMs = now;
        }
        return;

      case 1:  // fast ramp 25% → 100%
        {
          const uint32_t WARP_RAMP_MS = 1000;
          uint32_t elapsed = now - nacelleWarpPhaseMs;
          if (elapsed >= WARP_RAMP_MS) {
            ledcWrite(PIN_NAC_R, 255); ledcWrite(PIN_NAC_L, 255);
            nacelleCurrentDuty = 255;
            nacelleWarpPhase   = 2;
            nacelleRampDoneMs  = now;   // reference for pulse phase
            return;
          }
          uint8_t duty = (uint8_t)(64 + (uint32_t)(255 - 64) * elapsed / WARP_RAMP_MS);
          ledcWrite(PIN_NAC_R, duty); ledcWrite(PIN_NAC_L, duty);
          nacelleCurrentDuty = duty;
        }
        return;

      case 2:  // steady hold — brightness scales with speed (80%–100%)
        {
          uint8_t duty = (uint8_t)map(nacelleWarpSpeed, 1, 10, 204, 255);
          ledcWrite(PIN_NAC_R, duty); ledcWrite(PIN_NAC_L, duty);
          nacelleCurrentDuty = duty;
        }
        return;
    }
  }

  const uint32_t period  = 5000;
  const uint8_t  minDuty = 80;    // subtle pulse at 45% ceiling
  const uint8_t  maxDuty = 115;   // warp can go brighter (up to 255)
  float tA    = (float)((now - nacelleRampDoneMs) % period) / period;
  float phA   = tA * 6.2832f + 1.5708f;
  uint8_t dutyA = (uint8_t)(minDuty + (sinf(phA) + 1.0f) * 0.5f * (maxDuty - minDuty));

  if (nacSync) {
    ledcWrite(PIN_NAC_R, dutyA); ledcWrite(PIN_NAC_L, dutyA);
    nacelleCurrentDuty = dutyA;
  } else {
    float tB    = (float)(((now - nacelleRampDoneMs) + period / 2) % period) / period;
    float phB   = tB * 6.2832f + 1.5708f;
    uint8_t dutyB = (uint8_t)(minDuty + (sinf(phB) + 1.0f) * 0.5f * (maxDuty - minDuty));
    ledcWrite(PIN_NAC_R, dutyA); ledcWrite(PIN_NAC_L, dutyB);
    nacelleCurrentDuty = dutyA;
  }
}

void updateDeflectorGlow() {
  if (!deflectorActive || currentEffect == EFF_ELEC_SHORT) return;
  if (effectsSimple || nacelleWarpMode) { ledcWrite(PIN_DEFLECTOR, 255); return; }
  ledcWrite(PIN_DEFLECTOR, 153);   // 60% normal brightness
}

void updateImpulsePulse() {
  if (!impulseActive || currentEffect == EFF_ELEC_SHORT) return;
  static uint32_t lastImpMs = 0;
  uint32_t now = millis();
  if (now - lastImpMs < 20) return;
  lastImpMs = now;
  if (effectsSimple) { ledcWrite(PIN_IMPULSE, 255); return; }
  const uint32_t period = 2000;
  float phase = (float)(now % period) / period * 6.2832f;
  uint8_t duty = (uint8_t)(130 + (sinf(phase) + 1.0f) * 0.5f * 125);
  ledcWrite(PIN_IMPULSE, duty);
}

void updatePhotonTorpedo() {
  uint32_t now = millis();
  if (phtCharging) {
    uint32_t elapsed = now - phtStartMs;
    if (elapsed >= PHOTON_CHARGE_MS) {
      ledcWrite(PIN_PHOTON, 255);
      phtCharging = false; phtFiring = true; phtStartMs = now; return;
    }
    ledcWrite(PIN_PHOTON, (uint8_t)(255.0f * elapsed / PHOTON_CHARGE_MS));
    return;
  }
  if (phtFiring) {
    uint32_t elapsed = now - phtStartMs;
    if (elapsed >= PHOTON_FIRE_MS) { ledcWrite(PIN_PHOTON, 0); phtFiring = false; return; }
    ledcWrite(PIN_PHOTON, (uint8_t)(255.0f * (1.0f - elapsed / (float)PHOTON_FIRE_MS)));
  }
}

void updateStartup() {
  if (seqStep >= SEQ_TOTAL_STEPS) {
    currentEffect   = EFF_IDLE;
    myData.status   = 2;
    myData.boardInd = 2;
    esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));
    return;
  }
  uint32_t delay_ms = (seqStep >= 2) ? spcStartupMs : winStartupMs;
  if (millis() - lastSeqMs < delay_ms) return;
  lastSeqMs = millis();

  const SeqStep &s = STARTUP_SEQ[seqStep];
  bool nacDone = false;
  for (int i = 0; i < s.count; i++) {
    int idx = s.idx[i]; if (idx < 0) continue;
    if (idx == IDX_NAV) {
      blinkActive = true; lastBlinkMs = millis();
      if (assembledMode) {
        struct_message relay; memset(&relay, 0, sizeof(relay));
        relay.ledGroup = -1; relay.ledCmd = LED_BLINK; relay.ledValue = blinkIntervalMs;
        esp_now_send(bridgeAddress, (uint8_t *)&relay, sizeof(relay));
      }
    } else if (idx == IDX_DEFLECTOR) { deflectorActive = true; }
    else if (idx == IDX_IMPULSE) {
      impulseActive = true;
      if (assembledMode) {
        struct_message relay; memset(&relay, 0, sizeof(relay));
        relay.ledGroup = -1; relay.ledCmd = LED_ENGINE; relay.ledValue = 0;
        esp_now_send(bridgeAddress, (uint8_t *)&relay, sizeof(relay));
      }
    }
    else if ((idx == IDX_NAC_RIGHT || idx == IDX_NAC_LEFT) && !nacDone) {
      if (!nacelleActive && !nacelleRamping) { nacelleRamping = true; nacelleRampStartMs = millis(); }
      nacDone = true;
    } else if (idx < NUM_WINDOWS) {
      digitalWrite(WIN_PINS[idx], HIGH); windowOn[idx] = true;
    }
  }
  seqStep++;
}

void updateShutdown() {
  if (seqStep < 0) {
    allOff();
    intentionallyOff = true;
    currentEffect   = EFF_IDLE;
    myData.status   = 2;
    myData.boardInd = 2;
    esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));
    if (assembledMode) {
      struct_message relay;
      memset(&relay, 0, sizeof(relay));
      relay.ledGroup = -1; relay.ledCmd = LED_SHUTDOWN; relay.ledValue = 0;
      esp_now_send(bridgeAddress, (uint8_t *)&relay, sizeof(relay));
    }
    return;
  }
  uint32_t delay_ms = (seqStep >= 2) ? spcStartupMs : winStartupMs;
  if (millis() - lastSeqMs < delay_ms) return;
  lastSeqMs = millis();

  const SeqStep &s = STARTUP_SEQ[seqStep];
  bool nacDone = false;
  for (int i = 0; i < s.count; i++) {
    int idx = s.idx[i]; if (idx < 0) continue;
    if      (idx == IDX_NAV)       { blinkActive = false; ledcWrite(PIN_NAV, 0); }
    else if (idx == IDX_DEFLECTOR) { deflectorActive = false; ledcWrite(PIN_DEFLECTOR, 0); }
    else if (idx == IDX_IMPULSE)   { impulseActive = false; ledcWrite(PIN_IMPULSE, 0); }
    else if ((idx == IDX_NAC_RIGHT || idx == IDX_NAC_LEFT) && !nacDone) {
      nacelleActive = false; nacelleRamping = false;
      nacelleRampDownStart = nacelleCurrentDuty;
      nacelleRampStartMs   = millis();
      nacelleRampingDown   = true;
      nacDone = true;
    } else if (idx < NUM_WINDOWS) {
      digitalWrite(WIN_PINS[idx], LOW); windowOn[idx] = false;
    }
  }
  seqStep--;
}

void initElecShort() {
  int numActive = random(2, 4);
  int picks[3]  = {-1, -1, -1};
  picks[0] = random(NUM_WINDOWS);
  do { picks[1] = random(NUM_WINDOWS); } while (picks[1] == picks[0]);
  if (numActive == 3) {
    do { picks[2] = random(NUM_WINDOWS); } while (picks[2] == picks[0] || picks[2] == picks[1]);
  }
  uint32_t now = millis();
  for (int i = 0; i < NUM_WINDOWS; i++) {
    bool isActive = (i == picks[0] || i == picks[1] || (numActive == 3 && i == picks[2]));
    winShort[i].active = isActive;
    if (!isActive) { digitalWrite(WIN_PINS[i], random(2) ? HIGH : LOW); continue; }
    winShort[i].flickerMode = (i == picks[0]);
    winShort[i].on          = windowOn[i];
    winShort[i].nextMs      = now + random(200, 600);
    if (winShort[i].flickerMode) ledcAttach(WIN_PINS[i], LEDC_FREQ, LEDC_BITS);
  }
  nacSync        = false;
  nextNacRDmgMs  = now + random(200, 600);
  nextNacLDmgMs  = now + random(200, 600);
}

void updateElecShort() {
  uint32_t now = millis();
  for (int i = 0; i < NUM_WINDOWS; i++) {
    if (!winShort[i].active || now < winShort[i].nextMs) continue;
    if (winShort[i].flickerMode) {
      ledcWrite(WIN_PINS[i], (random(100) < 15) ? 0 : random(80, 256));
      winShort[i].nextMs = now + random(40, 150);
    } else {
      winShort[i].on = !winShort[i].on;
      digitalWrite(WIN_PINS[i], winShort[i].on ? HIGH : LOW);
      winShort[i].nextMs = now + random(300, 900);
    }
  }
  if (now >= nextNacRDmgMs) {
    ledcWrite(PIN_NAC_R, (random(100) < 20) ? 0 : random(80, 220));
    nextNacRDmgMs = now + random(200, 700);
  }
  if (now >= nextNacLDmgMs) {
    ledcWrite(PIN_NAC_L, (random(100) < 20) ? 0 : random(80, 220));
    nextNacLDmgMs = now + random(200, 700);
  }
}

void stopElecShort() {
  for (int i = 0; i < NUM_WINDOWS; i++) {
    if (winShort[i].active && winShort[i].flickerMode) {
      ledcDetach(WIN_PINS[i]); pinMode(WIN_PINS[i], OUTPUT);
    }
    digitalWrite(WIN_PINS[i], preDmgWindowOn[i] ? HIGH : LOW);
    windowOn[i] = preDmgWindowOn[i];
  }
  nacSync = true;
  if (preDmgNacelleActive)   { nacelleActive = true; }
  if (preDmgNacelleRamping)  { nacelleRamping = true; nacelleRampStartMs = millis(); }
  if (preDmgDeflectorActive)   deflectorActive  = true;
  if (preDmgImpulseActive)     impulseActive    = true;
  if (preDmgBlinkActive)     { blinkActive = true; lastBlinkMs = millis(); }
  currentEffect = EFF_IDLE;
}

void updateAssemblyMode() {
  if (!asmMode) return;
  uint32_t now = millis();
  if (asmPhase == 0) {
    if (now - asmStartMs >= 10000) {
      asmPhase = 1; asmWalkIdx = 0; lastAsmMs = 0;
      for (int i = 0; i < NUM_ALL_PINS; i++) pinWrite(ALL_PINS[i], false);
      return;
    }
    bool on = ((now - asmStartMs) / 500) % 2;
    for (int i = 0; i < NUM_ALL_PINS; i++) pinWrite(ALL_PINS[i], on);
  } else {
    if (now - lastAsmMs >= 400) {
      lastAsmMs = now;
      for (int i = 0; i < NUM_ALL_PINS; i++) pinWrite(ALL_PINS[i], false);
      pinWrite(ALL_PINS[asmWalkIdx], true);
      Serial.printf("ASM pin %d (GPIO%d)\n", asmWalkIdx, ALL_PINS[asmWalkIdx]);
      asmWalkIdx = (asmWalkIdx + 1) % NUM_ALL_PINS;
    }
  }
}

void updateEffects() {
  if (asmMode) { updateAssemblyMode(); return; }
  updateBlink();
  updateNacellePulse();
  updateDeflectorGlow();
  updateImpulsePulse();
  updatePhotonTorpedo();
  if (currentEffect == EFF_STARTUP)    updateStartup();
  if (currentEffect == EFF_SHUTDOWN)   updateShutdown();
  if (currentEffect == EFF_ELEC_SHORT) updateElecShort();
}


// ═══════════════════════════════════════════════════════════════════════════════
// LED COMMAND DISPATCH
// ═══════════════════════════════════════════════════════════════════════════════

void handleLedCommand(int grp, int cmd, int val) {
  Serial.printf("LED cmd: grp=%d  cmd=%d  val=%d\n", grp, cmd, val);
  switch (cmd) {

    case LED_STARTUP:
      bootIndicatorActive = false;   // allOff() below cleans up indicator LEDs
      intentionallyOff = false;
      if (currentEffect == EFF_ELEC_SHORT) stopElecShort();
      allOff();
      seqStep = 0; lastSeqMs = millis(); currentEffect = EFF_STARTUP;
      break;

    case LED_SHUTDOWN:
      if (currentEffect == EFF_ELEC_SHORT) stopElecShort();
      seqStep = SEQ_TOTAL_STEPS - 1; lastSeqMs = millis(); currentEffect = EFF_SHUTDOWN;
      { struct_message snd; memset(&snd, 0, sizeof(snd));
        snd.ledCmd = 50; snd.ledValue = (random(2) ? 2 : 3);  // SND_CMD_PLAY, file 2 or 3
        esp_now_send(bridgeAddress, (uint8_t *)&snd, sizeof(snd)); }
      break;

    case LED_ELEC_SHORT:
      if (currentEffect == EFF_STARTUP || currentEffect == EFF_SHUTDOWN) return;
      preDmgNacelleActive   = nacelleActive;
      preDmgNacelleRamping  = nacelleRamping;
      preDmgDeflectorActive = deflectorActive;
      preDmgImpulseActive   = impulseActive;
      preDmgBlinkActive     = blinkActive;
      for (int i = 0; i < NUM_WINDOWS; i++)
        preDmgWindowOn[i] = (digitalRead(WIN_PINS[i]) == HIGH);
      damageTimeoutMs = (val > 0) ? (uint32_t)val : 120000;
      damageStartMs   = millis();
      initElecShort();
      currentEffect = EFF_ELEC_SHORT;
      break;

    case LED_ALL_OFF:
      if (currentEffect == EFF_ELEC_SHORT) stopElecShort();
      allOff();
      intentionallyOff = true;
      break;

    case LED_ON:
      if (grp == GRP_ALL || grp == GRP_ALL_WINDOWS || grp < 0) {
        for (int i = 0; i < NUM_WINDOWS; i++) { digitalWrite(WIN_PINS[i], HIGH); windowOn[i] = true; }
      } else if (grp == IDX_NAV) {
        blinkActive = false; ledcWrite(PIN_NAV, 255);
      } else if (grp == IDX_DEFLECTOR) {
        deflectorActive = true;
      } else if (grp == IDX_IMPULSE) {
        impulseActive = true;
      } else if (grp == IDX_NAC_RIGHT || grp == IDX_NAC_LEFT || grp == GRP_BOTH_NAC) {
        nacelleWarpMode = false; nacelleWarpSpeed = 0; nacelleWarpPhase = 0;
        nacelleRampingDown = false;
        if (!nacelleActive && !nacelleRamping) { nacelleRamping = true; nacelleRampStartMs = millis(); }
      } else if (grp == IDX_PHOTON) {
        phtFiring = false; phtCharging = true; phtStartMs = millis();
      } else if (grp >= 0 && grp < NUM_WINDOWS) {
        digitalWrite(WIN_PINS[grp], HIGH); windowOn[grp] = true;
      }
      break;

    case LED_OFF:
      if (grp == GRP_ALL || grp < 0) {
        if (currentEffect == EFF_ELEC_SHORT) stopElecShort();
      } else if (grp == GRP_ALL_WINDOWS) {
        for (int i = 0; i < NUM_WINDOWS; i++) { digitalWrite(WIN_PINS[i], LOW); windowOn[i] = false; }
      } else if (grp == IDX_NAV) {
        blinkActive = false; ledcWrite(PIN_NAV, 0);
      } else if (grp == IDX_DEFLECTOR) {
        deflectorActive = false; ledcWrite(PIN_DEFLECTOR, 0);
      } else if (grp == IDX_IMPULSE) {
        impulseActive = false; ledcWrite(PIN_IMPULSE, 0);
      } else if (grp == IDX_NAC_RIGHT || grp == IDX_NAC_LEFT || grp == GRP_BOTH_NAC) {
        nacelleWarpMode = false; nacelleWarpSpeed = 0; nacelleWarpPhase = 0;
        nacelleActive = false; nacelleRamping = false;
        nacelleRampDownStart = nacelleCurrentDuty;
        nacelleRampStartMs = millis(); nacelleRampingDown = true;
      } else if (grp == IDX_PHOTON) {
        phtCharging = false; phtFiring = false; ledcWrite(PIN_PHOTON, 0);
      } else if (grp >= 0 && grp < NUM_WINDOWS) {
        digitalWrite(WIN_PINS[grp], LOW); windowOn[grp] = false;
      }
      break;

    case LED_DIM:
      if (grp == GRP_ALL_WINDOWS || grp == GRP_ALL || grp < 0) {
        for (int i = 0; i < NUM_WINDOWS; i++) {
          ledcAttach(WIN_PINS[i], LEDC_FREQ, LEDC_BITS);
          ledcWrite(WIN_PINS[i], (uint8_t)constrain(val, 0, 255));
        }
      } else if (grp >= 0 && grp < NUM_WINDOWS) {
        ledcAttach(WIN_PINS[grp], LEDC_FREQ, LEDC_BITS);
        ledcWrite(WIN_PINS[grp], (uint8_t)constrain(val, 0, 255));
      }
      break;

    case LED_ENGINE:
      if (grp == GRP_BOTH_NAC || grp == IDX_NAC_RIGHT || grp == IDX_NAC_LEFT || grp < 0) {
        nacelleRampingDown = false; nacelleActive = false;
        nacelleRamping = true; nacelleRampStartMs = millis();
      }
      break;

    case LED_BLINK:
      if (val > 0) blinkIntervalMs = (uint32_t)val;
      blinkActive = true; lastBlinkMs = millis();
      break;

    case LED_SET_BLINK_MS:
      blinkIntervalMs = (uint32_t)val;
      prefs.begin("engroom", false); prefs.putUInt("blink_ms", blinkIntervalMs); prefs.end();
      break;

    case LED_SET_WIN_MS:
      winStartupMs = (uint32_t)val;
      prefs.begin("engroom", false); prefs.putUInt("win_ms", winStartupMs); prefs.end();
      break;

    case LED_SET_SPC_MS:
      spcStartupMs = (uint32_t)val;
      prefs.begin("engroom", false); prefs.putUInt("spc_ms", spcStartupMs); prefs.end();
      break;

    case LED_SET_AUTO_MS:
      autoStartDelayMs = (uint32_t)val;
      prefs.begin("engroom", false); prefs.putUInt("auto_ms", autoStartDelayMs); prefs.end();
      break;

    case LED_SET_CONN_MS:
      connLostMs = (uint32_t)val;
      prefs.begin("engroom", false); prefs.putUInt("conn_ms", connLostMs); prefs.end();
      break;

    case LED_EFFECTS_SIMPLE:
      effectsSimple = (val != 0);
      prefs.begin("engroom", false); prefs.putBool("eff_simple", effectsSimple); prefs.end();
      break;

    case LED_ASSEMBLY_MODE:
      asmMode = (val != 0);
      prefs.begin("engroom", false); prefs.putBool("asm_mode", asmMode); prefs.end();
      if (asmMode) {
        allOff(); asmPhase = 0; asmWalkIdx = 0; asmStartMs = millis(); lastAsmMs = 0;
        Serial.println("Assembly mode ON");
      } else {
        Serial.println("Assembly mode OFF");
      }
      break;

    case LED_WARP:
      if (val == 0) {
        nacelleWarpMode  = false;
        nacelleWarpSpeed = 0;
        nacelleWarpPhase = 0;
      } else if (nacelleWarpMode) {
        // already in warp — speed change only, no flash
        nacelleWarpSpeed = (uint8_t)constrain(val, 1, 10);
        nacelleWarpPhase = 2;
      } else {
        // first activation — full dim-flash-ramp sequence
        nacelleWarpSpeed   = (uint8_t)constrain(val, 1, 10);
        nacelleWarpMode    = true;
        nacelleWarpPhase   = 0;
        nacelleWarpPhaseMs = millis();
        if (!nacelleActive && !nacelleRamping) {
          nacelleRamping = true; nacelleRampStartMs = millis();
        }
      }
      break;

    case LED_SYNC_MODE:
      assembledMode = (val == 1);
      prefs.begin("engroom", false); prefs.putBool("sync_mode", assembledMode); prefs.end();
      break;
  }
}


// ═══════════════════════════════════════════════════════════════════════════════
// ESP-NOW CALLBACKS
// ═══════════════════════════════════════════════════════════════════════════════

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.printf("Send status: %s  ch=%d\n", status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL", wifiChannel);
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  espNowReceived = true;
  lastContactMs  = millis();

  if (connectionLost) {
    connectionLost = false; connPatternIdx = 0;
    Serial.println("DataPad connection restored");
  }

  if      (myData.startWifi == 3) { startWifi1 = 3; saveWifi1 = false; }
  else if (myData.startWifi == 2) { startWifi1 = 1; saveWifi1 = true;  }
  else if (myData.startWifi)      { startWifi1 = 1; saveWifi1 = false; }

  if (myData.boardInd == 3 && myData.status == 1) pendingStateReply = true;
  if (myData.ledCmd != 0) {
    pendingGroup = myData.ledGroup;
    pendingCmd   = myData.ledCmd;
    pendingValue = myData.ledValue;
    newLedCmd    = true;
  }

  Serial.printf("Recv %d bytes | boardInd=%d | ledCmd=%d grp=%d val=%d | startWifi=%d\n",
                len, myData.boardInd, myData.ledCmd, myData.ledGroup, myData.ledValue, myData.startWifi);
}


// ═══════════════════════════════════════════════════════════════════════════════
// SETUP
// ═══════════════════════════════════════════════════════════════════════════════

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < NUM_WINDOWS; i++) { pinMode(WIN_PINS[i], OUTPUT); digitalWrite(WIN_PINS[i], LOW); }
  pinMode(PIN_NAV,       OUTPUT); digitalWrite(PIN_NAV,       LOW);
  pinMode(PIN_DEFLECTOR, OUTPUT); digitalWrite(PIN_DEFLECTOR, LOW);
  pinMode(PIN_IMPULSE,   OUTPUT); digitalWrite(PIN_IMPULSE,   LOW);
  pinMode(PIN_NAC_R,     OUTPUT); digitalWrite(PIN_NAC_R,     LOW);
  pinMode(PIN_NAC_L,     OUTPUT); digitalWrite(PIN_NAC_L,     LOW);
  pinMode(PIN_PHOTON,    OUTPUT); digitalWrite(PIN_PHOTON,    LOW);

  ledcAttach(PIN_NAV,       LEDC_FREQ, LEDC_BITS);
  ledcAttach(PIN_DEFLECTOR, LEDC_FREQ, LEDC_BITS);
  ledcAttach(PIN_IMPULSE,   LEDC_FREQ, LEDC_BITS);
  ledcAttach(PIN_NAC_R,     LEDC_FREQ, LEDC_BITS);
  ledcAttach(PIN_NAC_L,     LEDC_FREQ, LEDC_BITS);
  ledcAttach(PIN_PHOTON,    LEDC_FREQ, LEDC_BITS);

  analogSetPinAttenuation(PIN_BAT_ADC, ADC_11db);

  prefs.begin("engroom", false);
  blinkIntervalMs  = prefs.getUInt("blink_ms",  500);
  winStartupMs     = prefs.getUInt("win_ms",    300);
  spcStartupMs     = prefs.getUInt("spc_ms",    800);
  autoStartDelayMs = prefs.getUInt("auto_ms", 30000);
  connLostMs       = prefs.getUInt("conn_ms", 300000);
  effectsSimple    = prefs.getBool("eff_simple", false);
  asmMode          = prefs.getBool("asm_mode",    false);
  assembledMode    = prefs.getBool("sync_mode",   true);
  bool wifiAuto    = prefs.getBool("wifi_auto",  false);
  if (wifiAuto) {
    String s = prefs.getString("wifi_ssid", "");
    String p = prefs.getString("wifi_pass", "");
    if (s.length() > 0) {
      strncpy(myData.ssid1,     s.c_str(), 32); myData.ssid1[32]     = '\0';
      strncpy(myData.password1, p.c_str(), 64); myData.password1[64] = '\0';
      startWifi1 = 1;
      Serial.printf("Auto-connecting to saved WiFi: %s\n", myData.ssid1);
    }
  }
  prefs.end();

  if (asmMode) { asmPhase = 0; asmWalkIdx = 0; asmStartMs = millis(); Serial.println("Assembly mode active"); }

  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) { Serial.println("ESP-NOW init failed"); return; }
  esp_now_register_recv_cb((esp_now_recv_cb_t)OnDataRecv);
  esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0; peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) { Serial.println("Failed to add DataPad peer"); return; }

  memcpy(peerInfo.peer_addr, bridgeAddress, 6);
  peerInfo.channel = 0; peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) { Serial.println("Failed to add Bridge peer"); return; }

  memcpy(peerInfo.peer_addr, dataPad2Address, 6);
  peerInfo.channel = 0; peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) { Serial.println("Failed to add DataPad2 peer"); return; }

  esp_wifi_get_channel(&wifiChannel, NULL);
  Serial.printf("ESP-NOW ready  ch=%d\n", wifiChannel);

  myData.status = 1; myData.boardInd = 2;
  myData.ledValue = asmMode ? 1 : 0;
  esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));
  esp_now_send(dataPad2Address, (uint8_t *)&myData, sizeof(myData));
  myData.ledValue = 0;

  ArduinoOTA
    .onStart([]() {
      Serial.println("OTA start: " + String(ArduinoOTA.getCommand() == U_FLASH ? "sketch" : "filesystem"));
    })
    .onEnd([]()   { Serial.println("\nOTA end"); })
    .onProgress([](unsigned int progress, unsigned int total) {
      if (millis() - last_ota_time > 500) {
        Serial.printf("OTA progress: %u%%\n", progress / (total / 100));
        last_ota_time = millis();
      }
    })
    .onError([](ota_error_t error) {
      const char* msg[] = {"Auth Failed","Begin Failed","Connect Failed","Receive Failed","End Failed"};
      if (error <= OTA_END_ERROR) Serial.printf("OTA Error[%u]: %s\n", error, msg[error]);
    });
  ArduinoOTA.setHostname("EngRoom");

  bootMs = millis(); lastContactMs = millis();
  Serial.printf("NVS: blink=%u win=%u spc=%u auto=%u conn=%u asm=%d simple=%d\n",
                blinkIntervalMs, winStartupMs, spcStartupMs, autoStartDelayMs, connLostMs,
                asmMode, effectsSimple);

  bootIndicatorActive  = true;
  bootIndicatorStartMs = millis();
  bootIndicatorBlinkMs = millis();
  bootIndicatorPhase   = true;
  ledcWrite(PIN_NAV,   255);
  ledcWrite(PIN_NAC_R, 255);
  ledcWrite(PIN_NAC_L, 255);
}


// ═══════════════════════════════════════════════════════════════════════════════
// LOOP
// ═══════════════════════════════════════════════════════════════════════════════

void loop() {
  if (startWifi1 == 3) {
    startWifi1 = 0; RevertWiFi();
  } else if (startWifi1 == 1) {
    bool doSave = saveWifi1; startWifi1 = 0; saveWifi1 = false; StartWiFi(doSave);
  }

  if (wifiActive) {
    if (WiFi.status() != WL_CONNECTED) {
      if (wifiDropMs == 0) wifiDropMs = millis();
      else if (millis() - wifiDropMs >= WIFI_REVERT_MS) {
        Serial.println("WiFi dropped — auto-reverting to ESP-NOW ch=1");
        wifiDropMs = 0; startWifi1 = 3;
      }
    } else { wifiDropMs = 0; }
  }

  if (bootIndicatorActive) {
    if (millis() - bootIndicatorStartMs >= 20000) {
      bootIndicatorActive = false;
      ledcWrite(PIN_NAV,   0);
      ledcWrite(PIN_NAC_R, 0);
      ledcWrite(PIN_NAC_L, 0);
    } else if (millis() - bootIndicatorBlinkMs >= 500) {
      bootIndicatorBlinkMs = millis();
      bootIndicatorPhase   = !bootIndicatorPhase;
      uint8_t v = bootIndicatorPhase ? 255 : 0;
      ledcWrite(PIN_NAV,   v);
      ledcWrite(PIN_NAC_R, v);
      ledcWrite(PIN_NAC_L, v);
    }
  }

  if (newLedCmd) {
    newLedCmd = false;
    handleLedCommand(pendingGroup, pendingCmd, pendingValue);
  }

  if (!autoStartFired && !espNowReceived &&
      autoStartDelayMs > 0 && millis() - bootMs >= autoStartDelayMs) {
    autoStartFired = true;
    Serial.println("Auto-start: no contact, running startup");
    handleLedCommand(-1, LED_STARTUP, 0);
  }

  if (!connectionLost && espNowReceived &&
      connLostMs > 0 && millis() - lastContactMs >= connLostMs) {
    connectionLost = true;
    Serial.println("DataPad connection lost — fault blink on NAV");
    if (!intentionallyOff) {
      if (currentEffect == EFF_IDLE && !nacelleActive && !blinkActive) {
        Serial.println("Running default startup");
        handleLedCommand(-1, LED_STARTUP, 0);
      }
      blinkActive = true; connPatternIdx = 0; connPatternMs = millis();
      ledcWrite(PIN_NAV, 255);
    }
  }

  if (currentEffect == EFF_ELEC_SHORT && millis() - damageStartMs >= damageTimeoutMs) {
    stopElecShort();
    struct_message reply; memset(&reply, 0, sizeof(reply));
    reply.boardInd = 2; reply.status = 3;
    esp_now_send(broadcastAddress, (uint8_t *)&reply, sizeof(reply));
    esp_now_send(dataPad2Address, (uint8_t *)&reply, sizeof(reply));
    Serial.println("Damage auto-cancelled, state restored");
  }

  if (pendingStateReply) {
    pendingStateReply = false;
    struct_message reply; memset(&reply, 0, sizeof(reply));
    reply.boardInd = 2; reply.status = 1;
    reply.ledValue = asmMode ? 1 : 0;
    esp_now_send(broadcastAddress, (uint8_t *)&reply, sizeof(reply));
  }

  static uint32_t lastStatusPingMs = 0;
  if (millis() - lastStatusPingMs >= 30000) {
    lastStatusPingMs = millis();
    struct_message ping; memset(&ping, 0, sizeof(ping));
    ping.boardInd = 2; ping.status = 1;
    ping.ledValue = asmMode ? 1 : 0;
    if (wifiActive && WiFi.status() == WL_CONNECTED) {
      ping.wifiStatus = 3;
      ping.ipAddress  = (uint32_t)WiFi.localIP();
    }
    esp_now_send(broadcastAddress, (uint8_t *)&ping, sizeof(ping));
    esp_now_send(dataPad2Address, (uint8_t *)&ping, sizeof(ping));

    struct_message batMsg; memset(&batMsg, 0, sizeof(batMsg));
    batMsg.boardInd = 2;
    batMsg.ledCmd   = LED_BAT_LEVEL;
    batMsg.ledValue = (int)(readBatteryVolts() * 1000.0f);
    esp_now_send(broadcastAddress, (uint8_t *)&batMsg, sizeof(batMsg));
  }

  updateEffects();
  ArduinoOTA.handle();
}


// ═══════════════════════════════════════════════════════════════════════════════
// WIFI CONNECT
// ═══════════════════════════════════════════════════════════════════════════════

void RevertWiFi() {
  prefs.begin("engroom", false); prefs.putBool("wifi_auto", false); prefs.end();
  WiFi.disconnect();
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  wifiChannel = 1;
  wifiActive  = false;
  wifiDropMs  = 0;
  Serial.printf("WiFi reverted. ESP-NOW ch=%d\n", wifiChannel);
  servicesStarted = false;
}

void StartWiFi(bool saveCredentials) {
  const char* ssid     = myData.ssid1;
  const char* password = myData.password1;
  Serial.printf("Connecting to: %s\n", ssid);

  bool connected = false;
  for (int attempt = 1; attempt <= 2 && !connected; attempt++) {
    Serial.printf("WiFi attempt %d/2...\n", attempt);
    WiFi.disconnect(); WiFi.begin(ssid, password);
    unsigned long startTime = millis();
    while (millis() - startTime < 10000) {
      if (WiFi.status() == WL_CONNECTED) { connected = true; break; }
      delay(100); Serial.print(".");
    }
    Serial.println();
  }

  if (!connected) {
    WiFi.disconnect();
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    wifiChannel = 1;
    prefs.begin("engroom", false); prefs.putBool("wifi_auto", false); prefs.end();
    Serial.printf("WiFi: did not connect after 2 attempts. ESP-NOW ch=%d\n", wifiChannel);
    myData.boardInd = 2; myData.wifiStatus = 2;
    esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));
    return;
  }

  Serial.printf("WiFi connected. IP: %s  ch=%d\n", WiFi.localIP().toString().c_str(), WiFi.channel());
  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_now_del_peer(broadcastAddress);
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0; peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
  esp_now_del_peer(bridgeAddress);
  memcpy(peerInfo.peer_addr, bridgeAddress, 6);
  peerInfo.channel = 0; peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
  wifiChannel = (uint8_t)WiFi.channel();
  Serial.printf("ESP-NOW peers refreshed  ch=%d\n", wifiChannel);

  myData.boardInd = 2; myData.wifiStatus = 3; myData.ipAddress = (uint32_t)WiFi.localIP();
  snprintf(myData.ssid1, sizeof(myData.ssid1), "%s", WiFi.macAddress().c_str());
  esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));

  if (saveCredentials) {
    prefs.begin("engroom", false);
    prefs.putString("wifi_ssid", String(myData.ssid1));
    prefs.putString("wifi_pass", String(myData.password1));
    prefs.putBool("wifi_auto",   true);
    prefs.end();
    Serial.println("WiFi credentials saved to NVS");
  }

  if (!servicesStarted) {
    if (MDNS.begin(mdnsName)) Serial.printf("mDNS started: http://%s.local\n", mdnsName);
    else                      Serial.println("mDNS failed — continuing without it");
    ArduinoOTA.begin();
    servicesStarted = true;
    wifiActive = true;
    wifiDropMs = 0;
  }
}
