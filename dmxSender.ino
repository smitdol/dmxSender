#define DMX_USE_PORT1
#include <DMXSerial.h>
#include <Wire.h>
#include <pins_arduino.h>
#include "pattern.h"

#define version "Version 1.00"
#define TESTVERSION 0
unsigned long msglen = 482 + TESTVERSION;  // _test + sequencenr + 16*30
uint8_t hoek = 0;
volatile uint8_t _test = 99;
char buffer[17];
#define cols 48  // 2x 24 motors
unsigned long _duration;
uint8_t msgbuffer[2 * cols];  // 2 rows of 2x24 motors
uint8_t bline[8];             // storage for motors 8-15
const uint8_t totalsteps = (sizeof(pattern) + sizeof(restpattern)) / (msglen * sizeof(pattern[0]));
unsigned long time;
unsigned long now;
unsigned long delta;
int tmp = 0;
volatile bool _stop = false;
uint8_t to;
long timeout;

const byte ledPin1 = 6;
const byte ledPin2 = 7;
const byte ledPin3 = 8;
const byte startPin = 21;  //interrupt pin, not 2; only 2,3 18 19 and 20/21 if lcd not used
const byte stopPin = 19;  //2 in use for dmx, 18 for tx
const byte homePin = 20;

volatile uint8_t _step;
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
  attachInterrupt(digitalPinToInterrupt(startPin), restart, CHANGE);

  pinMode(ledPin2, OUTPUT);
  digitalWrite(ledPin2, HIGH);
  pinMode(stopPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(stopPin), stopNow, CHANGE);

  pinMode(ledPin3, OUTPUT);
  digitalWrite(ledPin3, HIGH);
  pinMode(homePin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(homePin), homeNow, CHANGE);


  snprintf(buffer, 16, __DATE__); LogLine(buffer);
  snprintf(buffer, 16, __TIME__); LogLine(buffer);
  snprintf(buffer, 16, version); LogLine(buffer);
  snprintf(buffer, 16, "totalsteps: %i",totalsteps); LogLine(buffer);

  _stop =false;
  _test = 0;
  _duration = 0;
}

void restart() {
  cli();
  _step = 0;
  _stop = false;
  _test = 0;
  _duration = 0;
  sei();
}
void stopNow() {
  cli();
  _stop = true;
  sei();
}
void homeNow() {
  cli();
  if (_test == 0)
    _test = 99;
  else 
    _test = 0;
  sei();
}
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
        _test = Serial.parseInt();
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
  dmxWrite(channel++, 0);  // _test
  if (TESTVERSION == 1) {
    dmxWrite(channel++, 12);  // timeout
  }
  dmxWrite(channel++, 0);                   // sequence number
  for (int j = channel; j <= msglen; j++) {  //start at 0; full msglen transmission
    dmxWrite(channel++, 240);
  }
  delay(1000);
}
void home(uint8_t row) {
  snprintf(buffer, 16, "homing row %i", row);
  LogLine(buffer);
  channel = 1;
  dmxWrite(channel++, 0);  // _test
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
  snprintf(buffer, 16, "channel %i",channel);LogLine(buffer);

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
  /*
  if (digitalRead(startPin) == LOW) {
    _step = 0;
    _stop = false;
  }
  if (digitalRead(stopPin) == LOW) {
    _step = totalsteps - 1;
    _stop = true;
  }
  */
  snprintf(buffer, 16, "Step: %i", _step+1);
  LogLine(buffer);
  if (_step % 2 == 1) {
    digitalWrite(ledPin1, LOW);  // blink at 0
  } else {
    digitalWrite(ledPin1, HIGH);  // blink at 0
  }
  if (_stop){
    digitalWrite(ledPin2, HIGH);//_stop;
    digitalWrite(ledPin1, LOW); //stop
    _step = totalsteps-1;
    snprintf(buffer, 16, "stopped"); LogLine(buffer);
  } else {
    digitalWrite(ledPin2, LOW);
    snprintf(buffer, 16, "not stopped"); LogLine(buffer);
  }

  if (_test == 0) {
    digitalWrite(ledPin3, LOW);
    snprintf(buffer, 16, "running pattern"); LogLine(buffer);
  } else {
    digitalWrite(ledPin3, HIGH);
    snprintf(buffer, 16, "test %i", _test); LogLine(buffer);
  }

  channel = 1;
  switch (_test) {
    default:
      offset = msglen * _step;
      //snprintf(buffer, 16, "0ffset %i",offset);LogLine(buffer);
      if (offset > sizeof(pattern) / sizeof(pattern[0])) {
        data = restpattern;
        offset -= sizeof(pattern) / sizeof(pattern[0]);
      } else {
        data = pattern;
      }
      data += offset;
      to = pgm_read_byte_near(data++);
      timeout = 500L * to;      // timeout is in .5 seconds
      dmxWrite(channel++, 0);  // _test = 0, no _test
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
      if (!_stop) {
        _step = (++_step) % totalsteps;
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
      DMXSerial.write(TESTVERSION, 1);  // _test 1
      break;
    case 12:
      DMXSerial.write(TESTVERSION, 2);  // _test 2
      break;
    case 13:
      DMXSerial.write(TESTVERSION, 3);  // _test 3
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
        _test = 0;
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
        _test = 0;
        tmp = 0;
      }
      dmxWrite(channel++, 0);  // _test = 0, no _test
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
        _test = 0;
        tmp = 0;
      }
      dmxWrite(channel++, 0);  // _test = 0, no _test
      if (TESTVERSION == 1) {
        dmxWrite(channel++, 6);  // timeout
      }
      dmxWrite(channel++, tmp);  // sequencenr
      dmxWrite(tmp++, 8);
      if (tmp % 8 == 0) {
        tmp += 8;
      }
      break;
      case 99:
        fullhouse();
        break;
  }
  snprintf(buffer, 16, "Timeout: %i", timeout);
  LogLine(buffer);
  //break 88us + 8us + 513*44us (4 us +.8x4.+4+4 us) is minimaal 22.668 ms
  _duration = _duration + timeout;
  snprintf(buffer, 16, "duration: %ld", _duration);
  LogLine(buffer);
  if (_step == 0) _duration = 0;
  now = millis();
  delta = now - time;
  if (timeout > delta) {
    timeout = timeout - delta;
    delay(timeout);
  }

}
