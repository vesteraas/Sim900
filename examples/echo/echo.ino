/*
 * Send an SMS to the Sim900, and it will send the same message back to the sender
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
void ringHandler(char *callId) {
  Serial.print(callId);
  Serial.println(" is calling...");
}

// Connection terminated / Connection attempt failed
void noCarrierHandler() {
  Serial.println("Hanged up");
}

// An SMS is received
void smsHandler(char *from, char *message) {
  Serial.println("SMS received");
  sim900.sendSMS(from, message);
}

// The Sim900 want to send a character to the modem
void sendCharToSerial(char c) {
  mySerial.write(c);
}

// Try to read a character from the modem
char receiveCharFromSerial() {
  long start = millis();

  bool available = false;
  
  do {
    available = mySerial.available();
  } while(available == 0 && (millis() - start) < 2000);

  return available ? mySerial.read() : '\0';
}

void setup() {
  // Callback handlers for serial communication, must be set BEFORE using sendATCommand
  sim900.setSendCharToSerial(&sendCharToSerial);
  sim900.setReceiveCharFromSerial(&receiveCharFromSerial);

  Serial.begin(19200);
  mySerial.begin(19200);
  
  // Disable local echo
  sim900.sendATCommand("ATE0");

  // Enable Caller ID
  sim900.sendATCommand("AT+CLIP=1");

  // SMS'es will be in text format
  sim900.sendATCommand("AT+CMGF=1");

  // Set callback handlers
  sim900.setCallReadyHandler(&callReadyHandler);
  sim900.setPowerDownHandler(&powerDownHandler);
  sim900.setRingHandler(&ringHandler);
  sim900.setNoCarrierHandler(&noCarrierHandler);
  sim900.setSMSHandler(&smsHandler);
}

void loop() {
  if (mySerial.available()) {
    char c = mySerial.read();
    sim900.parse(c);
  }
}