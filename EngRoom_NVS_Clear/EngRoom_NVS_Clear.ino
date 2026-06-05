/*
  EngRoom_NVS_Clear
  Clears wifi_auto from EngRoom NVS, then connects to WiFi and waits for OTA
  so Engine_Room_ESP can be flashed back without USB access.

  HOW TO USE:
    1. Set WIFI_SSID and WIFI_PASS below.
    2. Flash this sketch to EngRoom (USB first time, OTA after).
    3. Watch Serial Monitor (115200) — wait for "Ready for OTA".
    4. In Arduino IDE select Tools > Port > EngRoom.local and upload Engine_Room_ESP.
*/

#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <Preferences.h>

const char* WIFI_SSID = "your_ssid";
const char* WIFI_PASS = "your_password";

void setup() {
  Serial.begin(115200);
  delay(500);

  Preferences prefs;
  prefs.begin("engroom", false);
  prefs.putBool("wifi_auto", false);
  prefs.remove("wifi_ssid");
  prefs.remove("wifi_pass");
  prefs.end();
  Serial.println("EngRoom NVS cleared.");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWiFi failed — check credentials and reboot.");
    return;
  }
  Serial.printf("\nConnected. IP: %s\n", WiFi.localIP().toString().c_str());

  if (MDNS.begin("EngRoom")) Serial.println("mDNS: EngRoom.local");

  ArduinoOTA.setHostname("EngRoom");
  ArduinoOTA.begin();
  Serial.println("Ready for OTA — upload Engine_Room_ESP now.");
}

void loop() {
  ArduinoOTA.handle();
}
