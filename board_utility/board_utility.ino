#include <WiFi.h>
#include <esp_wifi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ArduinoOTA.h>
#include <Preferences.h>

const char* AP_SSID = "enterprise";
const char* AP_PASS = "ncc-1701-d";

const char* namespaces[] = { "bridge", "engroom", "datapad", "warpcore" };
const char* labels[]     = { "Bridge", "EngRoom", "DataPad", "WarpCore" };
const int   NS_COUNT     = 4;

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

void handleRoot() {
  String html = R"rawhtml(
<!DOCTYPE html>
<html>
<head>
  <meta name='viewport' content='width=device-width, initial-scale=1'>
  <title>ESP32 Utility</title>
  <style>
    body { font-family: monospace; background: #1a1a2e; color: #e0e0e0;
           display: flex; justify-content: center; align-items: center;
           min-height: 100vh; margin: 0; padding: 20px; box-sizing: border-box; }
    .card { background: #16213e; border: 2px solid #e94560; border-radius: 12px;
            padding: 40px; text-align: center; max-width: 420px; width: 100%; }
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
  Serial.printf("AP IP: %s\n", WiFi.softAPIP().toString().c_str());

  dnsServer.start(53, "*", WiFi.softAPIP());

  webServer.on("/", handleRoot);
  webServer.on("/clear", handleClear);
  webServer.on("/generate_204", handleRedirect);
  webServer.on("/hotspot-detect.html", handleRedirect);
  webServer.on("/connecttest.txt", handleRedirect);
  webServer.on("/redirect", handleRedirect);
  webServer.on("/success.txt", handleRedirect);
  webServer.on("/ncsi.txt", handleRedirect);
  webServer.onNotFound(handleRedirect);
  webServer.begin();

  ArduinoOTA.setHostname("board_utility");
  ArduinoOTA.begin();
}

void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();
  ArduinoOTA.handle();
}
