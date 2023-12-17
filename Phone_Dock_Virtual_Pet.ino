#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1325.h>
#include <Preferences.h>


// If using software SPI, define CLK and MOSI
#define OLED_CLK 18 //13
#define OLED_MOSI 23 //11

// These are neede for both hardware & softare SPI
#define OLED_CS 5 //10
#define OLED_RESET 22 //9
#define OLED_DC 19 //8

// this is software SPI, slower but any pins
Adafruit_SSD1325 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

// Optionally include custom images
// Convereter site is - https://www.online-utility.org/image/convert/to/XBM
// SSD1325 convereter used is at - https://javl.github.io/image2cpp/
//#include "images.h"
#include "phone_pet_characters_v1.h"
#include "pet_sleep_visual.h"
#include "egg.h"
#include "stage_1.h"
#include "stage_2.h"
#include "stage_3.h"
#include "smiley.h"
#include "swirl.h"
#include "skull.h"
#include "gravestone_v2.h"

const char * StateKeys = "VirualPetStates";

int LOCK_STATE_MONITOR = 25;
int CYCLE_COMPLETE_LED_INDICATOR = 15;  //Cycle complete indictor - GREEN LED

typedef void (*Graphic)(void);

String percentage = "10";
int percentState = 0;
int mainImageState = 1; //default is egg image at startup
int storemainImageState = 1;
int cycleImageState = 6;

int undockedTime_L1 = 12; // AKA 12 hours 43200
int undockedTime_L2 = 24; // AKA 24 hours 86400
int undockedTime_L3 = 48; // AKA 48 hours 172800

Preferences pref;

/* create a hardware timer for timing dock states */
hw_timer_t * dock_timer = NULL;
int Current_Cycle_Seconds, TSCC_Seconds, TimerCount;

/* create a hardware timer for LED */
hw_timer_t * LED_timer = NULL;

/* LED state */
volatile byte state = LOW;


void pet_sleep_visual() {
  display.drawBitmap(0, 0, pet_sleep_visual_bits, 128, 64, 1);
}

void phone_pet_characters() {
  //display.drawXbm(0, 0, phone_pet_char_width, phone_pet_char_height, phone_pet_char_bits);
  display.drawBitmap(0, 0, phone_pet_char_bits, 128, 64, 1);
}

void egg() {
  //display.drawXbm(0, 0, egg_width, egg_height, egg_bits);
  display.drawBitmap(0, 0, egg_bits, 128, 64, 1);
}

void stage_1() {
  //display.drawXbm(0, 0, stage_1_width, stage_1_height, stage_1_bits);
  display.drawBitmap(0, 0, stage_1_bits, 128, 64, 1);
}

void stage_2() {
  //display.drawXbm(0, 0, stage_2_width, stage_2_height, stage_2_bits);
  display.drawBitmap(0, 0, stage_2_bits, 128, 64, 1);
}

void stage_3() {
  //display.drawXbm(0, 0, stage_3_width, stage_3_height, stage_3_bits);
  display.drawBitmap(0, 0, stage_3_bits, 128, 64, 1);
}

void smiley() {
  //display.drawXbm(0, 0, smiley_width, smiley_height, smiley_bits);
  display.drawBitmap(0, 0, smiley_bits, 128, 64, 1);
}

void swirl() {
  //display.drawXbm(0, 0, swirl_width, swirl_height, swirl_bits);
  display.drawBitmap(0, 0, swirl_bits, 128, 64, 1);
}

void skull() {
  //display.drawXbm(0, 0, skull_width, skull_height, skull_bits);
  display.drawBitmap(0, 0, skull_bits, 128, 64, 1);
}

void gravestone_v2() {
  //display.drawXbm(0, 0, gravestone_v2_width, gravestone_v2_height, gravestone_v2_bits);
  display.drawBitmap(0, 0, gravestone_v2_bits, 128, 64, 1);
}

Graphic graphics[] = {phone_pet_characters, pet_sleep_visual, egg, stage_1, stage_2, stage_3, smiley, swirl, skull, gravestone_v2};
int demoLength = (sizeof(graphics) / sizeof(Graphic));

