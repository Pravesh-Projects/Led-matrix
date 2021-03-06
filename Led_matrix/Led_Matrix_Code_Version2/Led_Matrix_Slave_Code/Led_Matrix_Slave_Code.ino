#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <Wire.h>
#define USE_UI_CONTROL 0
#if USE_UI_CONTROL
#include <MD_UISwitch.h>
#endif
#define DEBUG 0
#if DEBUG
#define PRINT(s, x) { Serial.print(F(s)); Serial.print(x); }
#define PRINTS(x) Serial.print(F(x))
#define PRINTX(x) Serial.println(x, HEX)
#else
#define PRINT(s, x)
#define PRINTS(x)
#define PRINTX(x)
#endif
#define MAX_DEVICES 8
#define CLK_PIN   13
#define DATA_PIN  11
#define CS_PIN    10
MD_Parola P = MD_Parola(CS_PIN, MAX_DEVICES);
int i=0;
int x=0;
#if USE_UI_CONTROL
const uint8_t SPEED_IN = A5;
const uint8_t DIRECTION_SET = 8; 
const uint8_t INVERT_SET = 9;    
const uint8_t SPEED_DEADBAND = 5;
#endif 
uint8_t scrollSpeed = 75;    
textEffect_t scrollEffect = PA_SCROLL_LEFT;
textPosition_t scrollAlign = PA_LEFT;
uint16_t scrollPause = 50; 
#define  BUF_SIZE  512
char curMessage[BUF_SIZE] = { "" };
char newMessage[BUF_SIZE] = { " Hi" };
bool newMessageAvailable = true;
#if USE_UI_CONTROL
MD_UISwitch_Digital uiDirection(DIRECTION_SET);
MD_UISwitch_Digital uiInvert(INVERT_SET);
void doUI(void)
{
  {
    int16_t speed = map(analogRead(SPEED_IN), 0, 1023, 10, 150);
    if ((speed >= ((int16_t)P.getSpeed() + SPEED_DEADBAND)) ||
      (speed <= ((int16_t)P.getSpeed() - SPEED_DEADBAND)))
    {
      P.setSpeed(speed);
      scrollSpeed = speed;
      PRINT("\nChanged speed to ", P.getSpeed());
    }
  }
  if (uiDirection.read() == MD_UISwitch::KEY_PRESS) // SCROLL DIRECTION
  {
    PRINTS("\nChanging scroll direction");
    scrollEffect = (scrollEffect == PA_SCROLL_LEFT ? PA_SCROLL_RIGHT : PA_SCROLL_LEFT);
    P.setTextEffect(scrollEffect, scrollEffect);
    P.displayClear();
    P.displayReset();
  }

  if (uiInvert.read() == MD_UISwitch::KEY_PRESS)  // INVERT MODE
  {
    PRINTS("\nChanging invert mode");
    P.setInvert(!P.getInvert());
  }
}
#endif // USE_UI_CONTROL
void readSerial(void)
{
  static char *cp = newMessage;
  i=0;
  while (Wire.available())
  { 
    *cp = (char)Wire.read();
 
      if(*cp == '@'){
        
          i=0;
          x=1;
      }
    Serial.print(*cp);
    Serial.print(" ");
    cp = newMessage+i;
    Serial.print(cp);
    Serial.println(" "); 
    newMessageAvailable = true;
    cp++;
    i++;
   }
}
void setup()
{
  Serial.begin(9600);
  Wire.begin(5);
  Wire.onReceive(readSerial);
  #if USE_UI_CONTROL
  uiDirection.begin();
  uiInvert.begin();
  pinMode(SPEED_IN, INPUT);
  doUI();
  #endif 
  P.begin();
  P.displayText(curMessage, scrollAlign, scrollSpeed, scrollPause, scrollEffect, scrollEffect);
}
void loop()
{
#if USE_UI_CONTROL
  doUI();
#endif 
if (P.displayAnimate())
  {
    if (newMessageAvailable)
    {
      strcpy(curMessage, newMessage);
      newMessageAvailable = false;
    }
    P.displayReset();
  }
  readSerial();
  if(x==1){P.displayClear();
    if (newMessageAvailable)
    {
    strcpy(curMessage, " hello");
      newMessageAvailable = false;
    x=0;
    }
    P.displayReset();
  }
}
