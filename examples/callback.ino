/*
 * Send an SMS to the Sim900, and it will call the number it received the SMS from
 */
#include <SoftwareSerial.h>
#include <Sim900.h>

SoftwareSerial mySerial(2, 3);

Sim900 sim900;

// The Sim900 is ready for calls
void callReadyHandler() {
  Serial.println("Call ready");
}

// The Sim900 is powered down
void powerDownHandler() {
  Serial.println("Powered down");
}

// Incoming call signal from network
void ringHandler() {
  Serial.println("Ringing...");
}

// Connection terminated / Connection attempt failed
void noCarrierHandler() {
  Serial.println("Hanged up");
}

// An SMS is received
void smsHandler(char *from, char *message) {
  Serial.println("SMS received");
  sim900.call(from);
}

// The Sim900 want to send a character to the modem
void sendCharToSerial(char c) {
  mySerial.write(c);
}

void setup() {
  Serial.begin(9600);
  mySerial.begin(19200);

  // Set callback handlers
  sim900.setCallReadyHandler(&callReadyHandler);
  sim900.setPowerDownHandler(&powerDownHandler);
  sim900.setRingHandler(&ringHandler);
  sim900.setNoCarrierHandler(&noCarrierHandler);
  sim900.setSMSHandler(&smsHandler);
  sim900.setSendCharToSerial(&sendCharToSerial);
}

void loop() {
  if (mySerial.available()) {
    char c = mySerial.read();
    sim900.parse(c);
  }
}