#include <EEPROM.h>
#include <Eeprom_at24c256.h>
#include "BLEDevice.h"
#include "Math.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "BluetoothSerial.h"

#define RXD2 16
#define TXD2 17

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial ESP32BT;

HTTPClient http;
Eeprom_at24c256 extEEPROM(0x50);

static BLEAddress *pServerAddress;
BLEScan* pBLEScan;
BLEClient*  pClient;

String beaconAddress[35];
String message = "";
String msgToSend;
int beaconLength = 0;
String beaconStatus[35];
char msg[1024];
String Status = "";
bool detected = false;
int state = 0, count = 0;
long endTime1 = 0.0;
long endTime2 = 0.0;
int i;
int counter = 0;
char BTmsg;
int EEPROMaddress[35];
String ESP_MAC;
String ESP_ID;



String url = "http://103.7.57.134:8080/andaman/studentTracker/backOffice/beaconList?esp_id=";
String sendBack = "http://103.7.57.134:8080/andaman/studentTracker/backOffice/espSendBack";


class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice Device) {
      Serial.print(state);
      pServerAddress = new BLEAddress(Device.getAddress());
      if (strcmp(pServerAddress->toString().c_str(), beaconAddress[state].c_str()) == 0) {
        detected = true;
      }
      if (detected) {
        //        int rssi = Device.getRSSI();
        //        if (rssi < -95) {
        //          Status = "U";
        //        } else {
        //          Status = "N";
        //        }
        Status = "N";
        Device.getScan()->stop();
      } else {
        Status = "U";
      }
    }
};


void Working() {
  Serial.println(state);
  beaconStatus[state] = "";
  detected = false;
  BLEScanResults scanResults = pBLEScan->start(5);
  beaconStatus[state] = Status;
  if (state < beaconLength) {
    state = state + 1;
  }
  if (state == beaconLength) {
    state = 0;
  }
}

void setWifi(String msg) {
  Serial.println(msg);
  String msgArr[3];
  int t = 0, r = 0;
  for (int c = 0; c < msg.length(); c++) {
    if (msg.charAt(c) == ',' || msg.charAt(c) == '#') {
      msgArr[t] = msg.substring(r, c);
      r = c + 1;
      t++;
    }
  }
  Serial.print("BT Serial : ");
  Serial.print(msgArr[0]);
  Serial.print(" : ");
  Serial.print(msgArr[1]);
  Serial.print(" : ");
  Serial.println(msgArr[2]);

  if (msgArr[0] == "wifi" || msgArr[0] == "Wifi") {
    String ssid = msgArr[1];
    String password = msgArr[2];
    Serial.println(ssid);
    EEPROM.writeString(0, ssid);
    EEPROM.commit();
    EEPROM.writeString(20, password);
    EEPROM.commit();
  }
  String id = EEPROM.readString(0);
  String pw = EEPROM.readString(20);
  Serial.print("SSID: ");
  Serial.print(id);
  Serial.print(" PASSWORD: ");
  Serial.println(pw);

  ESP.restart();
}

void saveMACtoEEPROM(String mac, String eeprom) {
  Serial.print("SAVE MAC : ");
  Serial.print(mac);
  Serial.print(" EEPROM ");
  Serial.println(eeprom);

  String beaconMac = "";
  int eepromInt = eeprom.toInt();
  char writemessage[18];
  strcpy(writemessage, mac.c_str());

  extEEPROM.write(EEPROMaddress[eepromInt - 1], (byte*) writemessage, sizeof(writemessage));
  delay(500);
  getBeaconAddressFromEEPROM();
}

void getBeaconAddressFromEEPROM() {
  beaconLength = 0;
  String beaconMac = "";
  char address[18];

  for (int i = 0; i < 35; i++) {
    extEEPROM.read(EEPROMaddress[i], (byte *) address, sizeof(address));
    Serial.print(EEPROMaddress[i]);
    Serial.print("EEPROM : ");
    Serial.print(sizeof(address));
    Serial.println(address);

    beaconMac = String(address);
    Serial.print("beaconMac : ");
    Serial.println(beaconMac);

    if (beaconMac.length() >= 17) {
      beaconAddress[beaconLength] = beaconMac;
      beaconLength++;
      Serial.print("beaconAddress : ");
      Serial.println(beaconAddress[beaconLength]);
    }
  }
}

