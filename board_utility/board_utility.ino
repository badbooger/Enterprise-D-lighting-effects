#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <Preferences.h>

const char* AP_SSID = "enterprise";
const char* AP_PASS = "ncc-1701-d";

const char* namespaces[] = { "bridge", "engroom", "datapad", "warpcore" };
const char* labels[]     = { "Bridge", "EngRoom", "DataPad", "WarpCore" };
const int   NS_COUNT     = 4;

#define LED_ON             1
#define LED_ASSEMBLY_MODE  27
#define LED_ALL_OFF        99
#define GRP_ALL           -1

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

uint8_t broadcastAddr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
esp_now_peer_info_t peerInfo;

DNSServer   dnsServer;
WebServer   webServer(80);
String      macAddress;
String      statusMsg = "";

void clearNamespace(const char* ns) {
  Preferences prefs;
  prefs.begin(ns, false);
  prefs.clear();
  prefs.end();
}

bool parseMAC(const String &str, uint8_t* out) {
  return sscanf(str.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                &out[0], &out[1], &out[2], &out[3], &out[4], &out[5]) == 6;
}

void sendCmd(const uint8_t* peer, int cmd, int group, int value) {
  struct_message msg;
  memset(&msg, 0, sizeof(msg));
  msg.boardInd = 0;
  msg.ledCmd   = cmd;
  msg.ledGroup = group;
  msg.ledValue = value;
  esp_now_send(peer, (uint8_t *)&msg, sizeof(msg));
}

