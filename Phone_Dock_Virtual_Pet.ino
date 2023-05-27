#include <Wire.h>               // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306Wire.h"        // legacy: #include "SSD1306.h"

// Optionally include custom images
// Convereter site is - https://www.online-utility.org/image/convert/to/XBM
//#include "images.h"
#include "phone_pet_characters_v1.h"
#include "pet_sleep_visual.h"
#include "egg.h"
#include "stage_1.h"
#include "stage_2.h"
#include "stage_3.h"

int LOCK_STATE_MONITOR = 19;
int CYCLE_COMPLETE_LED_INDICATOR = 15;  //Cycle complete indictor - GREEN LED

typedef void (*Graphic)(void);

String percentage = "10";
int percentState = 0;
int mainImageState = 1; //default is egg image at startup
int cycleImageState = 5;


// Initialize the OLED display using Arduino Wire:
SSD1306Wire display(0x3c, 26, 27);   // ADDRESS, SDA, SCL  -  SDA and SCL usually populate automatically based on your board's pins_arduino.h e.g. https://github.com/esp8266/Arduino/blob/master/variants/nodemcu/pins_arduino.h
// SSD1306Wire display(0x3c, D3, D5);  // ADDRESS, SDA, SCL  -  If not, they can be specified manually.
// SSD1306Wire display(0x3c, SDA, SCL, GEOMETRY_128_32);  // ADDRESS, SDA, SCL, OLEDDISPLAY_GEOMETRY  -  Extra param required for 128x32 displays.
// SH1106Wire display(0x3c, SDA, SCL);     // ADDRESS, SDA, SCL

// Initialize the OLED display using brzo_i2c:
// SSD1306Brzo display(0x3c, D3, D5);  // ADDRESS, SDA, SCL
// or
// SH1106Brzo display(0x3c, D3, D5);   // ADDRESS, SDA, SCL

// Initialize the OLED display using SPI:
// D5 -> CLK
// D7 -> MOSI (DOUT)
// D0 -> RES
// D2 -> DC
// D8 -> CS
// SSD1306Spi display(D0, D2, D8);  // RES, DC, CS
// or
// SH1106Spi display(D0, D2);       // RES, DC

/* create a hardware timer for timing dock states */
hw_timer_t * dock_timer = NULL;
int Current_Cycle_Seconds, TSCC_Seconds, TimerCount;

/* create a hardware timer for LED */
hw_timer_t * LED_timer = NULL;

/* LED state */
volatile byte state = LOW;

void phone_pet_characters() {
  // see http://blog.squix.org/2015/05/esp8266-nodemcu-how-to-create-xbm.html
  // on how to create xbm files
  display.drawXbm(0, 0, phone_pet_char_width, phone_pet_char_height, phone_pet_char_bits);
}

void pet_sleep_visual() {
  // see http://blog.squix.org/2015/05/esp8266-nodemcu-how-to-create-xbm.html
  // on how to create xbm files
  display.drawXbm(15, 15, pet_sleep_visual_width, pet_sleep_visual_height, pet_sleep_visual_bits);
}

void egg() {
  // see http://blog.squix.org/2015/05/esp8266-nodemcu-how-to-create-xbm.html
  // on how to create xbm files
  display.drawXbm(0, 0, egg_width, egg_height, egg_bits);
}

void stage_1() {
  // see http://blog.squix.org/2015/05/esp8266-nodemcu-how-to-create-xbm.html
  // on how to create xbm files
  display.drawXbm(0, 0, stage_1_width, stage_1_height, stage_1_bits);
}

void stage_2() {
  // see http://blog.squix.org/2015/05/esp8266-nodemcu-how-to-create-xbm.html
  // on how to create xbm files
  display.drawXbm(0, 0, stage_2_width, stage_2_height, stage_2_bits);
}

void stage_3() {
  // see http://blog.squix.org/2015/05/esp8266-nodemcu-how-to-create-xbm.html
  // on how to create xbm files
  display.drawXbm(0, 0, stage_3_width, stage_3_height, stage_3_bits);
}

