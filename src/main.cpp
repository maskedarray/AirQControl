#include <Arduino.h>
#include "FirebaseESP32.h"
#include "WiFi.h"

#define FIREBASE_HOST "airqcontrol-dbc92-default-rtdb.firebaseio.com" 
#define FIREBASE_AUTH "wy4VhHdDTtKxPNBIqzTSB9HcM0yFddrJHl2Rwlp4" 
#define WIFI_SSID "EiG" 
#define WIFI_PASSWORD "12344321" 

FirebaseData firebaseData;
FirebaseJson json;

void setup() {
  Serial.begin(115200);
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
  // put your main code here, to run repeatedly:
  Firebase.setDouble(firebaseData,"/Sensor/data1",369);
  delay(1000);
  json.set("/data", 1786);
  Firebase.setDouble(firebaseData,"/Sensor/data1",6978);
  delay(1000);
  double a;
  Firebase.getDouble(firebaseData,"/Sensor/data1", a);
  Serial.println(a);
  Serial.println("okay");
}