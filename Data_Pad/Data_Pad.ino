/*
  DataPad — Waveshare 7" touchscreen
  SquareLine Studio UI + ESP-NOW controller + OTA
*/

#include <Arduino.h>
#include <esp_display_panel.hpp>
#include <lvgl.h>
#include "lvgl_v8_port.h"
#include "ui.h"
#include <esp_now.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <NetworkUdp.h>
#include <ArduinoOTA.h>
#include "esp_lcd_panel_rgb.h"   // for esp_lcd_rgb_panel_restart()
#include "esp_wifi.h"            // for esp_wifi_set_ps()
#include <Preferences.h>         // for SaveWifi NVS storage

using namespace esp_panel::drivers;
using namespace esp_panel::board;

// Global board pointer — needed to restart the RGB panel after WiFi corruption
Board *dp_board = nullptr;

const char* mdnsName      = "DataPad";
uint32_t    last_ota_time = 0;
bool        servicesStarted = false;

uint8_t wifiChannel = 1;   // cached channel — updated from main task, safe to read in callbacks

// ESP-NOW peer MACs
uint8_t broadcastAddress1[]        = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // EngRoom — run MAC_address_retriver on your board
uint8_t broadcastAddress2[]        = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Bridge — run MAC_address_retriver on your board
uint8_t broadcastAddressWarpCore[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // WarpCore — run MAC_address_retriver on your board
uint8_t broadcastAll[]             = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// ── LED command constants (must match Bridge and EngRoom exactly) ─────────────
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
#define LED_SET_AUTO_MS   23
#define LED_SET_CONN_MS   24
#define LED_SYNC_MODE     25   // assembled/separated mode (ledValue 1=assembled 0=separated)
#define LED_EFFECTS_SIMPLE 26  // EngRoom: nacelle/deflector/impulse → constant ON
#define LED_ASSEMBLY_MODE  27  // Both ESPs: assembly blink mode (ledValue 1=on 0=off)
#define LED_WARP           28  // EngRoom nacelles + WarpCore speed (ledValue 0=normal, 1-10=warp speed)
#define LED_RADAR_EN       29  // DataPad → WarpCore: enable(1)/disable(0) radar
#define LED_RADAR_TRIG     30  // WarpCore → DataPad: presence detected
#define LED_BAT_LEVEL      31  // Bridge/EngRoom → DataPad: battery mV (ledValue = mV as int)
#define LED_ALL_OFF       99

// EngRoom-specific group constants
#define GRP_BOTH_NAC       20   // EngRoom nacelles (both together)
#define IDX_PHOTON         10   // EngRoom photon torpedo
#define IDX_ENGROOM_NAV       5   // EngRoom nav light
#define IDX_ENGROOM_DEFLECTOR 6   // EngRoom deflector dish
#define IDX_ENGROOM_IMPULSE   7   // EngRoom stardrive impulse engine

// Sound commands (must match Bridge constants exactly)
#define SND_CMD_PLAY    50   // ledValue = file number (1–4)
#define SND_CMD_STOP    51
#define SND_CMD_VOL     52   // ledValue = 0–30
#define SND_CMD_REPEAT  53   // ledValue = 0=off  1=loop one  2=loop all

// Sound file ranges — shared (files 1–25 identical on Bridge and DataPad SD cards)
#define SND_POWER_FIRST    1
#define SND_POWER_LAST     3
#define SND_ALERT_FIRST    4
#define SND_ALERT_LAST     6
#define SND_DAMAGE_FIRST   7
#define SND_DAMAGE_LAST   12
#define SND_WARP_FIRST    13
#define SND_WARP_LAST     18
#define SND_WEAPONS_FIRST 19
#define SND_WEAPONS_LAST  25

// DataPad-only sound file ranges (files 26–42, DY1703A SD card only)
#define SND_TOUCH_FIRST   26
#define SND_TOUCH_LAST    27
#define SND_VOICED_FIRST  28
#define SND_VOICED_LAST   31
#define SND_AMBIENT_FIRST 32
#define SND_AMBIENT_LAST  38
#define SND_EASTER_FIRST  39
#define SND_EASTER_LAST   44

// Battery alert thresholds (mV)
#define BAT_WARN_MV  7400   // yellow alert
#define BAT_CRIT_MV  7000   // red alert

// DataPad local sound hardware — DY1703A on RS485 connector
#define PAD_SND_TX  16
#define PAD_SND_RX  15

HardwareSerial padSnd(1);   // UART1

// forward declarations for static sound functions called from setup()
static void padSndSend(uint8_t cmd, uint8_t *data, uint8_t len);
static void padSndSetVol(uint8_t vol);

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

struct_message myData;
esp_now_peer_info_t peerInfo;

// Board online tracking
int online = 0;
int Board1 = 0;
int Board2 = 0;

// ── UI event handler state ────────────────────────────────────────────────────
uint8_t padAudioDest = 2;   // 0=BRIDGE  1=PAD  2=BOTH
uint8_t padSndVol    = 15;
uint8_t bridgeSndVol = 15;

bool ledsAreOn    = false;   // PowerUpAll: false=off→startup, true=on→shutdown
int  nvsSyncMode  = 1;       // 1=assembled (coordinated), 0=separated (each unit independent)
int  navState     = 0;       // NavLightsSet: 0=off 1=1s-blink 2=1.5s-blink 3=on
int  nvNavTiming  = 1;       // saved timing preference: 1=1.0s 2=1.5s 3=always on (NVS "nav_timing")
bool engOn        = false;   // ImpulsEng: engine flicker toggle
bool damageActive = false;   // DamageControl: timer lives on Bridge
bool wifiSaved    = false;   // SaveWifi: true when credentials are in NVS
bool nacelleOn    = false;   // EngRoom nacelle toggle state
bool warpCoreOn   = true;    // WarpCore on/off toggle state
bool radarEnabled            = true;   // LD2410C presence trigger enabled
bool redAlertSensorEnabled   = true;   // sensor triggers red alert when model already on
bool redAlertActive          = false;  // red alert screen currently showing
bool windowsOn               = false;  // Bridge window lights toggle state
uint32_t redAlertAutoOffMs   = 0;      // auto-dismiss timestamp (0=inactive)
uint32_t redAlertNextSndMs   = 0;      // next sound replay timestamp

float    bridgeBatVolts      = 0.0f;
float    engRoomBatVolts     = 0.0f;   // stays 0.0 until EngRoom sends LED_BAT_LEVEL
bool     batWarnFired        = false;
bool     batCritFired        = false;
bool     yellowAlertActive    = false;
uint32_t yellowAlertAutoOffMs = 0;
uint32_t yellowAlertNextSndMs = 0;
bool    deflectorOn    = false;   // EngRoom deflector toggle state
uint8_t  warpSpeedVal   = 0;       // current warp slider value
uint32_t warpAmbientMs  = 0;       // 0=inactive; >0=next ambient trigger time
bool     touchSoundEnabled = false;   // UI button press click sounds
bool     ambientEnabled    = false;   // Auto ambient loop on power-up
uint32_t ambientNextPlayMs = 0;       // millis() target for next ambient re-trigger (0=inactive)
int  torpedoSoundIdx = 0;    // alternates 0/1 → torpedo sound files 22/23
bool engAsmOn     = false;   // EngRoom assembly mode state
bool bridgeAsmOn  = false;   // Bridge assembly mode state
#define DAMAGE_AUTO_OFF_MS 120000   // 2 min — sent as ledValue to Bridge
#define AMBIENT_POLL_MS    30000UL  // re-trigger ambient every 30s (software loop)

// ── Screen timeout (battery save) ────────────────────────────────────────────
#define SCREEN_TIMEOUT_MS  (5UL * 60UL * 1000UL)   // 5 min idle → backlight off
#define WIFI_REVERT_MS     90000UL                   // auto-revert after 90s of dropped WiFi
bool screenAsleep = false;

// ── Per-unit connection watchdog
// Units send a status ping every 30s; if silent for UNIT_OFFLINE_MS declare offline
#define UNIT_OFFLINE_MS 90000   // 90 seconds = 3 missed pings
uint32_t lastBridgeMs   = 0;
uint32_t lastEngRoomMs  = 0;
bool     bridgeSeen     = false;   // true once first message received from Bridge
bool     engRoomSeen    = false;   // true once first message received from EngRoom
bool     bridgeOnline   = false;
bool     engRoomOnline  = false;

// ESP-NOW callback flag system — no LVGL calls or delays inside the callback
volatile bool       new_data_received = false;
struct_message      receivedData;

// Non-blocking WiFi state for StartWiFi event handler
static char    pending_ssid[33];
static char    pending_password[65];
static bool    do_wifi_connect   = false;
static bool    wifi_connecting   = false;
static unsigned long wifi_start_time = 0;
static int     wifi_retries      = 0;   // attempts used (max 2)


// ── ESP-NOW callbacks ─────────────────────────────────────────────────────────

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.printf("Send status: %s  ch=%d\n", status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL", wifiChannel);
}

// no LVGL calls, no delay() — just copy data and set flag
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&receivedData, incomingData, sizeof(receivedData));
  new_data_received = true;
  Serial.printf("Recv %d bytes | boardInd=%d | ledCmd=%d grp=%d val=%d | wifiStatus=%d | startWifi=%d\n",
                len, receivedData.boardInd, receivedData.ledCmd, receivedData.ledGroup,
                receivedData.ledValue, receivedData.wifiStatus, receivedData.startWifi);
}

