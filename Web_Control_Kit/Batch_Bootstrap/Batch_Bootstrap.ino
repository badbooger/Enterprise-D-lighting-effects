// Batch_Bootstrap — bench-only bootstrap firmware for batch-provisioning a run of
// Bridge (ESP32-S3) / EngRoom (Xiao ESP32-C3) boards. Flashed via USB onto bare dev
// boards, never onto a real production PCB, never shipped to a customer.
//
// Chip-mismatch safety check + board-role config, joining a dedicated bench WiFi
// network, ESP-NOW auto-adopt pairing (learned peer MAC written into NVS using the
// same namespace/key schema Bridge_ESP_Webpage/Engine_Room_ESP_Webpage read at
// boot), an HTTPUpdate OTA pull of the real firmware once paired, and progress
// reports to the bench dashboard (OTA_Server/bin_server.py).
//
// See Batch_Bootstrap/README.md for per-pass setup steps.

#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <HTTPUpdate.h>
#include <HTTPClient.h>
#include <WebServer.h>

// ============================================================================
// PER-BATCH-PASS CONFIG — EDIT THESE BEFORE EACH FLASHING PASS
// Same build every time within one pass; swap these, recompile, flash next pass.
// ============================================================================

// ESP.getChipModel() (Arduino-ESP32 core, checked at runtime in setup()) returns
// strings such as "ESP32-S3", "ESP32-C3". Set this to match whichever dev board
// this pass is flashing.
#define EXPECTED_CHIP   "ESP32-C3"     // "ESP32-S3" for a Bridge pass, "ESP32-C3" for an EngRoom pass
#define ROLE_BRIDGE     1
#define ROLE_ENGROOM    2
#define BOARD_ROLE      ROLE_ENGROOM    // set to ROLE_BRIDGE or ROLE_ENGROOM before flashing this pass

// Dedicated bootstrap-only WiFi — a single pinned AP, NOT board_utility.ino's
// buyer-facing "enterprise"/"ncc-1701-d" network. This firmware never ships, so
// these can be real bench credentials, just never the customer-facing ones.
// Must match Pi_Bench_Tools/bootstrap-ap/bootstrap-ap.env exactly.
#define BOOTSTRAP_WIFI_SSID   "EDIT_BEFORE_FLASHING_SSID"
#define BOOTSTRAP_WIFI_PASS   "EDIT_BEFORE_FLASHING_PASS"

// Phase 5 — where the compiled final-firmware .bin files are served from.
// The Pi's static address on the bench AP (Pi_Bench_Tools/bootstrap-ap); bin_server.py runs there as bin-server.service.
#define OTA_SERVER_HOST       "http://192.168.50.1:8000"
#define OTA_TRIGGER_DELAY_MS  5000UL   // wait this long after pairing confirmed before pulling
#define OTA_RETRY_PAUSE_MS    3000UL   // pause between the 2 pull attempts

// --- Status LED (SECONDARY indicator only — Serial output above is authoritative) ---
// ESP32-S3 dev board: confirmed on-board addressable NeoPixel (WS2812) on GPIO 48 —
// driven via the Arduino-ESP32 core's built-in neopixelWrite(), not digitalWrite()
// (a plain HIGH/LOW toggle doesn't drive a WS2812's single-wire protocol correctly).
// No new library needed — neopixelWrite() ships in the core already used elsewhere
// in this project.
//
// Xiao ESP32-C3 dev board: NOT YET CONFIRMED. Leave STATUS_LED_PIN at -1 for a C3
// pass until checked against real hardware/schematic — check the GPIO9 strapping-
// pin/boot-loop history (GPIO9 on the C3) before picking a C3 pin. If the C3's onboard
// LED turns out to be a plain GPIO LED (not a NeoPixel), set STATUS_LED_NEOPIXEL to
// 0 for that pass so blinkPattern()/setLedSolid() fall back to plain digitalWrite().
#define STATUS_LED_PIN         -1      // GPIO48 = S3 dev board built-in NeoPixel; -1 disables the LED entirely
#define STATUS_LED_NEOPIXEL    1       // 1 = STATUS_LED_PIN is a WS2812 NeoPixel, 0 = plain GPIO LED

// ============================================================================
// Timing constants
// ============================================================================
#define WIFI_CONNECT_TIMEOUT_MS   15000UL   // per-attempt timeout before giving up and retrying
#define WIFI_RETRY_PAUSE_MS        3000UL   // pause between retry attempts
#define HALT_REPRINT_MS            2000UL   // how often a halt loop re-prints its message
#define HEARTBEAT_MS              10000UL   // loop() status line interval
#define PAIR_BROADCAST_MS          2000UL   // how often to broadcast our own role while pairing