bool sendToTarget(const String &macStr, int cmd, int group, int value) {
  if (macStr.length() == 0) {
    sendCmd(broadcastAddr, cmd, group, value);
    return true;
  }
  uint8_t target[6];
  if (!parseMAC(macStr, target)) return false;
  memcpy(peerInfo.peer_addr, target, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
  sendCmd(target, cmd, group, value);
  esp_now_del_peer(target);
  return true;
}

void handleRoot() {
  String html = R"rawhtml(
<!DOCTYPE html>
<html>
<head>
  <meta name='viewport' content='width=device-width, initial-scale=1'>
  <title>ESP32 Utility</title>
  <style>
    body { font-family: monospace; background: #1a1a2e; color: #e0e0e0;
           display: flex; flex-direction: column; align-items: center;
           min-height: 100vh; margin: 0; padding: 20px; box-sizing: border-box; }
    .card { background: #16213e; border: 2px solid #e94560; border-radius: 12px;
            padding: 40px; text-align: center; max-width: 420px; width: 100%;
            margin-bottom: 20px; }
    h1 { color: #e94560; margin-top: 0; font-size: 1.4em; }
    h2 { color: #e94560; font-size: 1.1em; margin-top: 30px; }
    .mac { font-size: 2em; color: #0f3460; background: #e0e0e0;
           padding: 12px 20px; border-radius: 8px; margin: 20px 0;
           letter-spacing: 2px; user-select: all; }
    .info { color: #888; font-size: 0.85em; }
    .btn-row { display: flex; flex-wrap: wrap; gap: 8px; justify-content: center;
               margin: 10px 0; }
    .btn { background: #0f3460; color: #e0e0e0; border: 1px solid #e94560;
           border-radius: 6px; padding: 10px 16px; font-family: monospace;
           font-size: 0.95em; cursor: pointer; text-decoration: none; }
    .btn:hover { background: #e94560; }
    .btn-all { background: #4a1942; }
    .btn-all:hover { background: #e94560; }
    .status { color: #4ecca3; margin: 15px 0; font-size: 1em; min-height: 1.2em; }
    input[type=text] { background: #0f3460; color: #e0e0e0; border: 1px solid #e94560;
                       border-radius: 6px; padding: 8px 12px; font-family: monospace;
                       font-size: 0.95em; width: 100%; box-sizing: border-box;
                       text-align: center; letter-spacing: 1px; }
    input[type=text]::placeholder { color: #666; }
  </style>
</head>
<body>
  <div class='card'>
    <h1>ESP32 MAC Address</h1>
    <div class='mac'>)rawhtml" + macAddress + R"rawhtml(</div>
    <p class='info'>Tap the address to select it for copying.<br>
    OTA hostname: board_utility</p>

    <h2>Clear NVS Storage</h2>
    <p class='info'>Wipes saved WiFi credentials and settings for the selected unit.</p>
    <div class='status'>)rawhtml" + statusMsg + R"rawhtml(</div>
    <div class='btn-row'>
      <a class='btn' href='/clear?ns=bridge'>Bridge</a>
      <a class='btn' href='/clear?ns=engroom'>EngRoom</a>
      <a class='btn' href='/clear?ns=datapad'>DataPad</a>
      <a class='btn' href='/clear?ns=warpcore'>WarpCore</a>
    </div>
    <div class='btn-row'>
      <a class='btn btn-all' href='/clear?ns=all'>Clear All</a>
    </div>
  </div>

  <div class='card'>
    <h1>PCB Test</h1>
    <p class='info'>Send ESP-NOW commands to boards on channel 1.<br>
    Leave target empty to broadcast to all boards.</p>
    <input type='text' id='targetMac' placeholder='AA:BB:CC:DD:EE:FF'>
    <div class='status' id='cmdStatus'></div>
    <div class='btn-row'>
      <button class='btn' onclick="sendCmd('asm_on')">Asm ON</button>
      <button class='btn' onclick="sendCmd('asm_off')">Asm OFF</button>
    </div>
    <div class='btn-row'>
      <button class='btn' onclick="sendCmd('all_on')">All On</button>
      <button class='btn' onclick="sendCmd('all_off')">All Off</button>
    </div>
  </div>

  <script>
  function sendCmd(action) {
    var mac = document.getElementById('targetMac').value.trim();
    var url = '/cmd?action=' + action;
    if (mac) url += '&mac=' + encodeURIComponent(mac);
    fetch(url, {method:'POST'}).then(function(r){return r.json();}).then(function(d){
      document.getElementById('cmdStatus').textContent = d.msg;
    });
  }
  </script>
</body>
</html>
)rawhtml";
  webServer.send(200, "text/html", html);
}

void handleClear() {
  String ns = webServer.arg("ns");

  if (ns == "all") {
    for (int i = 0; i < NS_COUNT; i++) clearNamespace(namespaces[i]);
    statusMsg = "All NVS namespaces cleared.";
    Serial.println("NVS cleared: all namespaces");
  } else {
    bool found = false;
    for (int i = 0; i < NS_COUNT; i++) {
      if (ns == namespaces[i]) {
        clearNamespace(namespaces[i]);
        statusMsg = String(labels[i]) + " NVS cleared.";
        Serial.printf("NVS cleared: %s\n", namespaces[i]);
        found = true;
        break;
      }
    }
    if (!found) {
      statusMsg = "Unknown namespace.";
    }
  }

  webServer.sendHeader("Location", "/");
  webServer.send(302, "text/plain", "");
}

void handleCmd() {
  String action = webServer.arg("action");
  String mac    = webServer.arg("mac");

  int cmd = 0, group = GRP_ALL, value = 0;
  String label;

  if (action == "asm_on")       { cmd = LED_ASSEMBLY_MODE; value = 1; label = "Asm ON"; }
  else if (action == "asm_off") { cmd = LED_ASSEMBLY_MODE; value = 0; label = "Asm OFF"; }
  else if (action == "all_on")  { cmd = LED_ON;            value = 255; label = "All On"; }
  else if (action == "all_off") { cmd = LED_ALL_OFF;       value = 0; label = "All Off"; }
  else {
    webServer.send(200, "application/json", "{\"ok\":false,\"msg\":\"Unknown action\"}");
    return;
  }

  if (!sendToTarget(mac, cmd, group, value)) {
    webServer.send(200, "application/json", "{\"ok\":false,\"msg\":\"Bad MAC format\"}");
    return;
  }

  String target = (mac.length() > 0) ? mac : "broadcast";
  String msg = "Sent: " + label + " (" + target + ")";
  Serial.println(msg);
  webServer.send(200, "application/json", "{\"ok\":true,\"msg\":\"" + msg + "\"}");
}

void handleRedirect() {
  webServer.sendHeader("Location", "http://192.168.4.1/");
  webServer.send(302, "text/plain", "");
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASS);

  uint8_t baseMac[6];
  esp_wifi_get_mac(WIFI_IF_STA, baseMac);
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
           baseMac[0], baseMac[1], baseMac[2],
           baseMac[3], baseMac[4], baseMac[5]);
  macAddress = buf;

  Serial.printf("MAC Address: %s\n", buf);
  Serial.printf("AP SSID: %s\n", AP_SSID);
  Serial.printf("AP Pass: %s\n", AP_PASS);
  Serial.printf("AP IP:   http://%s/\n", WiFi.softAPIP().toString().c_str());
  Serial.printf("mDNS:    http://board-utility.local/\n");

  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  if (esp_now_init() == ESP_OK) {
    memcpy(peerInfo.peer_addr, broadcastAddr, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
    Serial.println("ESP-NOW ready");
  }

  dnsServer.start(53, "*", WiFi.softAPIP());

  webServer.on("/", handleRoot);
  webServer.on("/clear", handleClear);
  webServer.on("/cmd", HTTP_POST, handleCmd);
  webServer.on("/generate_204", handleRedirect);
  webServer.on("/hotspot-detect.html", handleRedirect);
  webServer.on("/connecttest.txt", handleRedirect);
  webServer.on("/redirect", handleRedirect);
  webServer.on("/success.txt", handleRedirect);
  webServer.on("/ncsi.txt", handleRedirect);
  webServer.onNotFound(handleRedirect);
  webServer.begin();

  MDNS.begin("board-utility");
  MDNS.addService("http", "tcp", 80);

  ArduinoOTA.setHostname("board_utility");
  ArduinoOTA.begin();
}

void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();
  ArduinoOTA.handle();
}
