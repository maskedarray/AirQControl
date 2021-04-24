#include <Arduino.h>
#include "FirebaseESP32.h"
#include "WiFi.h"
#include "ArduinoJson.h"
#include <HTTPClient.h>
#include <ESP32Ping.h>
#include <RTClib.h>

#define FIREBASE_HOST "airqcontrol-1473a-default-rtdb.firebaseio.com" 
#define FIREBASE_AUTH "n4FwWS7EO2gyQ4aRi8Zcay4gpWaImAvJWJAgm3Ih" 
#define WIFI_SSID "EiG" 
#define WIFI_PASSWORD "12344321" 
#define CLIENT_ID "/987"
#define HYSTERISIS_TIME 120000

String serverName = "http://192.168.0.113:80/air-data/latest";
IPAddress pingip (8, 8, 8, 8);

int hum, voc, co2, particulate;
float temperature;
int hum_set, voc_set, co2_set, particulate_set;
int timezone;
long prevTime[4];
int delayTime[4];
int relay_states[4];
int pins[5] = {5,18,19,21,22};  //hum,voc,co2,pm25,comb
bool useTimed[4];
bool combo[4];
String dtime;
FirebaseData firebaseData;
IPAddress localip;
String header;
WiFiServer server(80);
unsigned long currentTime = millis();
unsigned long previousTime = 0; 
const long timeoutTime = 2000;
StaticJsonDocument<500> doc;
TaskHandle_t Shandler, TimedOp;
void vServerHandler( void *pvParameters );
void vTimedOp( void *pvParameters );
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
  bool newstate[4];
  newstate[0] = (hum > hum_set)? HIGH:LOW;
  newstate[1] = (voc > voc_set)? HIGH:LOW;
  newstate[3] = (co2 > co2_set)? HIGH:LOW;
  newstate[4] = (particulate > particulate_set)? HIGH:LOW;
  if(!useTimed[0] && ((abs(millis() - prevTime[0]) > HYSTERISIS_TIME) || !newstate[0])){
    if(newstate[0] != relay_states[0])
      prevTime[0] = millis();
    digitalWrite(pins[0], (hum > hum_set)? HIGH:LOW);
    relay_states[0] = (hum > hum_set)? 1:0;
  }
  if(!useTimed[1] && ((abs(millis() - prevTime[1]) > HYSTERISIS_TIME) || !newstate[1])){
    if(newstate[1] != relay_states[1])
      prevTime[1] = millis();
    digitalWrite(pins[1], (voc > voc_set)? HIGH:LOW);
    relay_states[1] = (voc > voc_set)? 1:0;
  }
  if(!useTimed[2] && ((abs(millis() - prevTime[2]) > HYSTERISIS_TIME) || !newstate[2])){
    if(newstate[2] != relay_states[2])
      prevTime[2] = millis();
    digitalWrite(pins[2], (co2 > co2_set)? HIGH:LOW);
    relay_states[2] = (co2 > co2_set)? 1:0;
  }
  if(!useTimed[3] && ((abs(millis() - prevTime[3]) > HYSTERISIS_TIME) || !newstate[3])){
    if(newstate[3] != relay_states[3])
      prevTime[3] = millis();
    digitalWrite(pins[3], (particulate > particulate_set)? HIGH:LOW);  
    relay_states[3] = (particulate > particulate_set)? 1:0;
  }
  if((combo[0] && (hum > hum_set)) || (combo[1] && (voc > voc_set)) || 
      (combo[2] && (co2 > co2_set)) || (combo[3] && (particulate > particulate_set))){
    digitalWrite(pins[4], HIGH);
  } else{
    digitalWrite(pins[4], LOW);
  }
}

