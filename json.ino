String serializationToJson_index()
{
  DynamicJsonDocument doc(1024);
  doc["delayOff"] = delayOff;
  doc["relayMode"] = relayMode;
  doc["sensor1Use"] = sensor1Use;
  doc["sensor2Use"] = sensor2Use;
  doc["relayState"] = relayState;
  doc["sensor1State"] = sensor1State;
  doc["sensor2State"] = sensor2State;
  doc["numbOn"] = numbOn;
  doc["timeRelayOn"] = timeRelayOn;
  doc["mdTimeRelayOn"] = mdTimeRelayOn;
  doc["timeESPOn"] = timeESPOn;
  String output = "";
  serializeJson(doc, output);
  return output;
}


String serializationToJson_setup()
{
  DynamicJsonDocument doc(1024);
  doc["p_ssid"] = p_ssid;
  doc["p_password"] = p_password;
  doc["p_ssidAP"] = p_ssidAP;
  doc["p_passwordAP"] = p_passwordAP;
  JsonArray ipJsonArray = doc.createNestedArray("ip");
  for (int n = 0; n < 4; n++)  ipJsonArray.add(ip[n]);
  JsonArray sbntJsonArray = doc.createNestedArray("sbnt");
  for (int n = 0; n < 4; n++)  sbntJsonArray.add(sbnt[n]);
  JsonArray gtwJsonArray = doc.createNestedArray("gtw");
  for (int n = 0; n < 4; n++)  gtwJsonArray.add(gtw[n]);
  doc["wifiAP_mode"] = wifiAP_mode;
  doc["static_IP"] = static_IP;
  doc["conIndic"] = conIndic;
  String output = "";
  serializeJson(doc, output);
  return output;
}


void deserealizationFromJson(String json) {
  DynamicJsonDocument doc(1024);
  //ReadBufferingStream bufferingStream(file, 64);
  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    Serial.println(F("Failed to deserialization data from client"));
  }
  else if (doc["page"].as<String>() == "index") {
    delayOff = doc["delayOff"];          //Serial.println(delayOff);
    relayMode = doc["relayMode"];        //Serial.println(relayMode);
    sensor1Use = doc["sensor1Use"];      //Serial.println(sensor1Use);
    sensor2Use = doc["sensor2Use"];      //Serial.println(sensor2Use);
    saveFile(FILE_CONFIG);
    saveFile(FILE_STAT);
    dataUpdateBit = 1;
  }
  else if (doc["page"].as<String>() == "setup") {
    String stemp = doc["p_ssid"].as<String>();
    p_ssid = new char [stemp.length() + 1];
    stemp.toCharArray(p_ssid, stemp.length() + 1);
    //Serial.print(F("p_ssid="));   Serial.println(p_ssid);
    stemp = doc["p_password"].as<String>();
    p_password = new char [stemp.length() + 1];
    stemp.toCharArray(p_password, stemp.length() + 1);
    //Serial.print(F("p_password="));   Serial.println(p_password);
    stemp = doc["p_ssidAP"].as<String>();
    p_ssidAP = new char [stemp.length() + 1];
    stemp.toCharArray(p_ssidAP, stemp.length() + 1);
    //Serial.print(F("p_ssidAP="));   Serial.println(p_ssidAP);
    stemp = doc["p_passwordAP"].as<String>();
    p_passwordAP = new char [stemp.length() + 1];
    stemp.toCharArray(p_passwordAP, stemp.length() + 1);
    ip[0] = doc["ip"][0];    //Serial.println(ip[0]);
    ip[1] = doc["ip"][1];    //Serial.println(ip[1]);
    ip[2] = doc["ip"][2];    //Serial.println(ip[2]);
    ip[3] = doc["ip"][3];    //Serial.println(ip[3]);
    sbnt[0] = doc["sbnt"][0];    //Serial.println(sbnt[0]);
    sbnt[1] = doc["sbnt"][1];    //Serial.println(sbnt[1]);
    sbnt[2] = doc["sbnt"][2];    //Serial.println(sbnt[2]);
    sbnt[3] = doc["sbnt"][3];    //Serial.println(sbnt[3]);
    gtw[0] = doc["gtw"][0];    //Serial.println(gtw[0]);
    gtw[1] = doc["gtw"][1];    //Serial.println(gtw[1]);
    gtw[2] = doc["gtw"][2];    //Serial.println(gtw[2]);
    gtw[3] = doc["gtw"][3];    //Serial.println(gtw[3]);
    wifiAP_mode = doc["wifiAP_mode"];  //Serial.println(wifiAP_mode);
    static_IP = doc["static_IP"];      //Serial.println(static_IP);
    conIndic = doc["conIndic"];        //Serial.println(conIndic);
    saveFile(FILE_CONFIG);
    saveFile(FILE_STAT);
    sendToMqttServer(serializationToJson_setup());
  }
}
