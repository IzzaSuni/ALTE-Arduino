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


float p = 3.1415926;
WiFiMulti WiFiMulti;
SocketIOclient socketIO;
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial2);
uint8_t fingerID;
#define USE_SERIAL Serial
String controlValue, enroll = "OFF", fingerId;
#define button1 25
#define button2 27
#define button3 14
#define button4 12
#define relay1 22
#define relay2 2
#define relay3 19
#define irR 4
#define irB1 34
#define irB2 13
#define irB3 32
#define MagnetSensor 21
#define Buzzer 17

void setup() {

  USE_SERIAL.begin(9600);
  USE_SERIAL.setDebugOutput(true);

  Serial2.begin(115200);

  while (!USE_SERIAL);
  delay(100);

  tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.println("ALTE!\n");
  tft.println("BY KARABAR");
  USE_SERIAL.println("\n\nAdafruit Fingerprint sensor enrollment");

  finger.begin(57600);
  delay(1000);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(1);
  tft.println("Inisialisasi sistem\n");

  if (finger.verifyPassword()) {
    USE_SERIAL.println("Found fingerprint sensor!");
    tft.println("Fingerprint terdeteksi");
  } else {
    tft.println("Fingerprint tidak ditemukan :(");
  }

  pinMode(button1, INPUT);
  pinMode(button2, INPUT);
  pinMode(button3, INPUT);
  pinMode(button4, INPUT);
  pinMode(irR, INPUT);
  pinMode(MagnetSensor, INPUT_PULLUP);
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);
  pinMode(irB1, OUTPUT);
  pinMode(irB2, OUTPUT);
  pinMode(irB3, OUTPUT);
  pinMode(Buzzer, OUTPUT);

  USE_SERIAL.println();

  for (uint8_t t = 4; t > 0; t--) {
    USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
    tft.println("\nSetup Boot...");
    USE_SERIAL.flush();
    delay(1000);
  }
  const char *ssid = "SKYRIM";
  const char *pass = "WINTERHOLD1809";
  const char *HOST = "192.168.1.7";

  WiFiMulti.addAP(ssid, pass);

  //WiFi.disconnect();
  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(100);
  }

  String ip = WiFi.localIP().toString();
  USE_SERIAL.printf("[SETUP] WiFi Connected %s\n", ip.c_str());
  tft.println("\nWifi Terhubung ");

  // server address, port and URL
  socketIO.begin(HOST , 3000, "/socket.io/?EIO=4");

  // event handler
  socketIO.onEvent(socketIOEvent);
}

unsigned long messageTimestamp = 0;

void loop() {
//  ReadFinger();
  socketIO.loop();
  uint64_t now = millis();
  if (digitalRead(2) == HIGH) {
    digitalWrite(4, HIGH);
    DynamicJsonDocument doc(1024);
    JsonArray array = doc.to<JsonArray>();
    array.ad
    d("control");
    // add payload (parameters) for the event
    JsonObject param1 = array.createNestedObject();
    param1["msg"] = "halo from arduino";
    // JSON to String (serializion)
    String output;
    serializeJson(doc, output);
    // Send event
    socketIO.sendEVENT(output);
    // Print JSON for debugging
    USE_SERIAL.println(output);
  }
}


uint8_t readnumber(void) {
  uint8_t num = 0;

  while (num == 0) {
    while (! USE_SERIAL.available());
    num = USE_SERIAL.parseInt();
  }
  return num;
}

