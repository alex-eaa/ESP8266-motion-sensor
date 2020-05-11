StringStream stream;


void mqtt_init() {
  mqtt.setServer(mqtt_server, mqtt_server_port);
  mqtt.setCallback(callback);
  mqtt.setStream(stream);
  pubTopic = mqttUser + "/" + DEVICE_TYPE + String(ESP.getChipId(), HEX) + "/fromDevice";
  Serial.print("pubTopic: ");        Serial.println(pubTopic);
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
    saveFile(CONFIG_FILE);
    saveFile(STAT_FILE);
    delay(100);
    ESP.reset();
  }
  else if (stream.str() == "RESETSTAT") {
    SPIFFS.remove(STAT_FILE);
    delay(500);
    ESP.reset();
  }
  else {
    deserealizationFromJson(stream.str());
  }
  stream.str(""); // очищаем буфер
}


void sendToMqttServer(String json) {
  Serial.print("To MQTT server: ");   Serial.println(json);
  mqtt.beginPublish(pubTopic.c_str(), json.length(), false);
  mqtt.print(json);
  mqtt.endPublish();
}


void reconnect() {
  Serial.println(F("Attemp MQTT connection..."));
  String mdnsNameStr = DEVICE_TYPE + String(ESP.getChipId(), HEX);

  if (mqtt.connect(mdnsNameStr.c_str(), mqttUser.c_str(), mqttPass.c_str())) {
    Serial.println(F("connected to MQTT"));
    sendToMqttServer("{\"hello\":\"world\"}");

    String subTopicStr = mqttUser + "/" + DEVICE_TYPE + String(ESP.getChipId(), HEX) + "/fromClient";
    mqtt.subscribe(subTopicStr.c_str());
    Serial.print(F("subscribe to topic = "));   Serial.println(subTopicStr);
  } else {
    Serial.print(F("mqtt failed, rc="));   Serial.println(mqtt.state());
  }
}
