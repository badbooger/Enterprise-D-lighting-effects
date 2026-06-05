// WarpCore_ESP32.ino — Arduino Nano ESP32
//
// Chase effect uses digitalWrite on all 10 LED groups (confirmed working).
// Ambient LED (A5) uses analogWrite for dim/bright — one LEDC channel only.
// WiFi/OTA, NVS, ESP-NOW, LD2410C presence detection preserved from prior version.
//
// Wiring:
//   Chase LEDs : A4 A3 A2 A1 A0 D2 D3 D4 D5 D6  (NPN transistors)
//                Use D2-D6 constants, NOT raw 2-6 — Nano ESP32: D2=GPIO5, raw 2=GPIO2
//   Ambient LED: A5
//   Speed pot  : A7 (wiper; ends to 3.3V and GND)
//   LD2410C OUT: D7 (push-pull, HIGH=presence)

// ── Includes ──────────────────────────────────────────────────────────────────

#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <Preferences.h>

// ── LED command constants (must match all other sketches) ─────────────────────

#define LED_ON         1
#define LED_OFF        2
#define LED_STARTUP    7
#define LED_SHUTDOWN   8
#define LED_WARP      28
#define LED_RADAR_EN  29  // DataPad → WarpCore: enable(1)/disable(0) radar
#define LED_RADAR_TRIG 30  // WarpCore → DataPad: presence detected

// ── Peer MAC addresses ────────────────────────────────────────────────────────

