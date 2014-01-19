#include <Wire.h>
#include <RTClib.h>
#include <Adafruit_NeoPixel.h>

#define DS1307_ADDR   0x68              // I2C real time clock

#define LED_PIN 6
#define BTN_PIN 2
#define ANALOG_PIN A3
#define SHORT_PRESS_MILLIS 100000
#define LONG_PRESS_MILLIS 1500000
// theme color arrays: [background, hour, minute, second]
#define GRAY {0x000000, 0x999999, 0xeeeeee, 0x444444}
#define RGB {0x000000, 0x0000bb, 0x00bb00, 0xbb0000}
#define NEG {0xcccccc, 0x0000bb, 0x00bb00, 0xbb0000}

long themes[][4] = {RGB, GRAY, NEG};
int theme = 0;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, LED_PIN, NEO_GRB + NEO_KHZ800);

DateTime now;
uint8_t hours = 0;
uint8_t minutes = 0;
uint8_t seconds = 0;
uint8_t mode = 0;
uint8_t even = 0;
uint8_t dark = false;
volatile int btn_level = HIGH;
volatile unsigned long last_time = 0;

RTC_DS1307 rtc;

void btn_press() {
  btn_level = digitalRead(2);
  // found that sampling was much more reliable than simply inverting previous value
  //btn_level = !btn_level;
  unsigned long now = micros();
  if (btn_level == HIGH) { // pressed
    if (now - last_time > LONG_PRESS_MILLIS) {
      // mode0 : timekeeping mode
      // mode1 : setting hours
      // mode2 : setting minutes
      // mode3 : save time
      if (mode == 0) { mode = 1; }
      else if (mode == 1) { mode = 2; }
      else if (mode == 2) { mode = 3; }
    }
    else if (now - last_time > SHORT_PRESS_MILLIS) { // short press
      if (mode == 0) {
        theme++;
        theme = theme % 3;
      }
      else if (mode == 1) hours++;
      else if (mode == 2) minutes++;
    }
  }
  last_time = now;
}

void setup() {
  attachInterrupt(0, btn_press, CHANGE);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  // for debug only (and not on a trinket)
  Serial.begin(9600);
  Wire.begin();
  Init_RTC();
}

// this takes 0=background, 1=hourss, 2=minutes, 3=seconds
// and takes theme and dimming into account
long color(int which) {
  long ret = themes[theme][which];
  if (dark) {
    ret = ((ret & 0x0000ff) / 2) | ((((ret & 0x00ff00) >> 8) / 2) << 8) | ((((ret & 0xff0000) >> 16) / 2) << 16);
  }
  //Serial.print("color : ");
  //Serial.print(which);
  //Serial.print(" : ");
  //Serial.println(String(ret, HEX));
  return ret;
}

void loop() {
  if (mode == 0) Read_RTC();
  if (mode == 3) {
    Set_RTC();
    mode = 0;
  }
  // clear pixels
  int bg_color = color(0);
  for (int k=0; k<60; k++) {
    strip.setPixelColor(k, bg_color);
  }
  // display time
  if (mode != 1 or even == 1) {
    int c = color(1);
    int hours_pos = hours * 5;
    if (hours_pos == 0) {
      strip.setPixelColor(59, c);
    } else {
      strip.setPixelColor(hours_pos-1, c);
    }
    strip.setPixelColor(hours_pos, c);
    strip.setPixelColor(hours_pos+1, c);
  }
  if (mode != 2 or even == 1) {
    strip.setPixelColor(minutes, color(2));
  }
  strip.setPixelColor(seconds, color(3));
  strip.show();
  
  Serial.print("time : ");
  Serial.print(hours);
  Serial.print(":");
  Serial.print(minutes);
  Serial.print(":");
  Serial.print(seconds);
  Serial.print("  mode :");
  Serial.println(mode);
  
  // read analog
  int a_value = analogRead(ANALOG_PIN);
  if (a_value < 190) { // it's dark in here
    dark = true;
  }
  if (a_value > 190) { // somewhat brighter in here
    dark = false;
  }
  if (even == 0) even = 1;
  else even = 0;
  // wait a bit
  delay(1000);
}

void Init_RTC() {
  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(__DATE__, __TIME__));
  }
}

void Read_RTC() {
  now = rtc.now();
  seconds = now.second();  // handle the 3 bytes received
  minutes = now.minute();      
  hours = now.hour() % 12;
}

void Set_RTC() {
  rtc.adjust(DateTime(now.year(), now.month(), now.day(), hours, minutes, seconds));
}

