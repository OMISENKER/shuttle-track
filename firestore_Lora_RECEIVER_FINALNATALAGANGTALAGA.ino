
#include <SPI.h>
#include <Arduino.h>
#if defined(ESP32)
  #include <WiFiManager.h> 
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <LoRa.h>


#include <Firebase_ESP_Client.h>

//Provide the token generation process info.
#include "addons/TokenHelper.h"



// Insert Firebase project API Key
#define API_KEY "AIzaSyCkNFnLbmsKVK8dIL3VqZWhILDddiBpk0w"

#define FIREBASE_PROJECT_ID "shuttletrack-424318"

FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;
FirebaseJson content;

WiFiManager wm;


//define the pins used by the transceiver module
#define ss 5
#define rst 14
#define dio0 2

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

void setup() {
  WiFi.mode(WIFI_STA);
  //initialize Serial Monitor
  Serial.begin(115200);
  
  

  
  while (!Serial);
  Serial.println("LoRa Receiver");

  //setup LoRa transceiver module
  LoRa.setPins(ss, rst, dio0);
  
 
  while (!LoRa.begin(915E6)) {
    Serial.println(".");
    delay(500);
  }
   // Change sync word (0xF3) to match the receiver
  // The sync word assures you don't get LoRa messages from other LoRa transceivers
  // ranges from 0-0xFF
  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa Initializing OK!");

  bool res;

    res = wm.autoConnect("ShuttleTrack","password"); // password protected ap

    if(!res) {
        Serial.println("Failed to connect");
        ESP.restart();
    } 
    else {
        //if you get here you have connected to the WiFi    
        Serial.println("connected...yeey :)");
    }
  

  /* Assign the api key (required) */
  config.api_key = API_KEY;

 

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {

  String documentPath = "Philippines/Locations";

  // try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // received a packet
    Serial.println("Received packet:");
    
    String incoming = "";
    
  while (LoRa.available()) {
      incoming += (char)LoRa.read();
    }
    Serial.println(incoming);
    // Parse out the latitude, longitude and seat
    int latIndex = incoming.indexOf("LAT:");
    int lonIndex = incoming.indexOf(",LONG:");
    int seatIndex = incoming.indexOf("Seat Availability:");
 
    float lat = incoming.substring(latIndex + 4).toFloat();
    float lon = incoming.substring(lonIndex + 6).toFloat();
    int seat = incoming.substring(seatIndex + 18).toInt();
 

    content.set("fields/Latitude/stringValue", String(lat, 6));
    content.set("fields/Longitude/stringValue", String(lon, 6));
    content.set("fields/Seat/stringValue", String(seat));
    // Reformat the strings with specified number of digits after decimal
    String latString = String(lat, 6);
    String lonString = String(lon, 6);
    String seatString = String(seat); 
    
    Serial.print("Latitude: ");
    Serial.println(latString);
    Serial.print("Longitude: ");
    Serial.println(lonString);
    Serial.print("Seat Availability: ");
    Serial.println(seatString);

    //-------------------------------------------------------------
    if(Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "Latitude"))
      {print_ok();}
    else
      {print_fail();}
    //-------------------------------------------------------------
    if(Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "Longitude"))
      {print_ok();}
    else
      {print_fail();}
   //-------------------------------------------------------------
     if(Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "Seat"))
      {print_ok();}
    else
      {print_fail();}
   //-------------------------------------------------------------
    
    // print RSSI of packet
    Serial.print("RSSI: ");
    Serial.println(LoRa.packetRssi());
    delay(1000);
  }
}


void print_fail()
{
    Serial.println("------------------------------------");
    Serial.println("FAILED");
    Serial.println("REASON: " + fbdo.errorReason());
    Serial.println("------------------------------------");
    Serial.println();
}

void print_ok()
{
    Serial.println("------------------------------------");
    Serial.println("OK");
    Serial.println("PATH: " + fbdo.dataPath());
    Serial.println("TYPE: " + fbdo.dataType());
    Serial.println("ETag: " + fbdo.ETag());
    Serial.println("------------------------------------");
    Serial.println();
}