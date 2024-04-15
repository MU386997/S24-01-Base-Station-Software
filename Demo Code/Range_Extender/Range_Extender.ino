// includes
#include <RH_RF95.h>

// pins
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3

// constants
#define RF95_FREQ 915.0
#define RF95_TX_POWER 23
#define PACKET_SIZE_BYTES 16
#define SEND_INTERVAL 1000 // 1 second

RH_RF95 rf95(RFM95_CS, RFM95_INT);
uint8_t message[PACKET_SIZE_BYTES];
uint8_t receivedPackets[PACKET_SIZE_BYTES]; // Array to store received packets
uint8_t receivedPacketSize = 0; // Size of the received packet

unsigned long lastSendTime = 0; // Last time a packet was sent

void setup() {
  Serial.begin(9600);

  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  if (!rf95.init()) {
    Serial.println("LoRa initialization failed!");
    while (1);
  }
  rf95.setFrequency(RF95_FREQ);
  rf95.setTxPower(RF95_TX_POWER, false);
}

void loop() {
  // Check if it's time to send a packet
  if (millis() - lastSendTime >= SEND_INTERVAL) {
    // Send the stored packet, if any
    if (receivedPacketSize > 0) {
      rf95.send(receivedPackets, receivedPacketSize);
      rf95.waitPacketSent();
      Serial.println("Packet retransmitted!");
      receivedPacketSize = 0; // Clear the stored packet after sending
    }
    lastSendTime = millis(); // Update last send time
  }

  // Check for incoming packets
  if (rf95.available()) {
    uint8_t buf[PACKET_SIZE_BYTES];
    uint8_t len = sizeof(buf);
    if (rf95.recv(buf, &len)) {
      Serial.println("Received packet!");
      if (len <= PACKET_SIZE_BYTES) {
        // Store the received packet
        memcpy(receivedPackets, buf, len);
        receivedPacketSize = len;
      } else {
        Serial.println("Packet too large, discarding!");
      }
    } else {
      Serial.println("Receive failed!");
    }
  }
}
