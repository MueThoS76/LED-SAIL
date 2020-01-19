/*
    Achtung Dateinahmen mit mehr als 30 Zeichen werden beim Upload von links gekürzt
    urlDecode wurde hinzugefügt
    Funktioniert jetzt auch über DynDNS (z.B.: My Fritz)
*/


// Werte die auf der Webseite eingegeben wurden werden hier gespeichert
// ToDo: Abfrage ob sich überhaupt was geändert hat! Wenn nicht dann auch nicht ins EEPROM
void changeValue() {
  String arg1 = server.arg("formDeviceID");
  String arg2 = server.arg("formDevices");
  String arg3 =  server.arg("formLEDs");
  eepromdata.DeviceID = arg1.toInt();
  eepromdata.anzDevices = arg2.toInt();
  eepromdata.anzLEDs = arg3.toInt();
  EEPROM.put(0, eepromdata);
  boolean ok2 = EEPROM.commit();
  Serial.println((ok2) ? "Daten gespeichert" : "Commit failed");
  Serial.print("Geräte ID: "); Serial.println(eepromdata.DeviceID);
  Serial.print("Anzahl Geräte: "); Serial.println(eepromdata.anzDevices);
  Serial.print("Anzahl LEDs: "); Serial.println(eepromdata.anzLEDs);
  server.sendContent(header);
}





void handleRoot() {                     // HTML Startseite
  FSInfo fs_info;
  SPIFFS.info(fs_info);
  String temp;
  temp += "<!DOCTYPE HTML>\n<html lang='de'>\n<head><meta charset='UTF-8'><meta name= viewport content='width=device-width, initial-scale=1.0,'>";
  temp += "<style type='text/css'><!-- DIV.container { min-height: 10em; display: table-cell; vertical-align: middle }.button {height:35px; width:90px; font-size:16px} -->";
  temp += "body {background-color: powderblue;}</style>\n<script>function jn(text) {return confirm(text);}</script>\n</head>\n<body><center><h2>" + HostnameString + "<br></h2></center><p>\n";

  // Mein Formular:
  temp += "<p><form action='/changeValue' method='POST'>";
  temp += "<br>Device ID:<br><input type=\"text\"  onkeypress=\"return event.charCode >= 48 && event.charCode <= 57\"   name=\"formDeviceID\" value=\"" + String(eepromdata.DeviceID) + "\">";
  temp += "<br>Anzahl Devices:<br><input type=\"text\"  onkeypress=\"return event.charCode >= 48 && event.charCode <= 57\"   name=\"formDevices\" value=\"" + String(eepromdata.anzDevices) + "\">";
  temp += "<br>Anzahl LEDs:<br><input type=\"text\" onkeypress=\"return event.charCode >= 48 && event.charCode <= 57\"  name=\"formLEDs\" value=\"" + String(eepromdata.anzLEDs) + "\">";
  temp += "<br><input type=\"submit\" value=\"Senden\"></form></p>\r\n";


  /*  Dieser Kram ist für Files - brauche ich aber derzeit nicht
    Dir dir = SPIFFS.openDir("/");         //Listet die Aktuell im SPIFFS vorhandenen Dateien auf
    while (dir.next())
    {
      temp += "<a title=\"Download\" href =\"" + dir.fileName() + "\" download=\"" + dir.fileName() + "\">" + dir.fileName().substring(1) + "</a> " + formatBytes(dir.fileSize());
      temp += "<a href =\"" + dir.fileName() + "?delete=" + dir.fileName() + "\"> Löschen </a><br>\n";
    }
    temp += "<form method='POST' action='/upload' enctype='multipart/form-data' style='height:35px;'><input type='file' name='upload' style='height:35px; font-size:13px;' required>\r\n<input type='submit' value='Upload' class='button'></form>";
    temp += "Datei hochladen<p><p>SPIFFS: " + formatBytes(fs_info.usedBytes) + " von " + formatBytes(fs_info.totalBytes) + "<p>\r\n";
    temp += "<p><form action='/format' method='POST'><input name='ButtonName' type='submit' value='Format SPIFFS' onclick=\"return jn('Wirklich formatieren? Alle Daten gehen verloren')\"></form>Kann bis zu 30 Sekunden dauern";
  */

  temp += "</body></html>\r\n";
  server.send(200, "text/html", temp);
}




String formatBytes(size_t bytes) {            // lesbare Anzeige der Speichergrößen
  if (bytes < 1024) {
    return String(bytes) + " Byte";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + " KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + " MB";
  }
}

String getContentType(String filename) {              // ContentType für den Browser
  if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path) {
  if (server.hasArg("delete")) {
    SPIFFS.remove(server.arg("delete"));                    //hier wir gelöscht
    server.sendContent(header);
  }
  if (path.endsWith("/")) path += "index.html";
  path = server.urlDecode(path);
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
    if (SPIFFS.exists(pathWithGz))
      path += ".gz";
    //   Serial.println("handleFileRead: " + path);
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, getContentType(path));
    file.close();
    return true;
  }
  return false;
}

void handleFileUpload() {                                  // Dateien vom Rechnenknecht oder Klingelkasten ins SPIFFS schreiben
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    Serial.print("handleFileUpload Name: "); Serial.println(filename);
    if (filename.length() > 30) {
      int x = filename.length() - 30;
      filename = filename.substring(x, 30 + x);
    }
    filename = server.urlDecode(filename);
    filename = "/" + filename;
    Serial.print("handleFileUpload Name: "); Serial.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    Serial.print("handleFileUpload Data: "); Serial.println(upload.currentSize);
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile)
      fsUploadFile.close();
    yield();
    Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
    server.sendContent(header);
  }
}

void formatSpiffs() {       //Formatiert den Speicher
  SPIFFS.format();
  server.sendContent(header);
}