boolean timer_state = false;
boolean timer_started = false;
boolean HIGH_State_Set = false;
boolean cycleImageDisplayed = false;

int dockstate = 2;
int cycleCount = 0;
int cycleCompleted = 0;

int progress = 0;

long log_interval = 1000;  //DO NOT RECORD TO EEPROM
long previousMillis_log = 0;  //DO NOT RECORD TO EEPROM

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
      //TSCC_Seconds = 0; //Reset the time since complete cycle after phone is returned to dock
      mainImageState = 1;  //Sets main image back to sleeping when phone is docked
      HIGH_State_Set = true;  //Forces this logic to run once
      cycleImageDisplayed = false;
      //cycleImageState = 6; //set back to default after phone is docked again
      digitalWrite(CYCLE_COMPLETE_LED_INDICATOR, HIGH);
      displayScreen();
  }

  if ((digitalRead(LOCK_STATE_MONITOR) == LOW) && (HIGH_State_Set == true)) {
      //Turns timer off
      //timer_state = false;
      //timer_started = true;

      if ((timer_state == false) && (timer_started == false)) {
        startTimer();
        Serial.println("Timer Start Back up!!");
        timer_state = true;
      }
      
      if (cycleCompleted == 0) {
        percentState = 0;
        cycleCompleted = 1;// Not sure this is correct here...
      }
      HIGH_State_Set = false;
      dockstate = 0;
      if (storemainImageState >= 2){
        mainImageState = storemainImageState;
      }
      Serial.print("Main Image state = ");
      Serial.println(mainImageState);
      stop_LED();
      digitalWrite(CYCLE_COMPLETE_LED_INDICATOR, LOW);
      displayCycleImage();
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
}

void stopTimer(){
  if (dock_timer != NULL) {
    timerAlarmDisable(dock_timer);
    timerDetachInterrupt(dock_timer);
    timerEnd(dock_timer);
    dock_timer = NULL;
    dockstate = 2;
    Serial.println("Timer Stopped");
  }
}


void setProgressBar(int percentage){
  int x = percentage;
  progress = (x) % 110;
  display.drawProgressBar(0, 0, 127, 10, progress);  //x,y,width,thickness,progress
  display.display();
  percentage = 0;
}

void displayScreen(){
    display.clearDisplay();
    display.setCursor(5, 25);
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.print(String(cycleCount));
    display.display();
    //display.drawString(10, 25, String(cycleCount));
    if (cycleCompleted == 0) setProgressBar(percentState);
    graphics[mainImageState]();
    display.display();
}

void displayCycleImage(){
  display.clearDisplay();
  graphics[cycleImageState]();
  display.display();
  graphics[mainImageState]();
  display.display();
}

void checkCycleCount(){  //Cyclecount drives the transition of main image states

      if (cycleCompleted == 1) {
        if ((cycleCount > 0) && (cycleCount <= 2)) {
          mainImageState = 2;
          storemainImageState = 2;
        }

        if ((cycleCount > 2) && (cycleCount <= 12)) {
          mainImageState = 3;
          storemainImageState = 3;          
        }

        if ((cycleCount > 12) && (cycleCount <= 22)) {
          mainImageState = 4;
          storemainImageState = 4;  
        }

        if (cycleCount > 22) {
          mainImageState = 5;
          cycleCount = 0;
          pref.putInt("cycleCount", cycleCount);
        }
      }
}

