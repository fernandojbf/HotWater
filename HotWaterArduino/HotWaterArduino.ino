#include <SPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <string.h>
#include "Timer.h"

#define O5 3 // Relay pin (3)
#define I0 A0
#define I1 A1

WiFiUDP UDP;
IPAddress ip;
IPAddress broadcast(255, 255, 255, 255);

// wireless configurations
int status = WL_IDLE_STATUS;
char ssid[] = ""; // Insert WIFI SSID
char pass[] = ""; // Insert WIFI PASSWORD
int keyIndex = 0;

// Udp configurations
short listenPort = 5070;
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];
char  ReplyBuffer[] = "ack";

// Comunications
char orderRelayOn[] = "A00";
char orderRelayOff[] = "B00";
char success[] = "AAA";
char failure[] = "FFF";

const int maxWaterPinValue = 894;
const int minWaterPinValue = 9;

const int maxTemperaturePinValue = 1018;
const int minTemperaturePinValue = 2;

int waterValue = 0;
int temperatureValue = 0;
const int temperaturePin = I0;
const int waterPin = I1;

Timer t;


void setup() {
  // Start Serial
  Serial.begin(9600);

  // Pins Output
  pinMode(O5, OUTPUT);
  // Pins Input

  Serial.print("Try to connect ");
  Serial.println(ssid);
  status = WiFi.begin(ssid, pass);

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
  int udpStatus = 0;
  while (!udpStatus) {
    Serial.print("Try to listen port ");
    Serial.println(listenPort);
    udpStatus = UDP.begin(listenPort);
  }
  waterValue = waterInMl(analogRead(waterPin));
  temperatureValue = temperatureInCelsius(analogRead(temperaturePin));

  int broadcastEvent = t.every(500, sendBroadCastDiscovery);

  while (1) {
    
    t.update();
    int packetSize = UDP.parsePacket();
    if (packetSize) {
      int len = UDP.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
      if (len > 0) {
        packetBuffer[len] = 0;
      }

      if (strcmp(packetBuffer, "%%%KKK%%%AAA%%%") == 0) {
        IPAddress ip = UDP.remoteIP();
        Serial.print("Server ");
        Serial.print(String(ip));
        Serial.println(" added.");
        t.stop(broadcastEvent);
        break;
      }
    }
  }

  t.every(500, sendGlobalLogs);
  Serial.println("Finish Startup With Success");

}

void loop() {
  waterValue = waterInMl(analogRead(waterPin));
  temperatureValue = temperatureInCelsius(analogRead(temperaturePin));
  t.update();
  //  Serial.print("water ");
  //  Serial.println(waterValue);
  //  Serial.print("temperature ");
  //  Serial.println(temperatureValue);
  if (digitalRead(O5)) {
    checkRelay();
  }

  int packetSize = UDP.parsePacket();
  if (packetSize) {
    int len = UDP.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    if (len > 0) {
      packetBuffer[len] = 0;
    }

    String stringBuffer(packetBuffer);

    if (strcmp(packetBuffer, orderRelayOn) == 0) {
      setRelayOn();
    }
    if (strcmp(packetBuffer, orderRelayOff) == 0) {
      setRelayOff();
    }
  }
}

// Operation Funtions
int waterInMl(int value) {
  return int(((1000) / float(maxWaterPinValue - minWaterPinValue) * (value - minWaterPinValue)));
}

int temperatureInCelsius(int value) {
  return int(((120) / float(maxTemperaturePinValue - minTemperaturePinValue) * (value - minTemperaturePinValue)));
}


void setRelayOn() {
  String values[3];

  if (temperatureValue > 100 || waterValue < 200 || digitalRead(O5) ) {
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
    UDP.write("%%%A00%%%AAA%%%+++000+++%%%");
    UDP.endPacket();
  }
}

void selfRelayOff() {
  digitalWrite(O5, LOW);
}

void setRelayOff() {
  if (!digitalRead(O5) ) {
    digitalWrite(O5, LOW);
    UDP.beginPacket(ip, listenPort);
    UDP.print("%%%B00%%%FFF%%%+++FB1+++%%%");
    UDP.endPacket();
  } else {
    digitalWrite(O5, LOW);
    UDP.beginPacket(ip, listenPort);
    UDP.write("%%%B00%%%AAA%%%+++000+++%%%");
    UDP.endPacket();
  }
}

void checkRelay() {
  if (temperatureValue > 100 || waterValue < 200) {
    selfRelayOff();
  }
}

// Feedback Functions

void printWifiStatus() {
  // Print arduino ip
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}

void sendBroadCastDiscovery() {
  Serial.println("Serching Server...");
  UDP.beginPacket(broadcast, listenPort);
  UDP.print("%%%KKK%%%");
  UDP.endPacket();
}

void sendCommandResponseWithValues(char command[3], char status[3], String values[4], int loopLength) {
  UDP.beginPacket(ip, listenPort);

  UDP.print("%%%");
  UDP.print(command);
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
}

void sendGlobalLogs () {
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
}
