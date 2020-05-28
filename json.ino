String serializationToJson_index()
{
  DynamicJsonDocument doc(1024);
  relay.serialize(&doc, SERIALYZE_ALL);
  doc["sensor0State"] = pirSensor0.read();
  doc["sensor1State"] = pirSensor1.read();
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

  String output = "";
  serializeJson(doc, output);
  return output;
}


void deserealizationFromJson(const String &json) {
  DynamicJsonDocument doc(1024);
  //ReadBufferingStream bufferingStream(file, 64);
  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    Serial.println(F("Failed to deserialization data from client"));
  }
  else if (doc["page"].as<String>() == "index") {
    relay.delayOff = doc["delayOff"];
    relay.relayMode = doc["relayMode"];
    if (doc["sensor0Use"])   relay.atachPirSensor(0, &pirSensor0);
    else relay.detachPirSensor(0);
    if (doc["sensor1Use"])   relay.atachPirSensor(1, &pirSensor1);
    else relay.detachPirSensor(1);
    relay.dataUpdateBit = 1;
    saveFile(FILE_RELAY);
  }
  else if (doc["page"].as<String>() == "setup") {
    delete[] p_passwordAP;
    delete[] p_ssidAP;
    delete[] p_password;
    delete[] p_ssid;

    String stemp = doc["p_ssid"].as<String>();
    p_ssid = new char [stemp.length() + 1];
    stemp.toCharArray(p_ssid, stemp.length() + 1);

    stemp = doc["p_password"].as<String>();
    p_password = new char [stemp.length() + 1];
    stemp.toCharArray(p_password, stemp.length() + 1);

    stemp = doc["p_ssidAP"].as<String>();
    p_ssidAP = new char [stemp.length() + 1];
    stemp.toCharArray(p_ssidAP, stemp.length() + 1);

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

    saveFile(FILE_NETWORK);

    sendToMqttServer(serializationToJson_setup());

  }
}