Graphic graphics[] = {phone_pet_characters, pet_sleep_visual, egg, stage_1, stage_2, stage_3};
int demoLength = (sizeof(graphics) / sizeof(Graphic));

boolean timer_state = false;
boolean timer_started = false;
boolean HIGH_State_Set = false;

int TimerState = 1;

int dockstate = 2;
int cycleCount = 0;
int cycleCompleted = 0;
int counter = 1;

int progress = 0;

long log_interval = 1000;
long previousMillis_log = 0;

void IRAM_ATTR onoffTimer(){

  switch (cycleCompleted){
     
    case 0:
      Current_Cycle_Seconds++;
      //Serial.println("Docked timer is " + String(Docked_seconds) + " seconds");
      break;

    case 1:
      TSCC_Seconds++; // Time Since Completed Cycle
      //Serial.println("TSCC is " + String(Undocked_seconds) + " seconds");
      break;
  }
}

void IRAM_ATTR LEDonoffTimer(){
  state = !state;
  digitalWrite(CYCLE_COMPLETE_LED_INDICATOR, state);
}

void checkDockState(){
  if ((digitalRead(LOCK_STATE_MONITOR) == HIGH) && (HIGH_State_Set == false)) {
      //Turns timer on
      //startTimer();
      if ((timer_state == false) && (timer_started == false)) {
        startTimer();
        timer_state = true;
      }
      cycleCompleted = 0;
      percentState = 0;
      dockstate = 1; 
      Current_Cycle_Seconds = 1;
      mainImageState = 1;  //Sets main image back to sleeping when phone is docked
      HIGH_State_Set = true;  //Forces this logic to run once
      digitalWrite(CYCLE_COMPLETE_LED_INDICATOR, HIGH);
      displayScreen();
  }

  if ((digitalRead(LOCK_STATE_MONITOR) == LOW) && (HIGH_State_Set == true)) {
      //Turns timer off
      //timer_state = false;
      //timer_started = true;
      if (cycleCompleted == 0) {
        percentState = 0;
        cycleCompleted = 1;
      }
      HIGH_State_Set = false;
      dockstate = 0;
      //graphics[mainImageState]();
      Serial.print("Main Image state = ");
      Serial.println(mainImageState);
      stop_LED();
      digitalWrite(CYCLE_COMPLETE_LED_INDICATOR, LOW);
      displayScreen();
  }
}

void startTimer(){
  dock_timer = timerBegin(0, 80, true);
  timerAttachInterrupt(dock_timer, &onoffTimer, true);
  timerAlarmWrite(dock_timer, 1000000, true);
  yield();
  timerAlarmEnable(dock_timer);
  timer_started = true;
  //HIGH_State_Set = true;
  Serial.println("Timer Started");
  //setProgressBar(percentState); //need logic to set progress bar state
  //mainImageState = 3; //pet_sleep_visual
  //graphics[mainImageState]();
  //display.display();
}

void setProgressBar(int percentage){
  int x = percentage;
  progress = (x) % 110;
  //Serial.print("Progress = ");
  //Serial.println(progress);
  display.drawProgressBar(20, 5, 100, 10, progress);  //x,y,width,thickness,progress
  display.display();
  percentage = 0;
}

void displayScreen(){
    display.clear();
    display.display();
    //display.drawString(10, 25, String(cycleCount));
    if (cycleCompleted == 0) setProgressBar(percentState);
    graphics[mainImageState]();
    //graphics[cycleImageState]();
    display.display();
}

void checkCycleCount(){  //Cyclecount drives the transition of main image states

      if (cycleCompleted == 1) {
        if ((cycleCount > 0) && (cycleCount <= 2)) {
          mainImageState = 2;
        }

        if ((cycleCount > 2) && (cycleCount <= 12)) {
          mainImageState = 3;
        }

        if ((cycleCount > 12) && (cycleCount <= 22)) {
          mainImageState = 4;
        }

        if (cycleCount > 22) {
          mainImageState = 5;
        }

        display.drawString(15, 25, String(cycleCount));
        display.display();
      }
}
        
      

