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
    deserealizationFromJson(stream.str());
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
  String subTopicStr = mqttUser;
  subTopicStr += "/";
  subTopicStr += mdnsNameStr;

  if (mqtt.connect(mdnsNameStr.c_str(), mqttUser.c_str(), mqttPass.c_str())) {
    Serial.println(F("connected to MQTT"));
    String pubTopicStr = subTopicStr;
    pubTopicStr += "/fromDevice";
    mqtt.publish(pubTopicStr.c_str(), "{\"hello\":\"world\"}");

    subTopicStr += "/fromClient";
    mqtt.subscribe(subTopicStr.c_str());
    Serial.print(F("subscribe to topic = "));   Serial.println(subTopicStr);
  } else {
    Serial.print(F("mqtt failed, rc="));   Serial.println(mqtt.state());
  }
}
