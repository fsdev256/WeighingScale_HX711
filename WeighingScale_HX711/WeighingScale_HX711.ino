/*

  WeighingScale_HX711.ino

  DIY 1KG Weighing Scale with HX711 & OLED display

  References Library:
  1. u8g2 - https://github.com/olikraus/u8g2
  2. HX711_ADC - https://github.com/olkal/HX711_ADC

  Limitation: Minimum weight is 5g. Less than 5g will automatically do self-tare

  Copyright (c) 2020, FanSin Khew
  Copyright (c) 2017 Olav Kallhovd
  Copyright (c) 2016, olikraus@gmail.com
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, 
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this list 
    of conditions and the following disclaimer.
    
  * Redistributions in binary form must reproduce the above copyright notice, this 
    list of conditions and the following disclaimer in the documentation and/or other 
    materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  

*/

#include <U8g2lib.h>
#include <HX711_ADC.h>

#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

// ESP-01s
const int scl = 0;
const int sda = 2;
const int HX711_sck = 3; //RX
const int HX711_dout = 1; //TX

// U8g2 Contructor
U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);   // ESP32 Thing, HW I2C with pin remapping

//HX711 constructor
HX711_ADC LoadCell(HX711_dout, HX711_sck);

// U8g2
const char *text = "HX711 1Kg Scale";
const char *unit = "g";
u8g2_uint_t unit_width;
u8g2_uint_t unit_offset;

//HX711 
const float calibrationValue = 1038.79; // calibration value
const int tareThreshold = 6;
const int tareThresholdCountMax = 150;
volatile int tareThresholdCount = 0; 
volatile int currentWeight = 0;
volatile int currentDisplayWeight = -99; //Ensure first display is not same as 0
volatile boolean newScaleDataReady;
volatile boolean doTare = false;

void draw_circle_until_fullscreen()
{
  uint8_t rad;
  for(rad = 0; rad < 61; rad++)
  {
    // picture loop  
    u8g2.firstPage();  
    do {
      u8g2.drawDisc(64,32,rad);
    } while( u8g2.nextPage() );
    delay(10);
  }
}

void welcome_screen() {
  const char *scroll_string = "Made by fsdev256";

  u8g2_uint_t offset = u8g2.getDisplayWidth();
  u8g2_uint_t text_width;      // pixel width of the scrolling text (must be lesser than 128 unless U8G2_16BIT is defined
  u8g2_uint_t scroll_cnt, scroll_end;
  
  u8g2.setFont(u8g2_font_wqy14_t_gb2312a);  // set the target font to calculate the pixel width
  text_width = u8g2.getUTF8Width(text);    // calculate the pixel width of the text

  scroll_end = u8g2.getDisplayWidth();

  // First display
  draw_circle_until_fullscreen();

  // Second display
  for(scroll_cnt=0; scroll_cnt <= scroll_end; scroll_cnt+=2)
  {
    u8g2.firstPage();
    do {
      // Line 1
      u8g2.setFont(u8g2_font_wqy14_t_gb2312a);
      u8g2.drawUTF8(0, 20, "Load Cell 1Kg");
      // Line 2
      u8g2.setFont(u8g2_font_open_iconic_thing_1x_t);
      u8g2.drawGlyph(0, 40, 80);
      u8g2.setFont(u8g2_font_wqy14_t_gb2312a);
      u8g2.drawUTF8(10, 40, text);
      u8g2.setFont(u8g2_font_open_iconic_thing_1x_t);
      u8g2.drawGlyph(10 + text_width + 2, 40, 80);
      // Line 3
      u8g2.setFont(u8g2_font_wqy12_t_gb2312a);    // set the target font
      u8g2.drawUTF8(offset, 62, scroll_string);     // draw the scolling text
  
    } while ( u8g2.nextPage() );
  
    offset-=2;              // scroll by two pixel      
    delay(10);              // do some small delay
  }
}

void update_screen(const char *s)
{
  u8g2_uint_t s_width;
  u8g2.setFont(u8g2_font_logisoso38_tr);
  s_width = u8g2.getUTF8Width(s);    // calculate the pixel width of the text

  u8g2.firstPage();
  do {
    // Header
    u8g2.setFont(u8g2_font_wqy12_t_gb2312a);  // set the target font to calculate the pixel width
    u8g2.drawUTF8(2, 13, text);
    // Weight
    u8g2.setFont(u8g2_font_logisoso38_tr);
    u8g2.drawUTF8(unit_offset-3-s_width, 58, s);
    u8g2.setFont(u8g2_font_logisoso26_tr);
    u8g2.drawUTF8(unit_offset, 58, unit);
  } while ( u8g2.nextPage() );
}

void update_status_screen(const char *s)
{
  u8g2_uint_t s_width;
  u8g2.setFont(u8g2_font_logisoso24_tr);
  s_width = u8g2.getUTF8Width(s);    // calculate the pixel width of the text

  u8g2.firstPage();
  do {
    // Header
    u8g2.setFont(u8g2_font_wqy12_t_gb2312a);  // set the target font to calculate the pixel width
    u8g2.drawUTF8(2, 13, text);
    // Weight
    u8g2.setFont(u8g2_font_logisoso24_tr);
    u8g2.drawUTF8(u8g2.getDisplayWidth()-2-s_width, 58, s);
  } while ( u8g2.nextPage() );
}

void update_scale_screen()
{
  char cstr[7];

  if(currentWeight != currentDisplayWeight)
  {
    currentDisplayWeight = currentWeight;
    itoa (currentDisplayWeight,cstr,10); 
    update_screen(cstr);
  }
}

void init_display()
{
  u8g2.begin();
  u8g2.enableUTF8Print();    // enable UTF8 support for the Arduino print() function
  u8g2.setFontMode(0);    // enable transparent mode, which is faster

  u8g2.setFont(u8g2_font_logisoso26_tr);  // set the target font to calculate the pixel width
  unit_width = u8g2.getUTF8Width(unit);    // calculate the pixel width of the text
  unit_offset = u8g2.getDisplayWidth() - 5 - unit_width;
}

void init_scale()
{
  LoadCell.begin();
  long stabilizingtime = 2000; // tare preciscion can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);
  LoadCell.setCalFactor(calibrationValue); // set calibration value (float)

  attachInterrupt(digitalPinToInterrupt(HX711_dout), scaleDataReadyISR, FALLING);
}

//interrupt routine:
ICACHE_RAM_ATTR void scaleDataReadyISR() {
  if (LoadCell.update()) {
    newScaleDataReady = 1;
  }
}

void setup(void) {
  Wire.begin(sda, scl);
  init_display();
  welcome_screen();
  init_scale();
}

void loop(void) {
  if (newScaleDataReady) {
    float w = LoadCell.getData();
    currentWeight = (int)w;
    newScaleDataReady = 0;
    if(!doTare)
      update_scale_screen();
  }

  // Assume no items is weighted
  // Do tare when weight is less than 5g
  if(currentWeight < tareThreshold && currentWeight != 0)
  {
    tareThresholdCount++;
    if(tareThresholdCount > tareThresholdCountMax)
    {
      tareThresholdCount=0;
      if(!doTare) {
        doTare = true;
        update_status_screen("Calibrate");
        LoadCell.tareNoDelay();
      }
    }
    delay(10);
  }
  else
  {
    tareThresholdCount = 0;
  }

  //check if last tare operation is complete
  if(doTare == true) {
    if (LoadCell.getTareStatus() == true) {
      doTare = false;
      update_status_screen("Complete");
      delay(1000);
    }
  }
}
