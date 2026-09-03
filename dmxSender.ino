#define DMX_USE_PORT1
#include <DMXSerial.h>
#include <Wire.h>
#include <pins_arduino.h>
#include "pattern.h"

#define version "Version 0.97"
#define TESTVERSION 0
unsigned long msglen = 482 + TESTVERSION;  // test + sequencenr + 16*30
uint8_t hoek = 0;
uint8_t test = 0;
char buffer[17];
#define cols 48  // 2x 24 motors

uint8_t msgbuffer[2 * cols];  // 2 rows of 2x24 motors
uint8_t bline[8];             // storage for motors 8-15
const uint8_t totalsteps = (sizeof(pattern) + sizeof(restpattern)) / (msglen * sizeof(pattern[0]));
unsigned long time;
unsigned long now;
unsigned long delta;
int tmp = 0;
bool stop = false;
uint8_t to;
long timeout;

const byte ledPin1 = 6;
const byte ledPin2 = 7;
const byte startPin = 9;  //not 2;  // input pin that the interruption will be attached to
const byte stopPin = 10;  //not 2;  // input pin that the interruption will be attached to

uint8_t step;
unsigned long offset;
const uint8_t* data;
int channel;

void setup() {
  Serial.begin(9600);  //define baud rate
  DMXSerial.init(DMXController);        // pin 2 is used for direction;
  DMXSerial.maxChannel(DMXSERIAL_MAX);  //32*16 = 512; msglen < DMXSERIAL_MAX
  for (uint8_t i = 1; i < 11; i++) {
    //    home(i);
  }
  fullhouse();


  pinMode(ledPin1, OUTPUT);
  digitalWrite(ledPin1, HIGH);
  pinMode(startPin, INPUT_PULLUP);
  //  attachInterrupt(digitalPinToInterrupt(startPin), restart, CHANGE);

  pinMode(ledPin2, OUTPUT);
  digitalWrite(ledPin2, HIGH);
  pinMode(stopPin, INPUT_PULLUP);
  //  attachInterrupt(digitalPinToInterrupt(stopPin), stopNow, CHANGE);

  snprintf(buffer, 16, __DATE__); LogLine(buffer);
  snprintf(buffer, 16, __TIME__); LogLine(buffer);
  snprintf(buffer, 16, version); LogLine(buffer);
}

/*
void restart() {
  cli();
  step = 0;
  stop = false;
  sei();
}
void stopNow() {
  cli();
  stop = true;
  sei();
}
*/
void LogLine(const char* s) {
  Serial.println(s);
}

void CheckSerial() {
  if (Serial.available() > 0)  //if something comes
  {
    char receivedCommand = Serial.read();  // this will read the command character
    int parsedInt;
    switch (receivedCommand) {
      case 't':
        test = Serial.parseInt();
        break;
      default:
        snprintf(buffer, 16, "received %c", receivedCommand);
        LogLine(buffer);
        break;
    }
  }
}

void dmxWrite(int channel, uint8_t value) {
  switch (channel) {
    case 0:
      DMXSerial.write(channel, 0);
      break;
    case 1:  // timeout, but not for installed base
      if (TESTVERSION == 0) {
        DMXSerial.write(channel, 0);
      } else {
        DMXSerial.write(channel, value);
      }
      break;
    case 2:  // sequencenumber or timeout
      DMXSerial.write(channel, value);
      break;
    case 3:
      if (TESTVERSION == 0) {
        dmxWriteData(channel, value);
      } else {
        DMXSerial.write(channel, value);
      }
      break;
    default:  // data
      dmxWriteData(channel, value);
      break;
  }
  if (channel > msglen){
    snprintf(buffer, 16, "%i %1", channel,value);
    LogLine(buffer);
  }
}
void dmxWriteData(int channel, uint8_t value) {
  if (value > 8 && value < 240) {
    DMXSerial.write(channel, 8);
  } else {
    DMXSerial.write(channel, value);
  }
}

