//Сохранение файлов в формате JSON
bool saveFile(char *filename)
{
  //Serial.print(F("Save file: "));   Serial.println(filename);
  SPIFFS.remove(filename);

  DynamicJsonDocument doc(1024);
  if (filename == FILE_CONFIG) {
    doc["delayOff"] = delayOff;
    doc["relayMode"] = relayMode;
    doc["sensor1Use"] = sensor1Use;
    doc["sensor2Use"] = sensor2Use;
    doc["wifiAP_mode"] = wifiAP_mode;
    doc["static_IP"] = static_IP;
    doc["p_ssidAP"] = p_ssidAP;
    doc["p_passwordAP"] = p_passwordAP;
    doc["p_ssid"] = p_ssid;
    doc["p_password"] = p_password;
    doc["conIndic"] = conIndic;
    JsonArray ipJsonArray = doc.createNestedArray("ip");
    for (int n = 0; n < 4; n++)  ipJsonArray.add(ip[n]);
    JsonArray sbntJsonArray = doc.createNestedArray("sbnt");
    for (int n = 0; n < 4; n++)  sbntJsonArray.add(sbnt[n]);
    JsonArray gtwJsonArray = doc.createNestedArray("gtw");
    for (int n = 0; n < 4; n++)  gtwJsonArray.add(gtw[n]);
  } else if (filename == FILE_STAT) {
    doc["numbOn"] = numbOn;
    doc["timeRelayOn"] = timeRelayOn;
  }

  File file = SPIFFS.open(filename, "w");
  if (!file) {
    Serial.print(F("Failed to open file for writing"));   Serial.println(filename);
    return 0;
  }
  bool rezSerialization = serializeJson (doc, file);
  file.close();

  if (rezSerialization == 0)  Serial.print(F("Failed write to file: "));
  else   //Serial.print(F("Save file complited: "));
  return rezSerialization;
}


//Чтение из файла в формате JSON
bool loadFile(char *filename) {
  //Serial.print(F("Load file: "));   Serial.println(filename);
  File file = SPIFFS.open(filename, "r");
  if (!file) {
    Serial.print(F("Failed read file: "));   Serial.println(filename);
    return 0;
  }

  DynamicJsonDocument doc(1024);
  //ReadBufferingStream bufferedFile {file, 64 };
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.print(F("Failed deserialization file: "));   Serial.println(filename);
    file.close();
    return 0;
  }

  // Copy values from the JsonDocument to the value
  if (filename == FILE_CONFIG) {
    delayOff = doc["delayOff"];        //Serial.println(delayOff);
    relayMode = doc["relayMode"];      //Serial.println(relayMode);
    sensor1Use = doc["sensor1Use"];    //Serial.println(sensor1Use);
    sensor2Use = doc["sensor2Use"];    //Serial.println(sensor2Use);
    wifiAP_mode = doc["wifiAP_mode"];    //Serial.println(wifiAP_mode);
    conIndic = doc["conIndic"];          //Serial.println(conIndic);
    String stemp = doc["p_ssid"].as<String>();
    p_ssid = new char [stemp.length() + 1];
    stemp.toCharArray(p_ssid, stemp.length() + 1);
    //Serial.print(F("p_ssid="));   Serial.println(p_ssid);
    stemp = doc["p_password"].as<String>();
    p_password = new char [stemp.length() + 1];
    stemp.toCharArray(p_password, stemp.length() + 1);
    //Serial.print(F("p_password="));   Serial.println(p_password);
    stemp = doc["p_passwordAP"].as<String>();
    p_passwordAP = new char [stemp.length() + 1];
    stemp.toCharArray(p_passwordAP, stemp.length() + 1);
    //Serial.print(F("p_passwordAP="));   Serial.println(p_passwordAP);
    stemp = doc["p_ssidAP"].as<String>();
    p_ssidAP = new char [stemp.length() + 1];
    stemp.toCharArray(p_ssidAP, stemp.length() + 1);
    //Serial.print(F("p_ssidAP="));   Serial.println(p_ssidAP);
    static_IP = doc["static_IP"];     //Serial.println(static_IP);
    ip[0] = doc["ip"][0];             //Serial.println(ip[0]);
    ip[1] = doc["ip"][1];             //Serial.println(ip[1]);
    ip[2] = doc["ip"][2];             //Serial.println(ip[2]);
    ip[3] = doc["ip"][3];             //Serial.println(ip[3]);
    sbnt[0] = doc["sbnt"][0];         //Serial.println(sbnt[0]);
    sbnt[1] = doc["sbnt"][1];         //Serial.println(sbnt[1]);
    sbnt[2] = doc["sbnt"][2];         //Serial.println(sbnt[2]);
    gtw[0] = doc["gtw"][0];           //Serial.println(gtw[0]);
    sbnt[3] = doc["sbnt"][3];         //Serial.println(sbnt[3]);
    gtw[1] = doc["gtw"][1];           //Serial.println(gtw[1]);
    gtw[2] = doc["gtw"][2];           //Serial.println(gtw[2]);
    gtw[3] = doc["gtw"][3];           //Serial.println(gtw[3]);
  } else if (filename == FILE_STAT) {
    numbOn = doc["numbOn"];                //Serial.print("numbOn= "); Serial.println(numbOn);
    timeRelayOn = doc["timeRelayOn"];      //Serial.print("timeRelayOn = "); Serial.println(timeRelayOn);
  }

  file.close();
  return 1;
}


//START Prints the content of a file to the Serial
void printFile(const char *filename)
{
  Serial.print(F("Print file: "));   Serial.println(filename);
  File file = SPIFFS.open(filename, "r");
  if (!file) {
    Serial.print(F("Failed to read file: "));   Serial.println(filename);
    return;
  }

  // Extract each characters by one by one
  Serial.print(F("   "));
  while (file.available()) {
    Serial.print((char)file.read());
  }
  file.close();
  Serial.println();
}


//format bytes
String formatBytes(size_t bytes)
{
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  } else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}


void scanAllFile()
{
  //Serial.println("\nScan files:");
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    String fileName = dir.fileName();
    size_t fileSize = dir.fileSize();
    Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
  }
}
