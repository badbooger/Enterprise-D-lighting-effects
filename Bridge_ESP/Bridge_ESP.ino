/*
  Bridge ESP — ESP32-S3
  18 LED groups + DY-SV17F sound + ESP-NOW + OTA
  GPIO outputs: see "GPIO outputs Bridge.txt"
*/

#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <NetworkUdp.h>
#include <ArduinoOTA.h>

// ── Network / OTA ─────────────────────────────────────────────────────────────
void RevertWiFi();
void StartWiFi(bool saveCredentials = false);

const char* mdnsName      = "Bridge";
bool        servicesStarted = false;
bool        wifiActive      = false;   // true after StartWiFi() succeeds
uint32_t    wifiDropMs      = 0;       // millis() when WiFi drop first detected
#define WIFI_REVERT_MS 90000UL         // auto-revert after 90s of dropped WiFi
uint8_t     wifiChannel   = 1;   // cached channel — updated from main task, safe to read in callbacks

// ── MAC addresses ─────────────────────────────────────────────────────────────
uint8_t broadcastAddress1[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // EngRoom  — run MAC_address_retriver on your board
uint8_t broadcastAddress2[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // DataPad — run MAC_address_retriver on your board

// ── Sound (DY-SV17F on UART2) ────────────────────────────────────────────────
#define USE_SOUND
#define SOUND_TX    17
#define SOUND_RX    18
#define SOUND_BAUD  9600

// File ranges on the DY-SV17F — files named 00001.mp3 … 00025.mp3
// Each effect picks a random file from its range at play time.
#define SND_POWERUP_FILE      1   // 01 tng_poweringup — always this on startup
#define SND_SHUTDOWN_FIRST    2   // 02 ship_shutdown
#define SND_SHUTDOWN_LAST     3   // 03 tng_poweringdown

#define SND_ALERT_FIRST    4   // 04 alertklaxon_clean2
#define SND_ALERT_LAST     6   // 05 alert01  06 alert08

// Damage sequence: alert klaxon plays first, then cuts to a random damage sound
#define SND_DMG_ALERT_1    4   // alert08 (short beep)
#define SND_DMG_ALERT_2    6   // alertklaxon_clean2 (long — gets cut off at DMG_ALERT_MS)
#define DMG_ALERT_MS    2500   // ms of alert before switching to damage sound

#define SND_DAMAGE_FIRST   7   // 07 largeexplosion1  08 largeexplosion3
#define SND_DAMAGE_LAST   12   // 09 shield_sizzle  10 smallexplosion2
                               // 11 starboardnacellenotfunctional  12 tng_powerloss

#define SND_WARP_FIRST    13   // 13 tng_flyby2  14 tng_warp4  15 tng_warp7
#define SND_WARP_LAST     18   // 16 tng_warp_exit  17 tng_warp_flash  18 tng_warp_out2

#define SND_WEAPONS_FIRST 19   // 19 tng_fireallweapons  20 tng_phaser  21 tng_phaser_strike
#define SND_WEAPONS_LAST  25   // 22 tng_torpedo2  23 tng_torpedo  24 tng_weapons2  25 tng_weapons

// Sound commands — sent via ledCmd field from DataPad (must match DataPad constants)
#define SND_CMD_PLAY    50   // ledValue = file number 1–25
#define SND_CMD_STOP    51
#define SND_CMD_VOL     52   // ledValue = 0–30
#define SND_CMD_REPEAT  53   // ledValue = 0=off  1=loop one  2=loop all

// ── LED command constants (identical across all three units) ──────────────────
#define GRP_ALL_WINDOWS   18
#define GRP_BOTH_ENGINES  19
#define GRP_ALL          -1
#define LED_ON            1
#define LED_OFF           2
#define LED_DIM           3
#define LED_BLINK         4
#define LED_ENGINE        5
#define LED_ELEC_SHORT    6
#define LED_STARTUP       7
#define LED_SHUTDOWN      8
#define LED_SET_BLINK_MS  20
#define LED_SET_WIN_MS    21
#define LED_SET_SPC_MS    22
#define LED_SET_AUTO_MS   23   // set auto-start timeout; ledValue = ms (0 = disable)
#define LED_SET_CONN_MS   24   // set connection-lost timeout; ledValue = ms
#define LED_SYNC_MODE     25   // assembled/separated mode (ledValue 1=assembled 0=separated)
#define LED_EFFECTS_SIMPLE 26  // EngRoom only: nacelle/deflector/impulse → constant ON
#define LED_ASSEMBLY_MODE  27  // toggle assembly blink mode (ledValue 1=on 0=off)
#define LED_WARP           28  // nacelle/WarpCore warp speed (ledValue 0=off, 1-10=speed)
#define LED_BAT_LEVEL     31   // Bridge → DataPad: battery voltage (ledValue = mV)
#define LED_ALL_OFF       99

// ── Shared message struct (identical across all three units) ──────────────────
typedef struct struct_message {
  int      status;
  int      boardInd;
  char     password1[65];
  char     ssid1[33];
  int      startWifi;
  int      wifiStatus;
  uint32_t ipAddress;
  int      ledGroup;
  int      ledCmd;
  int      ledValue;
} struct_message;

struct_message      myData;
esp_now_peer_info_t peerInfo;

// ── ESP-NOW incoming command flags ────────────────────────────────────────────
volatile int  startWifi1       = 0;
volatile bool newLedCmd        = false;
volatile bool pendingStateReply = false;
int pendingGroup = 0;
int pendingCmd   = 0;
int pendingValue = 0;

// ── Pin map — indices 0-17, ordered to match STARTUP_SEQ ─────────────────────
//               [0] [1] [2] [3] [4]  [5]  [6]  [7]  [8]  [9] [10] [11] [12] [13] [14] [15] [16] [17]
int pinArray[] = { 6,  7, 35,  5, 15,  21,  16,  14,  13,  12,  11,  10,   8,   3,   9,  38,  36,  37};
//               G01 G02 G03 G04 G05  G06  G07  G08  G09  G10  G11  G12  G13  G14  G15  NAV  IML  IMR
#define NUM_GROUPS  18
#define NUM_WINDOWS 15  // indices 0-14
#define IDX_NAV     15
#define IDX_ENG_A   16
#define IDX_ENG_B   17

// ── LEDC (ESP32 Arduino core 3.x API — pin-based, no channel numbers) ─────────
#define LEDC_FREQ  5000
#define LEDC_BITS  8
// Convenience pin accessors
#define PIN_ENG_A    pinArray[IDX_ENG_A]   // GPIO 36
#define PIN_ENG_B    pinArray[IDX_ENG_B]   // GPIO 37
#define PIN_NAV      pinArray[IDX_NAV]     // GPIO 38
#define PIN_BAT_ADC  2                     // ADC1 — voltage divider R1=270kΩ R2=100kΩ

bool windowZoneDimActive = false;   // true when all windows are ledcAttach'd for zone dim

// ── NVS timing settings ───────────────────────────────────────────────────────
Preferences  prefs;
uint32_t blinkIntervalMs  = 500;    // NAV blink toggle interval
uint32_t winStartupMs     = 300;    // delay between window steps in startup
uint32_t spcStartupMs     = 800;    // delay before NAV and engines in startup
uint32_t autoStartDelayMs = 10000;  // auto-startup if no ESP-NOW contact (0 = disabled)

// ── Assembly mode ─────────────────────────────────────────────────────────────
// Alternates odd/even window groups to help identify panels during reassembly
bool     asmMode    = false;
uint32_t lastAsmMs  = 0;
bool     asmPhase   = false;   // false = odd groups ON, true = even groups ON

// ── Assembled/Separated sync mode ────────────────────────────────────────────
bool assembledMode = true;   // false = each unit runs independently
bool startupSolo   = false;  // set per LED_STARTUP command; true = run full sequence

// ── Auto-start fallback ───────────────────────────────────────────────────────
uint32_t bootMs               = 0;
bool     bootIndicatorActive  = false;
uint32_t bootIndicatorStartMs = 0;
uint32_t bootIndicatorBlinkMs = 0;
bool     bootIndicatorPhase   = false;
bool     espNowReceived  = false;
bool     autoStartFired  = false;

// ── WiFi save flag (startWifi=2 = connect + save to NVS) ─────────────────────
volatile bool saveWifi1 = false;

// ── Connection monitor ────────────────────────────────────────────────────────
uint32_t connLostMs      = 300000; // 5 min — declare lost if no message for this long
uint32_t lastContactMs   = 0;      // updated on every received ESP-NOW message
bool     connectionLost  = false;

// NAV fault-blink pattern: 3 quick flashes then a long pause
// Array is alternating on/off durations in ms; even index = ON, odd index = OFF
const uint16_t CONN_LOST_PATTERN[] = {120, 80, 120, 80, 120, 1800};
#define CONN_PATTERN_LEN 6
uint8_t  connPatternIdx = 0;
uint32_t connPatternMs  = 0;

// ── Global effect state ───────────────────────────────────────────────────────
enum Effect { EFF_IDLE, EFF_STARTUP, EFF_SHUTDOWN, EFF_ELEC_SHORT };
Effect   currentEffect = EFF_IDLE;

// ── Blink (NAV) state ─────────────────────────────────────────────────────────
bool     blinkState    = false;
bool     blinkActive   = false;
uint32_t lastBlinkMs   = 0;

// ── Engine flicker state ──────────────────────────────────────────────────────
bool     engActive     = false;
bool     engSync       = true;   // true = A+B together, false = independent (damage)

// ── Startup / shutdown sequencer ──────────────────────────────────────────────
struct SeqStep { int idx[2]; int count; };

const SeqStep STARTUP_SEQ[] = {
  {{14, -1}, 1},   // Step 0:  G15 alone
  {{13, -1}, 1},   // Step 1:  G14 alone
  {{ 1, -1}, 1},   // Step 2:  G02 alone
  {{ 0,  2}, 2},   // Step 3:  G01 + G03 (G02 already on — G01,G02,G03 now lit)
  {{ 8, -1}, 1},   // Step 4:  G09 alone
  {{ 7,  9}, 2},   // Step 5:  G08 + G10
  {{ 6, 10}, 2},   // Step 6:  G07 + G11
  {{ 5, 11}, 2},   // Step 7:  G06 + G12
  {{ 4, 12}, 2},   // Step 8:  G05 + G13
  {{ 3, -1}, 1},   // Step 9:  G04 alone
  {{15, -1}, 1},   // Step 10: NAV — starts blinking  (spcStartupMs delay before this)
  {{16, 17}, 2},   // Step 11: IM LEFT + IM RIGHT      (spcStartupMs delay before this)
};
#define SEQ_TOTAL_STEPS 12

int      seqStep   = 0;
uint32_t lastSeqMs = 0;

// ── Electrical short state ────────────────────────────────────────────────────
struct WinShortState {
  bool     active;         // true = participates in effect; false = frozen at pre-damage state
  bool     flickerMode;    // true = PWM flicker (ledcAttach'd), false = slow on/off
  bool     on;
  uint32_t nextMs;
};
WinShortState winShort[NUM_WINDOWS];
uint32_t nextEngDmgAMs = 0;
uint32_t nextEngDmgBMs = 0;

// Damage auto-cancel state (timer and pre-damage restore)
bool     preDmgEngActive        = false;
bool     preDmgBlinkActive      = false;
bool     preDmgZoneActive       = false;
bool     preDmgWindowOn[NUM_WINDOWS];
uint32_t damageTimeoutMs   = 120000;
uint32_t damageStartMs     = 0;

// Damage sound sequencing: phase 1 = alert playing, phase 2 = damage sound playing
uint8_t  dmgSndPhase  = 0;
uint32_t dmgSndNextMs = 0;


// ═══════════════════════════════════════════════════════════════════════════════
// LED HELPERS
// ═══════════════════════════════════════════════════════════════════════════════

void winWrite(int idx, bool on) {
  if (windowZoneDimActive) return;   // zone dim has control, ignore individual writes
  digitalWrite(pinArray[idx], on ? HIGH : LOW);
}

void allWindowsOff() {
  if (windowZoneDimActive) {
    for (int i = 0; i < NUM_WINDOWS; i++) ledcDetach(pinArray[i]);
    windowZoneDimActive = false;
  }
  for (int i = 0; i < NUM_WINDOWS; i++) {
    pinMode(pinArray[i], OUTPUT);
    digitalWrite(pinArray[i], LOW);
  }
}

void allOff() {
  allWindowsOff();
  ledcWrite(PIN_ENG_A, 0);
  ledcWrite(PIN_ENG_B, 0);
  ledcWrite(PIN_NAV,   0);
  blinkActive   = false;
  engActive     = false;
  blinkState    = false;
  currentEffect = EFF_IDLE;
}

void setZoneDim(uint8_t brightness) {
  if (!windowZoneDimActive) {
    for (int i = 0; i < NUM_WINDOWS; i++)
      ledcAttach(pinArray[i], LEDC_FREQ, LEDC_BITS);
    windowZoneDimActive = true;
  }
  for (int i = 0; i < NUM_WINDOWS; i++) ledcWrite(pinArray[i], brightness);
}

void releaseZoneDim(bool leaveOn) {
  if (!windowZoneDimActive) return;
  for (int i = 0; i < NUM_WINDOWS; i++) {
    ledcDetach(pinArray[i]);
    pinMode(pinArray[i], OUTPUT);
    digitalWrite(pinArray[i], leaveOn ? HIGH : LOW);
  }
  windowZoneDimActive = false;
}


// ═══════════════════════════════════════════════════════════════════════════════
// BATTERY ADC
// ═══════════════════════════════════════════════════════════════════════════════

float batVolts = 0.0f;

float readBatteryVolts() {
  return analogReadMilliVolts(PIN_BAT_ADC) * 3.70f / 1000.0f;
}


// ═══════════════════════════════════════════════════════════════════════════════
// SOUND (DY-SV17F)
// ═══════════════════════════════════════════════════════════════════════════════
#ifdef USE_SOUND

uint8_t soundVolume = 15;

void soundSend(uint8_t cmd, uint8_t *data, uint8_t len) {
  uint8_t sum = 0xAA + cmd + len;
  Serial2.write(0xAA);
  Serial2.write(cmd);
  Serial2.write(len);
  for (uint8_t i = 0; i < len; i++) { Serial2.write(data[i]); sum += data[i]; }
  Serial2.write(sum & 0xFF);
}

void soundSetVolume(uint8_t vol) {
  if (vol > 30) vol = 30;
  soundVolume = vol;
  soundSend(0x13, &vol, 1);
}

void soundPlayFile(uint8_t fileNum) {
  uint8_t data[2] = {0x00, fileNum};
  soundSend(0x07, data, 2);
}

void soundPlayRandom(uint8_t first, uint8_t last) {
  soundPlayFile(first + random(last - first + 1));
}

void soundStop() {
  soundSend(0x04, nullptr, 0);
}

void soundSetRepeat(uint8_t mode) {
  // mode: 0=play once  1=loop one  2=loop all
  soundSend(0x11, &mode, 1);
}

void soundInit() {
  delay(500);  // let module settle after power-on
  soundSetVolume(soundVolume);
}

#endif  // USE_SOUND

// ═══════════════════════════════════════════════════════════════════════════════
// EFFECT UPDATES  (all non-blocking, called from loop())
// ═══════════════════════════════════════════════════════════════════════════════

void updateBlink() {
  if (!blinkActive) return;
  uint32_t now = millis();

  if (connectionLost) {
    // 3-flash fault pattern instead of normal blink
    if (now - connPatternMs >= CONN_LOST_PATTERN[connPatternIdx]) {
      connPatternMs  = now;
      connPatternIdx = (connPatternIdx + 1) % CONN_PATTERN_LEN;
      ledcWrite(PIN_NAV, (connPatternIdx % 2 == 0) ? 255 : 0);  // even=ON, odd=OFF
    }
  } else {
    if (now - lastBlinkMs >= blinkIntervalMs) {
      lastBlinkMs = now;
      blinkState  = !blinkState;
      ledcWrite(PIN_NAV, blinkState ? 255 : 0);
    }
  }
}

void updateEnginePulse() {
  if (!engActive || currentEffect == EFF_ELEC_SHORT) return;

  static uint32_t lastPulseMs = 0;
  uint32_t now = millis();
  if (now - lastPulseMs < 20) return;
  lastPulseMs = now;

  const uint32_t period   = 2000;   // ms per full pulse cycle
  const uint8_t  minDuty  = 130;
  const uint8_t  maxDuty  = 255;

  float phase = (now % period) / (float)period * 6.2832f;
  uint8_t duty = (uint8_t)(minDuty + (sinf(phase) + 1.0f) * 0.5f * (maxDuty - minDuty));

  if (engSync) {
    ledcWrite(PIN_ENG_A, duty);
    ledcWrite(PIN_ENG_B, duty);
  } else {
    // Independent mode: B offset by half a period
    float phaseB  = ((now + period / 2) % period) / (float)period * 6.2832f;
    uint8_t dutyB = (uint8_t)(minDuty + (sinf(phaseB) + 1.0f) * 0.5f * (maxDuty - minDuty));
    ledcWrite(PIN_ENG_A, duty);
    ledcWrite(PIN_ENG_B, dutyB);
  }
}

void updateStartup() {
  if (seqStep >= SEQ_TOTAL_STEPS) {
    currentEffect = EFF_IDLE;
    myData.status   = 2;
    myData.boardInd = 1;
    esp_now_send(broadcastAddress2, (uint8_t *)&myData, sizeof(myData));
    return;
  }

  uint32_t delay_ms = (seqStep >= 10) ? spcStartupMs : winStartupMs;
  if (millis() - lastSeqMs < delay_ms) return;
  lastSeqMs = millis();

  const SeqStep &s = STARTUP_SEQ[seqStep];
  for (int i = 0; i < s.count; i++) {
    if (s.idx[i] < 0) continue;
    int idx = s.idx[i];
    if (idx == IDX_NAV) {
      if (!espNowReceived || startupSolo) { blinkActive = true; lastBlinkMs = millis(); }
    } else if (idx == IDX_ENG_A || idx == IDX_ENG_B) {
      if (!espNowReceived || startupSolo) { engActive = true; }
    } else {
      winWrite(idx, true);
    }
  }
  seqStep++;
  if (seqStep == 10 && !startupSolo) {
    struct_message relay;
    memset(&relay, 0, sizeof(relay));
    relay.ledGroup = -1; relay.ledCmd = LED_STARTUP; relay.ledValue = 0;
    esp_now_send(broadcastAddress1, (uint8_t *)&relay, sizeof(relay));
  }
}

void updateShutdown() {
  if (seqStep < 0) {
    allOff();   // guarantee everything dark regardless of pin mode
    currentEffect = EFF_IDLE;
    myData.status   = 2;
    myData.boardInd = 1;
    esp_now_send(broadcastAddress2, (uint8_t *)&myData, sizeof(myData));
    return;
  }

  uint32_t delay_ms = (seqStep >= 10) ? spcStartupMs : winStartupMs;
  if (millis() - lastSeqMs < delay_ms) return;
  lastSeqMs = millis();

  const SeqStep &s = STARTUP_SEQ[seqStep];
  for (int i = 0; i < s.count; i++) {
    if (s.idx[i] < 0) continue;
    int idx = s.idx[i];
    if (idx == IDX_NAV) {
      blinkActive = false;
      ledcWrite(PIN_NAV, 0);
    } else if (idx == IDX_ENG_A || idx == IDX_ENG_B) {
      engActive = false;
      ledcWrite(PIN_ENG_A, 0);
      ledcWrite(PIN_ENG_B, 0);
    } else {
      winWrite(idx, false);
    }
  }
  seqStep--;
}

void initElecShort() {
  releaseZoneDim(true);

  // Pick 2 or 3 windows to be affected; all others freeze at pre-damage state
  int numActive = random(2, 4);  // 2 or 3
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
    if (!isActive) {
      // Random on or off at damage start, then held there
      digitalWrite(pinArray[i], random(2) ? HIGH : LOW);
      continue;
    }
    // First active pick gets PWM flicker; others do slow on/off
    winShort[i].flickerMode = (i == picks[0]);
    winShort[i].on          = preDmgWindowOn[i];
    winShort[i].nextMs      = now + random(200, 600);
    if (winShort[i].flickerMode) ledcAttach(pinArray[i], LEDC_FREQ, LEDC_BITS);
  }

  // Engines go to independent damage mode
  engSync = false;
  nextEngDmgAMs = now + random(200, 600);
  nextEngDmgBMs = now + random(200, 600);

#ifdef USE_SOUND
  // Phase 1: play alert klaxon or beep — will cut to damage sound after DMG_ALERT_MS
  soundPlayFile(random(2) ? SND_DMG_ALERT_1 : SND_DMG_ALERT_2);
  dmgSndPhase  = 1;
  dmgSndNextMs = now + DMG_ALERT_MS;
#endif
}

void updateElecShort() {
  uint32_t now = millis();

#ifdef USE_SOUND
  if (dmgSndPhase == 1 && now >= dmgSndNextMs) {
    soundStop();
    soundPlayRandom(SND_DAMAGE_FIRST, SND_DAMAGE_LAST);
    dmgSndPhase = 2;
  }
#endif

  for (int i = 0; i < NUM_WINDOWS; i++) {
    if (!winShort[i].active || now < winShort[i].nextMs) continue;
    if (winShort[i].flickerMode) {
      // PWM flicker: occasional dip, mostly mid-bright
      uint8_t duty = (random(100) < 15) ? 0 : random(80, 256);
      ledcWrite(pinArray[i], duty);
      winShort[i].nextMs = now + random(40, 150);
    } else {
      // Slow on/off fault — holds each state for a while
      winShort[i].on = !winShort[i].on;
      digitalWrite(pinArray[i], winShort[i].on ? HIGH : LOW);
      winShort[i].nextMs = now + random(300, 900);
    }
  }

  // Engine damage flicker (independent)
  if (now >= nextEngDmgAMs) {
    ledcWrite(PIN_ENG_A, (random(100) < 20) ? 0 : random(80, 220));
    nextEngDmgAMs = now + random(200, 700);
  }
  if (now >= nextEngDmgBMs) {
    ledcWrite(PIN_ENG_B, (random(100) < 20) ? 0 : random(80, 220));
    nextEngDmgBMs = now + random(200, 700);
  }
}

void stopElecShort() {
  // Restore windows
  for (int i = 0; i < NUM_WINDOWS; i++) {
    if (winShort[i].active && winShort[i].flickerMode) {
      ledcDetach(pinArray[i]);
      pinMode(pinArray[i], OUTPUT);
    }
    if (!preDmgZoneActive) digitalWrite(pinArray[i], preDmgWindowOn[i] ? HIGH : LOW);
  }
  if (preDmgZoneActive) setZoneDim(255);

  // Restore engines and nav
  engSync = true;
  if (preDmgEngActive)   { engActive = true; }
  if (preDmgBlinkActive) { blinkActive = true; lastBlinkMs = millis(); }

  currentEffect = EFF_IDLE;

#ifdef USE_SOUND
  dmgSndPhase = 0;
  soundStop();
#endif
}

void updateAssemblyMode() {
  if (!asmMode) return;
  uint32_t now = millis();
  if (now - lastAsmMs < 800) return;
  lastAsmMs = now;
  asmPhase  = !asmPhase;
  // Odd indices: 0,2,4,6,8,10,12,14 (G01,G03,G05,G07,G09,G11,G13,G15)
  // Even indices: 1,3,5,7,9,11,13   (G02,G04,G06,G08,G10,G12,G14)
  for (int i = 0; i < NUM_WINDOWS; i++) {
    bool on = asmPhase ? (i % 2 != 0) : (i % 2 == 0);
    winWrite(i, on);
  }
}

void updateEffects() {
  if (asmMode) { updateAssemblyMode(); return; }
  updateBlink();
  if (currentEffect != EFF_ELEC_SHORT) updateEnginePulse();
  if (currentEffect == EFF_STARTUP)    updateStartup();
  if (currentEffect == EFF_SHUTDOWN)   updateShutdown();
  if (currentEffect == EFF_ELEC_SHORT) updateElecShort();
}


// ═══════════════════════════════════════════════════════════════════════════════
// LED COMMAND DISPATCH
// ═══════════════════════════════════════════════════════════════════════════════

void handleLedCommand(int grp, int cmd, int val) {
  switch (cmd) {

    case LED_STARTUP:
      bootIndicatorActive = false;   // allOff() below cleans up indicator LEDs
      startupSolo = (val == 1);
      if (currentEffect == EFF_ELEC_SHORT) stopElecShort();
      releaseZoneDim(false);
      allOff();
      seqStep       = 0;
      lastSeqMs     = millis();
      currentEffect = EFF_STARTUP;
#ifdef USE_SOUND
      soundPlayFile(SND_POWERUP_FILE);
#endif
      break;

    case LED_SHUTDOWN:
      if (currentEffect == EFF_ELEC_SHORT) stopElecShort();
      releaseZoneDim(false);
      seqStep       = SEQ_TOTAL_STEPS - 1;
      lastSeqMs     = millis();
      currentEffect = EFF_SHUTDOWN;
      if (!espNowReceived || !assembledMode) soundPlayRandom(SND_SHUTDOWN_FIRST, SND_SHUTDOWN_LAST);
      break;

    case LED_ELEC_SHORT:
      if (currentEffect == EFF_STARTUP || currentEffect == EFF_SHUTDOWN) return;
      preDmgEngActive   = engActive;
      preDmgBlinkActive = blinkActive;
      preDmgZoneActive  = windowZoneDimActive;
      if (!windowZoneDimActive) {
        for (int i = 0; i < NUM_WINDOWS; i++)
          preDmgWindowOn[i] = (digitalRead(pinArray[i]) == HIGH);
      }
      damageTimeoutMs   = (val > 0) ? (uint32_t)val : 120000;
      damageStartMs     = millis();
      initElecShort();
      currentEffect = EFF_ELEC_SHORT;
      break;

    case LED_ALL_OFF:
      if (currentEffect == EFF_ELEC_SHORT) stopElecShort();
      allOff();
      break;

    case LED_ON:
      if (grp == GRP_ALL || grp == GRP_ALL_WINDOWS) {
        releaseZoneDim(true);
        for (int i = 0; i < NUM_WINDOWS; i++) { pinMode(pinArray[i], OUTPUT); digitalWrite(pinArray[i], HIGH); }
      } else if (grp == GRP_BOTH_ENGINES) {
        engActive = true; engSync = true;
      } else if (grp == IDX_NAV) {
        blinkActive = false; ledcWrite(PIN_NAV, 255);
      } else if (grp == IDX_ENG_A) {
        engActive = true;
      } else if (grp == IDX_ENG_B) {
        engActive = true;
      } else if (grp >= 0 && grp < NUM_WINDOWS) {
        winWrite(grp, true);
      }
      break;

    case LED_OFF:
      if (grp == GRP_ALL || grp < 0) {
        // Damage cancel — always restore, never go all-dark from this path
        if (currentEffect == EFF_ELEC_SHORT) stopElecShort();
        // If damage wasn't active, ignore (DataPad state may be out of sync)
      } else if (grp == GRP_ALL_WINDOWS) {
        allWindowsOff();
      } else if (grp == GRP_BOTH_ENGINES) {
        engActive = false; ledcWrite(PIN_ENG_A, 0); ledcWrite(PIN_ENG_B, 0);
      } else if (grp == IDX_NAV) {
        blinkActive = false; ledcWrite(PIN_NAV, 0);
      } else if (grp == IDX_ENG_A) {
        ledcWrite(PIN_ENG_A, 0);
      } else if (grp == IDX_ENG_B) {
        ledcWrite(PIN_ENG_B, 0);
      } else if (grp >= 0 && grp < NUM_WINDOWS) {
        winWrite(grp, false);
      }
      break;

    case LED_DIM:
      if (grp == GRP_ALL_WINDOWS || grp == GRP_ALL || grp < 0) {
        setZoneDim((uint8_t)constrain(val, 0, 255));
      } else if (grp == IDX_ENG_A) {
        engActive = false; ledcWrite(PIN_ENG_A, (uint8_t)constrain(val, 0, 255));
      } else if (grp == IDX_ENG_B) {
        engActive = false; ledcWrite(PIN_ENG_B, (uint8_t)constrain(val, 0, 255));
      } else if (grp == GRP_BOTH_ENGINES) {
        engActive = false;
        ledcWrite(PIN_ENG_A, (uint8_t)constrain(val, 0, 255));
        ledcWrite(PIN_ENG_B, (uint8_t)constrain(val, 0, 255));
      }
      break;

    case LED_ENGINE:
      engActive = true;
      engSync   = (grp == GRP_BOTH_ENGINES || grp < 0);
      break;

    case LED_BLINK:
      if (val > 0) blinkIntervalMs = (uint32_t)val;
      blinkActive = true;
      lastBlinkMs = millis();
      break;

    case LED_SET_BLINK_MS:
      blinkIntervalMs = (uint32_t)val;
      prefs.begin("bridge", false);
      prefs.putUInt("blink_ms", blinkIntervalMs);
      prefs.end();
      break;

    case LED_SET_WIN_MS:
      winStartupMs = (uint32_t)val;
      prefs.begin("bridge", false);
      prefs.putUInt("win_ms", winStartupMs);
      prefs.end();
      break;

    case LED_SET_SPC_MS:
      spcStartupMs = (uint32_t)val;
      prefs.begin("bridge", false);
      prefs.putUInt("spc_ms", spcStartupMs);
      prefs.end();
      break;

    case LED_SET_AUTO_MS:
      autoStartDelayMs = (uint32_t)val;
      prefs.begin("bridge", false);
      prefs.putUInt("auto_ms", autoStartDelayMs);
      prefs.end();
      break;

    case LED_SET_CONN_MS:
      connLostMs = (uint32_t)val;
      prefs.begin("bridge", false);
      prefs.putUInt("conn_ms", connLostMs);
      prefs.end();
      break;

    case LED_ASSEMBLY_MODE:
      asmMode = (val != 0);
      prefs.begin("bridge", false);
      prefs.putBool("asm_mode", asmMode);
      prefs.end();
      allOff(); lastAsmMs = 0; asmPhase = false;
      break;

    case LED_SYNC_MODE:
      assembledMode = (val == 1);
      prefs.begin("bridge", false);
      prefs.putBool("sync_mode", assembledMode);
      prefs.end();
      break;

#ifdef USE_SOUND
    case SND_CMD_PLAY:
      soundPlayFile((uint8_t)constrain(val, 1, 25));
      break;
    case SND_CMD_STOP:
      soundStop();
      break;
    case SND_CMD_VOL:
      soundSetVolume((uint8_t)constrain(val, 0, 30));
      break;
    case SND_CMD_REPEAT:
      soundSetRepeat((uint8_t)constrain(val, 0, 2));
      break;
#endif
  }
}



// ═══════════════════════════════════════════════════════════════════════════════
// ESP-NOW CALLBACKS
// ═══════════════════════════════════════════════════════════════════════════════

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  espNowReceived = true;
  lastContactMs  = millis();

  if (connectionLost) {
    connectionLost  = false;
    connPatternIdx  = 0;
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
}


// ═══════════════════════════════════════════════════════════════════════════════
// SETUP
// ═══════════════════════════════════════════════════════════════════════════════

void setup() {
  // Load NVS timing values
  prefs.begin("bridge", false);
  blinkIntervalMs  = prefs.getUInt("blink_ms", 500);
  winStartupMs     = prefs.getUInt("win_ms",   300);
  spcStartupMs     = prefs.getUInt("spc_ms",   800);
  autoStartDelayMs = prefs.getUInt("auto_ms",  30000);
  connLostMs       = prefs.getUInt("conn_ms",  300000);
  asmMode          = prefs.getBool("asm_mode",   false);
  assembledMode    = prefs.getBool("sync_mode",  true);
  bool wifiAuto    = prefs.getBool("wifi_auto", false);
  if (wifiAuto) {
    String s = prefs.getString("wifi_ssid", "");
    String p = prefs.getString("wifi_pass", "");
    if (s.length() > 0) {
      strncpy(myData.ssid1,     s.c_str(), 32); myData.ssid1[32]     = '\0';
      strncpy(myData.password1, p.c_str(), 64); myData.password1[64] = '\0';
      startWifi1 = 1;
    }
  }
  prefs.end();

  // Output pins
  for (int j = 0; j < NUM_GROUPS; j++) {
    pinMode(pinArray[j], OUTPUT);
    digitalWrite(pinArray[j], LOW);
  }

  analogSetPinAttenuation(PIN_BAT_ADC, ADC_11db);

  // LEDC — attach permanent PWM pins (new API: ledcAttach replaces ledcSetup+ledcAttachPin)
  ledcAttach(PIN_ENG_A, LEDC_FREQ, LEDC_BITS);   // Engine A
  ledcAttach(PIN_ENG_B, LEDC_FREQ, LEDC_BITS);   // Engine B
  ledcAttach(PIN_NAV,   LEDC_FREQ, LEDC_BITS);   // NAV blink
  // Window zone dim and elec short flicker pins are attached dynamically as needed

  // Start boot indicator immediately — WiFi/sound init below blocks for several seconds
  bootIndicatorActive  = true;
  bootIndicatorStartMs = millis();
  bootIndicatorBlinkMs = millis();
  bootIndicatorPhase   = true;
  ledcWrite(PIN_NAV, 255);
  winWrite(13, true);   // G14
  winWrite(14, true);   // G15

#ifdef USE_SOUND
  Serial2.begin(SOUND_BAUD, SERIAL_8N1, SOUND_RX, SOUND_TX);
  soundInit();
#endif

  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);   // set ch=1 before first send

  if (esp_now_init() != ESP_OK) return;
  esp_now_register_recv_cb((esp_now_recv_cb_t)OnDataRecv);

  // EngRoom peer
  memcpy(peerInfo.peer_addr, broadcastAddress1, 6);
  peerInfo.channel = 0; peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) return;

  // DataPad peer
  memcpy(peerInfo.peer_addr, broadcastAddress2, 6);
  peerInfo.channel = 0; peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) return;

  esp_wifi_get_channel(&wifiChannel, NULL);

  // Announce online — include asmMode so DataPad can sync button state
  myData.boardInd = 1;
  myData.status   = 1;
  myData.ledValue = asmMode ? 1 : 0;
  esp_now_send(broadcastAddress2, (uint8_t *)&myData, sizeof(myData));
  myData.ledValue = 0;

  // OTA setup
  ArduinoOTA.setHostname("Bridge");

  bootIndicatorBlinkMs = millis();  // reset: timer ran during soundInit/WiFi init, so first toggle is 500ms into loop() not immediately
  bootMs        = millis();
  lastContactMs = millis();
}


