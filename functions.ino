//Print configuration parameters
void printConfiguration () {
  Serial.println(F("\nPrint ALL VARIABLE:"));
  Serial.print(F("wifiAP_mode="));  Serial.println(wifiAP_mode);
  Serial.print(F("p_ssidAP="));     Serial.println(p_ssidAP);
  Serial.print(F("p_passwordAP=")); Serial.println(p_passwordAP);
  Serial.print(F("p_ssid="));       Serial.println(p_ssid);
  Serial.print(F("p_password="));   Serial.println(p_password);
  Serial.print(F("static_IP="));     Serial.println(static_IP);
  Serial.print(F("ip="));    Serial.print(ip[0]);   Serial.print(":");  Serial.print(ip[1]);  Serial.print(":");  Serial.print(ip[2]);  Serial.print(":");  Serial.println(ip[3]);
  Serial.print(F("sbnt="));  Serial.print(sbnt[0]); Serial.print(":");  Serial.print(sbnt[1]);  Serial.print(":");  Serial.print(sbnt[2]);  Serial.print(":");  Serial.println(sbnt[3]);
  Serial.print(F("gtw="));   Serial.print(gtw[0]);  Serial.print(":");  Serial.print(gtw[1]);  Serial.print(":");  Serial.print(gtw[2]);  Serial.print(":");  Serial.println(gtw[3]);
}




void printChipInfo() {
  Serial.print(F("\n<-> LAST RESET REASON: "));  Serial.println(ESP.getResetReason());
  Serial.print(F("<-> ESP8266 CHIP ID: "));      Serial.println(String(ESP.getChipId(), HEX));
  Serial.print(F("<-> CORE VERSION: "));         Serial.println(ESP.getCoreVersion());
  Serial.print(F("<-> SDK VERSION: "));          Serial.println(ESP.getSdkVersion());
  Serial.print(F("<-> CPU FREQ MHz: "));         Serial.println(ESP.getCpuFreqMHz());
  Serial.print(F("<-> FLASH CHIP ID: "));        Serial.println(String(ESP.getFlashChipId(), HEX));
  Serial.print(F("<-> FLASH CHIP SIZE: "));      Serial.println(ESP.getFlashChipSize());
  Serial.print(F("<-> FLASH CHIP REAL SIZE: ")); Serial.println(ESP.getFlashChipRealSize());
  Serial.print(F("<-> FLASH CHIP SPEED: "));     Serial.println(ESP.getFlashChipSpeed());
  Serial.print(F("<-> FREE MEMORY: "));          Serial.println(ESP.getFreeHeap());
  Serial.print(F("<-> SKETCH SIZE: "));          Serial.println(ESP.getSketchSize());
  Serial.print(F("<-> FREE SKETCH SIZE: "));     Serial.println(ESP.getFreeSketchSpace());
  Serial.print(F("<-> CYCLE COUNTD: "));         Serial.println(ESP.getCycleCount());
  Serial.println("");
}

int getRstInfo() {
  struct rst_info *restart_info = system_get_rst_info();
  return restart_info->reason;

  //REASON_DEFAULT_RST = 0, /* normal startup by power on */
  //REASON_WDT_RST = 1, /* hardware watch dog reset */
  //REASON_EXCEPTION_RST = 2, /* exception reset, GPIO status won't change */
  //REASON_SOFT_WDT_RST   = 3, /* software watch dog reset, GPIO status won't change */
  //REASON_SOFT_RESTART = 4, /* software restart ,system_restart , GPIO status won't change */
  //REASON_DEEP_SLEEP_AWAKE = 5, /* wake up from deep-sleep */
  //REASON_EXT_SYS_RST      = 6 /* external hardware reset */
}


void saveRstInfoToFile() {
  String filename = "/rst.txt";
  //SPIFFS.remove(filename);
  File file = SPIFFS.open(filename, "a");
  if (!file) {
    Serial.print(F("Failed to open file for writing"));   Serial.println(filename);
  } else {
    file.print(getRstInfo());
    file.print(" - ");
    file.println(getDataTimeStr());
    //Serial.print(F("Saved to file "));   Serial.println(filename);
  }
  file.close();
}




void saveTimeOnRelay() {
  if (flagLog == 1) {
    String filename = "/inout.txt";
    //SPIFFS.remove(filename);
    File file = SPIFFS.open(filename, "a");
    if (!file) {
      Serial.print(F("Failed to open file for writing"));   Serial.println(filename);
    } else {
      if (pirSensor0.read())   file.print("IN  ");
      if (pirSensor1.read())   file.print("OUT ");
      file.print(" - ");
      file.println(getDataTimeStr());
      //Serial.print(F("Saved to file "));   Serial.println(filename);
    }
    file.close();
  }
}


String getDataTimeStr() {
  time_t t = timeClient.getEpochTime();
  String data_time = String(year(t));
  data_time += ".";
  data_time += String(month(t));
  data_time += ".";
  data_time += String(day(t));
  data_time += " ";
  data_time += String(timeClient.getFormattedTime());
  return data_time;
}
