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
FirebaseData firebaseData;

// bool readDataFirebase(){
//   if( Firebase.getInt(firebaseData, CLIENT_ID + String("/humidity_set"), hum_set) &
//   Firebase.getInt(firebaseData, CLIENT_ID + String("/voc_set"), voc_set) &
//   Firebase.getInt(firebaseData, CLIENT_ID + String("/co2_set"), co2_set) &
//   Firebase.getInt(firebaseData, CLIENT_ID + String("/particulate_set"), particulate_set)){
//     return true;
//   }
//   return false;
// }

// bool readDataLocal(){

// }

// bool setDataFirebase(){

// }

// bool setDataLocal(){

// }

void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); 
  Serial.print("connecting"); 
  while (WiFi.status() != WL_CONNECTED) { 
    Serial.print("."); 
    delay(500); 
  } 
  // Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  // Firebase.reconnectWiFi(true);
  // Firebase.setReadTimeout(firebaseData, 1000 * 20);
  // Firebase.setwriteSizeLimit(firebaseData, "tiny");

}
StaticJsonDocument<200> doc;
void loop() {
  // readDataFirebase();
  // Firebase.setDouble(firebaseData,"/Sensor/data1",369);
  // delay(1000);
  // Firebase.setDouble(firebaseData,"/Sensor/data1",6978);
  // delay(1000);
  // double a;
  // Firebase.getDouble(firebaseData,"/Sensor/data1", a);
  // Serial.println(a);
  // Serial.println("okay");
  if(WiFi.status()== WL_CONNECTED){
      HTTPClient http;

      String serverPath = serverName;
      
      // Your Domain name with URL path or IP address with path
      http.begin(serverPath.c_str());
      
      // Send HTTP GET request
      int httpResponseCode = http.GET();
      
      if (httpResponseCode>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);
        deserializeJson(doc, payload);
        double res = doc["humid"];
        Serial.println(res);
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      // Free resources
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
}