// Called from loop() with LVGL mutex held
void handleReceivedData() {
  if (receivedData.ledCmd == LED_BAT_LEVEL) {
    if (receivedData.boardInd == 1) {
      bridgeBatVolts = receivedData.ledValue / 1000.0f;
      updateBatDisplay();
      if (receivedData.ledValue > 0) {
        if (receivedData.ledValue > BAT_WARN_MV) batWarnFired = false;
        if (receivedData.ledValue > BAT_CRIT_MV) batCritFired = false;
        if (!redAlertActive && !yellowAlertActive) {
          if (!batCritFired && receivedData.ledValue <= BAT_CRIT_MV) {
            batCritFired = true; batWarnFired = true;
            RedAlertStart(NULL);
          } else if (!batWarnFired && receivedData.ledValue <= BAT_WARN_MV) {
            batWarnFired = true;
            YellowAlertStart(NULL);
          }
        }
      }
    } else if (receivedData.boardInd == 2) {
      engRoomBatVolts = receivedData.ledValue / 1000.0f;
      updateBatDisplay();
      if (receivedData.ledValue > BAT_WARN_MV) batWarnFired = false;
      if (receivedData.ledValue > BAT_CRIT_MV) batCritFired = false;
      if (!redAlertActive && !yellowAlertActive) {
        if (!batCritFired && receivedData.ledValue <= BAT_CRIT_MV) {
          batCritFired = true; batWarnFired = true;
          RedAlertStart(NULL);
        } else if (!batWarnFired && receivedData.ledValue <= BAT_WARN_MV) {
          batWarnFired = true;
          YellowAlertStart(NULL);
        }
      }
    }
    return;
  }
  if (receivedData.ledCmd == LED_RADAR_TRIG) {
    if (!ledsAreOn) PowerUpAll(NULL);
    else if (radarEnabled && redAlertSensorEnabled && !redAlertActive) RedAlertStart(NULL);
    return;
  }
  // Pocket remote (2.8") red alert trigger: boardInd=3, status=5
  if (receivedData.boardInd == 3 && receivedData.status == 5) {
    if (!redAlertActive) RedAlertStart(NULL);
    return;
  }

  // Pocket remote (2.8") power/damage sync — mirror UI state only;
  // commands already went directly to Bridge/EngRoom from the remote
  if (receivedData.boardInd == 3) {
    if (receivedData.ledCmd == LED_STARTUP && !ledsAreOn) {
      ledsAreOn = true;
      lv_label_set_text(lv_obj_get_child(ui_Button13, 0), "POWER DOWN");
      syncButtonsOn();
    } else if (receivedData.ledCmd == LED_SHUTDOWN && ledsAreOn) {
      ledsAreOn = false;
      lv_label_set_text(lv_obj_get_child(ui_Button13, 0), "POWER UP ALL");
      syncButtonsOff();
    } else if (receivedData.ledCmd == LED_ELEC_SHORT && !damageActive) {
      damageActive = true;
      lv_label_set_text(lv_obj_get_child(ui_Button49, 0), "DMG ACTIVE");
    } else if (receivedData.ledCmd == LED_ON && damageActive) {
      damageActive = false;
      lv_label_set_text(lv_obj_get_child(ui_Button49, 0), "DAMAGE CTRL");
    } else if (receivedData.ledCmd == SND_CMD_PLAY) {
      padSndPlay((uint8_t)receivedData.ledValue);
    }
    return;
  }

  // Update per-unit last-contact timestamp on every message — used for offline detection
  if (receivedData.boardInd == 1) {
    lastBridgeMs = millis();
    if (!bridgeSeen) {
      bridgeSeen = true; bridgeOnline = true;
      if (ui_BridgeStatusBtn) {
        lv_label_set_text(lv_obj_get_child(ui_BridgeStatusBtn, 0), "BRIDGE\nONLINE");
        lv_obj_set_style_bg_color(ui_BridgeStatusBtn, lv_color_hex(0xCC7722), LV_PART_MAIN | LV_STATE_DEFAULT);
      }
      updateSyncModeBtn();
    }
    if (receivedData.status == 1) {
      Board1 = 1;
      bridgeAsmOn = (receivedData.ledValue == 1);
      if (ui_Button52) {
        lv_obj_set_style_bg_color(ui_Button52,
          bridgeAsmOn ? lv_color_hex(0x00AA55) : lv_color_hex(0xFF9C00),
          LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(lv_obj_get_child(ui_Button52, 0),
          bridgeAsmOn ? "Bridge ASM\nON" : "Bridge ASM");
      }
    }
    if (receivedData.status == 2 && ledsAreOn) {
      switch (nvNavTiming) {
        case 1: sendLedCmd(15, LED_BLINK, 1000); break;
        case 2: sendLedCmd(15, LED_BLINK, 1500); break;
        case 3:
          sendLedCmdTo(broadcastAddress2, 15, LED_ON, 255);
          sendLedCmdTo(broadcastAddress1, IDX_ENGROOM_NAV, LED_ON, 255);
          break;
      }
    }
  }
  if (receivedData.boardInd == 2) {
    lastEngRoomMs = millis();
    if (!engRoomSeen) {
      engRoomSeen = true; engRoomOnline = true;
      if (ui_EngRoomStatusBtn) {
        lv_label_set_text(lv_obj_get_child(ui_EngRoomStatusBtn, 0), "ENGROOM\nONLINE");
        lv_obj_set_style_bg_color(ui_EngRoomStatusBtn, lv_color_hex(0xCC7722), LV_PART_MAIN | LV_STATE_DEFAULT);
      }
      updateSyncModeBtn();
    }
    if (receivedData.status == 1) {
      Board2 = 1;
      engAsmOn = (receivedData.ledValue == 1);
      if (ui_Button53) {
        lv_obj_set_style_bg_color(ui_Button53,
          engAsmOn ? lv_color_hex(0x00AA55) : lv_color_hex(0xFF9C00),
          LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(lv_obj_get_child(ui_Button53, 0),
          engAsmOn ? "EngRoom ASM\nON" : "EngRoom ASM");
      }
    }
  }

  // WiFi status reply from EngRoom (wifiStatus 2 = fail, 3 = connected)
  if (receivedData.wifiStatus == 3) {
    lv_obj_add_state(ui_Button45, LV_STATE_CHECKED);
    IPAddress ip(receivedData.ipAddress);
    static char engBuf[64];
    snprintf(engBuf, sizeof(engBuf), "%s\n%s", ip.toString().c_str(), receivedData.ssid1);
    lv_label_set_text(ui_Label18, engBuf);
  } else if (receivedData.wifiStatus == 2) {
    lv_obj_clear_state(ui_Button45, LV_STATE_CHECKED);
    lv_obj_add_state(ui_Button45, LV_STATE_DEFAULT);
    lv_label_set_text(ui_Label18, "did not connect\nbattle bridge");
  }

  // WiFi status reply from Bridge (wifiStatus 4 = fail, 5 = connected)
  if (receivedData.wifiStatus == 5) {
    lv_obj_add_state(ui_Button46, LV_STATE_CHECKED);
    IPAddress ip(receivedData.ipAddress);
    lv_label_set_text(ui_Label19, ip.toString().c_str());
  } else if (receivedData.wifiStatus == 4) {
    lv_obj_clear_state(ui_Button46, LV_STATE_CHECKED);
    lv_obj_add_state(ui_Button46, LV_STATE_DEFAULT);
    lv_label_set_text(ui_Label19, "did not connect\nbridge");
  }

  // WiFi status reply from WarpCore (wifiStatus 6 = fail, 7 = connected)
  if (receivedData.wifiStatus == 7) {
    IPAddress ip(receivedData.ipAddress);
    if (ui_LabelWCIP) lv_label_set_text(ui_LabelWCIP, ip.toString().c_str());
    if (ui_WarpCoreStsBtn) lv_obj_set_style_bg_color(ui_WarpCoreStsBtn, lv_color_hex(0x2D802B), LV_PART_MAIN | LV_STATE_DEFAULT);
  } else if (receivedData.wifiStatus == 6) {
    if (ui_LabelWCIP) lv_label_set_text(ui_LabelWCIP, "did not connect\nwarpcore");
    if (ui_WarpCoreStsBtn) lv_obj_set_style_bg_color(ui_WarpCoreStsBtn, lv_color_hex(0xC41313), LV_PART_MAIN | LV_STATE_DEFAULT);
  }

  // Bridge auto-cancelled damage control (status=3) — reset button
  if (receivedData.boardInd == 1 && receivedData.status == 3) {
    damageActive = false;
    lv_label_set_text(lv_obj_get_child(ui_Button49, 0), "DAMAGE CTRL");
  }
}


// ── Setup ─────────────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);

  // DataPad local sound — DY1703A on RS485 connector
  padSnd.begin(9600, SERIAL_8N1, PAD_SND_RX, PAD_SND_TX);
  delay(1000);
  { uint8_t qry[4] = {0xAA, 0x01, 0x00, 0xAB}; padSnd.write(qry, 4); }
  { uint32_t t = millis(); bool ok = false;
    while (millis() - t < 800)
      if (padSnd.available() && padSnd.read() == 0xAA) { ok = true; break; }
    while (padSnd.available()) padSnd.read();
    Serial.println(ok ? "Pad sound OK" : "Pad sound: no response");
  }
  padSndSetVol(padSndVol);

  WiFi.mode(WIFI_STA);
  esp_wifi_set_ps(WIFI_PS_NONE);   // disable WiFi power save — prevents RGB DMA corruption
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);   // set ch=1 before first send

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }
  esp_now_register_recv_cb((esp_now_recv_cb_t)OnDataRecv);
  esp_now_register_send_cb((esp_now_send_cb_t)OnDataSent);

  // Register EngRoom peer
  memcpy(peerInfo.peer_addr, broadcastAddress1, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add EngRoom peer");
    return;
  }

  // Register Bridge peer
  memcpy(peerInfo.peer_addr, broadcastAddress2, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add Bridge peer");
    return;
  }

  // Register WarpCore peer
  memcpy(peerInfo.peer_addr, broadcastAddressWarpCore, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add WarpCore peer");
  }

  // Register broadcast peer (for WiFi credential push)
  memcpy(peerInfo.peer_addr, broadcastAll, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  esp_wifi_get_channel(&wifiChannel, NULL);
  Serial.printf("ESP-NOW ready  ch=%d\n", wifiChannel);

  // Boot ping — triggers Bridge and EngRoom to reply with their current state
  {
    struct_message bootPing; memset(&bootPing, 0, sizeof(bootPing));
    bootPing.boardInd = 3; bootPing.status = 1;
    esp_now_send(0, (uint8_t *)&bootPing, sizeof(bootPing));
  }

  // OTA callbacks — ArduinoOTA.begin() called in loop() after WiFi connects
  ArduinoOTA
    .onStart([]() {
      dp_board->getBacklight()->setBrightness(0);
      lvgl_port_lock(-1);
      Serial.println("OTA start: " + String(ArduinoOTA.getCommand() == U_FLASH ? "sketch" : "filesystem"));
    })
    .onEnd([]() {
      lvgl_port_unlock();
      dp_board->getBacklight()->setBrightness(100);
      Serial.println("\nOTA end");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      if (millis() - last_ota_time > 500) {
        Serial.printf("OTA progress: %u%%\n", progress / (total / 100));
        last_ota_time = millis();
      }
    })
    .onError([](ota_error_t error) {
      lvgl_port_unlock();
      dp_board->getBacklight()->setBrightness(100);
      const char* msg[] = {"Auth Failed","Begin Failed","Connect Failed","Receive Failed","End Failed"};
      if (error <= OTA_END_ERROR) Serial.printf("OTA Error[%u]: %s\n", error, msg[error]);
    });
  ArduinoOTA.setHostname("DataPad");

  // ── Auto-connect BEFORE display init ─────────────────────────────────────────
  // WiFi.begin() causes RGB LCD DMA corruption. Running it here — before the
  // display is initialised — means there is no DMA to disrupt.
  bool     autoConnected = false;
  IPAddress autoIP;

  {
    Preferences prefs;
    prefs.begin("datapad", true);
    bool autoWifi = prefs.getBool("wifi_auto", false);
    if (autoWifi) {
      String s = prefs.getString("wifi_ssid", "");
      String p = prefs.getString("wifi_pass", "");
      if (s.length() > 0) {
        strncpy(pending_ssid,     s.c_str(), 32); pending_ssid[32]     = '\0';
        strncpy(pending_password, p.c_str(), 64); pending_password[64] = '\0';
        wifiSaved = true;
        Serial.printf("Auto-connecting to: %s\n", pending_ssid);

        // Notify Bridge and EngRoom to connect to the same network before
        // we switch channels — same as the manual SaveWifi path does
        strncpy(myData.ssid1,     pending_ssid,     32); myData.ssid1[32]     = '\0';
        strncpy(myData.password1, pending_password, 64); myData.password1[64] = '\0';
        myData.startWifi = 1;
        esp_now_send(0, (uint8_t *)&myData, sizeof(myData));
        myData.startWifi = 0;
        delay(150);   // let the send complete before WiFi.begin changes channel

        WiFi.disconnect();
        WiFi.begin(pending_ssid, pending_password);
        unsigned long t = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - t < 10000) {
          delay(100);
          Serial.print(".");
        }
        Serial.println();
        if (WiFi.status() == WL_CONNECTED) {
          autoConnected = true;
          autoIP = WiFi.localIP();
          wifiChannel = (uint8_t)WiFi.channel();
          Serial.printf("Connected. IP: %s  ch=%d\n", autoIP.toString().c_str(), wifiChannel);
          if (MDNS.begin(mdnsName)) Serial.printf("mDNS: http://%s.local\n", mdnsName);
          else                      Serial.println("mDNS failed");
          ArduinoOTA.begin();
          servicesStarted = true;
        } else {
          // Failed — disconnect cleanly but stay in STA mode (already set at top of setup)
          // Do NOT call WiFi.mode() here — it reinitialises the driver right before
          // dp_board->init() and can prevent the display from starting.
          WiFi.disconnect();
          delay(300);   // let the WiFi stack settle before display init
          Serial.println("Auto-connect failed — display will start, retry via loop()");
          do_wifi_connect = false;   // don't auto-retry in loop(); user can press SaveWifi again
        }
      }
    }
    prefs.end();
  }

  {
    Preferences prefs;
    prefs.begin("datapad", true);
    nvsSyncMode        = prefs.getInt( "sync_mode", 1);
    touchSoundEnabled  = prefs.getBool("touch_snd", false);
    ambientEnabled     = prefs.getBool("ambient_on", false);
    radarEnabled             = prefs.getBool("radar_on",    true);
    redAlertSensorEnabled    = prefs.getBool("alert_radar", true);
    nvNavTiming              = prefs.getInt( "nav_timing",  1);
    prefs.end();
  }

  // ── Board + display init (radio is settled before we touch the DMA) ───────────
  Serial.println("Initializing board");
  dp_board = new Board();
  dp_board->init();

