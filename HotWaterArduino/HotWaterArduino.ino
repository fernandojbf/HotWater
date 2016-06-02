// Fernando Ferreira
// 2016

#include <SPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <OneWire.h>
#include <string.h>
#include "Timer.h"


#define O5 3 // Digital pin 3 (Relay)
#define I0 A0 // Temperature Potentiometer (Not used anymore)
#define I1 A1 // Water Potentiometer
#define NUM_READS 200 // Number of iteration to remove water noise
#define DS18S20_Pin 2 //Digital 2 (DS18S20 Temperature Sensor)
#define temperaturePin I0 // Tempperature  Pin (Not Used anymore)
#define waterPin I1 // Water pin

OneWire ds(DS18S20_Pin);

WiFiUDP UDP;

IPAddress ip(192, 168, 10, 89); // Raspi IP (HARDCODED)
IPAddress broadcast(255, 255, 255, 255); // BroadCast IP

// wireless configurations
int status = WL_IDLE_STATUS;
char ssid[] = "SE1516"; // Insert WIFI SSID
char pass[] = ""; // Insert WIFI PASSWORD
int keyIndex = 0;

// Udp configurations
short listenPort = 5070;
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];

// Comunications
char orderRelayOn[] = "A00";
char orderRelayOff[] = "B00";
char success[] = "AAA";
char failure[] = "FFF";

const int maxWaterPinValue = 894; // MAX Water Potentiometer Value
const int minWaterPinValue = 9; // MIN Water Potentiometer Value

const int maxTemperaturePinValue = 1018; // MAX Temperature Potentiometer Value (NOT USED ANYMORE)
const int minTemperaturePinValue = 2; // MIN Temperature Potentiometer Value (NOT USED ANYMORE)

int waterValue = 0; // Water Value
int temperatureValue = 0; // Temperature Value

int logWater = 0; //  Last Water Log
int logTemperature = 0; // Last Temperature Log
int logRelayOn = 0; // Last Realy Log

Timer t; // Set Timer


void setup() {
  // Start Serial
  Serial.begin(9600);

  // Pins Output
  pinMode(O5, OUTPUT);

  Serial.print("Try to connect ");
  Serial.println(ssid);
  status = WiFi.begin(ssid);

  // Wifi connection failed
  if ( status != WL_CONNECTED) {
    Serial.println("Couldn't get a wifi connection");
    while (true);
  }
  // Wifi Connected
  else {
    Serial.println("Connected");
    printWifiStatus();
  }
  
  // Listen Port
  int udpStatus = 0;
  while (!udpStatus) {
    Serial.print("Try to listen port ");
    Serial.println(listenPort);
    udpStatus = UDP.begin(listenPort);
  }
  // Set BroadCast Send Message Timer
  int broadcastEvent = t.every(500, sendBroadCastDiscovery);

  // Search For Server (not used WiFiUDP bug)
  while (1) {
    t.update();
    int packetSize = UDP.parsePacket();
    if (packetSize) {
      int len = UDP.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
      if (len > 0) {
        packetBuffer[len] = 0;
      }

      if (strcmp(packetBuffer, "%%%KKK%%%AAA%%%") == 0) {
        //        IPAddress ip = UDP.remoteIP();
        Serial.print("Server ");
        Serial.print(String(ip));
        Serial.println(" added.");
        t.stop(broadcastEvent);
        break;
      }
    }
  }

  // Update Values
  waterValue = waterInMl(analogRead(waterPin));
  temperatureValue = temperatureInCelsius(analogRead(temperaturePin));

  t.every(500, sendGlobalLogs); // Send logs 0.5 seg (if values change) Timer
  t.every(30000, sendlLogs); // Send logs 30 seg Timer
  
  Serial.println("Finish Startup With Success");

}