// ═══════════════════════════════════════════════════════════════════════════════
// LOOP
// ═══════════════════════════════════════════════════════════════════════════════

void loop() {
  if (startWifi1 == 3) {
    startWifi1 = 0;
    RevertWiFi();
  } else if (startWifi1 == 1) {
    bool doSave = saveWifi1;
    startWifi1 = 0;
    saveWifi1  = false;
    StartWiFi(doSave);
  }

  if (wifiActive) {
    if (WiFi.status() != WL_CONNECTED) {
      if (wifiDropMs == 0) wifiDropMs = millis();
      else if (millis() - wifiDropMs >= WIFI_REVERT_MS) {
        wifiDropMs = 0; startWifi1 = 3;
      }
    } else { wifiDropMs = 0; }
  }

  if (bootIndicatorActive) {
    if (millis() - bootIndicatorStartMs >= 20000) {
      bootIndicatorActive = false;
      ledcWrite(PIN_NAV, 0);
      winWrite(13, false);
      winWrite(14, false);
    } else if (millis() - bootIndicatorBlinkMs >= 500) {
      bootIndicatorBlinkMs = millis();
      bootIndicatorPhase   = !bootIndicatorPhase;
      ledcWrite(PIN_NAV, bootIndicatorPhase ? 255 : 0);
      winWrite(13, bootIndicatorPhase);
      winWrite(14, bootIndicatorPhase);
    }
  }

  if (newLedCmd) {
    newLedCmd = false;
    handleLedCommand(pendingGroup, pendingCmd, pendingValue);
  }

  // Auto-start: if no ESP-NOW contact within autoStartDelayMs, run startup sequence
  if (!autoStartFired && !espNowReceived &&
      autoStartDelayMs > 0 && millis() - bootMs >= autoStartDelayMs) {
    autoStartFired = true;
    handleLedCommand(-1, LED_STARTUP, 0);
  }

  // Connection monitor: if contact lost for connLostMs, go to default state + fault blink
  if (!connectionLost && espNowReceived &&
      connLostMs > 0 && millis() - lastContactMs >= connLostMs) {
    connectionLost = true;
    if (currentEffect == EFF_IDLE && !engActive && !blinkActive) {
      handleLedCommand(-1, LED_STARTUP, 0);
    }
    // Force NAV on with fault pattern even if it was off
    blinkActive    = true;
    connPatternIdx = 0;
    connPatternMs  = millis();
    ledcWrite(PIN_NAV, 255);  // start on the first flash immediately
  }

  // Damage control auto-cancel
  if (currentEffect == EFF_ELEC_SHORT && millis() - damageStartMs >= damageTimeoutMs) {
    stopElecShort();   // handles currentEffect, engines, nav, windows
    struct_message reply;
    memset(&reply, 0, sizeof(reply));
    reply.boardInd = 1;
    reply.status   = 3;   // damage auto-cancelled
    esp_now_send(broadcastAddress2, (uint8_t *)&reply, sizeof(reply));
  }

  if (pendingStateReply) {
    pendingStateReply = false;
    struct_message reply; memset(&reply, 0, sizeof(reply));
    reply.boardInd = 1; reply.status = 1;
    reply.ledValue = asmMode ? 1 : 0;
    esp_now_send(broadcastAddress2, (uint8_t *)&reply, sizeof(reply));
  }

  // Status ping — tell DataPad we're still alive every 30 seconds
  static uint32_t lastStatusPingMs = 0;
  if (millis() - lastStatusPingMs >= 30000) {
    lastStatusPingMs = millis();
    struct_message ping;
    memset(&ping, 0, sizeof(ping));
    ping.boardInd = 1;
    ping.status   = 1;
    ping.ledValue = asmMode ? 1 : 0;
    if (wifiActive && WiFi.status() == WL_CONNECTED) {
      ping.wifiStatus = 5;
      ping.ipAddress  = (uint32_t)WiFi.localIP();
    }
    esp_now_send(broadcastAddress2, (uint8_t *)&ping, sizeof(ping));
  }

  static uint32_t lastBatMs = 0;
  if (millis() - lastBatMs >= 30000) {
    lastBatMs = millis();
    batVolts = readBatteryVolts();
    struct_message batPing;
    memset(&batPing, 0, sizeof(batPing));
    batPing.boardInd = 1;
    batPing.ledCmd   = LED_BAT_LEVEL;
    batPing.ledValue = (int)(batVolts * 1000.0f);
    esp_now_send(broadcastAddress2, (uint8_t *)&batPing, sizeof(batPing));
  }

  updateEffects();
  ArduinoOTA.handle();
}