void fullhouse() {
  channel = 1;
  dmxWrite(channel++, 0);  // test
  if (TESTVERSION == 1) {
    dmxWrite(channel++, 12);  // timeout
  }
  dmxWrite(channel++, 0);                   // sequence number
  for (int j = channel; j < msglen; j++) {  //start at 0; full msglen transmission
    dmxWrite(channel++, 240);
  }
  delay(12000);
}
void home(uint8_t row) {
  snprintf(buffer, 16, "homing row %i", row);
  LogLine(buffer);
  channel = 1;
  dmxWrite(channel++, 0);  // test
  if (TESTVERSION == 1) {
    dmxWrite(channel++, 12);  // timeout
  }
  dmxWrite(channel++, 0);  // sequence number
  uint8_t emptyRows = (row - 1) / 2;
  snprintf(buffer, 16, "empty rows %i", emptyRows);
  LogLine(buffer);

  for (uint8_t j = 0; j < emptyRows; j++) {
    for (uint8_t i = 0; i < cols; i++) {
      dmxWrite(channel++, 0);
    }
  }
  //snprintf(buffer, 16, "channel %i",channel);LogLine(buffer);

  for (uint8_t i = 0; i < 6; i++) {
    for (uint8_t j = 0; j < 8; j++) {
      if (row % 2 == 0) {
        dmxWrite(channel++, 0);
        Serial.print("0,");
      } else {
        dmxWrite(channel++, 240);
        Serial.print("240,");
      }
    }
    for (uint8_t j = 0; j < 8; j++) {
      if (row % 2 == 0) {
        dmxWrite(channel++, 240);
        Serial.print("240,");
      } else {
        dmxWrite(channel++, 0);
        Serial.print("0,");
      }
    }
    Serial.println();
  }
  //snprintf(buffer, 16, "channel %i",channel);LogLine(buffer);
  if (channel < msglen) {
    while (channel < msglen) {
      dmxWrite(channel++, 0);
      Serial.print(channel);
      Serial.print(", ");
      if (channel % 32 == 0)
        Serial.println();
    }
  }
  snprintf(buffer, 16, "rest of rows done");
  LogLine(buffer);
}