uint32_t lastHeartbeatMs = 0;

void reportStatus(const char *stage, const String &detail = "");

// ============================================================================
// Phase 4 — ESP-NOW auto-adopt pairing. Both roles always broadcast their own
// role and always process incoming broadcasts (never gated on already having a
// peer) — a peer that gets CLEARMAC'd or swapped is picked back up automatically
// without needing to also clear the other side. NVS is only written when the
// received MAC actually differs from what's already stored.
// ============================================================================

typedef struct {
  uint8_t role;   // ROLE_BRIDGE or ROLE_ENGROOM — the sender's MAC comes free
                  // from OnDataRecv's own mac parameter, no need to include it.
} bootstrap_pair_msg;

uint8_t  broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
bool     hasPeerMac = false;
uint8_t  peerMac[6];

volatile bool    pendingAdopt = false;
volatile uint8_t pendingMac[6];

uint32_t lastBroadcastMs = 0;

// ============================================================================
// Phase 5 — OTA pull. Fires automatically once hasPeerMac is true (a fresh
// Phase 4 adoption or an already-known NVS peer at boot), after a short settle
// delay. One attempt, one retry; on a second failure the board keeps running
// normally (not a halt) — Serial + a distinct LED blink + the status webpage
// all reflect the outcome.
// ============================================================================

uint32_t pairedAtMs = 0;
bool     otaAttempted = false;
String   lastOtaResult = "not attempted yet";

WebServer webServer(80);

// ============================================================================
// Status LED helpers — no-op whenever STATUS_LED_PIN < 0
// ============================================================================

// Dim red — enough to see, not blinding for a status indicator sitting a few
// inches from the operator's eyes on a bench.
#define STATUS_LED_BRIGHTNESS   40

void ledSet(bool on) {
  if (STATUS_LED_PIN < 0) return;
  if (STATUS_LED_NEOPIXEL) {
    neopixelWrite(STATUS_LED_PIN, on ? STATUS_LED_BRIGHTNESS : 0, 0, 0);
  } else {
    digitalWrite(STATUS_LED_PIN, on ? HIGH : LOW);
  }
}

void blinkPattern(int times, int onMs, int offMs) {
  if (STATUS_LED_PIN < 0) return;
  for (int i = 0; i < times; i++) {
    ledSet(true);  delay(onMs);
    ledSet(false); delay(offMs);
  }
}

void setLedSolid(bool on) {
  ledSet(on);
}

// ============================================================================
// Boot banner / debug helpers
// ============================================================================

const char* boardRoleName() {
  if (BOARD_ROLE == ROLE_BRIDGE)  return "Bridge";
  if (BOARD_ROLE == ROLE_ENGROOM) return "EngRoom";
  return "INVALID";
}

String getStaMac() {
  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_STA, mac);
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

void printBanner() {
  Serial.println("=== Batch_Bootstrap ===");
  Serial.printf("Config: EXPECTED_CHIP=%s  BOARD_ROLE=%s\n", EXPECTED_CHIP, boardRoleName());
  Serial.printf("Detected chip (ESP.getChipModel()): %s\n", ESP.getChipModel());
  Serial.printf("STA MAC: %s\n", getStaMac().c_str());
}