uint8_t getFingerprintEnroll() {
  int p = -1;
  int timer = 0;
  while (timer <= 100) {
    timer++;
    if (timer > 100)return true;
    USE_SERIAL.print("Waiting for valid finger to enroll as #"); USE_SERIAL.println(fingerID);
    while (p != FINGERPRINT_OK  ) {
      p = finger.getImage();
      timer++;
      if (timer > 100)return true;
      USE_SERIAL.println(timer);
      switch (p) {
        case FINGERPRINT_OK:
          USE_SERIAL.println("Image taken");
          break;
        case FINGERPRINT_NOFINGER:
          USE_SERIAL.println(".");
          break;
        case FINGERPRINT_PACKETRECIEVEERR:
          USE_SERIAL.println("Communication error");
          break;
        case FINGERPRINT_IMAGEFAIL:
          USE_SERIAL.println("Imaging error");
          break;
        default:
          USE_SERIAL.println("Unknown error");
          break;
      }
    }

    // OK success!

    p = finger.image2Tz(1);
    switch (p) {
      case FINGERPRINT_OK:
        USE_SERIAL.println("Image converted");
        break;
      case FINGERPRINT_IMAGEMESS:
        USE_SERIAL.println("Image too messy");
        return p;
      case FINGERPRINT_PACKETRECIEVEERR:
        USE_SERIAL.println("Communication error");
        return p;
      case FINGERPRINT_FEATUREFAIL:
        USE_SERIAL.println("Could not find fingerprint features");
        return p;
      case FINGERPRINT_INVALIDIMAGE:
        USE_SERIAL.println("Could not find fingerprint features");
        return p;
      default:
        USE_SERIAL.println("Unknown error");
        return p;
    }

    Serial.println("Remove finger");
    delay(2000);
    p = 0;
    while (p != FINGERPRINT_NOFINGER  ) {
      if (timer > 100)return true;
      p = finger.getImage();
    }
    if (timer > 100)return true;
    Serial.print("ID "); Serial.println(fingerID);
    p = -1;
    Serial.println("Place same finger again");
    while (p != FINGERPRINT_OK  ) {
      if (timer > 100)return true;
      timer++;
      USE_SERIAL.println(1500);
      p = finger.getImage();
      switch (p) {
        case FINGERPRINT_OK:
          USE_SERIAL.println("Image taken");
          break;
        case FINGERPRINT_NOFINGER:
          USE_SERIAL.print(".");
          break;
        case FINGERPRINT_PACKETRECIEVEERR:
          USE_SERIAL.println("Communication error");
          break;
        case FINGERPRINT_IMAGEFAIL:
          USE_SERIAL.println("Imaging error");
          break;
        default:
          USE_SERIAL.println("Unknown error");
          break;
      }
    }

    // OK success!

    p = finger.image2Tz(2);
    switch (p) {
      case FINGERPRINT_OK:
        USE_SERIAL.println("Image converted");
        break;
      case FINGERPRINT_IMAGEMESS:
        USE_SERIAL.println("Image too messy");
        return p;
      case FINGERPRINT_PACKETRECIEVEERR:
        USE_SERIAL.println("Communication error");
        return p;
      case FINGERPRINT_FEATUREFAIL:
        USE_SERIAL.println("Could not find fingerprint features");
        return p;
      case FINGERPRINT_INVALIDIMAGE:
        USE_SERIAL.println("Could not find fingerprint features");
        return p;
      default:
        USE_SERIAL.println("Unknown error");
        return p;
    }

    // OK converted!
    USE_SERIAL.print("Creating model for #");  USE_SERIAL.println(fingerID);

    p = finger.createModel();
    if (p == FINGERPRINT_OK) {
      USE_SERIAL.println("Prints matched!");
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
      USE_SERIAL.println("Communication error");
      return p;
    } else if (p == FINGERPRINT_ENROLLMISMATCH) {
      USE_SERIAL.println("Fingerprints did not match");
      return p;
    } else {
      USE_SERIAL.println("Unknown error");
      return p;
    }

    USE_SERIAL.print("ID "); USE_SERIAL.println(fingerID);
    p = finger.storeModel(fingerID);
    if (p == FINGERPRINT_OK) {
      USE_SERIAL.println("Stored!");
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
      USE_SERIAL.println("Communication error");
      return p;
    } else if (p == FINGERPRINT_BADLOCATION) {
      USE_SERIAL.println("Could not store in that location");
      return p;
    } else if (p == FINGERPRINT_FLASHERR) {
      USE_SERIAL.println("Error writing to flash");
      return p;
    } else {
      USE_SERIAL.println("Unknown error");
      return p;
    }
  }
  timer = 0;

  return true;
}