void loop() {
  // Update Values
  waterValue = waterInMl(waterPin);
  temperatureValue = int(getTemp());
  // Update Timer
  t.update();

  // Check Conditions if Relay ON
  if (digitalRead(O5)) {
    checkRelay();
  }
  // Read Package
  int packetSize = UDP.parsePacket();
  if (packetSize) {
    int len = UDP.read(packetBuffer, 3);
    if (len > 0) {
      packetBuffer[len] = 0;
    }

    String stringBuffer(packetBuffer);
    Serial.println(stringBuffer);

    if (strcmp(packetBuffer, orderRelayOn) == 0) {
      setRelayOn();
    }
    if (strcmp(packetBuffer, orderRelayOff) == 0) {
      setRelayOff();
    }
  }
}

// Operation Funtions

// Water Value to Ml
int waterInMl(int sensorpin) {
  int sortedValues[NUM_READS];
  for (int i = 0; i < NUM_READS; i++) {
    int value = analogRead(sensorpin);
    int j;
    if (value < sortedValues[0] || i == 0) {
      j = 0; //insert at first position
    }
    else {
      for (j = 1; j < i; j++) {
        if (sortedValues[j - 1] <= value && sortedValues[j] >= value) {
          // j is insert position
          break;
        }
      }
    }
    for (int k = i; k > j; k--) {
      // move all values higher than current reading up one position
      sortedValues[k] = sortedValues[k - 1];
    }
    sortedValues[j] = value; //insert current reading
  }
  //return scaled mode of 10 values
  float returnval = 0;
  for (int i = NUM_READS / 2 - 5; i < (NUM_READS / 2 + 5); i++) {
    returnval += sortedValues[i];
  }
  returnval = returnval / 10;
  returnval =  returnval * 1100 / 1023;

  return int(((1000) / float(maxWaterPinValue - minWaterPinValue) * (returnval - minWaterPinValue)));
}
// Potentiometer Temp (Not used anymore)
int temperatureInCelsius(int value) {
  return int(((120) / float(maxTemperaturePinValue - minTemperaturePinValue) * (value - minTemperaturePinValue)));
}

// Relay ON Order
void setRelayOn() {
  String values[3];

  if (temperatureValue > 70 || waterValue < 200 || digitalRead(O5) ) {
    if (digitalRead(O5)) {
      values[0] =  "AF1";

    }
    if (temperatureValue > 100) {
      values[1] =  "AF2";

    }
    if (waterValue < 200) {
      values[2] =  "AF3";
    }

    sendCommandResponseWithValues("A00", "FFF", values, 3);

  } else {
    digitalWrite(O5, HIGH);
    UDP.beginPacket(ip, listenPort);
    UDP.print("---");
    UDP.endPacket();
    delay(500);
    UDP.beginPacket(ip, listenPort);
    UDP.print("%%%A00%%%AAA%%%+++000+++%%%");
    UDP.endPacket();
  }
}
// Relay OFF Order
void setRelayOff() {
  if (!digitalRead(O5) ) {
    UDP.beginPacket(ip, listenPort);
    UDP.print("---");
    UDP.endPacket();
    delay(500);
    UDP.beginPacket(ip, listenPort);
    UDP.print("%%%B00%%%FFF%%%+++FB1+++%%%");
    UDP.endPacket();
  } else {
    digitalWrite(O5, LOW);
    UDP.beginPacket(ip, listenPort);
    UDP.print("---");
    UDP.endPacket();
    delay(500);
    UDP.beginPacket(ip, listenPort);
    UDP.write("%%%B00%%%AAA%%%+++000+++%%%");
    UDP.endPacket();
  }

}

// Relay OFF Without Feedback
void selfRelayOff() {
  digitalWrite(O5, LOW);
}

// Check Conditions
void checkRelay() {
  if (temperatureValue > 70 || waterValue < 200) {
    selfRelayOff();
  }
}

// returns the temperature from one DS18S20 in DEG Celsius
float getTemp() {

  byte data[12];
  byte addr[8];

  if ( !ds.search(addr)) {
    //no more sensors on chain, reset search
    ds.reset_search();
    return -1000;
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
    Serial.println("CRC is not valid!");
    return -1000;
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) {
    Serial.print("Device is not recognized");
    return -1000;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1); // start conversion, with parasite power on at the end

  byte present = ds.reset();
  ds.select(addr);
  ds.write(0xBE); // Read Scratchpad


  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds.read();
  }

  ds.reset_search();

  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;

  return TemperatureSum;

}