void loop() {
  time = millis();
  CheckSerial();
  if (digitalRead(startPin) == LOW) {
    step = 0;
    stop = false;
  }
  if (digitalRead(stopPin) == LOW) {
    step = totalsteps - 1;
    stop = true;
  }
  snprintf(buffer, 16, "Step: %i", step);
  LogLine(buffer);
  if (step % 2 == 0) {
    digitalWrite(ledPin1, LOW);  // blink at 0
  } else {
    digitalWrite(ledPin1, HIGH);  // blink at 0
  }
  if (stop){
    digitalWrite(ledPin2, HIGH);
  } else {
    digitalWrite(ledPin2, LOW);
  } 
  channel = 1;
  switch (test) {
    default:
      offset = msglen * step;
      snprintf(buffer, 16, "0ffset %i",offset);LogLine(buffer);
      if (offset > sizeof(pattern) / sizeof(pattern[0])) {
        data = restpattern;
        offset -= sizeof(pattern) / sizeof(pattern[0]);
      } else {
        data = pattern;
      }
      data += offset;
      to = pgm_read_byte_near(data++);
      timeout = 500L * to;      // timeout is in .5 seconds
      dmxWrite(channel++, 0);  // test = 0, no test
      if (TESTVERSION == 1) {
        dmxWrite(channel++, to);  // timeout
      }
      dmxWrite(channel++, pgm_read_byte_near(data++));  // sequencenr

      for (uint8_t doublerow = 0; doublerow < 5; doublerow++) {
        for (uint8_t row = 1; row <= 2; row++) {
          for (int j = 0; j < cols; j++) {
            // invert columns, optimized/limited to two rows * 5
            msgbuffer[row * cols - 1 - j] = pgm_read_byte_near(data++);  // read byte and increment data ptr
          }
        }
        for (uint8_t i = 0; i < 6; i++) {  // 0..6 modules
          int astart = i * 8;           // start of the module first motor of the even row
          for (int j = astart; j < astart + 8; j++) {
            dmxWrite(channel++, msgbuffer[j]);  // 8 bytes in normal order
          }
          int bstart = astart + 48;  // start of the module first motor of the odd row (=6*8)
          uint8_t l = 0;
          for (int k = bstart + 7; k >= bstart; k--) {
            bline[l++] = msgbuffer[k];  // 8 bytes in reverse order
          }
          for (uint8_t k = 0; k < 8; k++) {
            if (k % 2 == 0) {  // swap bytes
              tmp = bline[k];
              bline[k] = bline[k + 1];
              bline[k + 1] = tmp;
            }
          }
          for (uint8_t j = 0; j < 8; j++) {
            dmxWrite(channel++, bline[j]);  // 8 bytes in swapped reversed order
          }
        }
      }
      tmp = 0;
      if (!stop) {
        step = (++step) % totalsteps;
      }
      break;
    case 1:
      home(1);
      break;
    case 2:
      home(2);
      break;
    case 3:
      home(3);
      break;
    case 4:
      home(4);
      break;
    case 5:
      home(5);
      break;
    case 6:
      home(6);
      break;
    case 7:
      home(7);
      break;
    case 8:
      home(6);
      break;
    case 9:
      home(9);
      break;
    case 10:
      home(10);
      break;
    case 11:
      DMXSerial.write(TESTVERSION, 1);  // test 1
      break;
    case 12:
      DMXSerial.write(TESTVERSION, 2);  // test 2
      break;
    case 13:
      DMXSerial.write(TESTVERSION, 3);  // test 3
      break;
    case 14:
      snprintf(buffer, 16, "case 14 %i", tmp);
      LogLine(buffer);

      for (int j = 2; j < tmp; j++) {  //start at 0; full msglen transmission
        dmxWrite(channel++, 1);
      }
      for (int j = tmp; j < msglen; j++) {
        dmxWrite(channel++, 0);
      }
      dmxWrite(TESTVERSION, 0);
      tmp++;
      if (tmp > msglen) {
        test = 0;
        tmp = 0;
      }
      break;
    case 15:
      snprintf(buffer, 16, "case 15 %i", tmp);
      LogLine(buffer);
      if (tmp == 0) {
        memset(DMXSerial.getBuffer(), 0, DMXSERIAL_MAX + 1);
      }
      if (tmp > msglen) {
        test = 0;
        tmp = 0;
      }
      dmxWrite(channel++, 0);  // test = 0, no test
      if (TESTVERSION == 1) {
        dmxWrite(channel++, 6);  // timeout
      }
      dmxWrite(channel++, tmp);  // sequencenr
      dmxWrite(tmp++, 8);
      break;
    case 16:
      if (tmp == 0) {
        memset(DMXSerial.getBuffer(), 0, DMXSERIAL_MAX + 1);
      }
      if (tmp > msglen) {
        test = 0;
        tmp = 0;
      }
      dmxWrite(channel++, 0);  // test = 0, no test
      if (TESTVERSION == 1) {
        dmxWrite(channel++, 6);  // timeout
      }
      dmxWrite(channel++, tmp);  // sequencenr
      dmxWrite(tmp++, 8);
      if (tmp % 8 == 0) {
        tmp += 8;
      }
      break;
  }
  snprintf(buffer, 16, "Timeout: %i", timeout);
  LogLine(buffer);
  //break 88us + 8us + 513*44us (4 us +.8x4.+4+4 us) is minimaal 22.668 ms
  now = millis();
  delta = now - time;
  if (timeout > delta) {
    timeout = timeout - delta;
    delay(timeout);
  }
}
