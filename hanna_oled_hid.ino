//-----------------------------------------------------------------------------
//  Tiny Gaming LLC, Hanna Button Box Thingy
//  Copyright (C) 2022 Tiny Gaming LLC  - All Rights Reserved 
//    
//  This file is part of {Hanna Button Box Thingy}
//  Unauthorized copying of this file, via any medium is strictly
//  prohibited proprietary and confidential
//
//  {Tiny Gaming Hanna Button Box Thingy} can not be copied and/or distributed
//  without the express permission of {Tiny Gaming LLC}
//
//  Authors: McCawley Mark
//  Version 1.3.0
//
//  Functions:
//  --------------------------------------
//  void read_drake_buttons(int polltime)
//  void read_drake_switch(int polltime)
//  void press_keyboard_key(int polltime)
//  void read_drake_keypad(int polltime)
//  void set_profile(int polltime)
//  void oled_write(String text)
//  void setup()
//  void loop()
//
//             Arduino Pro Micro wiring diagram
//           ____________________________________
//
//                         +-----+
//                +--------| USB |-------+
//                |        +-----+       |
//   Switch Five  | [1] TX         [RAW] |
//                | [0] RX         [GND] |  Ground
//        Ground  | [GND]          [RST] |
//        Ground  | [GND]          [VCC] |  Voltage for OLED
//      OLED SDA  | [2] SDA      A3 [21] |  Small Button
//      OLED SCL  | [3] SCL      A2 [20] |  Large Button
//  Keypad Row 1  | [4] A6       A1 [19] |  Switch One
//  Keypad Row 2  | [5]          A0 [18] |  L.E.D. Control input
//  Keypad Row 3  | [6] A7     SCLK [15] |  Switch Four
//  Keypad Col 1  | [7]        MISO [14] |  Switch Three
//  Keypad Col 2  | [8] A8     MOSI [16] |  Switch Two
//  Keypad Col 3  | [9] A9      A10 [10] |  L.E.D. PWM Vcc Out
//                |                      |
//                +----------------------+
//
//  NOTE:
//  1. Switch One and Switch Five is each half of a 2-way switch, center to ground
//  2. All switches are connected from the arduino GPIO to ground with Internal pullups.
//  3. L.E.D key backlighting uses one 330ohm resistor per three L.E.Ds
//
//  4. This version of code uses two OLED screens. One side mounted 128x32
//     And onea top mounted 128x64. However they are both set to 128x64 and on
//     the same I2C address 0x3C [duplicated screens]
//     The plan was to have seperate screens, but the OLEDs I have are not I2C
//     address modifyable, plus it came out looking just fine afer some formatting.
//  
//         I2C OLED Display      I2C OLED Display
//         1.3 Inch SSD1306      0.91 Inch SSD1306
//         OLED 128x64           OLED 128x32
//         Address: 0x3C         Address: 0x3C
//          _____________        _______________________
//         |O    ****   O|      | |                   |*|
//         |-------------|      | | Oled Menu 128x32  |*|
//         |  Oled Menu  |      |_|___________________|*|
//         |   128x64    |
//         |-------------|
//         |O____---____O|
//
//-----------------------------------------------------------------------------

// Include header files
#include <Keyboard.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <Bounce2.h>
#include <EEPROM.h>
#include <Keypad.h>
#include <Arduino.h>
#include "menumap.h"

// Defines
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define LOGO_HEIGHT 16
#define OLED_RESET -1
#define LOGO_WIDTH 16
#define led_ctrl 18
#define key_row1 4
#define key_row2 5
#define key_row3 6
#define key_col1 7
#define key_col2 8
#define key_col3 9
#define but_1 20
#define sw_1 15
#define sw_2 14
#define sw_3 16
#define sw_4 19
#define sw_5 1
#define sw_6 21

