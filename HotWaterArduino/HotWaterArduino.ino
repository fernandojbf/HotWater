#include <SPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#define O5 3 // Relay pin (3)

WiFiUDP UDP;
IPAddress ip(192, 168, 1, 81);

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
  Serial.println("Finish Startup With Success");
}

void loop() {
  int packetSize = UDP.parsePacket();
  if (packetSize) {
    int len = UDP.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    if (len > 0) {
      packetBuffer[len] = 0;
    }

    Serial.println(packetBuffer);

    String stringBuffer(packetBuffer);

    if (strcmp(packetBuffer, orderRelayOn) == 0) {
      relayOn();
    } else if (strcmp(packetBuffer, orderRelayOff) == 0) {
      relayOff();
    }
  }
}

// Operation Funtions

void relayOn() {
  digitalWrite(O5, HIGH);
  UDP.beginPacket(ip, listenPort);
  UDP.write("Relay On");
  UDP.endPacket();
}

void relayOff() {
  digitalWrite(O5, LOW);
  UDP.beginPacket(ip, listenPort);
  UDP.write("Relay OFF");
  UDP.endPacket();
}

// Feedback Functions

void printWifiStatus() {
  // Print arduino ip
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}