#if LVGL_PORT_AVOID_TEARING_MODE
  auto lcd = dp_board->getLCD();
  lcd->configFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);
#if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB && CONFIG_IDF_TARGET_ESP32S3
  auto lcd_bus = lcd->getBus();
  if (lcd_bus->getBasicAttributes().type == ESP_PANEL_BUS_TYPE_RGB) {
    static_cast<BusRGB *>(lcd_bus)->configRGB_BounceBufferSize(lcd->getFrameWidth() * 10);
  }
#endif
#endif
  assert(dp_board->begin());

  Serial.println("Initializing LVGL");
  lvgl_port_init(dp_board->getLCD(), dp_board->getTouch());

  Serial.println("Creating UI");
  lvgl_port_lock(-1);
  ui_init();

  // Restore UI state to match auto-connect result
  if (wifiSaved) {
    lv_obj_add_state(ui_Button50, LV_STATE_CHECKED);
    lv_label_set_text(lv_obj_get_child(ui_Button50, 0), "WIFI SAVED");
  }
  if (autoConnected) {
    lv_obj_add_state(ui_Button47, LV_STATE_CHECKED);
    lv_label_set_text(ui_Label15, autoIP.toString().c_str());
  } else if (wifiSaved) {
    // Credentials saved but connection failed — show clearly on the WiFi screen
    lv_label_set_text(ui_Label15, "Auto-connect failed\nPress Save WiFi to retry");
  }

  lvgl_port_unlock();
}


// ── Loop ──────────────────────────────────────────────────────────────────────