// Constant declarations
const byte ROWS = 3; // Matrix keypad three rows
const byte COLS = 3; // Matrix keypad three columns
const long LOOP_INTERVAL = 10;  // Polling rate for main loop
const long BUTTON_INTERVAL = 1; // Polling rate for box buttons
const long KEYPAD_INTERVAL = 1; // Polling rate for box keypad
const long BRIGHT_INTERVAL = 0; // Polling rate for LED brightness button
const unsigned long DISP_TIMEOUT = 300; // OLED refresh DISP_TIMEOUT

// Init variables other mix
char key_pressed;
char keymap_1[ROWS][COLS] ={{0, 1, 2}, {3, 4, 5}, {6, 7, 8}};
uint8_t rowPins[ROWS] = {9, 8, 7}; // Connect to the row pinouts of the keypad
uint8_t colPins[COLS] = {4, 5, 6}; // Connect to the column pinouts of the keypad
int profile = 1;        // Set default profile to one
int led_vcc_out = 10;   // L.E.Ds are connected to digital pin 10
float led_pwm_duty = 0; // Variable to store the LED PWM duty cycle
unsigned long oled_cnt; // Counter for OLED text to timeout
String profile_text;    // Variablew to hold text to display on OLED

OLEDMENU menuSRAM; // Menu to overwrite for reading back from PROGMEM

// Initialize an instance of Keypad and OLED screen
Keypad keypad_keys = Keypad(makeKeymap(keymap_1), rowPins, colPins, ROWS, COLS);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Instantiate an instance of a Bounce object for corresponding buttons
Bounce b_but_l = Bounce();
Bounce b_but_1 = Bounce();
Bounce b_sw_t1 = Bounce();
Bounce b_sw_t2 = Bounce();
Bounce b_sw_2 = Bounce();
Bounce b_sw_3 = Bounce();
Bounce b_sw_4 = Bounce();
Bounce b_sw_6 = Bounce();

/*!
 * @function    read_drake_buttons
 * @abstract    Poll for button change.
 * @discussion  Writes text to OLED, and presses key
 * @param       polling delay in milliseconds
 * @result      void
*/
void read_drake_buttons(int polltime)
{
  b_but_1.update();
  if (b_but_1.fell()) {
    memcpy_P( &menuSRAM, &buttonMenu[0], sizeof(OLEDMENU));
    oled_write(String(menuSRAM.name));
    Keyboard.press(menuSRAM.keyId);
    delay(50);
    Keyboard.release(menuSRAM.keyId);
  }
  delay(polltime);
}

/*!
 * @function    read_drake_switch
 * @abstract    Poll for switch state change.
 * @discussion  Writes text to OLED, and presses key
 * @param       polling delay in milliseconds
 * @result      void
*/
void read_drake_switch(int polltime)
{
  b_sw_2.update();
  b_sw_3.update();
  b_sw_4.update();
  b_sw_6.update();
  b_sw_t1.update();
  b_sw_t2.update();

  if (b_sw_2.changed()) {
    memcpy_P( &menuSRAM, &switchMenu[0], sizeof(OLEDMENU));
    press_keyboard_key(menuSRAM.keyId);
    oled_write(String(menuSRAM.name));
  }
  if (b_sw_3.changed()) {
    memcpy_P( &menuSRAM, &switchMenu[1], sizeof(OLEDMENU));
    press_keyboard_key(menuSRAM.keyId);
    oled_write(String(menuSRAM.name));
  }
  if (b_sw_4.changed()) {
    memcpy_P( &menuSRAM, &switchMenu[2], sizeof(OLEDMENU));
    press_keyboard_key(menuSRAM.keyId);
    oled_write(String(menuSRAM.name));
  }
  if (b_sw_6.changed()) {
    memcpy_P( &menuSRAM, &switchMenu[3], sizeof(OLEDMENU));
    press_keyboard_key(menuSRAM.keyId);
    oled_write(String(menuSRAM.name));
  }
  if (b_sw_t1.changed() || b_sw_t2.changed()) {
    set_profile(!digitalRead(sw_1), !digitalRead(sw_5));
  }
  delay(polltime);
}

