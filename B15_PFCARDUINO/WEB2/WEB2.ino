#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>

// WiFi Config
const char* ssid = "IdoomFibre_ATeY9YUTd";
const char* password = "hUsX2sbM";

// RFID Config
MFRC522DriverPinSimple ss_pin(5);  
MFRC522DriverSPI driver{ss_pin};
MFRC522 rfid{driver};

// WebSocket Config
WebSocketsClient webSocket;
const char* ws_server = "192.168.100.8";
const int ws_port = 8080;

// Buzzer Config
const int buzzerPin = 27;

void setup() {
  Serial.begin(115200);
  
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);
  SPI.begin();
  rfid.PCD_Init();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("WiFi connected");

  webSocket.begin(ws_server, ws_port, "/");
  webSocket.onEvent([](WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_CONNECTED) {
      Serial.println("WebSocket Connected");
      webSocket.sendTXT("{\"type\":\"esp\"}");
    }
    else if (type == WStype_TEXT) {
      Serial.printf("Server: %s\n", payload);
      DynamicJsonDocument doc(128);
      deserializeJson(doc, payload);
    
      if(doc["type"] == "beep") {
        int duration = doc["duration"] | 1500;
        Serial.printf("Low quantity beep for %dms\n", duration);
        digitalWrite(buzzerPin, HIGH);
        delay(duration);
        digitalWrite(buzzerPin, LOW);
      }
    }
  });
  webSocket.setReconnectInterval(5000);
}

String readUID() {
  String uid;
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  rfid.PICC_HaltA();
  uid.toUpperCase();
  return uid;
}

void loop() {
  webSocket.loop();

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String uid = readUID();
    Serial.print("Tag scanned: ");
    Serial.println(uid);

    DynamicJsonDocument doc(128);
    doc["uid"] = uid;
    String output;
    serializeJson(doc, output);
    webSocket.sendTXT(output);

    // Normal scan beep (100ms)
    digitalWrite(buzzerPin, HIGH);
    delay(100);
    digitalWrite(buzzerPin, LOW);

    delay(1000); // Anti-spam delay
  }
}
