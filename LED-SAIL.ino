#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <ArduinoOTA.h>         // https://github.com/jandrassy/ArduinoOTA
#include "FS.h"
#include <ESP_EEPROM.h>         // https://github.com/jwrw/ESP_EEPROM

#include "MSGEQ7.h"              // https://github.com/NicoHood/MSGEQ7
#define pinAnalog A0
#define pinReset 6
#define pinStrobe 4
#define MSGEQ7_INTERVAL ReadsPerSecond(50)
#define MSGEQ7_SMOOTH true

CMSGEQ7<MSGEQ7_SMOOTH, pinReset, pinStrobe, pinAnalog> MSGEQ7;
int allFreq[7];

struct MyEEPROMStruct {
  int     DeviceID;
  int     anzDevices;
  int     anzLEDs;
  float   anfloat;
  byte    someBytes[12];
  boolean state;
} eepromdata;



String HostnameString = "LED-Sail";


//Flag - Wenn True wurde die Config verändert
bool shouldSaveConfig = false;

// Set web server port number to 80
ESP8266WebServer server(80); //Server on port 80

const String header = "HTTP/1.1 303 OK\r\nLocation:/\r\nCache-Control: no-cache\r\n\r\n";

File fsUploadFile;                      //Hält den aktuellen Upload





//  ##########################################################################   SETUP ########################################################
void setup() {
  // Serielle Schnittstelle öffnen (Bei der Rate sieht man auch den boot)
  Serial.begin(74880);

  //WiFiManager
  //Prüft alte Daten und wenn keine Verbindung wird ein AP auf gemacht
  WiFiManager wifiManager;
  //Löscht die alten Daten (Für zum Testen)
  //wifiManager.resetSettings();

  // Startet den Dateimanager
  SPIFFS.begin();

  // EEPROM starten und auslesen
  EEPROM.begin(sizeof(MyEEPROMStruct));
  // Prüfen ob das EEPROM valide Daten hat
  if (EEPROM.percentUsed() >= 0) {
    //Daten ins Strict aus lesen und ausgeben
    EEPROM.get(0, eepromdata);
    Serial.print("Geräte ID: "); Serial.println(eepromdata.DeviceID);
    Serial.print("Anzahl Geräte: "); Serial.println(eepromdata.anzDevices);
    Serial.print("Anzahl LEDs: "); Serial.println(eepromdata.anzLEDs);
    HostnameString = HostnameString + String(eepromdata.DeviceID);
    Serial.println("EEPROM has data from a previous run.");
    Serial.print(EEPROM.percentUsed());
    Serial.println("% of ESP flash space currently used");
  } else {
    Serial.println("EEPROM size changed - EEPROM data zeroed - commit() to make permanent");
  }


  // Routine wird gestartet wenn keine Verbindung bevor der AP auf gemacht wird
  wifiManager.setAPCallback(configModeCallback);
  // Routine wird gestartet wenn Config abgeschlossen um daten zu speichern
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  // Setzt einen Timeout im Config-Menü
  wifiManager.setConfigPortalTimeout(180);
  // Wenn keine Verbindung mit den alten Daten - neuen AP auf machen
  wifiManager.autoConnect("LED-Sail", "LED-Sail");
  // Hostnamen setzen
  WiFi.hostname(HostnameString);

  // HTTP Server starten
  server.begin();
  server.on("/", handleRoot);
  server.on("/changeValue", changeValue);
  server.on("/format", formatSpiffs);
  server.on("/upload", HTTP_POST, []() {
    server.send(200, "text/plain", "");
  }, handleFileUpload);
  server.onNotFound([]() {
    if (!handleFileRead(server.uri()))
      server.send(404, "text/plain", "FileNotFound");
  });

  // Wenn wir hier ankommen sind wir mit irgendwas verbunden
  Serial.println("connected...yeey :)");

  // Over the Air Update vorbereiten
  ArduinoOTA.setPassword((const char *)"LED-Sail"); // Setze Passwort für OTA
  ArduinoOTA.setHostname("LED-Sail"); // Setze den Hostnamen für OTA
  ArduinoOTA.begin();                       // https://arduino-esp8266.readthedocs.io/en/latest/ota_updates/readme.html

  // Starte die MSGEQ-Lib
  //MSGEQ7.begin();
   // verursacht reboots
}



// ####################################################    LOOP ##########################################################
void loop() {
  // höre auf OTA anfragen
  ArduinoOTA.handle();
  // höre auf HTTP anfragen
  server.handleClient();

  // MSGEQ7 abfragen mit intervall
  bool newReading = MSGEQ7.read(MSGEQ7_INTERVAL);
  // Wenn es hier was neues gibt
  if (newReading) {
    // Geh die 7 Frequenzen durch
    for (int freq = 0; freq < 7; freq++) {
      // Wert holen
      allFreq[freq] = MSGEQ7.get(freq);
      // Rauschen glätten
      allFreq[freq] = mapNoise(allFreq[freq]);
    }
  }


}



void configModeCallback (WiFiManager *myWiFiManager) {
  // Wird ausgeführt bevor man in den Config-Mode kommt zum laden von Werten zum Beispiel
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void saveConfigCallback () {
  // Wird ausgeführt wenn man den Config-Mode verlässt und etwas geändert hat zum Speichern von eigenen Variablen
  Serial.println("Should save config");
  shouldSaveConfig = true;
}