/*!
 * @function    press_keyboard_key
 * @abstract    press a key
 * @discussion  press a key on the keyboard
 * @param       char key: The key to press
 * @result      void
*/
void press_keyboard_key(char key)
{
  Keyboard.press(key);
  delay(50);
  Keyboard.release(key);
  delay(50);
}

/*!
 * @function    read_drake_keypad
 * @abstract    Poll for matrix buttons change.
 * @discussion  Determine keypress based on profile
 * @param       polling delay in milliseconds
 * @result      void
*/
void read_drake_keypad(int polltime)
{
  if (keypad_keys.getKeys()) {
    for (int i = 0; i < LIST_MAX; i++) {
      if ( keypad_keys.key[i].stateChanged ) {

        // Determine keypress based on profile currently selected
        if (profile == 1) {
          memcpy_P( &menuSRAM, &defaultMenu[keypad_keys.key[i].kcode], sizeof(OLEDMENU));
        } else if (profile == 2) {
          memcpy_P( &menuSRAM, &powerMenu[keypad_keys.key[i].kcode], sizeof(OLEDMENU));
        } else if (profile == 3) {
          memcpy_P( &menuSRAM, &shieldMenu[keypad_keys.key[i].kcode], sizeof(OLEDMENU));
        } else {
          memcpy_P( &menuSRAM, &defaultMenu[keypad_keys.key[i].kcode], sizeof(OLEDMENU));
        }

        // Assign the key press and text from OLEDMENU (menuSRAM)        
        oled_write(menuSRAM.name);
        key_pressed = menuSRAM.keyId;

        // Press and release the assigned key
        switch (keypad_keys.key[i].kstate) {
          case PRESSED:
            Keyboard.press(key_pressed);
            break;
          case RELEASED:
            Keyboard.release(key_pressed);
            break;
          default:
            break;
        }
      }
    }
  }
  delay(polltime);
}

/*!
 * @function    set_led_brightness
 * @abstract    Poll for LED brightness change.
 * @discussion  Polls for LED brightness button press
 * @param       polling delay in milliseconds
 * @result      void
*/
void set_led_brightness(int polltime)
{
  b_but_l.update();
  if (b_but_l.fell()) {
    led_pwm_duty += 25.5;
    if (led_pwm_duty > 255) led_pwm_duty = 0;
    analogWrite(led_vcc_out, led_pwm_duty);
    if (led_pwm_duty == 0) {
      oled_write("LED: OFF");
    } else {
      int duty_percentage = (led_pwm_duty * 100) / 255;
      oled_write("LED: " + String(duty_percentage) + "%");
    }
    // Save the new LED duty cycle to EEPROM for next boot
    EEPROM.write(0, led_pwm_duty);
  }
  delay(polltime);
}

/*!
 * @function    set_profile(int sw1, int sw2)
 * @abstract    Set the keypad mapping based on switch position.
 * @discussion  Switch mode. Switch one and five is a two way switch
 * @param       sw1 switch one state
 * @param       sw2 switch five state
 * @result      void
*/
void set_profile(int sw1, int sw2)
{
  if (sw1 == 1 && sw2 == 0) {
    profile = 1;
    profile_text = "GENERAL";
  } else if (sw1 == 0 && sw2 == 1) {
    profile = 3;
    profile_text = "SHIELDS";
  } else if (sw1 == 0 && sw2 == 0) {
    profile = 2;
    profile_text = "POWER";
  }
  oled_write("Profile"); // Draw text on the screen
}

/*!
 * @function    oled_write(String text)
 * @abstract    Write test to the OLED screen
 * @discussion  Send a string of text to the OLED screen
 * @param       String text: the text to write to screen
 * @result      void
*/
void oled_write(String text)
{
  int font_width = SCREEN_WIDTH / 11;
  int start;
  
  display.setTextSize(2); // Font size
  display.clearDisplay(); // Clear OLED
  
  // White box around profile name
  display.fillRoundRect(2, 2, 124, 28, 3, SSD1306_WHITE);

  // First line of the display
  display.setTextColor(BLACK);
  start = (font_width - profile_text.length()) * 5;
  display.setCursor(start, 9);
  display.println(profile_text);
  
  // Second line of the display
  display.setTextColor(WHITE);
  start = (font_width - text.length()) * 5;
  display.setCursor(start, 42);
  display.println(text);

  display.display(); // Write to screen
  oled_cnt = 0; // Incr text timeout
}

