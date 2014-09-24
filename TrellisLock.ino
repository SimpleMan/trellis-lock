//Trellis
#include <Wire.h>
#include <Adafruit_Trellis.h>
Adafruit_Trellis trellis = Adafruit_Trellis();
#define numKeys 16

int currentFlash = 0;
int numTimes = 0;
int currentInterval = 0;
long previousMillis = 0;

boolean flashDisplay = false;
boolean flashOn = false;

void setupFlash(long interval, int times)
{
  currentInterval = interval;
  numTimes = times;
  flashDisplay = true;
}

void flash()
{
  unsigned long currentMillis = millis();

  if((currentMillis - previousMillis > currentInterval)) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    if (!flashOn) {
      for (uint8_t i=0; i<numKeys; i++) {
        trellis.setLED(i);
      }
      trellis.writeDisplay();
      flashOn = true;
    } else {
      trellis.clear();
      trellis.writeDisplay();
      flashOn = false;
      currentFlash += 1;
    }
  }

  //Clear Vars and set controls
  if(numTimes == currentFlash) {
    flashDisplay = false;
    currentFlash = 0;
    numTimes = 0;
    currentInterval = 0;
    previousMillis = 0;
    currentMillis = 0;
    trellis.clear();
    // Bottom Left and Right Controls
    trellis.setLED(12);
    trellis.setLED(15);
    trellis.writeDisplay();
  }
}

int passCode = 15;
int passTmp;
boolean isAuthenticated = false;

boolean checkPasscode()
{
  for (uint8_t i=0; i<numKeys; i++) {
    if (trellis.justPressed(i)) {
      // Alternate the LED
      if (trellis.isLED(i)) {
        trellis.clrLED(i);
      } else {
        trellis.setLED(i);
      }

    } else if (trellis.justReleased(i)) {
      if (trellis.isLED(i)) {
        bitWrite(passTmp, i, 1);
      } else {
        bitWrite(passTmp, i, 0);
      }
      // Serial.print("Temp Pass:   ");
      // Serial.println(passTmp, BIN);
      // Serial.print("Actual Pass: ");
      // Serial.println(passCode, BIN);

      if (passTmp == passCode) {
        Serial.println("Authenticated!");
        isAuthenticated = true;

        //Flash success
        setupFlash(500, 2);
      }
    }
  }

  trellis.writeDisplay();
}

void checkControls()
{
  //Left Button
  if(trellis.justPressed(12)) {
    Serial.println("Left Control Pressed");
  }
  //Right Button
  if(trellis.justPressed(15)) {
    Serial.println("Right Control Pressed");
  }
}

void setup()
{
  Serial.begin(9600);
  Serial.println("Locked");
  // Trellis setup
  trellis.begin(0x70);
  trellis.clear();
  trellis.writeDisplay();
}


void loop()
{
  //delay(30);

  //If Trellis button press
  if(trellis.readSwitches()){
    if(!isAuthenticated){
      checkPasscode();
    } else {
      checkControls();
    }

  }

  if (flashDisplay) {
    flash();
  }
}