String macToString(const uint8_t *mac) {
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

// ============================================================================
// Phase 4 helpers — NVS namespace/key match Phase 1's schema exactly, own-role
// based, so the real firmware (Bridge_ESP.ino/Engine_Room_ESP.ino) picks up
// whatever this bootstrap firmware learns without any further change.
// ============================================================================

const char* peerNvsNamespace() {
  return (BOARD_ROLE == ROLE_BRIDGE) ? "bridge" : "engroom";
}

const char* peerMacKey() {
  return (BOARD_ROLE == ROLE_BRIDGE) ? "mac_engroom" : "mac_bridge";
}

const char* peerRoleName() {
  return (BOARD_ROLE == ROLE_BRIDGE) ? "EngRoom" : "Bridge";
}

void loadPeerMacFromNvs() {
  Preferences prefs;
  prefs.begin(peerNvsNamespace(), true);
  if (prefs.getBytesLength(peerMacKey()) == 6) {
    prefs.getBytes(peerMacKey(), peerMac, 6);
    hasPeerMac = true;
  }
  prefs.end();
}

void printPairingStatus() {
  if (hasPeerMac) {
    Serial.printf("Peer (%s) MAC already known: %s\n", peerRoleName(), macToString(peerMac).c_str());
  } else {
    Serial.println("Not yet paired — will listen/broadcast for peer.");
  }
}

// Writes the peer MAC to NVS, then reads it straight back and compares before
// declaring success — Phase 4's own test requirement is "not trusting it blind."
void savePeerMac(const uint8_t *mac) {
  Preferences prefs;
  prefs.begin(peerNvsNamespace(), false);
  prefs.putBytes(peerMacKey(), mac, 6);
  prefs.end();

  uint8_t verify[6];
  prefs.begin(peerNvsNamespace(), true);
  bool ok = (prefs.getBytesLength(peerMacKey()) == 6);
  if (ok) prefs.getBytes(peerMacKey(), verify, 6);
  prefs.end();

  if (ok && memcmp(verify, mac, 6) == 0) {
    memcpy(peerMac, mac, 6);
    hasPeerMac = true;
    Serial.printf("[adopt] PAIRED with %s MAC: %s — saved to NVS (namespace=\"%s\" key=\"%s\"), read-back verified.\n",
                  peerRoleName(), macToString(mac).c_str(), peerNvsNamespace(), peerMacKey());
    reportStatus("paired");
  } else {
    Serial.println("[adopt] WARNING: NVS read-back did not match what was written!");
  }
}

// NOTE: this project's other sketches use a legacy `(const uint8_t *mac, ...)`
// OnDataRecv signature cast to esp_now_recv_cb_t. On the Arduino-ESP32 core
// installed here (3.3.11), the real typedef is
// `(const esp_now_recv_info_t *info, const uint8_t *data, int len)` — the first
// field of esp_now_recv_info_t is itself a pointer (src_addr), so that cast
// silently reads the struct's raw bytes instead of the real sender MAC. Nothing
// else in this project relies on that parameter's correctness (peers are
// identified by payload content instead), but Phase 4 does — so this callback
// uses the real signature and pulls the MAC from info->src_addr.
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  if (len != sizeof(bootstrap_pair_msg)) return;
  bootstrap_pair_msg msg;
  memcpy(&msg, incomingData, sizeof(msg));

  const uint8_t *mac = info->src_addr;
  if (msg.role == BOARD_ROLE) return;              // our own broadcast looped back
  if (hasPeerMac && memcmp(mac, peerMac, 6) == 0) return;   // already know this exact peer

  memcpy((void*)pendingMac, mac, 6);
  pendingAdopt = true;
}

void initEspNow() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("[adopt] ESP-NOW init failed!");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastMac, 6);
  peerInfo.channel = 0;   // already STA-associated to the bench AP — use its channel
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
}

void handleSerialCommand() {
  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line == "CLEARMAC") {
    Preferences prefs;
    prefs.begin(peerNvsNamespace(), false);
    prefs.remove(peerMacKey());
    prefs.end();
    hasPeerMac = false;
    Serial.println("[CLEARMAC] cleared — will re-adopt on next broadcast from peer.");
  }
}

// ============================================================================
// Progress reports to the Pi's dashboard (bin_server.py /report). Best-effort:
// a failed report never blocks pairing or the OTA pull.
// ============================================================================

String urlEncode(const String &in) {
  String out;
  for (size_t i = 0; i < in.length(); i++) {
    char c = in[i];
    if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == ':') out += c;
    else { char buf[4]; snprintf(buf, sizeof(buf), "%%%02X", (uint8_t)c); out += buf; }
  }
  return out;
}

void reportStatus(const char *stage, const String &detail) {
  String url = String(OTA_SERVER_HOST) + "/report?mac=" + getStaMac() +
               "&role=" + boardRoleName() + "&stage=" + stage +
               "&peer=" + (hasPeerMac ? macToString(peerMac) : String("")) +
               "&detail=" + urlEncode(detail);
  HTTPClient http;
  http.setTimeout(2000);
  http.begin(url);
  int code = http.GET();
  http.end();
  Serial.printf("[report] %s -> %d\n", stage, code);
}

// ============================================================================
// Phase 5 — OTA pull helpers
// ============================================================================

const char* otaBinFilename() {
  return (BOARD_ROLE == ROLE_BRIDGE) ? "Bridge_ESP.bin" : "Engine_Room_ESP.bin";
}