/*
 * @function    setup
 * @abstract    Inital setup method
 * @discussion  Define pin pullups and set variables
 * @param       
 * @result      none
*/
void setup()
{
  Serial.begin(9600);
  Keyboard.begin();

  // Serial.print("void setup()");
  // Serial.println("Inital setup method");

  // Set internal pullups on buttons
  pinMode(but_1, INPUT_PULLUP);
  pinMode(sw_1, INPUT_PULLUP);
  pinMode(sw_2, INPUT_PULLUP);
  pinMode(sw_3, INPUT_PULLUP);
  pinMode(sw_4, INPUT_PULLUP);
  pinMode(sw_5, INPUT_PULLUP);
  pinMode(sw_6, INPUT_PULLUP);
  pinMode(key_row1, INPUT_PULLUP);
  pinMode(key_row2, INPUT_PULLUP);
  pinMode(key_row3, INPUT_PULLUP);
  pinMode(key_col1, INPUT_PULLUP);
  pinMode(key_col2, INPUT_PULLUP);
  pinMode(key_col3, INPUT_PULLUP);
  pinMode(led_ctrl, INPUT_PULLUP);
  pinMode(led_vcc_out, OUTPUT);
  
  // Attach debouncers to buttons
  b_but_l.attach(led_ctrl,INPUT_PULLUP);
  b_but_1.attach(but_1,INPUT_PULLUP);
  b_sw_2.attach(sw_2,INPUT_PULLUP);
  b_sw_3.attach(sw_3,INPUT_PULLUP);
  b_sw_4.attach(sw_4,INPUT_PULLUP);
  b_sw_6.attach(sw_6,INPUT_PULLUP);
  b_sw_t1.attach(sw_1,INPUT_PULLUP);
  b_sw_t2.attach(sw_5,INPUT_PULLUP);

  // Set button debounce intervals
  b_but_l.interval(5);
  b_but_1.interval(5);
  b_sw_2.interval(5);
  b_sw_3.interval(5);
  b_sw_4.interval(5);
  b_sw_6.interval(5);
  b_sw_t1.interval(5);
  b_sw_t2.interval(5);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Setup initial OLED display
  oled_cnt = 0;
  display.display();
  display.setRotation(2); // Screen mounted upside down, so rotate 90 twice
  display.clearDisplay(); // Clear the buffer

  // Read stored LED brightness from EEPROM
  led_pwm_duty = EEPROM.read(0);
  analogWrite(led_vcc_out, led_pwm_duty);
  delay(50);

  // Draw Drake Interplanitary boot logo
  display.clearDisplay();
  display.drawBitmap(0, 0, drakeLogo, 128, 64, WHITE);
  display.display();
  delay(2500);

  // Load profile based on current switch position
  set_profile(!digitalRead(sw_1), !digitalRead(sw_5));
}

/*!
 * @function    loop
 * @abstract    Main program loop.
 * @discussion  Polls for changes in switch, pad,  and button state.
 * @param       
 * @result      none
*/
void loop()
{
  // Read brightness, buttons, and switches
  set_led_brightness(BRIGHT_INTERVAL);
  read_drake_buttons(BUTTON_INTERVAL);
  read_drake_switch(KEYPAD_INTERVAL);
  read_drake_keypad(KEYPAD_INTERVAL);

  // Text on OLED will timeout and return to this message
  if (oled_cnt >= DISP_TIMEOUT) {
    oled_write("DRAKE");
    oled_cnt = 0;
  }
  ++oled_cnt; // Increment text timeout
  delay(LOOP_INTERVAL);
}