void loop() {
  // handle received ESP-NOW data here with LVGL mutex — not inside callback
  if (new_data_received) {
    new_data_received = false;
    lvgl_port_lock(-1);
    handleReceivedData();
    lvgl_port_unlock();
  }

  // System online — set once when both units first check in
  if (online == 0 && Board1 == 1 && Board2 == 1) {
    lvgl_port_lock(-1);
    lv_label_set_text(ui_Label8, "SYSTEM ONLINE");
    lvgl_port_unlock();
    online = 1;
    Serial.println("System online");
  }

  // Connection watchdog — update status label whenever a unit's online state changes
  if (online) {
    bool bOk  = !bridgeSeen  || (millis() - lastBridgeMs  <= UNIT_OFFLINE_MS);
    bool erOk = !engRoomSeen || (millis() - lastEngRoomMs <= UNIT_OFFLINE_MS);

    if (bOk != bridgeOnline || erOk != engRoomOnline) {
      bridgeOnline  = bOk;
      engRoomOnline = erOk;
      Serial.printf("Status: Bridge=%s  EngRoom=%s\n", bOk?"ON":"OFF", erOk?"ON":"OFF");

      const char* msg;
      if (bOk && erOk)    msg = "SYSTEM ONLINE";
      else if (!bOk && !erOk) msg = "ALL UNITS OFFLINE";
      else if (!bOk)      msg = "BRIDGE OFFLINE";
      else                msg = "ENGROOM OFFLINE";

      lvgl_port_lock(-1);
      lv_label_set_text(ui_Label8, msg);
      if (ui_BridgeStatusBtn) {
        lv_label_set_text(lv_obj_get_child(ui_BridgeStatusBtn, 0), bOk ? "BRIDGE\nONLINE" : "BRIDGE\nOFFLINE");
        lv_obj_set_style_bg_color(ui_BridgeStatusBtn, lv_color_hex(bOk ? 0xCC7722 : 0x333355), LV_PART_MAIN | LV_STATE_DEFAULT);
      }
      if (ui_EngRoomStatusBtn) {
        lv_label_set_text(lv_obj_get_child(ui_EngRoomStatusBtn, 0), erOk ? "ENGROOM\nONLINE" : "ENGROOM\nOFFLINE");
        lv_obj_set_style_bg_color(ui_EngRoomStatusBtn, lv_color_hex(erOk ? 0xCC7722 : 0x333355), LV_PART_MAIN | LV_STATE_DEFAULT);
      }
      updateSyncModeBtn();
      lvgl_port_unlock();
    }
  }

  // Red alert auto-dismiss and sound repeat
  if (redAlertActive) {
    uint32_t now = millis();
    if (now >= redAlertAutoOffMs) {
      RedAlertDismiss(NULL);
    } else if (now >= redAlertNextSndMs) {
      padSndPlay(28);
      redAlertNextSndMs = now + 16000;
    }
  }

  // Yellow alert auto-dismiss and sound repeat
  if (yellowAlertActive) {
    uint32_t now = millis();
    if (now >= yellowAlertAutoOffMs) {
      YellowAlertDismiss(NULL);
    } else if (now >= yellowAlertNextSndMs) {
      padSndPlay(28);
      yellowAlertNextSndMs = now + 16000;
    }
  }

  // Ambient software loop — re-triggers padSndPlay every AMBIENT_POLL_MS
  if (ambientNextPlayMs && millis() >= ambientNextPlayMs) {
    if (ambientEnabled && ledsAreOn && !warpAmbientMs) {
      padSndPlay(SND_AMBIENT_FIRST + 1 + random(0, SND_AMBIENT_LAST - SND_AMBIENT_FIRST - 1));
      ambientNextPlayMs = millis() + AMBIENT_POLL_MS;
    } else if (ambientEnabled && ledsAreOn) {
      ambientNextPlayMs = millis() + AMBIENT_POLL_MS;  // warp active — defer, don't override warp sounds
    } else {
      ambientNextPlayMs = 0;
    }
  }

  // Screen timeout — backlight off after SCREEN_TIMEOUT_MS of no touch
  {
    uint32_t idleMs = lv_disp_get_inactive_time(NULL);
    if (!screenAsleep && idleMs >= SCREEN_TIMEOUT_MS) {
      dp_board->getBacklight()->setBrightness(0);
      screenAsleep = true;
      Serial.println("Screen off (idle timeout)");
    } else if (screenAsleep && idleMs < 1000) {
      dp_board->getBacklight()->setBrightness(100);
      screenAsleep = false;
      Serial.println("Screen on (touch wake)");
    }
  }

  // non-blocking WiFi connect — StartWiFi() sets do_wifi_connect flag and returns
  if (do_wifi_connect && !wifi_connecting) {
    do_wifi_connect  = false;
    wifi_connecting  = true;
    wifi_retries     = 1;
    wifi_start_time  = millis();
    WiFi.disconnect();
    WiFi.begin(pending_ssid, pending_password);
    Serial.printf("WiFi attempt 1/2: %s\n", pending_ssid);
  }

  if (wifi_connecting) {
    // Poll at 100ms intervals — reduces CPU pressure on the RGB LCD DMA
    static uint32_t lastWifiCheckMs = 0;
    if (millis() - lastWifiCheckMs < 100) goto skip_wifi_check;
    lastWifiCheckMs = millis();

    if (WiFi.status() == WL_CONNECTED) {
      wifi_connecting = false;
      wifiChannel = (uint8_t)WiFi.channel();
      Serial.printf("DataPad WiFi connected. IP: %s  ch=%d\n", WiFi.localIP().toString().c_str(), wifiChannel);
      lvgl_port_lock(-1);
      lv_obj_add_state(ui_Button47, LV_STATE_CHECKED);
      lv_label_set_text(ui_Label15, WiFi.localIP().toString().c_str());
      lvgl_port_unlock();

      if (!servicesStarted) {
        if (MDNS.begin(mdnsName)) {
          Serial.printf("mDNS started: http://%s.local\n", mdnsName);
        } else {
          Serial.println("mDNS failed — continuing without it");
        }
        ArduinoOTA.begin();
        servicesStarted = true;
      }
      resetDisplay();   // reset after MDNS/OTA start — radio fully settled by now

      // Re-establish ESP-NOW after WiFi connects: disable power save and refresh
      // peers on the new channel so sends don't fail
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
      Serial.printf("ESP-NOW peers refreshed  ch=%d\n", wifiChannel);

    } else if (millis() - wifi_start_time > 10000) {
      if (wifi_retries < 2) {
        wifi_retries++;
        wifi_start_time = millis();
        WiFi.disconnect();
        WiFi.begin(pending_ssid, pending_password);
        Serial.printf("WiFi attempt %d/2: %s\n", wifi_retries, pending_ssid);
      } else {
        wifi_connecting = false;
        WiFi.disconnect();
        esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
        wifiChannel = 1;
        Serial.printf("DataPad WiFi: did not connect after 2 attempts. ESP-NOW ch=%d\n", wifiChannel);
        lvgl_port_lock(-1);
        lv_obj_clear_state(ui_Button47, LV_STATE_CHECKED);
        lv_obj_add_state(ui_Button47, LV_STATE_DEFAULT);
        lv_label_set_text(ui_Label15, "Did not Connect");
        lvgl_port_unlock();
        resetDisplay();
      }
    }
    skip_wifi_check:;
  }

  // WiFi drop watchdog — auto-revert if connected but WiFi lost for WIFI_REVERT_MS
  {
    static uint32_t wifiDropMs = 0;
    if (servicesStarted && !wifi_connecting) {
      if (WiFi.status() != WL_CONNECTED) {
        if (wifiDropMs == 0) wifiDropMs = millis();
        else if (millis() - wifiDropMs >= WIFI_REVERT_MS) {
          wifiDropMs = 0;
          Serial.println("WiFi dropped — auto-reverting to ESP-NOW ch=1");
          revertWifiLocal();
          // Reset contact timestamps so watchdog immediately sees them as offline
          // They'll come back online naturally on their first heartbeat after reverting
          lastBridgeMs  = 0;
          lastEngRoomMs = 0;
          bridgeOnline  = false;
          engRoomOnline = false;
          lvgl_port_lock(-1);
          lv_obj_clear_state(ui_Button50, LV_STATE_CHECKED);
          lv_obj_add_state(ui_Button50, LV_STATE_DEFAULT);
          lv_label_set_text(lv_obj_get_child(ui_Button50, 0), "SAVE WIFI");
          lv_label_set_text(ui_Label15, "WiFi lost — reverted");
          lv_label_set_text(ui_Label8, "ALL UNITS OFFLINE");
          if (ui_BridgeStatusBtn) {
            lv_label_set_text(lv_obj_get_child(ui_BridgeStatusBtn, 0), "BRIDGE\nOFFLINE");
            lv_obj_set_style_bg_color(ui_BridgeStatusBtn, lv_color_hex(0x333355), LV_PART_MAIN | LV_STATE_DEFAULT);
          }
          if (ui_EngRoomStatusBtn) {
            lv_label_set_text(lv_obj_get_child(ui_EngRoomStatusBtn, 0), "ENGROOM\nOFFLINE");
            lv_obj_set_style_bg_color(ui_EngRoomStatusBtn, lv_color_hex(0x333355), LV_PART_MAIN | LV_STATE_DEFAULT);
          }
          updateSyncModeBtn();
          lvgl_port_unlock();
        }
      } else { wifiDropMs = 0; }
    }
  }

  // Warp ambient — re-trigger tng_engine_1 every 17s (DataPad only; Bridge has no file 37)
  if (warpAmbientMs && millis() >= warpAmbientMs && warpSpeedVal > 0) {
    padSndPlay(37);
    warpAmbientMs = millis() + 17000UL;
  }

  // Heartbeat — send a keepalive ping to both prop units every 30 seconds
  // Bridge/EngRoom use this to detect connection loss
  static uint32_t lastHeartbeatMs = 0;
  if (millis() - lastHeartbeatMs >= 30000) {
    lastHeartbeatMs = millis();
    struct_message ping;
    memset(&ping, 0, sizeof(ping));
    ping.boardInd = 3;   // DataPad
    ping.status   = 0;   // heartbeat — no action required on receivers
    esp_now_send(0, (uint8_t *)&ping, sizeof(ping));   // broadcast to all peers
  }

  ArduinoOTA.handle();
}


// ── Display recovery ─────────────────────────────────────────────────────────
// Restarts the RGB LCD DMA to clear corruption caused by WiFi radio operations.
void resetDisplay() {
  if (dp_board == nullptr) return;
  auto handle = dp_board->getLCD()->getHandle();
  if (handle == nullptr) return;
  lvgl_port_lock(-1);
  esp_lcd_rgb_panel_restart(handle);
  lv_obj_invalidate(lv_scr_act());
  lvgl_port_unlock();
  Serial.println("Display DMA restarted");
}