// One HTTPUpdate attempt. Returns true on success (device reboots on its own
// before this would ever return in that case — httpUpdate.update() only
// returns here on a non-OK outcome in practice, but check the return value
// anyway rather than assuming).
bool doOtaAttempt() {
  String url = String(OTA_SERVER_HOST) + "/" + otaBinFilename();
  Serial.printf("[OTA] Pulling %s ...\n", url.c_str());
  WiFiClient client;
  // Reboot ourselves after reporting, so the dashboard hears about the success.
  httpUpdate.rebootOnUpdate(false);
  t_httpUpdate_return ret = httpUpdate.update(client, url);
  if (ret == HTTP_UPDATE_OK) {
    lastOtaResult = "SUCCESS";
    Serial.println("[OTA] Success — rebooting into new firmware.");
    reportStatus("flashed", otaBinFilename());
    delay(200);
    ESP.restart();
    return true;
  }
  lastOtaResult = "FAILED: " + String(httpUpdate.getLastErrorString().c_str());
  Serial.printf("[OTA] attempt failed (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
  return false;
}

void attemptOtaPull() {
  if (doOtaAttempt()) return;
  reportStatus("flash_retry", lastOtaResult);
  lastOtaResult += " — retrying";
  Serial.println("[OTA] Retrying once...");
  delay(OTA_RETRY_PAUSE_MS);
  if (doOtaAttempt()) return;
  lastOtaResult = "FAILED (both attempts): " + lastOtaResult;
  Serial.println("[OTA] Both attempts failed — continuing to run Batch_Bootstrap normally.");
  reportStatus("flash_failed", lastOtaResult);
  blinkPattern(4, 150, 150);   // distinct from the 5-blink chip-mismatch / 3-blink invalid-role
                               // halts; one-shot only, not a halt — board keeps running afterward
}

// ============================================================================
// Phase 5 — status/retry webpage. Plain HTML, no theming — same "oh-shit debug
// page" precedent as the EngRoom fallback-AP page (no LCARS styling on
// bench-only emergency/debug pages, just buttons and text).
// ============================================================================

void handleStatus() {
  String html = "<html><body><h3>Batch_Bootstrap: " + String(boardRoleName()) + "</h3>";
  html += "<p>Chip: " + String(EXPECTED_CHIP) + "<br>";
  html += "IP: " + WiFi.localIP().toString() + "<br>";
  html += "Peer: " + (hasPeerMac ? macToString(peerMac) : String("unpaired")) + "<br>";
  html += "Last OTA result: " + lastOtaResult + "</p>";
  html += "<form action='/otaretry'><button type='submit'>Retry OTA Pull</button></form>";
  html += "</body></html>";
  webServer.send(200, "text/html", html);
}

void handleOtaRetry() {
  webServer.send(200, "text/plain", "Retrying OTA pull...");
  attemptOtaPull();
}

// ============================================================================
// Halt paths — both are infinite loops that never return. A mismatch here means
// the wrong physical board is plugged in, or the per-pass config was fumbled;
// only a human fixing it and power-cycling should retry, not a software retry.
// ============================================================================

void haltChipMismatch(String actual) {
  while (true) {
    Serial.println("================================================");
    Serial.println("BOOTSTRAP HALT: CHIP MISMATCH");
    Serial.printf("  Expected chip (EXPECTED_CHIP): %s\n", EXPECTED_CHIP);
    Serial.printf("  Detected chip (ESP.getChipModel()): %s\n", actual.c_str());
    Serial.println("  Wrong physical dev board is plugged into this port.");
    Serial.println("  Swap in the correct board, then power-cycle to retry.");
    Serial.println("================================================");
    blinkPattern(5, 100, 100);   // 5 fast blinks — distinguishes this from the role halt
    delay(HALT_REPRINT_MS);
  }
}

void haltInvalidRoleConfig() {
  while (true) {
    Serial.println("================================================");
    Serial.println("BOOTSTRAP HALT: INVALID BOARD_ROLE CONFIG");
    Serial.printf("  BOARD_ROLE is set to %d — must be ROLE_BRIDGE (%d) or ROLE_ENGROOM (%d).\n",
                  BOARD_ROLE, ROLE_BRIDGE, ROLE_ENGROOM);
    Serial.println("  Fix the config block and reflash.");
    Serial.println("================================================");
    blinkPattern(3, 100, 100);   // 3 fast blinks — distinguishes this from the chip halt
    delay(HALT_REPRINT_MS);
  }
}

void checkBoardRoleConfig() {
  if (BOARD_ROLE != ROLE_BRIDGE && BOARD_ROLE != ROLE_ENGROOM) {
    haltInvalidRoleConfig();   // never returns
  }
}

void checkChipMatch() {
  String actual = ESP.getChipModel();
  if (actual != EXPECTED_CHIP) {
    haltChipMismatch(actual);   // never returns
  }
}

// ============================================================================
// Dedicated bootstrap WiFi join — retries forever on failure, never halts.
// A WiFi hiccup (AP still booting, bench router blip) is very likely transient,
// unlike a chip mismatch which needs a human to physically swap hardware.
// ============================================================================

void connectBootstrapWifi() {
  WiFi.mode(WIFI_STA);
  Serial.printf("Joining bootstrap WiFi \"%s\" ...\n", BOOTSTRAP_WIFI_SSID);

  while (true) {
    WiFi.begin(BOOTSTRAP_WIFI_SSID, BOOTSTRAP_WIFI_PASS);
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
      blinkPattern(1, 500, 500);   // slow "still trying" blink while waiting
      delay(250);
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("WiFi joined.");
      Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
      setLedSolid(true);
      return;
    }
    Serial.println("WiFi join attempt timed out — retrying...");
    delay(WIFI_RETRY_PAUSE_MS);
  }
}

