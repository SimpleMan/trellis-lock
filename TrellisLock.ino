//Trellis
#include <Wire.h>
#include <Adafruit_Trellis.h>
Adafruit_Trellis trellis = Adafruit_Trellis();
#define numKeys 16
//Pass
#include <avr/eeprom.h>
#include <EEPROM.h>

int currentFlash = 0;
int numTimes = 0;
int currentInterval = 0;
long previousMillis = 0;

boolean flashDisplay = false;
boolean flashOn = false;
boolean acceptingCommands = false;

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
    trellis.clear();
    acceptingCommands = true;
  }
}

int passCode = 15;
int passTmp;
boolean isPassSet;
boolean isAuthenticated = false;

void checkPasscode()
{
  //Serial.println("checkPasscode");
  for (uint8_t i=0; i<numKeys; i++) {

    if (trellis.justReleased(i)) {
      if (trellis.isLED(i)) {
        trellis.clrLED(i);
        bitWrite(passTmp, i, 0);
      } else {
        trellis.setLED(i);
        bitWrite(passTmp, i, 1);
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

int newPass;
boolean settingPass = false;

void setPassword()
{
  //Serial.println("setPassword");
  //Calculate password from trellis
  for (uint8_t i=0; i<numKeys; i++) {
    if (trellis.justReleased(i)) {
      if (trellis.isLED(i)) {
        trellis.clrLED(i);
        bitWrite(newPass, i, 0);
      } else {
        trellis.setLED(i);
        bitWrite(newPass, i, 1);
      }
    }
  }

  trellis.writeDisplay();
  // eeprom_write_word( (uint16_t *) 1, pass );    //write a 16-bit int to EEPROM address 1
  // EEPROM.write(0,1);
}

//boolean isPassSet = false;
boolean passSet = false;

void getPassword()
{
  //passSet = EEPROM.read(0,1);

  if(passSet) {
    passCode = eeprom_read_word( (uint16_t *) 1 );    //read a 16-bit int from address 1
    isPassSet = true;
  } else {
    isPassSet = false;
  }

}

void clearPassword()
{
  //EEPROM.write(0,1);
}

long btnInterval = 3000;
unsigned long btnHoldStart = 0;
boolean btnState = false;
boolean btnLastState = false;
boolean allow = false;
int btnCurrent = -1;

void checkButtonHold()
{
  //Serial.println("checkButtonHold");
  for (uint8_t i=0; i<numKeys; i++) {
    if(trellis.isKeyPressed(i)) {
      btnCurrent = i;
    }
  }

  btnState = trellis.isKeyPressed(btnCurrent);

  if(btnState && !btnLastState){
    btnHoldStart = millis();
    allow = true;
  }

  if(allow && btnState && btnLastState){
    if(millis() - btnHoldStart >= btnInterval) {
      btnHoldStart = 0;
      allow = false;
      // Serial.print(btnCurrent);
      // Serial.println(" Button held for 3 seconds");
      setupFlash(500, 1);

      //Bottom Left Button
      if(btnCurrent == 12) {
        Serial.println("Setting new password");
        settingPass = true;
        trellis.clear();
        trellis.writeDisplay();
      }

      //Bottom Right button
      if(btnCurrent == 15 && settingPass) {
        Serial.println("Password set");
        passCode = newPass;
        newPass = 0;
        passTmp = 0;
        settingPass = false;
      }

      //Top Left Button
      if(btnCurrent == 0) {
        Serial.println("Locked");
        isAuthenticated = false;
        acceptingCommands = false;
        passTmp = 0;
      }
    }
  }

  btnLastState = btnState;
}

void setup()
{
  Serial.begin(9600);
  Serial.println("Locked");
  // Trellis setup
  trellis.begin(0x70);
  trellis.clear();
  trellis.writeDisplay();

  //Check if password is set
  //settingPass = true;
  //getPassword();
}

void loop()
{
  delay(30);

  if(!isAuthenticated || acceptingCommands) {
    trellis.readSwitches();

    if(settingPass) {
      setPassword();
      checkButtonHold();
    }else if(!isAuthenticated) {
      checkPasscode();
    } else {
      checkButtonHold();
    }
  }

  if (flashDisplay) {
    flash();
  }
}