int ReadFinger() {
  finger.LEDcontrol(FINGERPRINT_LED_ON, 0, FINGERPRINT_LED_BLUE);

  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)
  {


    //    Serial.println("Waiting For Valid Finger");
    return -1;
  }

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)
  {

    Serial.println("Messy Image Try Again");
    delay(3000);

    return -1;
  }

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  {


    Serial.println("Not Valid Finger");
    delay(3000);

    return -1;
  }

  Serial.println("Door Unlocked Welcome");
  Serial.println(finger.fingerID);
  return finger.fingerID;
}

void socketIOEvent(socketIOmessageType_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case sIOtype_DISCONNECT:
      USE_SERIAL.printf("[IOc] Disconnected!\n");
      break;
    case sIOtype_CONNECT:
      USE_SERIAL.printf("[IOc] Connected to url: %s\n", payload);
      socketIO.send(sIOtype_CONNECT, "/");
      break;
    case sIOtype_EVENT: {
        char * sptr = NULL;
        int id = strtol((char *)payload, &sptr, 10);
        USE_SERIAL.printf("[IOc] get event: %s id: %d\n", payload, id);

        if (id) {
          payload = (uint8_t *)sptr;
        }

        DynamicJsonDocument doc(1024);
        DynamicJsonDocument controlDoc(256);
        DeserializationError error = deserializeJson(doc, payload, length);

        if (error) {
          USE_SERIAL.print(F("deserializeJson() failed: "));
          USE_SERIAL.println(error.c_str());
          return;
        }

        String eventName = doc[0];

        USE_SERIAL.printf("[IOc] event name: %s\n", eventName.c_str());

        controlValue = eventName.c_str();
        DeserializationError controlJSON = deserializeJson(controlDoc, controlValue);

        String cRelay1 = controlDoc["relay1"];
        String cRelay2 = controlDoc["relay2"];
        String cRelay3 = controlDoc["relay3"];

        String Nama = controlDoc["name"];
        String Nim = controlDoc["nim"];
        String Angkatan = controlDoc["angkatan"];
        String Role = controlDoc["role"];
        uint8_t finger = controlDoc["finger_id"];

        String iRState = controlDoc["ir_state"];

        if (iRState == "control") {

        }
        else if (iRState == "enroll") {

        }

        //handle enroll finger
        if (Nama != "null") {
          fingerID = finger;
          int count = 0;
          USE_SERIAL.println("Ready to enroll a fingerprint! with ID:");
          USE_SERIAL.println(fingerID);
          USE_SERIAL.print("Enrolling ID #");
          USE_SERIAL.println(fingerID);
          while (! getFingerprintEnroll()) {
            getFingerprintEnroll();
          }
        }

        if (cRelay1 == "ON") {
          digitalWrite(relay1, HIGH);
        }
        if (cRelay2 == "ON") {
          digitalWrite(relay2, HIGH);
        }
        if (cRelay3 == "ON") {
          digitalWrite(relay3, HIGH);
        }
        if (cRelay1 == "OFF") {
          digitalWrite(relay1, LOW);
        }
        if (cRelay2 == "OFF") {
          digitalWrite(relay2, LOW);
        }
        if (cRelay3 == "OFF") {
          digitalWrite(relay3, LOW);
        }

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
      USE_SERIAL.printf("[IOc] get ack: %u\n", length);
      break;
    case sIOtype_ERROR:
      USE_SERIAL.printf("[IOc] get error: %u\n", length);
      break;
    case sIOtype_BINARY_EVENT:
      USE_SERIAL.printf("[IOc] get binary: %u\n", length);
      break;
    case sIOtype_BINARY_ACK:
      USE_SERIAL.printf("[IOc] get binary ack: %u\n", length);
      break;
  }
}