void startOtaAndMdns() {
  String hostname = (BOARD_ROLE == ROLE_BRIDGE) ? "bootstrap-bridge" : "bootstrap-engroom";
  MDNS.begin(hostname.c_str());
  ArduinoOTA.setHostname(hostname.c_str());
  ArduinoOTA.begin();
  Serial.printf("OTA hostname: %s.local\n", hostname.c_str());
}

// ============================================================================

void setup() {
  Serial.begin(115200);
  delay(1000);   // let native-USB CDC (S3) enumerate before first prints; harmless on C3's UART

  // NeoPixel is driven via neopixelWrite() (RMT peripheral internally) — no
  // pinMode() needed or wanted. A plain GPIO LED still needs it.
  if (STATUS_LED_PIN >= 0 && !STATUS_LED_NEOPIXEL) {
    pinMode(STATUS_LED_PIN, OUTPUT);
  }

  printBanner();
  checkBoardRoleConfig();   // halts if BOARD_ROLE isn't ROLE_BRIDGE/ROLE_ENGROOM
  checkChipMatch();         // halts if ESP.getChipModel() != EXPECTED_CHIP
  loadPeerMacFromNvs();     // Phase 4 — check for an already-adopted peer before printing
  printPairingStatus();
  connectBootstrapWifi();   // blocks until connected; retries forever, never halts
  initEspNow();             // Phase 4 — needs WiFi already STA-associated
  reportStatus("joined", ESP.getChipModel());
  if (hasPeerMac) reportStatus("paired", "from NVS");
  startOtaAndMdns();

  webServer.on("/", handleStatus);
  webServer.on("/otaretry", handleOtaRetry);
  webServer.begin();
}

void loop() {
  ArduinoOTA.handle();
  webServer.handleClient();
  if (Serial.available()) handleSerialCommand();

  if (millis() - lastBroadcastMs >= PAIR_BROADCAST_MS) {
    lastBroadcastMs = millis();
    bootstrap_pair_msg msg = { (uint8_t)BOARD_ROLE };
    esp_now_send(broadcastMac, (uint8_t*)&msg, sizeof(msg));
  }

  if (pendingAdopt) {
    pendingAdopt = false;
    savePeerMac((uint8_t*)pendingMac);
  }

  if (hasPeerMac) {
    if (pairedAtMs == 0) pairedAtMs = millis();
    if (!otaAttempted && millis() - pairedAtMs >= OTA_TRIGGER_DELAY_MS) {
      otaAttempted = true;
      attemptOtaPull();
    }
  }

  if (millis() - lastHeartbeatMs >= HEARTBEAT_MS) {
    lastHeartbeatMs = millis();
    Serial.printf("[heartbeat] role=%s chip=%s ip=%s peer=%s ota=%s uptime=%lus\n",
                  boardRoleName(), EXPECTED_CHIP,
                  WiFi.localIP().toString().c_str(),
                  hasPeerMac ? macToString(peerMac).c_str() : "unpaired",
                  lastOtaResult.c_str(),
                  millis() / 1000);
  }
}