// ── LED command helper ────────────────────────────────────────────────────────

static void sendLedCmd(int group, int cmd, int value) {
  struct_message msg;
  memset(&msg, 0, sizeof(msg));
  msg.ledGroup = group; msg.ledCmd = cmd; msg.ledValue = value;
  esp_now_send(0, (uint8_t *)&msg, sizeof(msg));   // broadcast to all peers
}

static void sendLedCmdTo(uint8_t *addr, int group, int cmd, int value) {
  struct_message msg;
  memset(&msg, 0, sizeof(msg));
  msg.ledGroup = group; msg.ledCmd = cmd; msg.ledValue = value;
  esp_now_send(addr, (uint8_t *)&msg, sizeof(msg));
}


// ── LVGL event handlers ───────────────────────────────────────────────────────

static void syncButtonsOff() {
  nacelleOn = false; deflectorOn = false; navState = 0;
  engOn = false; warpCoreOn = false; warpSpeedVal = 0; windowsOn = false;
  stopWarpSounds();
  if (ui_Label5)         lv_label_set_text(ui_Label5, "NAC OFF");
  if (ui_WarpSlider)     lv_slider_set_value(ui_WarpSlider, 0, LV_ANIM_OFF);
  if (ui_Button17)       lv_label_set_text(lv_obj_get_child(ui_Button17, 0), "NAV OFF");
  if (ui_Button48)       lv_label_set_text(lv_obj_get_child(ui_Button48, 0), "ENG OFF");
  if (ui_DeflectorLabel) lv_label_set_text(ui_DeflectorLabel, "DEFLECT");
  if (ui_WarpCoreBtn)    lv_label_set_text(lv_obj_get_child(ui_WarpCoreBtn, 0), "WARP\nCORE");
  if (ui_WinBtn)         lv_label_set_text(lv_obj_get_child(ui_WinBtn, 0), "WINDOWS\nOFF");
}

static void syncButtonsOn() {
  nacelleOn = true; deflectorOn = true; navState = nvNavTiming;
  engOn = true; warpCoreOn = true; windowsOn = true;
  const char* navLbl = (nvNavTiming == 3) ? "NAV ON" : (nvNavTiming == 2) ? "NAV 1.5s" : "NAV 1.0s";
  if (ui_Label5)         lv_label_set_text(ui_Label5, "NAC ON");
  if (ui_Button17)       lv_label_set_text(lv_obj_get_child(ui_Button17, 0), navLbl);
  if (ui_Button48)       lv_label_set_text(lv_obj_get_child(ui_Button48, 0), "ENG ON");
  if (ui_DeflectorLabel) lv_label_set_text(ui_DeflectorLabel, "DEFLECT\nON");
  if (ui_WarpCoreBtn)    lv_label_set_text(lv_obj_get_child(ui_WarpCoreBtn, 0), "WARP\nCORE ON");
  if (ui_WinBtn)         lv_label_set_text(lv_obj_get_child(ui_WinBtn, 0), "WINDOWS\nON");
}

void PowerUpAll(lv_event_t *e) {
  bool bridgeUp  = bridgeOnline;
  bool engRoomUp = engRoomOnline;
  bool assembled = (nvsSyncMode == 1);

  if (!ledsAreOn) {
    if (!bridgeUp && !engRoomUp) return;
    sendSndCmd(SND_CMD_REPEAT, 0);
    if (!bridgeUp) padSndPlay(SND_POWER_FIRST);   // Bridge offline — play startup sound locally
    if (assembled && bridgeUp && engRoomUp) {
      sendLedCmdTo(broadcastAddress2, -1, LED_STARTUP, 0);   // Bridge relays to EngRoom at step 10
    } else if (assembled && bridgeUp) {
      sendLedCmdTo(broadcastAddress2, -1, LED_STARTUP, 1);   // EngRoom offline, Bridge runs solo
    } else if (assembled && engRoomUp) {
      sendLedCmdTo(broadcastAddress1, -1, LED_STARTUP, 0);   // Bridge offline, EngRoom runs solo
    } else {
      if (bridgeUp)  sendLedCmdTo(broadcastAddress2, -1, LED_STARTUP, 1);
      if (engRoomUp) sendLedCmdTo(broadcastAddress1, -1, LED_STARTUP, 0);
    }
    sendLedCmdTo(broadcastAddressWarpCore, 0, LED_STARTUP, 0);
    ledsAreOn = true;
    lv_label_set_text(lv_obj_get_child(ui_Button13, 0), "POWER DOWN");
    syncButtonsOn();
    if (ambientEnabled) {
      ambientNextPlayMs = millis() + 3000;  // delay first clip — avoids UART collision with touch sound
    }
  } else {
    if (!bridgeUp) padSndPlay(random(SND_POWER_FIRST + 1, SND_POWER_LAST + 1));  // Bridge offline
    if (assembled && engRoomUp) {
      sendLedCmdTo(broadcastAddress1, -1, LED_SHUTDOWN, 0);  // EngRoom relays to Bridge when done
    } else if (assembled && bridgeUp) {
      sendLedCmdTo(broadcastAddress2, -1, LED_SHUTDOWN, 0);  // EngRoom offline, Bridge direct
    } else {
      if (engRoomUp) sendLedCmdTo(broadcastAddress1, -1, LED_SHUTDOWN, 0);
      if (bridgeUp)  sendLedCmdTo(broadcastAddress2, -1, LED_SHUTDOWN, 0);
    }
    sendLedCmdTo(broadcastAddressWarpCore, 0, LED_SHUTDOWN, 0);
    ledsAreOn = false;
    lv_label_set_text(lv_obj_get_child(ui_Button13, 0), "POWER UP ALL");
    syncButtonsOff();
    if (ambientEnabled) { padSndStop(); ambientNextPlayMs = 0; }
  }
}

static void updateSyncModeBtn() {
  if (!ui_SyncModeBtn) return;
  const char *lbl;
  uint32_t col;
  if (nvsSyncMode == 0) {
    lbl = "SEPARATED"; col = 0xFF9C00;
  } else if (bridgeOnline && engRoomOnline) {
    lbl = "ASSEMBLED"; col = 0xFF9C00;
  } else {
    lbl = "PART\nONLINE"; col = 0xBB8800;  // one section offline in assembled mode
  }
  lv_label_set_text(lv_obj_get_child(ui_SyncModeBtn, 0), lbl);
  lv_obj_set_style_bg_color(ui_SyncModeBtn, lv_color_hex(col), LV_PART_MAIN | LV_STATE_DEFAULT);
}

void SyncModeToggle(lv_event_t *e) {
  nvsSyncMode = nvsSyncMode ? 0 : 1;
  {
    Preferences prefs;
    prefs.begin("datapad", false);
    prefs.putInt("sync_mode", nvsSyncMode);
    prefs.end();
  }
  sendLedCmdTo(broadcastAddress1, -1, LED_SYNC_MODE, nvsSyncMode);
  sendLedCmdTo(broadcastAddress2, -1, LED_SYNC_MODE, nvsSyncMode);
  updateSyncModeBtn();
}

void TouchSoundToggle(lv_event_t *e) {
  touchSoundEnabled = !touchSoundEnabled;
  {
    Preferences prefs;
    prefs.begin("datapad", false);
    prefs.putBool("touch_snd", touchSoundEnabled);
    prefs.end();
  }
  lv_label_set_text(lv_obj_get_child(lv_event_get_target(e), 0),
                    touchSoundEnabled ? "TOUCH SND\nON" : "TOUCH SND");
}

void AmbientToggle(lv_event_t *e) {
  ambientEnabled = !ambientEnabled;
  {
    Preferences prefs;
    prefs.begin("datapad", false);
    prefs.putBool("ambient_on", ambientEnabled);
    prefs.end();
  }
  if (ambientEnabled && ledsAreOn) {
    padSndPlay(SND_AMBIENT_FIRST + 1 + random(0, SND_AMBIENT_LAST - SND_AMBIENT_FIRST - 1));
    ambientNextPlayMs = millis() + AMBIENT_POLL_MS;
  } else if (!ambientEnabled) { padSndStop(); ambientNextPlayMs = 0; }
  lv_label_set_text(lv_obj_get_child(lv_event_get_target(e), 0),
                    ambientEnabled ? "AMBIENT\nON" : "AMBIENT");
}

void NavLightsSet(lv_event_t *e) {
  navState = (navState + 1) % 4;
  switch (navState) {
    case 1:
      sendLedCmd(15, LED_BLINK, 1000);
      lv_label_set_text(lv_obj_get_child(ui_Button17, 0), "NAV 1.0s");
      break;
    case 2:
      sendLedCmd(15, LED_BLINK, 1500);
      lv_label_set_text(lv_obj_get_child(ui_Button17, 0), "NAV 1.5s");
      break;
    case 3:
      sendLedCmdTo(broadcastAddress2, 15, LED_ON, 255);
      sendLedCmdTo(broadcastAddress1, IDX_ENGROOM_NAV, LED_ON, 255);
      lv_label_set_text(lv_obj_get_child(ui_Button17, 0), "NAV ON");
      break;
    case 0:
      sendLedCmdTo(broadcastAddress2, 15, LED_OFF, 0);
      sendLedCmdTo(broadcastAddress1, IDX_ENGROOM_NAV, LED_OFF, 0);
      lv_label_set_text(lv_obj_get_child(ui_Button17, 0), "NAV OFF");
      break;
  }
  if (navState != 0) {
    nvNavTiming = navState;
    Preferences prefs; prefs.begin("datapad", false);
    prefs.putInt("nav_timing", nvNavTiming);
    prefs.end();
  }
}