uint8_t dataPadAddress[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // run MAC_address_retriver on your board
uint8_t bridgeAddress[]  = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // run MAC_address_retriver on your board
uint8_t engRoomAddress[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // run MAC_address_retriver on your board

// ── Shared struct (must match all other sketches exactly) ─────────────────────

typedef struct struct_message {
  int status, boardInd;
  char password1[65], ssid1[33];
  int startWifi, wifiStatus;
  uint32_t ipAddress;
  int ledGroup, ledCmd, ledValue;
} struct_message;

struct_message myData;
esp_now_peer_info_t peerInfo;

// ── WarpCore state ────────────────────────────────────────────────────────────

bool    warpCoreActive = true;
bool    needAllOff     = false;
bool    remoteOverride = false;
uint8_t remoteSpeed    = 0;
bool    radarEnabled   = true;

// ── WiFi / OTA ────────────────────────────────────────────────────────────────

const char* mdnsName      = "WarpCore";
uint32_t    last_ota_time = 0;
bool        wifiActive      = false;
bool        servicesStarted = false;
uint32_t    wifiDropMs      = 0;
int         startWifi1      = 0;
bool        saveWifi1       = false;
uint8_t     wifiChannel     = 1;
#define WIFI_REVERT_MS 90000

// ── LD2410C presence sensor ───────────────────────────────────────────────────

#define PIN_RADAR        D7
#define RADAR_LOCKOUT_MS 3000

bool     radarLastState = false;
uint32_t radarLockoutMs = 0;

// ── Pin assignments ───────────────────────────────────────────────────────────

const uint8_t leds[]  = {A4, A3, A2, A1, A0, D2, D3, D4, D5, D6};
const uint8_t NLEDS   = sizeof(leds) / sizeof(uint8_t);
const uint8_t PIN_AMB = A5;
const uint8_t PIN_POT = A7;

const int AMB_DIM = 50;

// ── Chase state machine ───────────────────────────────────────────────────────

enum ChaseState { CHASE, PAUSE_HIGH, PAUSE_LOW };
ChaseState chaseState = CHASE;
int        chaseStep  = 0;
uint32_t   chaseTimer = 0;

int getDelay() {
  if (remoteOverride)
    return (int)map(remoteSpeed, 0, 10, 200, 20);
  return map(analogRead(PIN_POT), 0, 4095, 200, 20);
}

void updateChase() {
  uint32_t now = millis();
  int      d   = getDelay();

  switch (chaseState) {
    case CHASE:
      if (now - chaseTimer < (uint32_t)d) return;
      chaseTimer = now;
      if (chaseStep == 0) {
        digitalWrite(leds[0], HIGH);
      } else if (chaseStep < NLEDS) {
        digitalWrite(leds[chaseStep],     HIGH);
        digitalWrite(leds[chaseStep - 1], LOW);
      } else {
        digitalWrite(leds[NLEDS - 1], LOW);
        analogWrite(PIN_AMB, 255);
        chaseState = PAUSE_HIGH;
        chaseStep  = 0;
        break;
      }
      chaseStep++;
      break;

    case PAUSE_HIGH:
      if (now - chaseTimer < (uint32_t)(d * 2)) return;
      analogWrite(PIN_AMB, AMB_DIM);
      chaseState = PAUSE_LOW;
      chaseTimer = now;
      break;

    case PAUSE_LOW:
      if (now - chaseTimer < (uint32_t)(d * 2)) return;
      chaseState = CHASE;
      chaseTimer = now;
      break;
  }
}

// ── Helpers ───────────────────────────────────────────────────────────────────

void allLedsOff() {
  for (int i = 0; i < NLEDS; i++) digitalWrite(leds[i], LOW);
  analogWrite(PIN_AMB, 0);
  chaseState = CHASE;
  chaseStep  = 0;
  chaseTimer = 0;
}

// ── ESP-NOW receive ───────────────────────────────────────────────────────────

void OnDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
  if (len < (int)sizeof(struct_message)) return;
  memcpy(&myData, data, sizeof(myData));

  if      (myData.startWifi == 3) { startWifi1 = 3; saveWifi1 = false; }
  else if (myData.startWifi == 2) { startWifi1 = 1; saveWifi1 = true;  }
  else if (myData.startWifi)      { startWifi1 = 1; saveWifi1 = false; }

  switch (myData.ledCmd) {
    case LED_WARP:
      remoteSpeed    = (uint8_t)constrain(myData.ledValue, 0, 10);
      remoteOverride = (remoteSpeed > 0);
      break;
    case LED_RADAR_EN:
      radarEnabled = (myData.ledValue != 0);
      { Preferences prefs; prefs.begin("warpcore", false);
        prefs.putBool("radar_on", radarEnabled); prefs.end(); }
      break;
    case LED_STARTUP:
      warpCoreActive = true;
      remoteOverride = false;
      remoteSpeed    = 0;
      break;
    case LED_SHUTDOWN:
      warpCoreActive = false;
      needAllOff     = true;
      break;
    case LED_ON:
      warpCoreActive = true;
      break;
    case LED_OFF:
      warpCoreActive = false;
      needAllOff     = true;
      break;
  }
}

// ── WiFi / OTA ────────────────────────────────────────────────────────────────

static void refreshPeers() {
  const uint8_t* peers[] = {dataPadAddress, bridgeAddress, engRoomAddress};
  for (auto addr : peers) {
    esp_now_del_peer(addr);
    memcpy(peerInfo.peer_addr, addr, 6);
    peerInfo.channel = 0; peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
  }
  esp_wifi_get_channel(&wifiChannel, NULL);
  Serial.printf("ESP-NOW peers refreshed  ch=%d\n", wifiChannel);
}

void StartWiFi(bool saveCredentials) {
  const char* ssid     = myData.ssid1;
  const char* password = myData.password1;
  Serial.printf("Connecting to: %s\n", ssid);

  bool connected = false;
  for (int attempt = 1; attempt <= 2 && !connected; attempt++) {
    Serial.printf("WiFi attempt %d/2...\n", attempt);
    WiFi.disconnect(); WiFi.begin(ssid, password);
    unsigned long t = millis();
    while (millis() - t < 10000) {
      if (WiFi.status() == WL_CONNECTED) { connected = true; break; }
      delay(100); Serial.print(".");
    }
    Serial.println();
  }

  if (!connected) {
    WiFi.disconnect();
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    wifiChannel = 1;
    Serial.printf("WiFi: did not connect after 2 attempts. ESP-NOW ch=%d\n", wifiChannel);
    myData.boardInd   = 4;
    myData.wifiStatus = 6;
    esp_now_send(dataPadAddress, (uint8_t *)&myData, sizeof(myData));
    return;
  }

  Serial.printf("WiFi connected. IP: %s  ch=%d\n", WiFi.localIP().toString().c_str(), WiFi.channel());
  esp_wifi_set_ps(WIFI_PS_NONE);
  refreshPeers();
  myData.boardInd   = 4;
  myData.wifiStatus = 7;
  myData.ipAddress  = (uint32_t)WiFi.localIP();
  esp_now_send(dataPadAddress, (uint8_t *)&myData, sizeof(myData));

  if (saveCredentials) {
    Preferences prefs;
    prefs.begin("warpcore", false);
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
    wifiActive      = true;
    wifiDropMs      = 0;
  }
}

void RevertWiFi() {
  Preferences prefs;
  prefs.begin("warpcore", false); prefs.putBool("wifi_auto", false); prefs.end();
  WiFi.disconnect();
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  wifiChannel     = 1;
  wifiActive      = false;
  wifiDropMs      = 0;
  servicesStarted = false;
  Serial.printf("WiFi reverted. ESP-NOW ch=%d\n", wifiChannel);
}

// ── Arduino entry points ──────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  Serial.print("WarpCore MAC: ");

  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  Serial.println(WiFi.macAddress());

  {
    Preferences prefs;
    prefs.begin("warpcore", true);
    warpCoreActive = prefs.getBool("on_default", true);
    radarEnabled   = prefs.getBool("radar_on",   true);
    bool wifi_auto = prefs.getBool("wifi_auto",  false);
    if (wifi_auto) {
      String ssid = prefs.getString("wifi_ssid", "");
      String pass = prefs.getString("wifi_pass", "");
      strncpy(myData.ssid1,     ssid.c_str(), 32); myData.ssid1[32]     = '\0';
      strncpy(myData.password1, pass.c_str(), 64); myData.password1[64] = '\0';
      if (ssid.length() > 0) startWifi1 = 1;
    }
    prefs.end();
  }

  esp_now_init();
  esp_now_register_recv_cb(OnDataRecv);

  const uint8_t* peers[] = {dataPadAddress, bridgeAddress, engRoomAddress};
  for (auto addr : peers) {
    memcpy(peerInfo.peer_addr, addr, 6);
    peerInfo.channel = 0; peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
  }
  esp_wifi_get_channel(&wifiChannel, NULL);
  Serial.printf("ESP-NOW ready  ch=%d\n", wifiChannel);

  ArduinoOTA.setHostname(mdnsName);
  ArduinoOTA
    .onStart([]() { Serial.println("OTA start"); })
    .onEnd([]()   { Serial.println("\nOTA end"); })
    .onProgress([](unsigned int progress, unsigned int total) {
      if (millis() - last_ota_time > 500) {
        Serial.printf("OTA: %u%%\n", progress / (total / 100));
        last_ota_time = millis();
      }
    })
    .onError([](ota_error_t error) {
      const char* msg[] = {"Auth Failed","Begin Failed","Connect Failed","Receive Failed","End Failed"};
      if (error <= OTA_END_ERROR) Serial.printf("OTA Error[%u]: %s\n", error, msg[error]);
    });

  pinMode(PIN_RADAR, INPUT);

  for (int i = 0; i < NLEDS; i++) {
    pinMode(leds[i], OUTPUT);
    digitalWrite(leds[i], LOW);
  }
  pinMode(PIN_AMB, OUTPUT);
  analogWrite(PIN_AMB, warpCoreActive ? AMB_DIM : 0);
}

