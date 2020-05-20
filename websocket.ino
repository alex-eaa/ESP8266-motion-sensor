
// функция иннициализации сервера WebSocket
void webSocket_init()
{
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}


//функция обработки входящих на сервер WebSocket сообщений
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
  switch (type) {

    case WStype_DISCONNECTED:
      sendSpeedDataEnable[num] = 0;
      Serial.printf("[%u] Disconnected!\n", num);
      //Serial.printf("WStype_DISCONNECTED sendSpeedDataEnable [%u][%u][%u][%u][%u]\n", sendSpeedDataEnable[0], sendSpeedDataEnable[1], sendSpeedDataEnable[2], sendSpeedDataEnable[3], sendSpeedDataEnable[4]);
      break;

    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        if (strcmp((char *)payload, "/index.htm") == 0) {
          sendSpeedDataEnable[num] = 1;
          sendToWsClient(num, serializationToJson_index());
        } else if (strcmp((char *)payload, "/setup.htm") == 0) {
          sendSpeedDataEnable[num] = 0;
          sendToWsClient(num, serializationToJson_setup());
        }
        break;
      }

    case WStype_TEXT:
      {
        Serial.printf("[%u] get from WS: %s\n", num, payload);

        if (strcmp((char *)payload, "RESET") == 0) {
          //saveFile(FILE_CONFIG);
          //saveFile(FILE_STAT);
          //delay(50);
          ESP.reset();
        }
        else if (strcmp((char *)payload, "RESETSTAT") == 0) {
          SPIFFS.remove(FILE_STAT);
          delay(500);
          ESP.reset();
        }
        else {
          deserealizationFromJson((char *)payload);
        }
        break;
      }

    case WStype_BIN:
      Serial.printf("[%u] get binary length: %u\n", num, length);
      hexdump(payload, length);
      // send message to client
      // webSocket.sendBIN(num, payload, length);
      break;
  }

}


void sendToWsClient(int num, String json) {
  //Serial.println(json);
  webSocket.sendTXT(num, json);
}




// Проверка состояния соединения с websocket-клиентами. Отключение тех с которыми нет связи.
void checkPing() {
  //Serial.println("Start checkPing");
  for (uint8_t nums = 0; nums < 5; nums++) {
    //int timeStart = micros();
    if ( !webSocket.sendPing(nums, ping) )  webSocket.disconnect(nums);
    //int timeTotal = micros() - timeStart;
    //Serial.printf("TIME ping [%u]: %u\n", nums, timeTotal);
  }
}
