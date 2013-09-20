/***************************************************
  
This is an open hardware project to build a digital display for 
the mark and space tones of a radio tuned to a RTTY signal. Instead of 
using a traditional XY osciloscope, this project displays the tones on 
a small LCD panel. 

The project is based on an ATmega328 microcontroller. ADC accuracy is 
decreased in favor of quick samples. The mark and space tones are read
one after the other to minimze the lag (and hence phase distortion) between
the samples. 

There are two controls. The first is a double-ganged potentiometer that
directly controls op-amp gain for the mark and space channels. This should be
adjusted to keep the signal entirely on the display, without clipping. 
The second potentiometer controls the number of pixels on the display. The 
more pixels, the brighter, but the slower the refresh.

   Copyright [2013] Jack Welch

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 ****************************************************/

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>

//Set up hardware SPI for the ST7735 LCD display
#define cs   10
#define dc   9
#define rst  8  // you can also connect this to the Arduino reset
Adafruit_ST7735 tft = Adafruit_ST7735(cs, dc, rst);

//Set up the rest of the IO pins
//analog to read mark, space, and dot density
#define markPin 1
#define spacePin 0
#define dotDensityPin 5

//digital pins to read the foreground/background dip switch
#define colorPin0 0
#define colorPin1 1
#define colorPin2 2
#define colorPin3 3

#define MAX_DOT_BUFFER_SIZE 512 //later, read it from the dip

// defines for setting and clearing register bits
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

int markVal = 0;
int spaceVal= 0;
uint8_t xBuffer[MAX_DOT_BUFFER_SIZE];
uint8_t yBuffer[MAX_DOT_BUFFER_SIZE];
int bufIdx = 0;

int bufSize = 256;
int readBufferValue = 256;

#define ST7735_ORANGE 0xF9E0

uint16_t spectrum[8] = 
  { ST7735_RED,
    ST7735_ORANGE,
    ST7735_YELLOW,
    ST7735_GREEN,
    ST7735_CYAN,  
    ST7735_BLUE, 
    ST7735_MAGENTA,
    ST7735_WHITE};
    
uint8_t colorPin[4] =
  { colorPin0,
    colorPin1,
    colorPin2,
    colorPin3};
    
uint16_t backgroundColor, foregroundColor;

void splashScreen(void) {
  tft.fillScreen(ST7735_BLUE);
  tft.setRotation(3);
  tft.setTextColor(ST7735_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(40,10);
  tft.print("CROSSED");  
  tft.setCursor(40,30);
  tft.print("BANANAS");
  tft.setCursor(40,50);
  tft.print("DISPLAY");
  tft.setTextSize(1);
  tft.setTextColor(ST7735_WHITE);
  tft.setCursor(10,110);
  tft.print("(c)2013 by Jack Welch");
  delay(2000); 
}

void setup(void) {
  
  uint8_t switchSetting = 0;
  uint8_t dipValue = 0;
  
  //Serial.begin(9600);
  
  //read color dip switch
  for (int dipSwitchPosition = 0; dipSwitchPosition < 4; dipSwitchPosition++) {
    pinMode(colorPin[dipSwitchPosition],INPUT_PULLUP);
    dipValue = digitalRead(colorPin[dipSwitchPosition]);
    if(dipSwitchPosition == 3) {
      foregroundColor = spectrum[switchSetting];
      if(dipValue) {
        backgroundColor = ST7735_WHITE;
      }
      else {
        backgroundColor = ST7735_BLACK;     
      }
    }
    else {
      switchSetting <<= 1;
      switchSetting |= dipValue;
    }   
  }
  
  tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab
  splashScreen();
  tft.setRotation(0);
  tft.fillScreen(backgroundColor);
    
  // set ADC prescale to 16
  sbi(ADCSRA,ADPS2);
  cbi(ADCSRA,ADPS1);
  cbi(ADCSRA,ADPS0); 
}

void loop() {
  int delta;
  
  for (int scan=0;scan < 100; scan++) {
    
    markVal = analogRead(markPin);
    spaceVal = analogRead(spacePin);
    xBuffer[bufIdx] = map(markVal,0,1023,0,127);
    yBuffer[bufIdx] = map(spaceVal,0,1023,16,143);
    tft.drawPixel(xBuffer[bufIdx], yBuffer[bufIdx],foregroundColor);
    bufIdx++;
    if(bufIdx == bufSize) bufIdx = 0;
    //if(bufIdx > bufSize) Serial.println("SQUALK!!!!!!!!!!");//error condition; evil ensues.
    tft.drawPixel(xBuffer[bufIdx], yBuffer[bufIdx],backgroundColor);
  }
  //dot density pot 0 - 2.5V 
  readBufferValue = map(analogRead(dotDensityPin),0,512,10,MAX_DOT_BUFFER_SIZE-1);
  delta = readBufferValue - bufSize;
  if(abs(delta) > 5) {
    //Serial.println("Adjusting...");
    if (delta > 0) { //recently read value is significantly bigger than current buffer size
      for (int padding = bufSize; padding < readBufferValue; padding++) {
        xBuffer[padding] = 5;  //picked coordinates that are always background
        yBuffer[padding] = 5;  //no harm in resetting these later
      } 
    }
    else { //recently read value is significantly smaller than current buffer size
      for (int padding = (readBufferValue - 1); padding < bufSize; padding++) {
        tft.drawPixel(xBuffer[padding], yBuffer[padding],backgroundColor);
      }
      if(bufIdx > (readBufferValue - 1)) {
        bufIdx = readBufferValue-1;
      }
    }
    bufSize = readBufferValue;
  }
  //Serial.print("Dot Density: ");
  //Serial.println(readBufferValue,DEC);
}
