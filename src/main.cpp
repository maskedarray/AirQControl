#include <Arduino.h>
#include "FirebaseESP32.h"
#include "WiFi.h"
#include "ArduinoJson.h"
#include <HTTPClient.h>

#define FIREBASE_HOST "airqcontrol-dbc92-default-rtdb.firebaseio.com" 
#define FIREBASE_AUTH "wy4VhHdDTtKxPNBIqzTSB9HcM0yFddrJHl2Rwlp4" 
#define WIFI_SSID "EiG" 
#define WIFI_PASSWORD "12344321" 
#define CLIENT_ID "/1"

String serverName = "http://47.202.196.192:80/air-data/latest";

int hum, voc, co2, particulate;
int hum_set, voc_set, co2_set, particulate_set;
String dtime;
FirebaseData firebaseData;
StaticJsonDocument<200> doc;
/*
 * Things to get out of doc:
 * 1. voc
 * 2. co2
 * 3. pm25
 * 4. humid
 * 
 */

bool readDataFirebase(){
  if( Firebase.getInt(firebaseData, CLIENT_ID + String("/humidity_set"), hum_set) &
  Firebase.getInt(firebaseData, CLIENT_ID + String("/voc_set"), voc_set) &
  Firebase.getInt(firebaseData, CLIENT_ID + String("/co2_set"), co2_set) &
  Firebase.getInt(firebaseData, CLIENT_ID + String("/particulate_set"), particulate_set)){
    return true;
  }
  return false;
}

bool readDataLocal(){
    if(WiFi.status()== WL_CONNECTED){
      HTTPClient http;
      String serverPath = serverName;
      http.begin(serverPath.c_str());
      int httpResponseCode = http.GET();
      
      if (httpResponseCode>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);
        deserializeJson(doc, payload);
        hum = doc["humid"];
        voc = doc["voc"];
        co2 = doc["co2"];
        particulate = doc["pm25"];
        dtime = doc["timestamp"].as<String>();
        dtime.replace("T"," ");
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      http.end();
      return true;
    }
    else {
      Serial.println("WiFi Disconnected");
      return false;
    }
}

bool setDataFirebase(){
  if( Firebase.setInt(firebaseData, CLIENT_ID + String("/humidity"), hum) &
  Firebase.setInt(firebaseData, CLIENT_ID + String("/voc"), voc) &
  Firebase.setInt(firebaseData, CLIENT_ID + String("/co2"), co2) &
  Firebase.setInt(firebaseData, CLIENT_ID + String("/particulate"), particulate) &
  Firebase.setString(firebaseData, CLIENT_ID + String("/time"), dtime)){
    return true;
  }
  return false;
}

void setRelays(){
  digitalWrite(5, (voc > voc_set)? HIGH:LOW);
  digitalWrite(5, (co2 > co2_set)? HIGH:LOW);
  digitalWrite(5, (hum > hum_set)? HIGH:LOW);
  digitalWrite(5, (particulate > particulate_set)? HIGH:LOW);
}

void setup() {
  Serial.begin(115200);
  pinMode(5, OUTPUT);
  pinMode(18, OUTPUT);
  pinMode(19, OUTPUT);
  pinMode(21, OUTPUT);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); 
  Serial.print("connecting"); 
  while (WiFi.status() != WL_CONNECTED) { 
    Serial.print("."); 
    delay(500); 
  } 
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  Firebase.setReadTimeout(firebaseData, 1000 * 20);
  Firebase.setwriteSizeLimit(firebaseData, "tiny");

}

void loop() {
  readDataLocal();
  readDataFirebase();
  setDataFirebase();
  setRelays();
  
  Serial.println("okay");

}