// ═══════════════════════════════════════════════════════════════════════════════
// WIFI CONNECT
// ═══════════════════════════════════════════════════════════════════════════════

void RevertWiFi() {
  prefs.begin("bridge", false);
  prefs.putBool("wifi_auto", false);
  prefs.end();
  WiFi.disconnect();
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  wifiChannel = 1;
  wifiActive  = false;
  wifiDropMs  = 0;
  servicesStarted = false;
}

void StartWiFi(bool saveCredentials) {
  const char* ssid     = myData.ssid1;
  const char* password = myData.password1;

  bool connected = false;
  for (int attempt = 1; attempt <= 2 && !connected; attempt++) {
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    unsigned long startTime = millis();
    while (millis() - startTime < 10000) {
      if (WiFi.status() == WL_CONNECTED) { connected = true; break; }
      delay(100);
    }
  }

  if (!connected) {
    WiFi.disconnect();
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    wifiChannel = 1;
    prefs.begin("bridge", false); prefs.putBool("wifi_auto", false); prefs.end();
    myData.boardInd   = 1;
    myData.wifiStatus = 4;
    esp_now_send(broadcastAddress2, (uint8_t *)&myData, sizeof(myData));
    return;
  }

  // Re-establish ESP-NOW after WiFi connects: disable power save and refresh peers
  // on the new channel so sends don't fail
  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_now_del_peer(broadcastAddress1);
  esp_now_del_peer(broadcastAddress2);
  memcpy(peerInfo.peer_addr, broadcastAddress1, 6);
  peerInfo.channel = 0; peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
  memcpy(peerInfo.peer_addr, broadcastAddress2, 6);
  peerInfo.channel = 0; peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
  wifiChannel = (uint8_t)WiFi.channel();

  myData.boardInd   = 1;
  myData.wifiStatus = 5;
  myData.ipAddress  = (uint32_t)WiFi.localIP();
  esp_now_send(broadcastAddress2, (uint8_t *)&myData, sizeof(myData));

  if (saveCredentials) {
    prefs.begin("bridge", false);
    prefs.putString("wifi_ssid", String(myData.ssid1));
    prefs.putString("wifi_pass", String(myData.password1));
    prefs.putBool("wifi_auto",   true);
    prefs.end();
  }

  if (!servicesStarted) {
    MDNS.begin(mdnsName);
    ArduinoOTA.begin();
    servicesStarted = true;
    wifiActive = true;
    wifiDropMs = 0;
  }
}
