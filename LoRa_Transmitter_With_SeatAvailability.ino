

#include <SPI.h>
#include <LoRa.h>
#include <TinyGPS++.h>

//define the pins used by the transceiver module
#define ss 5
#define rst 14
#define dio0 2

// GPS PINS
#define RXD2 16
#define TXD2 17
HardwareSerial neogps(1);

// IR SENSOR 
#define SENSOR1 34
#define SENSOR2 35
#define SENSOR3 32


TinyGPSPlus gps;

unsigned long previousSeatCheckMillis = 0;
unsigned long previousGPSCheckMillis = 0;
const unsigned long seatCheckInterval = 1000; // Adjust the interval as needed
const unsigned long gpsCheckInterval = 1000;  // Adjust the interval as needed


void setup() {
  //initialize Serial Monitor
  Serial.begin(115200);
  neogps.begin(9600, SERIAL_8N1, RXD2, TXD2);
  pinMode(SENSOR1, INPUT_PULLUP);
  pinMode(SENSOR2, INPUT_PULLUP);
  pinMode(SENSOR3, INPUT_PULLUP);

  while (!Serial);
  Serial.println("LoRa Sender");

  //setup LoRa transceiver module
  LoRa.setPins(ss, rst, dio0);
  
  //replace the LoRa.begin(---E-) argument with your location's frequency 
  //433E6 for Asia
  //866E6 for Europe
  //915E6 for North America
  while (!LoRa.begin(915E6)) {
    Serial.println(".");
    delay(500);
  }
   // Change sync word (0xF3) to match the receiver
  // The sync word assures you don't get LoRa messages from other LoRa transceivers
  // ranges from 0-0xFF
  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa Initializing OK!");
}



void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousSeatCheckMillis >= seatCheckInterval) {
    previousSeatCheckMillis = currentMillis;
    handleSeatAndGPS();
  }
}

void handleSeatAndGPS() {
  int L = digitalRead(SENSOR3);
  int M = digitalRead(SENSOR1); // Left, middle, right sensor
  int R = digitalRead(SENSOR2);

  static int seatAvailability = 15;
  static bool mTriggeredFirst = false;
  static bool lrTriggeredFirst = false;

  if (M == 0) {
      mTriggeredFirst = true;
  } else if (R == 0 || L == 0) {
      lrTriggeredFirst = true;
  } else {
      mTriggeredFirst = false;
      lrTriggeredFirst = false;
  }

  if (mTriggeredFirst && (L == 0 || R == 0)) {
      seatAvailability += 1;
      mTriggeredFirst = false; // Reset after detecting the sequence
  }

  // Only increment if no sequence is detected
  if (lrTriggeredFirst && M == 0){
    seatAvailability -= 1;
    lrTriggeredFirst = false;
  }

  

  // Ensure seatAvailability stays within bounds
  if (seatAvailability < 0) {
    seatAvailability = 0;
  } else if (seatAvailability > 15) {
    seatAvailability = 15;
  }

  String loraSeatAvailability = String(seatAvailability); // Convert to string
  String dataSeatToSend = "Seat Availability:" + loraSeatAvailability;

  String gpsData = "No valid GPS data found.";

  smartdelay_gps(1000);

  if (gps.location.isValid()) {
    String latitude = String(gps.location.lat(), 6);
    String longitude = String(gps.location.lng(), 6);
    gpsData = "LAT:" + latitude + ",LONG:" + longitude;

    Serial.print("LAT:  ");
    Serial.println(latitude);  // Float to x decimal places
    Serial.print("LONG: ");
    Serial.println(longitude);
  } else {
    Serial.println("No valid GPS data found.");
  }

  String dataToSend = dataSeatToSend + ";" + gpsData;

  Serial.println("Seat Availability: ");
  Serial.println(loraSeatAvailability);
  Serial.println(dataToSend);

  LoRa.beginPacket();
  LoRa.print(dataToSend);
  LoRa.endPacket();
}

static void smartdelay_gps(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (neogps.available()) {
      gps.encode(neogps.read());
    }
  } while (millis() - start < ms);
}
