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
IPAddress localip;
String header;
WiFiServer server(80);
unsigned long currentTime = millis();
unsigned long previousTime = 0; 
const long timeoutTime = 2000;
StaticJsonDocument<500> doc;
/*
 * Things to get out of doc:
 * 1. voc
 * 2. co2
 * 3. pm25
 * 4. humid
 * 
 */

String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3])  ;
}

void setRelays(){
  digitalWrite(5, (voc > voc_set)? HIGH:LOW);
  digitalWrite(18, (co2 > co2_set)? HIGH:LOW);
  digitalWrite(19, (hum > hum_set)? HIGH:LOW);
  digitalWrite(21, (particulate > particulate_set)? HIGH:LOW);
}

bool readDataFirebase(){
  if( Firebase.getInt(firebaseData, CLIENT_ID + String("/humidity_set"), hum_set) &
  Firebase.getInt(firebaseData, CLIENT_ID + String("/voc_set"), voc_set) &
  Firebase.getInt(firebaseData, CLIENT_ID + String("/co2_set"), co2_set) &
  Firebase.getInt(firebaseData, CLIENT_ID + String("/particulate_set"), particulate_set)){
    Serial.println(hum_set);
    Serial.println(voc_set);
    Serial.println(co2_set);
    Serial.println(particulate_set);
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
      
      if (httpResponseCode == 200) {
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
        dtime = dtime.substring(0,19);
        Serial.println(voc);
        Serial.println(particulate);
        Serial.println(co2);
        Serial.println(hum);
        setRelays();
        http.end();
        return true;
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
        http.end();
        return false;
      }
      
    }
    else {
      Serial.println("WiFi Disconnected");
      return false;
    }
}

bool setDataFirebase(){
  localip = WiFi.localIP();
  String temp = IpAddress2String(localip);
  Firebase.setString(firebaseData, CLIENT_ID + String("/ip"), temp);
  if( Firebase.setInt(firebaseData, CLIENT_ID + String("/humidity"), hum) &
  Firebase.setInt(firebaseData, CLIENT_ID + String("/voc"), voc) &
  Firebase.setInt(firebaseData, CLIENT_ID + String("/co2"), co2) &
  Firebase.setInt(firebaseData, CLIENT_ID + String("/particulate"), particulate) &
  Firebase.setString(firebaseData, CLIENT_ID + String("/time"), dtime)){
    return true;
  }
  return false;
}


void handleServer(){
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // turns the GPIOs on and off
            if (header.indexOf("GET /hum") >= 0) {
              client.println(hum);
            } else if (header.indexOf("GET /voc") >= 0) {
              client.println(voc);
            } else if (header.indexOf("GET /co2") >= 0) {
              client.println(co2);
            } else if (header.indexOf("GET /particulate") >= 0) {
              client.println(particulate);
            } else if (header.indexOf("GET /time") >= 0) {
              client.println(dtime);
            }
            
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
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
  server.begin();

}

void loop() {
  if(readDataLocal()){
    Serial.println("reading local data success!");
    if(readDataFirebase())
      Serial.println("reading firebase data success!");
    else
      Serial.println("reading firebase data failure!");
    if(setDataFirebase())
      Serial.println("writing firebase data success!");
    else
      Serial.println("writing firebase data failure!");
    handleServer();
  }
  else
    Serial.println("reading local data failure!");
  
  delay(1000);
  Serial.println("params:");
  Serial.println(hum);
  Serial.println(voc);
  Serial.println(co2);
  Serial.println(particulate);
  Serial.println(hum_set);
  Serial.println(voc_set);
  Serial.println(co2_set);
  Serial.println(particulate_set);

}