void ImpulsEng(lv_event_t *e) {
  engOn = !engOn;
  if (engOn) {
    sendLedCmd(GRP_BOTH_ENGINES, LED_ENGINE, 0);
    sendLedCmdTo(broadcastAddress1, IDX_ENGROOM_IMPULSE, LED_ON, 0);
    lv_label_set_text(lv_obj_get_child(ui_Button48, 0), "ENG ON");
  } else {
    sendLedCmd(GRP_BOTH_ENGINES, LED_OFF, 0);
    sendLedCmdTo(broadcastAddress1, IDX_ENGROOM_IMPULSE, LED_OFF, 0);
    lv_label_set_text(lv_obj_get_child(ui_Button48, 0), "ENG OFF");
  }
}

void DamageControl(lv_event_t *e) {
  if (!damageActive) {
    damageActive = true;
    sendLedCmd(-1, LED_ELEC_SHORT, DAMAGE_AUTO_OFF_MS);  // Bridge runs the timer
    lv_label_set_text(lv_obj_get_child(ui_Button49, 0), "DMG ACTIVE");
  } else {
    damageActive = false;
    sendLedCmd(-1, LED_OFF, 0);   // manually cancel
    lv_label_set_text(lv_obj_get_child(ui_Button49, 0), "DAMAGE CTRL");
  }
}


static void stopWarpSounds() {
  if (!warpAmbientMs) return;
  warpAmbientMs = 0;
  stopToDest();
  if (padAudioDest != 1) sendSndCmd(SND_CMD_REPEAT, 0);
  if (ambientEnabled && ledsAreOn) ambientNextPlayMs = millis() + 3000;  // resume ambient after warp clears
}

void NacelleToggle(lv_event_t *e) {
  nacelleOn = !nacelleOn;
  if (nacelleOn) {
    sendLedCmdTo(broadcastAddress1, GRP_BOTH_NAC, LED_WARP, 0);  // clear any warp mode first
    sendLedCmdTo(broadcastAddress1, GRP_BOTH_NAC, LED_ON, 0);    // force normal sine
    lv_label_set_text(ui_Label5, "NAC ON");
    if (ui_WarpSlider) lv_slider_set_value(ui_WarpSlider, 0, LV_ANIM_OFF);
    warpSpeedVal = 0;
  } else {
    stopWarpSounds();
    sendLedCmdTo(broadcastAddress1, GRP_BOTH_NAC, LED_OFF, 0);
    sendLedCmdTo(broadcastAddress1, GRP_BOTH_NAC, LED_WARP, 0);
    lv_label_set_text(ui_Label5, "NAC OFF");
    if (ui_WarpSlider) lv_slider_set_value(ui_WarpSlider, 0, LV_ANIM_OFF);
    warpSpeedVal = 0;
  }
}

void WarpSpeed(lv_event_t *e) {
  uint8_t prev      = warpSpeedVal;
  warpSpeedVal      = (uint8_t)lv_slider_get_value(lv_event_get_target(e));
  sendLedCmdTo(broadcastAddress1, GRP_BOTH_NAC, LED_WARP, warpSpeedVal);
  sendLedCmdTo(broadcastAddressWarpCore, 0, LED_WARP, warpSpeedVal);
  if (warpSpeedVal > 0 && prev == 0) {
    if (!warpCoreOn) {
      sendLedCmdTo(broadcastAddressWarpCore, 0, LED_ON, 0);
      warpCoreOn = true;
      if (ui_WarpCoreBtn)
        lv_label_set_text(lv_obj_get_child(ui_WarpCoreBtn, 0), "WARP\nCORE ON");
    }
    // first activation — play startup sound, schedule ambient
    playToDest(14);                               // tng_warp4_clean
    warpAmbientMs = millis() + 8000;              // ambient after warp4 sound finishes (~8s)
    nacelleOn = true;
    lv_label_set_text(ui_Label5, "NAC ON");
  } else if (warpSpeedVal == 0 && prev > 0) {
    // deactivation
    stopWarpSounds();
    nacelleOn = false;
    lv_label_set_text(ui_Label5, "NAC OFF");
  }
}

void DeflectorToggle(lv_event_t *e) {
  deflectorOn = !deflectorOn;
  sendLedCmdTo(broadcastAddress1, IDX_ENGROOM_DEFLECTOR,
               deflectorOn ? LED_ON : LED_OFF, deflectorOn ? 255 : 0);
  lv_label_set_text(ui_DeflectorLabel, deflectorOn ? "DEFLECT\nON" : "DEFLECT");
}

void WarpCoreToggle(lv_event_t *e) {
  warpCoreOn = !warpCoreOn;
  sendLedCmdTo(broadcastAddressWarpCore, 0, warpCoreOn ? LED_ON : LED_OFF, 0);
  if (ui_WarpCoreBtn)
    lv_label_set_text(lv_obj_get_child(ui_WarpCoreBtn, 0), warpCoreOn ? "WARP\nCORE ON" : "WARP\nCORE");
}

void WindowsToggle(lv_event_t *e) {
  windowsOn = !windowsOn;
  sendLedCmdTo(broadcastAddress2, GRP_ALL_WINDOWS, windowsOn ? LED_ON : LED_OFF, 0);
  sendLedCmdTo(broadcastAddress1, GRP_ALL_WINDOWS, windowsOn ? LED_ON : LED_OFF, 0);
  if (ui_WinBtn)
    lv_label_set_text(lv_obj_get_child(ui_WinBtn, 0), windowsOn ? "WINDOWS\nON" : "WINDOWS\nOFF");
}

void RadarToggle(lv_event_t *e) {
  radarEnabled = !radarEnabled;
  {
    Preferences prefs;
    prefs.begin("datapad", false);
    prefs.putBool("radar_on", radarEnabled);
    prefs.end();
  }
  sendLedCmdTo(broadcastAddressWarpCore, 0, LED_RADAR_EN, radarEnabled ? 1 : 0);
  if (ui_RadarBtn)
    lv_label_set_text(lv_obj_get_child(ui_RadarBtn, 0), radarEnabled ? "SENSORS\nON" : "SENSORS\nOFF");
}

void RadarAlertToggle(lv_event_t *e) {
  redAlertSensorEnabled = !redAlertSensorEnabled;
  {
    Preferences prefs;
    prefs.begin("datapad", false);
    prefs.putBool("alert_radar", redAlertSensorEnabled);
    prefs.end();
  }
  if (ui_RadarAlertBtn)
    lv_label_set_text(lv_obj_get_child(ui_RadarAlertBtn, 0),
      redAlertSensorEnabled ? "ALERT\nSENSOR ON" : "ALERT\nSENSOR OFF");
}

static int voltsToPct(float v) {
  int p = (int)((v - 6.0f) / 2.4f * 100.0f);
  if (p < 0) p = 0;
  if (p > 100) p = 100;
  return p;
}

