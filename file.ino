//START Чтение JSON данных из файла
bool loadConfiguration()
{
  Serial.println(F("\nLoad configuration from file."));
  File file = SPIFFS.open("/config.txt", "r");
  if (!file) {
    Serial.println(F("!!! FAILED to read configuration file, using default configuration"));
    return 0;
  }

  DynamicJsonDocument doc(1024);

  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.println(F("Failed Deserialization configuration file"));
    file.close();
    return 1;
  }

  // Copy values from the JsonDocument to the value config
  delayOff = doc["delayOff"];        //Serial.println(delayOff);
  relayMode = doc["relayMode"];      //Serial.println(relayMode);
  sensor1Use = doc["sensor1Use"];    //Serial.println(sensor1Use);
  sensor2Use = doc["sensor2Use"];    //Serial.println(sensor2Use);

  wifiAP_mode = doc["wifiAP_mode"];    //Serial.println(wifiAP_mode);

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

  ip[0] = doc["ip"][0];    //Serial.println(ip[0]);
  ip[1] = doc["ip"][1];    //Serial.println(ip[1]);
  ip[2] = doc["ip"][2];    //Serial.println(ip[2]);
  ip[3] = doc["ip"][3];    //Serial.println(ip[3]);
  sbnt[0] = doc["sbnt"][0];    //Serial.println(sbnt[0]);
  sbnt[1] = doc["sbnt"][1];    //Serial.println(sbnt[1]);
  sbnt[2] = doc["sbnt"][2];    //Serial.println(sbnt[2]);
  gtw[0] = doc["gtw"][0];    //Serial.println(gtw[0]);
  sbnt[3] = doc["sbnt"][3];    //Serial.println(sbnt[3]);
  gtw[1] = doc["gtw"][1];    //Serial.println(gtw[1]);
  gtw[2] = doc["gtw"][2];    //Serial.println(gtw[2]);
  gtw[3] = doc["gtw"][3];    //Serial.println(gtw[3]);

  file.close();
  return 1;
}


//START Сохранение JSON данных в файл
void saveConfiguration()
{
  Serial.println(F("\nSave configuration to file."));
  char *filename = "/config.txt";

  SPIFFS.remove(filename);

  File file = SPIFFS.open(filename, "w");
  if (!file) {
    Serial.println(F("failed to open config file for writing"));
    return;
  }

  DynamicJsonDocument doc(1024);
  doc["delayOff"] = delayOff;
  doc["relayMode"] = relayMode;
  doc["sensor1Use"] = sensor1Use;
  doc["sensor2Use"] = sensor2Use;
  doc["wifiAP_mode"] = wifiAP_mode;
  doc["p_ssidAP"] = p_ssidAP;
  doc["p_passwordAP"] = p_passwordAP;
  doc["p_ssid"] = p_ssid;
  doc["p_password"] = p_password;
  JsonArray ipJsonArray = doc.createNestedArray("ip");
  for (int n = 0; n < 4; n++)  ipJsonArray.add(ip[n]);
  JsonArray sbntJsonArray = doc.createNestedArray("sbnt");
  for (int n = 0; n < 4; n++)  sbntJsonArray.add(sbnt[n]);
  JsonArray gtwJsonArray = doc.createNestedArray("gtw");
  for (int n = 0; n < 4; n++)  gtwJsonArray.add(gtw[n]);

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    Serial.println(F("Failed to write to file"));
  } else {
    Serial.println(F("Save config complited."));
  }
  file.close();
}


//Чтение данных из файла статистики
bool loadStat(char *filename)
{
  Serial.print(F("\nLoad stat from file: ")); Serial.println(filename);
  File file = SPIFFS.open(filename, "r");
  if (!file) {
    Serial.println(F("!!! FAILED to read stat file"));
    return 0;
  }

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.println(F("Failed Deserialization file stat"));
    file.close();
    return 1;
  }

  numbOn = doc["numbOn"];                Serial.print("numbOn= "); Serial.println(numbOn);
  timeRelayOn = doc["timeRelayOn"];      Serial.print("timeRelayOn = "); Serial.println(timeRelayOn);

  file.close();
  return 1;
}