// Feedback Functions

// Wifi Info
void printWifiStatus() {
  // Print arduino ip
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}

// BroadCast Discovery
void sendBroadCastDiscovery() {
  Serial.println("Serching Server...");
  UDP.beginPacket(broadcast, listenPort);
  UDP.print("%%%KKK%%%");
  UDP.endPacket();
  delay(1);
}

// Send Package With Values
void sendCommandResponseWithValues(char command[3], char status[3], String values[4], int loopLength) {
  UDP.beginPacket(ip, listenPort);
  UDP.print("---");
  UDP.endPacket();

  delay(500);

  UDP.beginPacket(ip, listenPort);
  UDP.print("%%%");
  UDP.print("A00");
  UDP.print("%%%");
  UDP.print(status);
  UDP.print("%%%");
  UDP.print("+++");
  for (int index = 0; index < loopLength; index++) {
    if (values[index].length() != 0) {
      UDP.print(values[index]);
      UDP.print("+++");
    }
  }
  UDP.print("%%%");
  UDP.endPacket();
  delay(1);
}

// Send logs if values change
void sendGlobalLogs () {
  // Relay LOG

  if (logRelayOn != digitalRead(O5)) {
    UDP.beginPacket(ip, listenPort);
    UDP.print("%%%");
    UDP.print("L00");
    UDP.print("%%%");
    UDP.print("+++");
    if (digitalRead(O5)) {
      UDP.print("L0A");
    } else {
      UDP.print("L0F");
    }
    UDP.print("+++");
    UDP.print("%%%");
    UDP.endPacket();
    delay(1);
    logRelayOn = digitalRead(O5);
  }

  if (logTemperature != temperatureValue) {
    // Temperature LOG
    UDP.beginPacket(ip, listenPort);
    UDP.print("%%%");
    UDP.print("L01");
    UDP.print("%%%");
    UDP.print("+++");
    UDP.print(temperatureValue);
    UDP.print("+++");
    UDP.print("%%%");
    UDP.endPacket();
    delay(1);

    logTemperature = temperatureValue;
  }

  if (logWater != waterValue) {
    // water LOG
    UDP.beginPacket(ip, listenPort);
    UDP.print("%%%");
    UDP.print("L02");
    UDP.print("%%%");
    UDP.print("+++");
    UDP.print(waterValue);
    UDP.print("+++");
    UDP.print("%%%");
    UDP.endPacket();
    delay(1);

    logWater = waterValue;
  }

}

// Send Logs
void sendlLogs () {
  // Relay LOG
  UDP.beginPacket(ip, listenPort);
  UDP.print("%%%");
  UDP.print("L00");
  UDP.print("%%%");
  UDP.print("+++");
  if (digitalRead(O5)) {
    UDP.print("L0A");
  } else {
    UDP.print("L0F");
  }
  UDP.print("+++");
  UDP.print("%%%");
  UDP.endPacket();
  delay(1);
  logRelayOn = digitalRead(O5);


  // Temperature LOG
  UDP.beginPacket(ip, listenPort);
  UDP.print("%%%");
  UDP.print("L01");
  UDP.print("%%%");
  UDP.print("+++");
  UDP.print(temperatureValue);
  UDP.print("+++");
  UDP.print("%%%");
  UDP.endPacket();
  delay(1);

  logTemperature = temperatureValue;

  // water LOG
  UDP.beginPacket(ip, listenPort);
  UDP.print("%%%");
  UDP.print("L02");
  UDP.print("%%%");
  UDP.print("+++");
  UDP.print(waterValue);
  UDP.print("+++");
  UDP.print("%%%");
  UDP.endPacket();
  delay(1);

  logWater = waterValue;
}