void loop() {
  if (startWifi1) {
    if      (startWifi1 == 3) RevertWiFi();
    else if (startWifi1 == 1) StartWiFi(saveWifi1);
    startWifi1 = 0;
  }

  if (servicesStarted) ArduinoOTA.handle();

  if (wifiActive) {
    if (WiFi.status() != WL_CONNECTED) {
      if (wifiDropMs == 0) wifiDropMs = millis();
      else if (millis() - wifiDropMs >= WIFI_REVERT_MS) {
        Serial.println("WiFi dropped — auto-reverting to ESP-NOW ch=1");
        wifiDropMs = 0; startWifi1 = 3;
      }
    } else { wifiDropMs = 0; }
  }

  if (needAllOff) { allLedsOff(); needAllOff = false; }

  bool radarNow = digitalRead(PIN_RADAR);
  if (radarNow != radarLastState)
    Serial.printf("Radar: %s\n", radarNow ? "PRESENCE" : "clear");
  if (radarEnabled && radarNow && !radarLastState && millis() - radarLockoutMs >= RADAR_LOCKOUT_MS) {
    radarLockoutMs = millis();
    Serial.println("Radar trigger — notifying DataPad");
    myData.boardInd = 4; myData.ledGroup = -1;
    myData.ledCmd   = LED_RADAR_TRIG; myData.ledValue = 0;
    esp_now_send(dataPadAddress, (uint8_t *)&myData, sizeof(myData));
    // TODO: send red alert cmd here (when red alert screen is implemented on DataPad)
  }
  radarLastState = radarNow;

  if (warpCoreActive) updateChase();
}
