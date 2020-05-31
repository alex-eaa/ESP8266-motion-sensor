StringStream stream;


void mqtt_init() {
  mqtt.setServer(mqtt_server, mqtt_server_port);
  mqtt.setCallback(callback);
  mqtt.setStream(stream);
  pubTopic = mqttUser + "/" + DEVICE_TYPE + String(ESP.getChipId(), HEX) + "/fromDevice";
  //Serial.print("pubTopic: ");        Serial.println(pubTopic);
}


void mqttConnect() {
  if (mqtt.connected()) {
    mqtt.loop();
    //Переподключение к MQTT серверу, если мы подключены к WIFI и связь с MQTT отсутствует, каждые 5 сек
  } else if (!mqtt.connected() && WiFi.status() == WL_CONNECTED && millis() - startMqttReconnectTime > TIME_ATTEMP_CON_MQTT) {
    reconnect();
    startMqttReconnectTime = millis();
  }
}


void callback(char* topic, byte* payload, unsigned int length) {
#ifdef DEBUG
  Serial.print("From MQTT server: ");               Serial.println(stream.str());
#endif

  if (stream.str() == "INDEX") {
    sendToMqttServer(serializationToJson_index());
  }
  else if (stream.str() == "SETUP") {
    sendToMqttServer(serializationToJson_setup());
  }
  else if (stream.str() == "RESET") {
    saveFile(FILE_RELAY);
    delay(50);
    ESP.reset();
  }
  else if (stream.str() == "RESETSTAT") {
    relay.resetStat();
    saveFile(FILE_RELAY);
  }
  else {
    deserealizationFromJson(stream.str());
  }
  stream.str(""); // очищаем буфер
}


void sendToMqttServer(String json) {
#ifdef DEBUG
  Serial.print("To MQTT server: ");   Serial.println(json);
#endif
  mqtt.beginPublish(pubTopic.c_str(), json.length(), false);
  mqtt.print(json);
  mqtt.endPublish();
}


void reconnect() {
#ifdef DEBUG
  Serial.println(F("Attemp MQTT connection..."));
#endif
  String mdnsNameStr = DEVICE_TYPE + String(ESP.getChipId(), HEX);

  if (mqtt.connect(mdnsNameStr.c_str(), mqttUser.c_str(), mqttPass.c_str())) {
#ifdef DEBUG
    Serial.println(F("connected to MQTT"));
#endif
    sendToMqttServer("{\"hello\":\"world\"}");

    String subTopicStr = mqttUser + "/" + DEVICE_TYPE + String(ESP.getChipId(), HEX) + "/fromClient";
    mqtt.subscribe(subTopicStr.c_str());
#ifdef DEBUG
    Serial.print(F("subscribe to topic = "));   Serial.println(subTopicStr);
#endif
  } else {
#ifdef DEBUG
    Serial.print(F("mqtt failed, rc="));   Serial.println(mqtt.state());
#endif
  }
}
