#define DMX_USE_PORT1
#include <DMXSerial.h>
#include <Wire.h>
#include <pins_arduino.h>
#include "pattern.h"

const int msglen = 482; // timeout + sequencenr + 16*30 


const uint8_t totalsteps = (sizeof(pattern) + sizeof(restpattern))/(481*sizeof(pattern[0]));

uint8_t i = 0;
void setup() {
  Serial.begin(9600); //define baud rate
  Serial.println("Moin Moin"); //print a message
  DMXSerial.init(DMXController);// pin 2 is used for direction;
  DMXSerial.maxChannel(DMXSERIAL_MAX); //32*16
}

void loop() {
  Serial.print("Step:");
  Serial.println(i);
  unsigned long offset = msglen*i;
  const uint8_t * data;
  Serial.println("calculate start of data");
  if (offset > sizeof(pattern)/sizeof(pattern[0])) {
    data = restpattern;
    offset -= sizeof(pattern)/sizeof(pattern[0]);
  } else {
    data = pattern;
  }
  data+=offset;
  uint8_t to = pgm_read_byte_near(data);
  unsigned long timeout = 500*to ; // timeout is in .5 seconds
  Serial.print("timout ");Serial.print(to); Serial.println();
  int channel = 0; 
  DMXSerial.write(channel++, 0); //preamble
  for (int j = 0; j < msglen; j++) { //start at 0; full msglen transmission
    uint8_t hoek = pgm_read_byte_near(data++); // read byte and increment data ptr
    DMXSerial.write(channel++, hoek); 
    Serial.print(hoek);
    Serial.print(",");
  }
  Serial.println();
  //break 88us + 8us + 513*44us (4 us +.8x4.+4+4 us) is minimaal 22.668 ms
  i= (++i)%totalsteps;
  delay(timeout); 
}
