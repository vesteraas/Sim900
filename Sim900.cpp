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
    
    parseBuffer = (char *)malloc(128 * sizeof(char));
    commandBuffer = (char *)malloc(128 * sizeof(char));
    callerId = (char *)malloc((15 + 1) * sizeof(char));
    lastSMSText = (char *)malloc((160 + 1) * sizeof(char));
    lastSMSFrom = (char *)malloc((15 + 1) * sizeof(char));
}

Sim900::~Sim900() {
    free(parseBuffer);
    free(commandBuffer);
    free(lastSMSText);
    free(lastSMSFrom);
}

void Sim900::process_char(char character) {
    char str[2];
    static char lastCharacter;
    static char countSinceLastCRLF;
    
    // Put character into a string, because strcat() does not support a single char as second parameter
    str[0] = character;
    str[1] = '\0';
    
    if (character=='\n' && lastCharacter=='\r') { // Are we at a CRLF boundary?
        countSinceLastCRLF = 0;
        parseBuffer[strlen(parseBuffer) - 1] = '\0'; // Strip CLRLF
        
        if (strlen(parseBuffer) > 0) { // Don't bother sending empty strings
            char *line = (char *)malloc((strlen(parseBuffer) + 1) * sizeof(char));
            strcpy(line, parseBuffer);
            process_line(line);
            free(line);
        }
        
        parseBuffer[0] = '\0'; // Clear buffer
    } else {
        countSinceLastCRLF++;
        strcat(parseBuffer, str); // Append to buffer;
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
        
        char *tmp = (char *)malloc((strlen(line) - 6 + 1) * sizeof(char));
        
        memcpy(tmp, &line[6], strlen(line) - 6 + 1);
        
        char **sms = split(tmp);
        
        strcpy(lastSMSFrom, sms[0]);
        free(sms);
        free(tmp);
    } else if (prefix("+CLIP: ", line)) {
        powered = true;
        char *tmp = (char *)malloc((strlen(line) - 7 + 1) * sizeof(char));
        
        memcpy(tmp, &line[7], strlen(line) - 7 + 1);
        
        char **clip = split(tmp);
        
        strcpy(callerId, clip[0]);
        
        free(clip);
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
                if (callerId && callerId[0] == '\0') {
                    onRing(NULL);
                } else {
                    onRing(callerId);
                }
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

char** Sim900::split(const char *input) {
    const char *_input;
    
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
    
    char **result = (char **)malloc(ec * sizeof(char *));
    
    input = _input;
    
    in_quote=0;
    ec = 0;
    
    char *part = (char *)malloc(64);
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

void Sim900::setRingHandler(void (*onRing)(char *)) {
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

void Sim900::setReceiveCharFromSerial(char (*receiveChar)()) {
    this->receiveChar = receiveChar;
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

bool Sim900::hangup() {
    if (powered) {
        send("ATH\r\n");
        
        return true;
    } else {
        return false;
    }
}

bool Sim900::answer() {
    if (powered) {
        send("ATA\r\n");
        
        return true;
    } else {
        return false;
    }
}


bool Sim900::sendSMS(char *receiver, char *message) {
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

bool Sim900::sendATCommand(const char *command) {
    commandBuffer[0] = '\0';
    
    send(command);
    send("\r\n");

    do {
        char received = receiveChar();
        
        if (received) {
            char str[2];
            
            str[0] = received;
            str[1] = '\0';
            
            strcat(commandBuffer, str);
            
            if (strncmp(commandBuffer, "\r\nOK\r\n", 6) == 0) {
                return true;
            } else if (strncmp(commandBuffer, "\r\nERROR\r\n", 9) == 0) {
                return false;
            }
        } else {
            return false;
        }
    } while(true);
}

void Sim900::parse(char c) {
    process_char(c);
}