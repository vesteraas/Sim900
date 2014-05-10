//
//  Created by Werner Vesterås on 25/04/14.
//  Copyright (c) 2014 Werner Vesterås. All rights reserved.
//

#include "Sim900.h"

Sim900::Sim900(void) {
    powered = false;
    ready = false;
    okReceived = false;
    errorReceived = false;
    
    messageSent = false;
    
    buffer = (char *)malloc(128 * sizeof(char));
    lastSMSText = (char *)malloc((160 + 1) * sizeof(char));
    lastSMSFrom = (char *)malloc((15 + 1) * sizeof(char));
}

Sim900::~Sim900() {
    free(buffer);
    free(lastSMSText);
    free(lastSMSFrom);
}

void Sim900::process_char(char character) {
    static char str[2];
    static char lastCharacter;
    static char countSinceLastCRLF;
    
    // Put character into a string, because strcat() does not support a single char as second parameter
    str[0] = character;
    str[1] = '\0';
    
    if (character=='\n' && lastCharacter=='\r') { // Are we at a CRLF boundary?
        countSinceLastCRLF = 0;
        buffer[strlen(buffer) - 1] = '\0'; // Strip CLRLF
        
        if (strlen(buffer) > 0) { // Don't bother sending empty strings
            char *line = (char *)malloc((strlen(buffer) + 1) * sizeof(char));
            strcpy(line, buffer);
            process_line(line);
            free(line);
        }
        
        buffer[0] = '\0'; // Clear buffer
    } else {
        countSinceLastCRLF++;
        strcat(buffer, str); // Append to buffer;
    }
    
    if (character==' ' && lastCharacter=='>' && countSinceLastCRLF == 2) { // Prompt to send message received
        if (powered) {
            send(messageToSend);
            str[0] = (char)26; // CTRL+Z
            send(str);
        }
    }
    
    lastCharacter = character;
}

bool Sim900::prefix(const char *pre, const char *str) {
    return strncmp(pre, str, strlen(pre)) == 0;
}

void Sim900::process_line(char *line) {
    static int smsReceived;
    
    if (prefix("+CMT: ", line)) {
        smsReceived = true;
        powered = true;
        
        char *tmp = (char *)malloc((strlen(line) - 6) * sizeof(char));
        
        memcpy(tmp, &line[6], strlen(line) - 6);
        
        char **sms = split(tmp);
        
        strcpy(lastSMSFrom, sms[0]);
        free(sms);
        free(tmp);
    } else {
        if (strncmp(line, "OK", 2) == 0) {
            okReceived = true;
            errorReceived = false;
            powered = true;
        } else if (strncmp(line, "ERROR", 5) == 0) {
            okReceived = false;
            errorReceived = true;
            powered = true;
        } else if (strncmp(line, "RING", 4) == 0) {
            okReceived = false;
            errorReceived = true;
            powered = true;
            if (onRing) {
                onRing();
            }
        } else if (strncmp(line, "NO CARRIER", 10) == 0) {
            okReceived = false;
            errorReceived = true;
            powered = true;
            if (onNoCarrier) {
                onNoCarrier();
            }
        } else if (strncmp(line, "Call Ready", 10) == 0) {
            okReceived = false;
            errorReceived = false;
            ready = true;
            powered = true;
            if (onCallReady) {
                onCallReady();
            }
        } else if (strncmp(line, "NORMAL POWER DOWN", 10) == 0) {
            okReceived = false;
            errorReceived = false;
            ready = false;
            powered = false;
            if (onPowerDown) {
                onPowerDown();
            }
        } else {
            if (smsReceived) {
                strcpy(lastSMSText, line);

                if (onSMS) {
                    onSMS(lastSMSFrom, lastSMSText);
                }

                smsReceived = false;
            }
            
            okReceived = false;
            errorReceived = false;
            powered = true;
        }
    }
}

char** Sim900::split(const char* input) {
    const char* _input;
    
    _input = input;
    
    bool in_quote = false;
    int ec = 1;
    
    while (*input) {
        if (*input == '"') {
            if (in_quote) {
                in_quote = false;
            } else {
                in_quote = true;
            }
        } else if (*input == ',') {
            if (!in_quote) {
                ec++;
            }
        }
        
        input++;
    }
    
    char **result = (char **)malloc(ec * sizeof(char*));
    
    input = _input;
    
    in_quote=0;
    ec = 0;
    
    char* part = (char *)malloc(64);
    strcpy(part, "");
    
    while (*input) {
        if (*input == '"') {
            if (in_quote) {
                in_quote = false;
            } else {
                in_quote = true;
            }
        } else if (*input == ',') {
            if (!in_quote) {
                result[ec] = (char *)malloc(64);
                strcpy(result[ec], part);
                part[0] = '\0';
                ec++;
            } else {
                char cb[2];
                cb[0] = (char)*input;
                cb[1]='\0';
                strcat(part, cb);
            }
        } else {
            char cb[2];
            cb[0] = (char)*input;
            cb[1]='\0';
            strcat(part, cb);
        }
        
        input++;
    }
    
    result[ec] = (char *)malloc(64);
    strcpy(result[ec], part);
    
    free(part);
    
    return result;
}

void Sim900::send(const char *str) {
    for(int i=0; str[i]; i++) {
        if (sendChar) {
            sendChar(str[i]);
        }
    }
}

void Sim900::setCallReadyHandler(void (*onCallReady)()) {
    this->onCallReady = onCallReady;
}

void Sim900::setPowerDownHandler(void (*onPowerDown)()) {
    this->onPowerDown = onPowerDown;
}

void Sim900::setRingHandler(void (*onRing)()) {
    this->onRing = onRing;
}

void Sim900::setNoCarrierHandler(void (*onNoCarrier)()) {
    this->onNoCarrier = onNoCarrier;
}

void Sim900::setSMSHandler(void (*onSMS)(char *, char *)) {
    this->onSMS = onSMS;
}

void Sim900::setSendCharToSerial(void (*sendChar)(char)) {
    this->sendChar = sendChar;
}

bool Sim900::call(char *recipient) {
    if (powered) {
        send("ATD");
        send(recipient);
        send(";\r\n");
        
        return true;
    } else {
        return false;
    }
}

bool Sim900::sendSMS(char* receiver, char* message) {
    this->messageToSend = message;
    if (powered) {
        send("AT+CMGS=\"");
        send(receiver);
        send("\"\r\n");
        return true;
    } else {
        return false;
    }
}

void Sim900::parse(char c) {
    process_char(c);
}