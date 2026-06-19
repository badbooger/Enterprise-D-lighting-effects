#include <WiFi.h>
#include <esp_wifi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ArduinoOTA.h>

const char* AP_SSID = "enterprise";
const char* AP_PASS = "ncc-1701-d";

DNSServer   dnsServer;
WebServer   webServer(80);
String      macAddress;

void handleRoot() {
  String html = R"rawhtml(
<!DOCTYPE html>
<html>
<head>
  <meta name='viewport' content='width=device-width, initial-scale=1'>
  <title>ESP32 MAC Address</title>
  <style>
    body { font-family: monospace; background: #1a1a2e; color: #e0e0e0;
           display: flex; justify-content: center; align-items: center;
           min-height: 100vh; margin: 0; }
    .card { background: #16213e; border: 2px solid #e94560; border-radius: 12px;
            padding: 40px; text-align: center; max-width: 400px; }
    h1 { color: #e94560; margin-top: 0; font-size: 1.4em; }
    .mac { font-size: 2em; color: #0f3460; background: #e0e0e0;
           padding: 12px 20px; border-radius: 8px; margin: 20px 0;
           letter-spacing: 2px; user-select: all; }
    .info { color: #888; font-size: 0.85em; }
  </style>
</head>
<body>
  <div class='card'>
    <h1>ESP32 MAC Address</h1>
    <div class='mac'>)rawhtml" + macAddress + R"rawhtml(</div>
    <p class='info'>Tap the address to select it for copying.<br>
    OTA hostname: web_mac_address</p>
  </div>
</body>
</html>
)rawhtml";
  webServer.send(200, "text/html", html);
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
  webServer.on("/generate_204", handleRedirect);
  webServer.on("/hotspot-detect.html", handleRedirect);
  webServer.on("/connecttest.txt", handleRedirect);
  webServer.on("/redirect", handleRedirect);
  webServer.on("/success.txt", handleRedirect);
  webServer.on("/ncsi.txt", handleRedirect);
  webServer.onNotFound(handleRedirect);
  webServer.begin();

  ArduinoOTA.setHostname("web_mac_address");
  ArduinoOTA.begin();
}

void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();
  ArduinoOTA.handle();
}