void getMAC() {
  String mac = WiFi.macAddress();
  String arr;
  int i = 0;
  while (i < mac.length()) {
    if (mac.charAt(i) == ':') {
    } else {
      arr = arr + mac.charAt(i);
    }
    i++;
  }
  ESP_ID = arr;
}

void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  getMAC();
  Serial.println(ESP_ID);
  url = url + String(ESP_ID);
  Serial.println(url);

  String BTName = "ESP_" + ESP_ID;
  ESP32BT.begin(BTName);
  EEPROM.begin(512);
  BLEDevice::init("");
  pClient  = BLEDevice::createClient();
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  Serial.println("Init BLE Done !");

  EEPROMaddress[0] = 0;
  for (int count = 1; count < 35; count++) {
    EEPROMaddress[count] = EEPROMaddress[count - 1] + 18;
    Serial.println(EEPROMaddress[count]);
  }

  getBeaconAddressFromEEPROM();
}

void makeString() {
  message = "";
  i = 0;
  Serial.println("");
  while (i < beaconLength) {
    Serial.print("i value : ");
    Serial.print(i);
    Serial.print(" address : ");
    Serial.print(beaconAddress[i]);
    Serial.print(" status : ");
    Serial.println(beaconStatus[i]);
    message = String(message) + String(beaconAddress[i]) + String("-") + String(beaconStatus[i]) + String("|");
    i++;
  }
}

void sendStrings() {
  snprintf (msg, 512, "#SHCOOL|%s|%s,%d,", ESP_ID.c_str(), msgToSend.c_str(), counter );
  Serial2.write(msg);
  Serial.println(msg);
  if (counter < 21) {
    counter = counter + 1;
  } else {
    counter = 0;
  }

}

void getData() {
  StaticJsonDocument<1024> doc;
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode == 200) {
    String content = http.getString();
    Serial.println("Content ---------");
    Serial.println(content);
    Serial.println("-----------------");
    DeserializationError error = deserializeJson(doc, content);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }
  } else {
    Serial.println("Fail. error code " + String(httpCode));
  }
  String returnMsg = "{\"message\":\"" + String(ESP_ID);
  String mac_length = doc["length"];
  int len = mac_length.toInt();
  if (len != 0) {
    for (int c = 0; c < len; c++) {
      String eeprom = doc["beacon"][c]["eeprom"];
      String mac = doc["beacon"][c]["mac"];
      returnMsg = returnMsg + String(",") + String(mac);
      saveMACtoEEPROM(mac, eeprom);
    }
    returnMsg = returnMsg + "\"}";
    Serial.println(returnMsg);
    http.begin(sendBack);
    http.addHeader("Content-Type", "application/json");
    int httpCode = http.POST(returnMsg);
    Serial.println(httpCode);
  }
  WiFi.disconnect();
}

void wifiConnect() {
  String ssid = EEPROM.readString(0);
  String password = EEPROM.readString(20);
  const char *wifiID = ssid.c_str();
  const char *wifiPass = password.c_str();

  if (WiFi.status() != WL_CONNECTED) {
    int counter = 0;
    Serial.print("WiFi CONNECTING TO :  ");
    Serial.println(wifiID);
    WiFi.begin(wifiID, wifiPass);
    while (WiFi.status() != WL_CONNECTED && counter < 10) {
      counter = counter + 1;
      delay(100);
      Serial.print("_");
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("Connected");
      getData();
    }
  } else {
    getData();
  }
}

void loop() {
  while (ESP32BT.available()) {
    String msg = ESP32BT.readString();
    setWifi(msg);
  }

  Working();
  delay(100);

  long nowtime1 = millis();
  if (nowtime1 - endTime1 > 60000) {
    endTime1 = nowtime1;
    sendStrings();
    wifiConnect();
  }
  if (state == 0) {
    makeString();
    msgToSend = message;
    memset(beaconStatus, 0, sizeof beaconStatus);
  }
}