void checkCycleCompleted(){
  
  if ((TSCC_Seconds == 23) || (TSCC_Seconds == 47)){  //23 and 47 seconds were used in the testing 43199/172799
    cycleImageDisplayed = false;
  }

  if ((cycleCompleted == 1) || (dockstate == 0)) {
    if ((TSCC_Seconds == undockedTime_L1) && (cycleImageDisplayed == false))
        {
          cycleImageState = 7; //swirl is shown
          Serial.println("Swirl is running...  :(");
          cycleImageDisplayed = true;
          displayCycleImage();
        }
    if ((TSCC_Seconds == undockedTime_L2) && (cycleImageDisplayed == false))
        {
          cycleImageState = 8; //skull is shown
          Serial.println("Skull is running...  :(");
          cycleImageDisplayed = true;
          displayCycleImage();
        } 
    if ((TSCC_Seconds == undockedTime_L3) && (cycleImageDisplayed == false))
        {
          cycleImageState = 9; //grave is shown
          display.clearDisplay();
          graphics[cycleImageState]();
          display.display();
          stopTimer();
          cycleCount = 0;
          pref.putInt("cycleCount", cycleCount);
        }
  }
}

void cycleTime_ProgressBar(){

  if (cycleCompleted == 1) return;

  switch (Current_Cycle_Seconds) {
    case 1:  //3240
      percentState = 0;
      setProgressBar(percentState);
      break;
    case 5: //6480
      percentState = 10;
      setProgressBar(percentState);
      break;
    case 10:  //9720
      percentState = 20;
      setProgressBar(percentState);
      break;
    case 15:  //12960
      percentState = 30;
      setProgressBar(percentState);
      break;
    case 20:  //16200
      percentState = 40;
      setProgressBar(percentState);
      break;
    case 25:  //19440
      percentState = 50;
      setProgressBar(percentState);
      break;
    case 30:  //22680
      percentState = 60;
      setProgressBar(percentState);
      break;
    case 35:  //25920
      percentState = 70;
      setProgressBar(percentState);
      break;
    case 40: //29160
      percentState = 80;
      setProgressBar(percentState);
      break;
    case 45:  //32400
      percentState = 100;
      TSCC_Seconds = 0;
      cycleImageState = 6;
      setProgressBar(percentState);
      cycleCount = cycleCount + 1;
      pref.putInt("cycleCount", cycleCount);
      cycleCompleted = 1;
      cycleImageState = 6;  //Cycle image resets on a completed cycle
      Current_Cycle_Seconds = Current_Cycle_Seconds + 1;  //+1 forces the switch to default
      blink_LED();

      //display.drawString(20, 25, String(cycleCount));

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
      displayScreen();
      displayCycleImage();
      timer_state = false;
      timer_started = false;
      stopTimer();
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

void Get_Key_States(){
  progress = pref.getUInt("progress",0);
}


void setup() {
  Serial.begin(115200);

  pref.begin(StateKeys, false);

  pref.clear();

  pinMode(LOCK_STATE_MONITOR, INPUT);
  pinMode(CYCLE_COMPLETE_LED_INDICATOR, OUTPUT);

  digitalWrite(LOCK_STATE_MONITOR, HIGH);
  digitalWrite(CYCLE_COMPLETE_LED_INDICATOR, LOW);
 
  

  
  if (pref.isKey("cycleCount"))
  {
    cycleCount = pref.getInt("cycleCount",0);
    Serial.print("Key found.....");
    Serial.println(cycleCount);

  } else {
    Serial.println("Key not found");
  }

  // Initialising the UI will init the display too.
  display.begin();

  //display.flipScreenVertically();
  //display.setFont(ArialMT_Plain_24);
  mainImageState = 1;
  //graphics[mainImageState]();  //Set default image for intial boot....states saved to EEPROM will dictate display images
  //display.display();
  display.clearDisplay();
  //display.drawBitmap(0, 0, pet_sleep_visual_bits, 128, 64, 1);
  graphics[mainImageState]();
  display.display();
}

void loop() {
  checkDockState();
  cycleTime_ProgressBar();
  checkCycleCount();
  checkCycleCompleted();
  //if (dockstate == 0 ) displayCycleImage();
  unsigned long currentMillis_log = millis();
  if(currentMillis_log - previousMillis_log > log_interval)
    {
      previousMillis_log = currentMillis_log;   
      Serial.println("TSCC is " + String(TSCC_Seconds) + " seconds");
      Serial.println(" CCS is " + String(Current_Cycle_Seconds) + " seconds");
      pref.putInt("CCS", Current_Cycle_Seconds);
    }
}