void cycleTime_ProgressBar(){

  if (cycleCompleted == 1) return;

  switch (Current_Cycle_Seconds) {
    case 1:
      percentState = 0;
      setProgressBar(percentState);
      break;
    case 5:
      percentState = 10;
      setProgressBar(percentState);
      break;
    case 10: 
      percentState = 20;
      setProgressBar(percentState);
      break;
    case 15:
      percentState = 30;
      setProgressBar(percentState);
      break;
    case 20:
      percentState = 40;
      setProgressBar(percentState);
      break;
    case 25:
      percentState = 50;
      setProgressBar(percentState);
      break;
    case 30: 
      percentState = 60;
      setProgressBar(percentState);
      break;
    case 35:
      percentState = 70;
      setProgressBar(percentState);
      break;
    case 40:
      percentState = 80;
      setProgressBar(percentState);
      break;
    //case 45:
    //  percentState = 90;
    //  setProgressBar(percentState);
    //  break;
    case 45:
      percentState = 100;
      setProgressBar(percentState);
      cycleCount = cycleCount + 1;
      cycleCompleted = 1;
      dockstate = 0; 
      Current_Cycle_Seconds = Current_Cycle_Seconds + 1;  //+1 forces the switch to default
      //percentState = 0;
      //Docked_seconds = 0;
      //Undocked_seconds = 0;
      blink_LED();
      //display.drawString(20, 25, String(cycleCount));
      //display.drawString(10, 25, "999");
      Serial.println("");
      Serial.print("Main Image state = ");
      Serial.println(mainImageState);
      Serial.println("");
      Serial.print("cycleCount = ");
      Serial.println(cycleCount);
      Serial.println("");
      //Serial.print("  Cycle = ");
      //Serial.println(cycleCount);
      //checkCycleCount();
      //displayScreen();
      break;
      
    default:
      break;
  }
}

void blink_LED() {
/* Use 1st timer of 4 */
  /* 1 tick take 1/(80MHZ/80) = 1us so we set divider 80 and count up */
  LED_timer = timerBegin(1, 80, true);

  /* Attach onTimer function to our timer */
  timerAttachInterrupt(LED_timer, &LEDonoffTimer, true);

  /* Set alarm to call onTimer function every second 1 tick is 1us
  => 1 second is 1000000us */
  /* Repeat the alarm (third parameter) */
  timerAlarmWrite(LED_timer, 1000000, true);

  /* Start an alarm */
  timerAlarmEnable(LED_timer);
  //Serial.println("LED Started");
}

void stop_LED(){
  if (LED_timer != NULL) {
    timerAlarmDisable(LED_timer);
    timerDetachInterrupt(LED_timer);
    timerEnd(LED_timer);
    LED_timer = NULL;
    Serial.println("LED Stopped");
  }
}

void setup() {
  pinMode(LOCK_STATE_MONITOR, INPUT);
  pinMode(CYCLE_COMPLETE_LED_INDICATOR, OUTPUT);

  digitalWrite(LOCK_STATE_MONITOR, HIGH);
  digitalWrite(CYCLE_COMPLETE_LED_INDICATOR, LOW);
 
  Serial.begin(115200);
  
  // Initialising the UI will init the display too.
  display.init();

  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_24);
  graphics[1]();  //Set default image for intial boot....states saved to EEPROM will dictate display images
  display.display();
  //startTimer();

}

void loop() {
  checkDockState();
  cycleTime_ProgressBar();
  checkCycleCount();

  
  unsigned long currentMillis_log = millis();
  if(currentMillis_log - previousMillis_log > log_interval)
    {
      previousMillis_log = currentMillis_log;   
      Serial.println("TSCC is " + String(TSCC_Seconds) + " seconds");
      Serial.println(" CCS is " + String(Current_Cycle_Seconds) + " seconds");
    }
}

