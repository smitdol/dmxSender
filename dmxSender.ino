#define DMX_USE_PORT1
#include <DMXSerial.h>
#include <Wire.h>
#include <pins_arduino.h>
#include "pattern.h"

#define version "Version 0.94"
#define msglen 482 // test + sequencenr + 16*30 
uint8_t hoek = 0;
uint8_t test = 0;
char buffer[17];
#define cols 48  // 2x 24 motors
#define TEST 1
uint8_t msgbuffer[2*cols]; // 2 rows of 2x24 motors
uint8_t bline[8]; // storage for motors 8-15
const uint8_t totalsteps = (sizeof(pattern) + sizeof(restpattern))/(msglen*sizeof(pattern[0]));
unsigned long time;
unsigned long now;
unsigned long delta;

uint8_t step = 0;
void setup() {
  Serial.begin(9600); //define baud rate
  snprintf(buffer, 16, __DATE__);LogLine(buffer);
  snprintf(buffer, 16, __TIME__);LogLine(buffer);
  snprintf(buffer, 16, version );LogLine(buffer);

  DMXSerial.init(DMXController);// pin 2 is used for direction;
  DMXSerial.maxChannel(DMXSERIAL_MAX); //32*16 = 512; msglen < DMXSERIAL_MAX
  for(uint8_t i = 1; i < 11; i++){
//    home(i);
  }
}

void LogLine(const char * s) {
  Serial.println(s);
}

void CheckSerial() {
  if (Serial.available() > 0) //if something comes
  {
    char receivedCommand = Serial.read(); // this will read the command character
    int parsedInt;
    switch (receivedCommand) {
      case 't':
        parsedInt = Serial.parseInt();
        if (parsedInt < 0 || parsedInt > 14)
        {
          snprintf(buffer, 16, "invalid %i",parsedInt);
          LogLine(buffer);
        } else {
          test = parsedInt;
          snprintf(buffer, 16, "test %i",parsedInt);
          LogLine(buffer);
        }
        break;
      default:
        snprintf(buffer, 16, "received %c",receivedCommand);
        LogLine(buffer);
        break;
    }
  }
}

void home(uint8_t row){
  snprintf(buffer, 16, "homing row %i",row);LogLine(buffer);
  uint8_t channel=1;
  DMXSerial.write(channel++, 0); // test
  DMXSerial.write(channel++, 12); // timeout
  DMXSerial.write(channel++, 0); // sequence number
  uint8_t emptyRows = (row-1)/2;
  for( uint8_t j = 0; j < emptyRows;j++) {
    for (uint8_t i = 0; i < cols; i++) {
      DMXSerial.write(channel++, 0);
    }
  }
  for (uint8_t i = 0; i < 6; i++) { 
    for (uint8_t j = 0; j < 4; j++)
    {
      if (row%2 ==  0){
        DMXSerial.write(channel++, 0);
        DMXSerial.write(channel++, 240);
      } else {
        DMXSerial.write(channel++, 240);
        DMXSerial.write(channel++, 0);
      }
    }
    for (uint8_t j = 0; j < 8; j++)
    {
      //b-line
      DMXSerial.write(channel++, 0);
    }
    for (uint8_t i = channel; i < msglen; i++) {
      DMXSerial.write(channel++, 0);
    }
  }
}

void loop() {
  time = millis();

  CheckSerial();
  snprintf(buffer, 16, "Step: %i", step);LogLine(buffer);
  unsigned long offset = msglen*step;
  const uint8_t * data;
  snprintf(buffer, 16, "calculate start");LogLine(buffer);
  if (offset > sizeof(pattern)/sizeof(pattern[0])) {
    data = restpattern;
    offset -= sizeof(pattern)/sizeof(pattern[0]);
  } else {
    data = pattern;
  }
  data+=offset;
  
  uint8_t to = 12; // 2x3 sec (x2 x .5 seconds)
  unsigned long timeout = 6000; //ms, timeout is in .5 seconds
  int channel = 1; 
  switch (test)
  {
    default:
      to = pgm_read_byte_near(data++);
      timeout = 500*to ; // timeout is in .5 seconds
      DMXSerial.write(channel++, 0); // test = 0, no test
      DMXSerial.write(channel++, to); // timeout
      DMXSerial.write(channel++, pgm_read_byte_near(data++)); // sequencenr
      for (uint8_t doublerow = 0; doublerow < 5; doublerow++) {
        for (uint8_t row = 1; row <= 2; row++) {
          for (int j = 0; j < cols; j++) { 
            // invert columns, optimized/limited to two rows * 5
            msgbuffer[row*cols -1 - j] = pgm_read_byte_near(data++); // read byte and increment data ptr
          }
        }
        uint8_t tmp;
        for (uint8_t i = 0; i <= 6; i++) { // 0..6 modules
          uint8_t astart=i*8; // start of the module first motor of the even row
          for (uint8_t j = astart; j <= astart+8; j++) {
            DMXSerial.write(channel++, msgbuffer[j]);  // 8 bytes in normaal order 
          }
          uint8_t bstart=astart+48; // start of the module first motor of the odd row
          uint8_t l = 0;
          for(uint8_t k = bstart+7; k >= bstart; k--){
            bline[l++] = msgbuffer[k];  // 8 bytes in reverse order
          }
          for(uint8_t k = 0; k < 8; k++) {
            if (k % 2 == 0) { // swap bytes
              tmp = bline[k];
              bline[k] = bline[k+1];
              bline[k+1] = tmp;
            }
          }
          for(uint8_t j = 0; j< 8; j++){
            DMXSerial.write(channel++, bline[j]);  // 8 bytes in swapped reversed order
          }
        }
      }
      step = (++step)%totalsteps;
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
      DMXSerial.write(TEST, 1); // test 1
      break;
    case 12:
        DMXSerial.write(TEST, 2); // test 2
      break;
    case 13:
        DMXSerial.write(TEST, 3); // test 2
      break;
    case 14:
      for (int j = channel; j < msglen; j++) { //start at 0; full msglen transmission
        if (j == 33) {
          snprintf(buffer, 16, "%i", hoek);LogLine(buffer);
          DMXSerial.write(channel++, hoek++);
          hoek=hoek%16;
        } else {
          snprintf(buffer, 16, "%i", 0);LogLine(buffer);
          DMXSerial.write(channel++, 0);
        }
        Serial.print(",");
      }
      break;
  }
  snprintf(buffer, 16, "Timeout: %i", timeout);LogLine(buffer);
  //break 88us + 8us + 513*44us (4 us +.8x4.+4+4 us) is minimaal 22.668 ms
  now = millis();
  delta = now - time;
  if (timeout > delta){
    timeout=timeout-delta;
    delay(timeout); 
  }
}