//Сохранение данных в файл статистики
void saveStat(char *filename)
{
  Serial.print(F("\nSave stat to file: ")); Serial.println(filename);
  //char *filename = "/stat.txt";

  //SPIFFS.remove(filename);

  File file = SPIFFS.open(filename, "w");
  if (!file) {
    Serial.println(F("failed to open config file for writing"));
    return;
  }

  DynamicJsonDocument doc(1024);
  doc["numbOn"] = numbOn;
  doc["timeRelayOn"] = timeRelayOn;

  if (serializeJson(doc, file) == 0) {
    Serial.println(F("Failed to write to file"));
  } else {
    Serial.println(F("Save file stat complited"));
  }
  file.close();
}


//START Prints the content of a file to the Serial
void printFile(const char *filename)
{
  File file = SPIFFS.open(filename, "r");
  if (!file) {
    Serial.println(F("Failed to read file"));
    return;
  }

  // Extract each characters by one by one
  Serial.println(F("Read file for print."));
  while (file.available()) {
    Serial.print((char)file.read());
  }
  Serial.println();
  file.close();
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
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    String fileName = dir.fileName();
    size_t fileSize = dir.fileSize();
    Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
  }
  Serial.println("\nScan file complited");
}

void scanSttFile()
{
  int maxNumberFile = 0;
  int minNumberFile = 1000000;
  
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    String fileName = dir.fileName();
    int index = 0;
    if (fileName.indexOf("/stt") == 0) {
      //Serial.println(fileName.c_str());
      int numberFile = fileName.substring(4, 10).toInt();
      //Serial.println(numberFile);
      if (numberFile > maxNumberFile) {
        maxNumberFile = numberFile;
      }
      if (numberFile < minNumberFile) {
        minNumberFile = numberFile;
      }
    }
  }

  //Serial.print("minNumberFile = ");  Serial.println(minNumberFile);
  //Serial.print("maxNumberFile = ");  Serial.println(maxNumberFile);

  char maxFileName[15];
  sprintf(maxFileName, "/stt%06d.txt", maxNumberFile);
  Serial.print("maxFileName = ");  Serial.println(maxFileName);
  loadStat(maxFileName);

  char minFileName[15];
  sprintf(minFileName, "/stt%06d.txt", minNumberFile);
  Serial.print("minFileName = ");  Serial.println(minFileName);

  sprintf(nextFileName, "/stt%06d.txt", maxNumberFile + 1);
  Serial.print("nextFileName = ");  Serial.println(nextFileName);

  if (SPIFFS.exists(minFileName)) {
    SPIFFS.rename(minFileName, nextFileName);
    delay(20);
  }
  saveStat(nextFileName);
  delay(20);
}


//Удаляет файлы статистики и создает новые
void deleteAndCreateSTTfiles() {
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    String fileName = dir.fileName();
    if (fileName.indexOf("/stt") == 0) {
      SPIFFS.remove(fileName);
      Serial.print("DELETED FILE - ");
      Serial.println(fileName.c_str());
      delay(100);
    }
  }
  File filez = SPIFFS.open("/stt0000000.txt", "w");
  filez.close();
  delay(100);
  filez = SPIFFS.open("/stt000001.txt", "w");
  filez.close();
  delay(100);
  filez = SPIFFS.open("/stt000002.txt", "w");
  filez.close();
  delay(100);
  filez = SPIFFS.open("/stt000003.txt", "w");
  filez.close();
  delay(100);
  filez = SPIFFS.open("/stt000004.txt", "w");
  filez.close();
  delay(100);
  filez = SPIFFS.open("/stt000005.txt", "w");
  filez.close();
  delay(100);
  filez = SPIFFS.open("/stt000006.txt", "w");
  filez.close();
  filez = SPIFFS.open("/stt000007.txt", "w");
  filez.close();
  delay(100);
  filez = SPIFFS.open("/stt000008.txt", "w");
  filez.close();
  delay(100);
  filez = SPIFFS.open("/stt000009.txt", "w");
  filez.close();
  delay(100);
}
