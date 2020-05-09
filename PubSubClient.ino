StringStream stream;


void mqtt_init() {
  mqtt.setServer(mqtt_server, mqtt_server_port);
  mqtt.setCallback(callback);
  mqtt.setStream(stream);
}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("From MQTT server: ");            Serial.println(stream.str());

  if (stream.str() == "INDEX") {
    sendToMqttServer(serializationToJson_index());
  }
  else if (stream.str() == "SETUP") {
    sendToMqttServer(serializationToJson_setup());
  }
  else if (stream.str() == "RESET") {
    ESP.reset();
  }
  else if (stream.str() == "RESETSTAT") {
    SPIFFS.remove(STAT_FILE);
    delay(500);
    ESP.reset();
  }
  else {
    DynamicJsonDocument doc(1024);
    //ReadBufferingStream bufferingStream(file, 64);
    DeserializationError error = deserializeJson(doc, stream.str());
    if (error) {
      Serial.println(F("Failed to deserialization data from client"));
    }

    if (doc["page"].as<String>() == "index") {
      delayOff = doc["delayOff"];          //Serial.println(delayOff);
      relayMode = doc["relayMode"];        //Serial.println(relayState);
      sensor1Use = doc["sensor1Use"];      //Serial.println(sensor1State);
      sensor2Use = doc["sensor2Use"];      //Serial.println(sensor2State);
      dataUpdateBit = 1;
      //saveFile(CONFIG_FILE);
      //saveFile(STAT_FILE);
      //sendToMqttServer(serializationToJson_index());
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
      //saveFile(CONFIG_FILE);
      //saveFile(STAT_FILE);
      sendToMqttServer(serializationToJson_setup());
    }
  }
  stream.str(""); // очищаем буфер
}


void sendToMqttServer(String json) {
  mqtt.beginPublish("user_889afb72/esplink_ms_1adffc/fromDevice", json.length(), false);
  mqtt.print(json);
  mqtt.endPublish();
}


void reconnect() {
  Serial.println(F("Attemp MQTT connection..."));
  String mdnsNameStr = HOST_NAME + String(ESP.getChipId(), HEX);
  String mqttUser = "user_889afb72";
  String mqttPass = "pass_7c9ca39a";
  // Attempt to connect
  if (mqtt.connect(mdnsNameStr.c_str(), mqttUser.c_str(), mqttPass.c_str())) {
    Serial.println(F("connected to MQTT"));
    // Once connected, publish an announcement...
    mqtt.publish("user_889afb72/esplink_ms_1adffc/fromDevice", "{\"hello\":\"world\"}");
    // ... and resubscribe
    String subTopicStr = "user_889afb72";
    subTopicStr += "/";
    subTopicStr += mdnsNameStr;
    subTopicStr += "/fromClient";
    Serial.print(F("subscribe to topic = "));   Serial.println(subTopicStr);
    mqtt.subscribe(subTopicStr.c_str());
  } else {
    Serial.print(F("mqtt failed, rc="));   Serial.println(mqtt.state());
  }
}