bool readDataFirebase(){
  if( Firebase.getInt(firebaseData, CLIENT_ID + String("/humidity_set"), hum_set) &
  Firebase.getInt(firebaseData, CLIENT_ID + String("/voc_set"), voc_set) &
  Firebase.getInt(firebaseData, CLIENT_ID + String("/co2_set"), co2_set) &
  Firebase.getInt(firebaseData, CLIENT_ID + String("/particulate_set"), particulate_set) &
  Firebase.getInt(firebaseData, CLIENT_ID + String("/hum_t"), delayTime[0]) &
  Firebase.getInt(firebaseData, CLIENT_ID + String("/voc_t"), delayTime[1]) &
  Firebase.getInt(firebaseData, CLIENT_ID + String("/co2_t"), delayTime[2]) &
  Firebase.getInt(firebaseData, CLIENT_ID + String("/part_t"), delayTime[3]) &
  Firebase.getBool(firebaseData, CLIENT_ID + String("/hum_t_b"), useTimed[0]) &
  Firebase.getBool(firebaseData, CLIENT_ID + String("/voc_t_b"), useTimed[1]) &
  Firebase.getBool(firebaseData, CLIENT_ID + String("/co2_t_b"), useTimed[2]) &
  Firebase.getBool(firebaseData, CLIENT_ID + String("/part_t_b"), useTimed[3]) &
  Firebase.getBool(firebaseData, CLIENT_ID + String("/hum_c"), combo[0]) &
  Firebase.getBool(firebaseData, CLIENT_ID + String("/voc_c"), combo[1]) &
  Firebase.getBool(firebaseData, CLIENT_ID + String("/co2_c"), combo[2]) &
  Firebase.getBool(firebaseData, CLIENT_ID + String("/part_c"), combo[3]) &
  Firebase.getInt(firebaseData, CLIENT_ID + String("/timezone"), timezone)
  ){
    Serial.println("Settings: ");
    Serial.println(hum_set);
    Serial.println(voc_set);
    Serial.println(co2_set);
    Serial.println(particulate_set);
    Serial.printf("Timed0: %d",useTimed[0]);
    Serial.printf("Timed1: %d",useTimed[1]);
    Serial.printf("Timed2: %d",useTimed[2]);
    Serial.printf("Timed3: %d",useTimed[3]);
    Serial.printf("Time0: %d",delayTime[0]);
    Serial.printf("Time1: %d",delayTime[1]);
    Serial.printf("Time2: %d",delayTime[2]);
    Serial.printf("Time3: %d",delayTime[3]);
    Serial.printf("combo0: %d",combo[0]);
    Serial.printf("combo1: %d",combo[1]);
    Serial.printf("combo2: %d",combo[2]);
    Serial.printf("combo3: %d",combo[3]);
    setRelays();
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
        temperature = doc["temp"];
        temperature = (temperature*1.8) + 32;
        dtime = doc["timestamp"].as<String>();
        dtime.replace("T"," ");
        dtime = dtime.substring(0,19);
        int iyear = dtime.substring(0,4).toInt();
        int imonth = dtime.substring(5,7).toInt();
        int iday = dtime.substring(8,10).toInt();
        int ihr = dtime.substring(11,13).toInt();
        int imin = dtime.substring(14,16).toInt();
        int isec = dtime.substring(17,19).toInt();
        DateTime temp1(iyear,imonth, iday, ihr, imin, isec);
        TimeSpan temp2(0,timezone, 0,0);
        temp1 = temp1 + temp2;
        dtime = String(temp1.year()) + "-" + String(temp1.month()) + "-" + String(temp1.day()) + " " + String(temp1.hour()) + ":" + String(temp1.minute()) + ":" + String(temp1.second());
        Serial.println(hum);
        Serial.println(voc);
        Serial.println(co2);
        Serial.println(particulate);
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
  Firebase.setInt(firebaseData, CLIENT_ID + String("/temp"), (int)round(temperature)) &
  Firebase.setInt(firebaseData, CLIENT_ID + String("/hum_state"), relay_states[0]) &
  Firebase.setInt(firebaseData, CLIENT_ID + String("/voc_state"), relay_states[1]) &
  Firebase.setInt(firebaseData, CLIENT_ID + String("/co2_state"), relay_states[2]) &
  Firebase.setInt(firebaseData, CLIENT_ID + String("/part_state"), relay_states[3]) &
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
              client.print(hum);
            } else if (header.indexOf("GET /voc") >= 0) {
              client.print(voc);
            } else if (header.indexOf("GET /co2") >= 0) {
              client.print(co2);
            } else if (header.indexOf("GET /particulate") >= 0) {
              client.print(particulate);
            } else if (header.indexOf("GET /time") >= 0) {
              client.print(dtime);
            }
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
  for (int i=0; i<5; i++){
    pinMode(pins[i], OUTPUT);
  }
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  int nwf = WiFi.scanNetworks();
  Serial.println("scan done");
  if (nwf == 0) {
    Serial.println("no networks found");
  } else {
    Serial.print(nwf);
    Serial.println(" networks found");
    for (int i = 0; i < nwf; ++i) {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
      delay(10);
    }
  }
  WiFi.mode(WIFI_STA);
  delay(1000);
  WiFi.disconnect();
  delay(1000);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); 
  Serial.print("connecting"); 
  int wf_con_counter = 0;
  while (WiFi.status() != WL_CONNECTED) { 
    Serial.print("."); 
    wf_con_counter++;
    if (wf_con_counter > 20){
      WiFi.disconnect();
      delay(1000);
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD); 
    }
    delay(500); 
  }  
  digitalWrite(2, HIGH);
  Serial.println("Connected");
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  Firebase.setReadTimeout(firebaseData, 1000 * 20);
  Firebase.setwriteSizeLimit(firebaseData, "tiny");
  // server.begin();
  // xTaskCreate(vServerHandler, "handles server", 50000, NULL, 2, &Shandler);
  xTaskCreate(vTimedOp, "timed relay operation", 10000, NULL, 2, &TimedOp);
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
    }
    else
      Serial.println("reading local data failure!");
    vTaskDelay(5000);
}


void vServerHandler ( void *pvParameters ){
  for(;;){
    handleServer();
    vTaskDelay(10);
  }
}

void vTimedOp( void *pvParameters ){
  int counter = 0;
  for(;;){
    for (int i = 0; i < 4; i++){
      if(useTimed[i]){
        if (counter < delayTime[i]){ 
          digitalWrite(pins[i], HIGH);
          relay_states[i] = 1;
        } else{
          digitalWrite(pins[i], LOW);
          relay_states[i] = 0;
        }
      }
    }
    counter++;
    if (counter >= 60 ){
      counter = 0;
    }
    vTaskDelay(60000);
  }
}