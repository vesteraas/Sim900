//
//  Created by Werner Vesterås on 25/04/14.
//  Copyright (c) 2014 Werner Vesterås. All rights reserved.
//

#ifndef __Sim900__Sim900__
#define __Sim900__Sim900__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

typedef enum { CREG, CMGR, CMGL } CMD;

class Sim900 {
private:
    bool powered;
    bool ready;
    bool okReceived;
    bool errorReceived;
    
    bool messageSent;
    
    char *buffer;
    char *lastSMSText;
    char *lastSMSFrom;
    char *messageToSend;
    
    void process_char(char c);
    bool prefix(const char *pre, const char *str);
    void process_line(char *line);
    char** split(const char* input);
    void send(const char *str);
    
    void (*onCallReady)();
    void (*onPowerDown)();
    void (*onRing)();
    void (*onNoCarrier)();
    void (*onSMS)(char*, char*);
    void (*sendChar)(char);
    
public:
    Sim900(void);
    virtual ~Sim900();
    
    void setCallReadyHandler(void (*onCRDY)());
    void setPowerDownHandler(void (*onPWRDWN)());
    void setRingHandler(void (*onRING)());
    void setNoCarrierHandler(void (*onNoCarrier)());
    void setSMSHandler(void (*onSMS)(char*, char*));
    void setSendCharToSerial(void (*sendChar)(char));
    
    bool call(char *recipient);
    bool sendSMS(char* receiver, char* message);
    
    void parse(char character);
};

#endif /* defined(__Sim900__Sim900__) */
