#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <SocketIOclient.h>
#include <Adafruit_Fingerprint.h>
#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>
#define TFT_CS     33
#define TFT_RST    14  // you can also connect this to the Arduino reset
#define TFT_DC     26
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);
#include <MFRC522.h>
#define SS_PIN 15
#define RST_PIN 16

int RelayPin = 5;

#define BUZZER 5 //buzzer pin
MFRC522 mfrc522(SS_PIN, RST_PIN);

WiFiMulti WiFiMulti;
SocketIOclient socketIO;

void setup() {

  pinMode(RelayPin, OUTPUT);
  SPI.begin();      
  mfrc522.PCD_Init(); 
  digitalWrite(RelayPin, LOW);
  Serial.begin(9600);
  Serial.setDebugOutput(true);
  Serial2.begin(115200);
  delay(100);
  
  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] BOOT WAIT %d...\n", t);
    tft.print("\nSetup Boot...");
    Serial.flush();
    delay(1000);
  }

  const char *ssid = "SSS";
  const char *pass = "Kawarizmi";
  const char *HOST = "104.196.232.237";

  WiFi.begin(ssid, pass);

  //WiFi.disconnect();
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }

  String ip = WiFi.localIP().toString();
  Serial.printf("[SETUP] WiFi Connected %s\n", ip.c_str());
  tft.println("\nWifi Terhubung ");

  // server address, port and URL
  socketIO.begin(HOST , 3000, "/socket.io/?EIO=4");

  // event handler
  socketIO.onEvent(socketIOEvent);
  
}

unsigned long messageTimestamp = 0;

void loop() {
  Serial.println("Put your card to the reader...");
  Serial.println();
  
  socketIO.loop();
  
  uint64_t now = millis();
 
  if ( ! mfrc522.PICC_IsNewCardPresent()) 
  {
    return;
  }
  
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return;
  }
  
  Serial.print("UID tag :");
  String content= "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
     Serial.print(mfrc522.uid.uidByte[i], HEX);
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
  Serial.print("Message : ");
  content.toUpperCase();
  
  if(content.substring(1) == "43 D4 CA 1B" ||content.substring(1) == "A4 B9 23 07") //change here the UID of the card/cards that you want to give access
  {
   
    DynamicJsonDocument doc(1024);
    JsonArray array = doc.to<JsonArray>();
    array.add("record");
    JsonObject param1 = array.createNestedObject();
    param1["code_tag"] = content.substring(1);
    String output;
    serializeJson(doc, output);
    socketIO.sendEVENT(output);
    Serial.println(output);

    Serial.println("Authorized access");
    Serial.println();
    digitalWrite(RelayPin, HIGH);
    Serial.println("HIDUP");
    delay(2000);
    digitalWrite(RelayPin, LOW);
    Serial.println("Hidup");
  }
  else{
    Serial.println(" Access denied");
    digitalWrite(RelayPin, LOW);
    Serial.println("Mati");
    delay(2000);
  }
}

uint8_t readnumber(void) {
  uint8_t num = 0;

  while (num == 0) {
    while (! Serial.available());
    num = Serial.parseInt();
  }
  return num;
}

void socketIOEvent(socketIOmessageType_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case sIOtype_DISCONNECT:
      Serial.printf("[IOc] Disconnected!\n");
      break;
    case sIOtype_CONNECT:
      Serial.printf("[IOc] Connected to url: %s\n", payload);
      socketIO.send(sIOtype_CONNECT, "/");
      break;
    case sIOtype_EVENT: {
        char * sptr = NULL;
        int id = strtol((char *)payload, &sptr, 10);
        Serial.printf("[IOc] get event: %s id: %d\n", payload, id);

        if (id) {
          payload = (uint8_t *)sptr;
        }

        DynamicJsonDocument doc(1024);
        DynamicJsonDocument controlDoc(256);
        DeserializationError error = deserializeJson(doc, payload, length);

        if (error) {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.c_str());
          return;
        }

        String eventName = doc[0];

        Serial.printf("[IOc] event name: %s\n", eventName.c_str());

        // Message Includes a ID for a ACK (callback)
        if (id) {
          // creat JSON message for Socket.IO (ack)
          DynamicJsonDocument docOut(1024);
          JsonArray array = docOut.to<JsonArray>();
          JsonObject param1 = array.createNestedObject();
          param1["now"] = millis();

          // JSON to String (serializion)
          String output;
          output += id;
          serializeJson(docOut, output);

          // Send event
          socketIO.send(sIOtype_ACK, output);
        }
      }
      break;
    case sIOtype_ACK:
      Serial.printf("[IOc] get ack: %u\n", length);
      break;
    case sIOtype_ERROR:
      Serial.printf("[IOc] get error: %u\n", length);
      break;
    case sIOtype_BINARY_EVENT:
      Serial.printf("[IOc] get binary: %u\n", length);
      break;
    case sIOtype_BINARY_ACK:
      Serial.printf("[IOc] get binary ack: %u\n", length);
      break;
  }
}