void updateBatDisplay() {
  if (ui_BridgeBatLabel) {
    char buf[20];
    if (bridgeBatVolts > 0.0f) {
      int pct = voltsToPct(bridgeBatVolts);
      uint32_t col = pct >= 50 ? 0x2D802B : (pct >= 20 ? 0xFF9C00 : 0xC41313);
      snprintf(buf, sizeof(buf), "BRIDGE %d%%", pct);
      lv_obj_set_style_text_color(ui_BridgeBatLabel, lv_color_hex(col), LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
      snprintf(buf, sizeof(buf), "BRIDGE --%%");
      lv_obj_set_style_text_color(ui_BridgeBatLabel, lv_color_hex(0x555555), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    lv_label_set_text(ui_BridgeBatLabel, buf);
  }
  if (ui_EngRoomBatLabel) {
    char buf[20];
    if (engRoomBatVolts > 0.0f) {
      int pct = voltsToPct(engRoomBatVolts);
      uint32_t col = pct >= 50 ? 0x2D802B : (pct >= 20 ? 0xFF9C00 : 0xC41313);
      snprintf(buf, sizeof(buf), "ENGROOM %d%%", pct);
      lv_obj_set_style_text_color(ui_EngRoomBatLabel, lv_color_hex(col), LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
      snprintf(buf, sizeof(buf), "ENGROOM --%%");
      lv_obj_set_style_text_color(ui_EngRoomBatLabel, lv_color_hex(0x555555), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    lv_label_set_text(ui_EngRoomBatLabel, buf);
  }
}

void YellowAlertStart(lv_event_t *e) {
  if (yellowAlertActive) return;
  yellowAlertActive    = true;
  yellowAlertNextSndMs = millis() + 16000;
  yellowAlertAutoOffMs = millis() + 60000;
  padSndPlay(28);
  _ui_screen_change(&ui_YellowAlert, LV_SCR_LOAD_ANIM_NONE, 0, 0, &ui_YellowAlert_screen_init);
}

void YellowAlertDismiss(lv_event_t *e) {
  if (!yellowAlertActive) return;
  yellowAlertActive    = false;
  yellowAlertAutoOffMs = 0;
  yellowAlertNextSndMs = 0;
  padSndStop();
  _ui_screen_change(&ui_Screen1, LV_SCR_LOAD_ANIM_NONE, 0, 0, &ui_Screen1_screen_init);
}

void RedAlertStart(lv_event_t *e) {
  if (redAlertActive) return;
  redAlertActive    = true;
  redAlertNextSndMs = millis() + 16000;
  redAlertAutoOffMs = millis() + 60000;
  padSndPlay(28);
  _ui_screen_change(&ui_RedAlert, LV_SCR_LOAD_ANIM_NONE, 0, 0, &ui_RedAlert_screen_init);
}

void RedAlertDismiss(lv_event_t *e) {
  redAlertActive    = false;
  redAlertNextSndMs = 0;
  redAlertAutoOffMs = 0;
  padSndStop();
  _ui_screen_change(&ui_Screen1, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, &ui_Screen1_screen_init);
}

void AllOff(lv_event_t *e) {
  sendLedCmdTo(broadcastAddress1, -1, LED_ALL_OFF, 0);
  sendLedCmdTo(broadcastAddress2, -1, LED_ALL_OFF, 0);
  sendLedCmdTo(broadcastAddressWarpCore, 0, LED_SHUTDOWN, 0);
  ledsAreOn = false;
  lv_label_set_text(lv_obj_get_child(ui_Button13, 0), "POWER UP ALL");
  syncButtonsOff();
  if (ambientEnabled) { padSndStop(); ambientNextPlayMs = 0; }
}

void AllOn(lv_event_t *e) {
  sendLedCmdTo(broadcastAddress2, GRP_ALL_WINDOWS, LED_ON, 0);
  sendLedCmdTo(broadcastAddress2, GRP_BOTH_ENGINES, LED_ON, 0);
  sendLedCmdTo(broadcastAddress2, GRP_ALL, LED_BLINK, 0);
  sendLedCmdTo(broadcastAddress1, GRP_ALL, LED_ON, 0);
  sendLedCmdTo(broadcastAddress1, GRP_ALL, LED_BLINK, 0);
  sendLedCmdTo(broadcastAddress1, IDX_ENGROOM_DEFLECTOR, LED_ON, 0);
  sendLedCmdTo(broadcastAddress1, IDX_ENGROOM_IMPULSE, LED_ON, 0);
  sendLedCmdTo(broadcastAddress1, GRP_BOTH_NAC, LED_ON, 0);
  sendLedCmdTo(broadcastAddressWarpCore, 0, LED_ON, 0);
  ledsAreOn = true;
  lv_label_set_text(lv_obj_get_child(ui_Button13, 0), "POWER DOWN");
  syncButtonsOn();
}

void PhotonFire(lv_event_t *e) {
  sendLedCmdTo(broadcastAddress1, IDX_PHOTON, LED_ON, 0);
  playToDest(22 + torpedoSoundIdx);
  torpedoSoundIdx ^= 1;
}

void BridgeAssemblyMode(lv_event_t *e) {
  bridgeAsmOn = !bridgeAsmOn;
  sendLedCmdTo(broadcastAddress2, 0, LED_ASSEMBLY_MODE, bridgeAsmOn ? 1 : 0);
  lv_obj_set_style_bg_color(ui_Button52,
    bridgeAsmOn ? lv_color_hex(0x00AA55) : lv_color_hex(0xFF9C00),
    LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(lv_obj_get_child(ui_Button52, 0), bridgeAsmOn ? "Bridge ASM\nON" : "Bridge ASM");
}

void EngRoomAssemblyMode(lv_event_t *e) {
  engAsmOn = !engAsmOn;
  sendLedCmdTo(broadcastAddress1, 0, LED_ASSEMBLY_MODE, engAsmOn ? 1 : 0);
  lv_obj_set_style_bg_color(ui_Button53,
    engAsmOn ? lv_color_hex(0x00AA55) : lv_color_hex(0xFF9C00),
    LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(lv_obj_get_child(ui_Button53, 0), engAsmOn ? "EngRoom ASM\nON" : "EngRoom ASM");
}

// ── Sound controls ────────────────────────────────────────────────────────────
// Wire each function to its LVGL object event in SquareLine Studio.
// Sound commands go to Bridge only — EngRoom has no sound hardware.

static uint8_t sndRepeat      = 0;
static uint8_t sndPowerIdx    = SND_POWER_FIRST;
static uint8_t sndAlertIdx    = SND_ALERT_FIRST;
static uint8_t sndDamageIdx   = SND_DAMAGE_FIRST;
static uint8_t sndWarpIdx     = SND_WARP_FIRST;
static uint8_t sndWeaponsIdx  = SND_WEAPONS_FIRST;
static uint8_t sndVoicedIdx   = SND_VOICED_FIRST;
static uint8_t sndAmbientIdx  = SND_AMBIENT_FIRST;
static uint8_t sndEasterIdx   = SND_EASTER_FIRST;
static uint8_t lastSndFile    = 0;
static bool    lastSndPadOnly = false;

static void sendSndCmd(int cmd, int value) {
  struct_message msg;
  memset(&msg, 0, sizeof(msg));
  msg.ledCmd   = cmd;
  msg.ledValue = value;
  esp_now_send(broadcastAddress2, (uint8_t *)&msg, sizeof(msg));
}

// ── DataPad local sound — DY1703A protocol ───────────────────────────────────
// Frame: AA [CMD] [LEN] [DATA...] [CHECKSUM]

static void padSndSend(uint8_t cmd, uint8_t *data, uint8_t len) {
  uint8_t sum = 0xAA + cmd + len;
  padSnd.write(0xAA); padSnd.write(cmd); padSnd.write(len);
  for (uint8_t i = 0; i < len; i++) { padSnd.write(data[i]); sum += data[i]; }
  padSnd.write(sum & 0xFF);
}

static void padSndPlay(uint8_t fileNum) {
  uint8_t d[2] = {0x00, fileNum};
  padSndSend(0x07, d, 2);
}

static void padSndStop()                  { padSndSend(0x04, nullptr, 0); }
static void padSndSetVol(uint8_t vol)     { if (vol > 30) vol = 30; padSndSend(0x13, &vol, 1); }

void play_touch_sound() {
  if (touchSoundEnabled) {
    padSndPlay(SND_TOUCH_FIRST + random(0, SND_TOUCH_LAST - SND_TOUCH_FIRST + 1));
    if (ambientEnabled && ledsAreOn)
      ambientNextPlayMs = millis() + 2000;  // resume ambient after click sound finishes
  }
}

// Dest-aware play/stop — checks padAudioDest before sending to Bridge or PAD or both
static void playToDest(uint8_t fileNum) {
  lastSndFile    = fileNum;
  lastSndPadOnly = false;
  if (padAudioDest != 1) sendSndCmd(SND_CMD_PLAY, fileNum);
  if (padAudioDest >= 1) padSndPlay(fileNum);
}

static void stopToDest() {
  if (padAudioDest != 1) sendSndCmd(SND_CMD_STOP, 0);
  if (padAudioDest >= 1) padSndStop();
}

#define SND_CYCLE(idx, first, last, label, name)                          \
  do {                                                                      \
    playToDest(idx);                                                        \
    if (label) {                                                            \
      char _buf[24];                                                        \
      snprintf(_buf, sizeof(_buf), name "\n%d / %d",                       \
               (int)(idx - first + 1), (int)(last - first + 1));           \
      lv_label_set_text(label, _buf);                                       \
    }                                                                       \
    idx = (idx >= last) ? first : (idx + 1);                               \
  } while(0)

void SoundPower(lv_event_t *e)   { SND_CYCLE(sndPowerIdx,   SND_POWER_FIRST,   SND_POWER_LAST,   ui_SndPlay1Label, "POWER");   }
void SoundAlerts(lv_event_t *e)  { SND_CYCLE(sndAlertIdx,   SND_ALERT_FIRST,   SND_ALERT_LAST,   ui_SndPlay2Label, "ALERTS");  }
void SoundDamage(lv_event_t *e)  { SND_CYCLE(sndDamageIdx,  SND_DAMAGE_FIRST,  SND_DAMAGE_LAST,  ui_SndPlay3Label, "DAMAGE");  }
void SoundWarp(lv_event_t *e)    { SND_CYCLE(sndWarpIdx,    SND_WARP_FIRST,    SND_WARP_LAST,    ui_SndPlay4Label, "WARP");    }
void SoundWeapons(lv_event_t *e) { SND_CYCLE(sndWeaponsIdx, SND_WEAPONS_FIRST, SND_WEAPONS_LAST, ui_SndPlay5Label, "WEAPONS"); }

// DataPad-only sounds — call padSndPlay directly, never sent to Bridge
void SoundVoiced(lv_event_t *e) {
  lastSndFile = sndVoicedIdx;  lastSndPadOnly = true;
  padSndPlay(sndVoicedIdx);
  if (ui_SndPlay6Label) {
    char buf[24];
    snprintf(buf, sizeof(buf), "VOICED\n%d / %d",
             (int)(sndVoicedIdx - SND_VOICED_FIRST + 1),
             (int)(SND_VOICED_LAST - SND_VOICED_FIRST + 1));
    lv_label_set_text(ui_SndPlay6Label, buf);
  }
  sndVoicedIdx = (sndVoicedIdx >= SND_VOICED_LAST) ? SND_VOICED_FIRST : sndVoicedIdx + 1;
}

void SoundAmbient(lv_event_t *e) {
  lastSndFile = sndAmbientIdx;  lastSndPadOnly = true;
  padSndPlay(sndAmbientIdx);
  if (ui_SndPlay7Label) {
    char buf[24];
    snprintf(buf, sizeof(buf), "AMBIENT\n%d / %d",
             (int)(sndAmbientIdx - SND_AMBIENT_FIRST + 1),
             (int)(SND_AMBIENT_LAST - SND_AMBIENT_FIRST + 1));
    lv_label_set_text(ui_SndPlay7Label, buf);
  }
  sndAmbientIdx = (sndAmbientIdx >= SND_AMBIENT_LAST) ? SND_AMBIENT_FIRST : sndAmbientIdx + 1;
}

void SoundExtras(lv_event_t *e) {
  lastSndFile = sndEasterIdx;  lastSndPadOnly = true;
  padSndPlay(sndEasterIdx);
  if (ui_SndPlay8Label) {
    char buf[24];
    snprintf(buf, sizeof(buf), "EXTRAS\n%d / %d",
             (int)(sndEasterIdx - SND_EASTER_FIRST + 1),
             (int)(SND_EASTER_LAST - SND_EASTER_FIRST + 1));
    lv_label_set_text(ui_SndPlay8Label, buf);
  }
  sndEasterIdx = (sndEasterIdx >= SND_EASTER_LAST) ? SND_EASTER_FIRST : sndEasterIdx + 1;
}

void EasterEggFunny(lv_event_t * e) {
  padSndPlay(random(SND_EASTER_FIRST, 43));  // files 39–42
}

void EasterEggStarWars(lv_event_t * e) {
  padSndPlay(random(43, SND_EASTER_LAST + 1));  // files 43–44
}

void SoundPlayLast(lv_event_t *e) {
  if (lastSndFile == 0) return;
  if (lastSndPadOnly)
    padSndPlay(lastSndFile);
  else
    playToDest(lastSndFile);
}

void SoundStop(lv_event_t *e) {
  stopToDest();
}

void SoundVolume(lv_event_t *e) {
  int vol = (int)lv_slider_get_value(lv_event_get_target(e));
  if (padAudioDest != 1) {
    sendSndCmd(SND_CMD_VOL, vol);
    bridgeSndVol = (uint8_t)vol;
    if (ui_BridgeVolSlider) lv_slider_set_value(ui_BridgeVolSlider, vol, LV_ANIM_OFF);
  }
  if (padAudioDest >= 1) {
    padSndSetVol((uint8_t)vol);
    padSndVol = (uint8_t)vol;
    if (ui_PadVolSlider) lv_slider_set_value(ui_PadVolSlider, vol, LV_ANIM_OFF);
  }
}

void SoundRepeat(lv_event_t *e) {
  sndRepeat = (sndRepeat + 1) % 3;
  sendSndCmd(SND_CMD_REPEAT, sndRepeat);
  const char* labels[] = {"REPEAT OFF", "LOOP ONE", "LOOP ALL"};
  lv_label_set_text(ui_SndRepeatLabel, labels[sndRepeat]);
}

// ── Screen2 — DataPad audio controls ─────────────────────────────────────────

void PadAudioDest(lv_event_t *e) {
  padAudioDest = (padAudioDest + 1) % 3;
  const char* labels[] = {"BRIDGE", "PAD", "BOTH"};
  lv_label_set_text(lv_obj_get_child(ui_PadDestBtn, 0), labels[padAudioDest]);
}

void PadVolume(lv_event_t *e) {
  padSndVol = (uint8_t)lv_slider_get_value(lv_event_get_target(e));
  padSndSetVol(padSndVol);
}

void BridgeVolume(lv_event_t *e) {
  bridgeSndVol = (uint8_t)lv_slider_get_value(lv_event_get_target(e));
  sendSndCmd(SND_CMD_VOL, bridgeSndVol);
}


// ── StartWiFi — LVGL event handler (called from button press) ────────────────
// No blocking WiFi code here — just set flags and return immediately so
// the LVGL task is not stalled. Actual connect happens in loop().

void StartWiFi(lv_event_t *e) {
  String ssid_str = lv_textarea_get_text(ui_TextArea1);
  String pass_str = lv_textarea_get_text(ui_TextArea2);
  if (pass_str == "*****") {
    Preferences prefs; prefs.begin("datapad", true);
    pass_str = prefs.getString("wifi_pass", "ncc-1701-d");
    prefs.end();
  }

  strncpy(pending_ssid,     ssid_str.c_str(), 32); pending_ssid[32]     = '\0';
  strncpy(pending_password, pass_str.c_str(), 64); pending_password[64] = '\0';

  strncpy(myData.ssid1,     pending_ssid,     32); myData.ssid1[32]     = '\0';
  strncpy(myData.password1, pending_password, 64); myData.password1[64] = '\0';
  myData.startWifi = 1;   // connect only (no save)
  esp_now_send(0, (uint8_t *)&myData, sizeof(myData));
  myData.startWifi = 0;

  lv_label_set_text(ui_Label15, "Connecting...");
  do_wifi_connect = true;
}

static void revertWifiLocal() {
  myData.startWifi = 3;
  esp_now_send(0, (uint8_t *)&myData, sizeof(myData));
  myData.startWifi = 0;
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  wifi_connecting = false;
  do_wifi_connect = false;
  wifiSaved = false;
  servicesStarted = false;
  Preferences prefs;
  prefs.begin("datapad", false);
  prefs.putBool("wifi_auto", false);
  prefs.end();
  Serial.println("WiFi reverted. ESP-NOW ch=1");
}

// ── SaveWifi — toggle: press to save+arm, press again to clear ───────────────

void populateWifiFields() {
  if (!ui_TextArea1 || !ui_TextArea2) return;
  Preferences prefs;
  prefs.begin("datapad", true);
  String s = prefs.getString("wifi_ssid", "");
  prefs.end();
  if (s.length() > 0) {
    lv_textarea_set_text(ui_TextArea1, s.c_str());
    lv_textarea_set_text(ui_TextArea2, "*****");
  }
}

void SaveWifi(lv_event_t *e) {
  if (!wifiSaved) {
    // ── ARM: save credentials, go green ─────────────────────────────────────
    String ssid_str = lv_textarea_get_text(ui_TextArea1);
    String pass_str = lv_textarea_get_text(ui_TextArea2);
    if (pass_str == "*****") {
      Preferences prefs;
      prefs.begin("datapad", true);
      pass_str = prefs.getString("wifi_pass", "ncc-1701-d");
      prefs.end();
    }

    if (ssid_str.length() == 0) {
      lv_label_set_text(ui_Label15, "Enter SSID first");
      return;
    }

    Preferences prefs;
    prefs.begin("datapad", false);
    prefs.putString("wifi_ssid", ssid_str);
    prefs.putString("wifi_pass", pass_str);
    prefs.putBool("wifi_auto",   true);
    prefs.end();

    strncpy(pending_ssid,     ssid_str.c_str(), 32); pending_ssid[32]     = '\0';
    strncpy(pending_password, pass_str.c_str(), 64); pending_password[64] = '\0';
    strncpy(myData.ssid1,     pending_ssid,     32); myData.ssid1[32]     = '\0';
    strncpy(myData.password1, pending_password, 64); myData.password1[64] = '\0';

    myData.startWifi = 2;   // connect + save on Bridge and EngRoom
    esp_now_send(broadcastAll, (uint8_t *)&myData, sizeof(myData));
    delay(500);
    esp_now_send(0, (uint8_t *)&myData, sizeof(myData));
    myData.startWifi = 0;

    delay(500);

    wifiSaved       = true;
    do_wifi_connect = true;

    lv_obj_clear_state(ui_Button50, LV_STATE_PRESSED);
    lv_obj_add_state(ui_Button50, LV_STATE_CHECKED);
    lv_label_set_text(lv_obj_get_child(ui_Button50, 0), "WIFI SAVED");
    lv_label_set_text(ui_Label15, "Saved & Connecting...");
    Serial.printf("WiFi credentials saved: %s\n", pending_ssid);

  } else {
    // ── CLEAR: wipe NVS flag, revert all units to station mode ───────────────
    revertWifiLocal();
    lv_obj_clear_state(ui_Button50, LV_STATE_CHECKED);
    lv_obj_add_state(ui_Button50,   LV_STATE_DEFAULT);
    lv_label_set_text(lv_obj_get_child(ui_Button50, 0), "SAVE WIFI");
    lv_label_set_text(ui_Label15, "WiFi Cleared");
  }
}
