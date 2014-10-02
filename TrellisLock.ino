//Trellis
#include <Wire.h>
#include <Adafruit_Trellis.h>
//Pass
#include <avr/eeprom.h>
#include <EEPROM.h>
//FPS
#include <FPS_GT511C3.h>
#include <SoftwareSerial.h>

// Hardware setup - FPS connected to:
//    digital pin 4(arduino rx, fps tx)
//    digital pin 5(arduino tx - 1K resistor fps tx - 2K resistor - ground)
//    this brings the 5v tx line down to about 3.2v so we dont fry our fps

FPS_GT511C3 fps(4, 5);

//Trellis
Adafruit_Trellis trellis = Adafruit_Trellis();
#define numKeys 16

void flash(long interval, int times)
{
  for(int i=0; i<times; i++){
      for (uint8_t i=0; i<numKeys; i++) {
        trellis.setLED(i);
      }
      trellis.writeDisplay();
      delay(interval);
      trellis.clear();
      trellis.writeDisplay();
      delay(interval);
  }
}

int passCode = 15;
int passTmp;
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
        flash(500, 2);
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

boolean isPassSet = false;
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

boolean fpsEnrolling = false;

void fpsEnroll()
{
  fps.SetLED(true);
  delay(100);
  // find open enroll id
  int enrollid = 0;
  bool usedid = true;
  while (usedid == true)
  {
    usedid = fps.CheckEnrolled(enrollid);
    if (usedid==true) enrollid++;
  }
  fps.EnrollStart(enrollid);

  // enroll
  Serial.print("Press finger to Enroll #");
  Serial.println(enrollid);
  while(fps.IsPressFinger() == false) delay(100);
  bool bret = fps.CaptureFinger(true);
  int iret = 0;
  if (bret != false)
  {
    Serial.println("Remove finger");
    fps.Enroll1();
    while(fps.IsPressFinger() == true) delay(100);
    Serial.println("Press same finger again");
    while(fps.IsPressFinger() == false) delay(100);
    bret = fps.CaptureFinger(true);
    if (bret != false)
    {
      Serial.println("Remove finger");
      fps.Enroll2();
      while(fps.IsPressFinger() == true) delay(100);
      Serial.println("Press same finger yet again");
      while(fps.IsPressFinger() == false) delay(100);
      bret = fps.CaptureFinger(true);
      if (bret != false)
      {
        Serial.println("Remove finger");
        iret = fps.Enroll3();
        if (iret == 0)
        {
          Serial.println("Enrolling Successfull");
        }
        else
        {
          Serial.print("Enrolling Failed with error code:");
          Serial.println(iret);
        }
      }
      else Serial.println("Failed to capture third finger");
    }
    else Serial.println("Failed to capture second finger");
  }
  else Serial.println("Failed to capture first finger");

  fpsEnrolling = false;
  fps.SetLED(false);
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
      flash(500, 1);

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
        passTmp = 0;
      }

      //Top Right Button
      if(btnCurrent == 3){
        Serial.println("Enrolling new finger");
        fpsEnrolling = true;
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

  flash(500, 2);

  //FPS Setup
  //fps.UseSerialDebug = true; // so you can see the messages in the serial debug screen
  fps.Open();


  //Check if password is set
  //settingPass = true;
  //getPassword();
}

void loop()
{
  delay(30);

  trellis.readSwitches();
  if(!isAuthenticated) {
    checkPasscode();
  }else if(settingPass) {
    setPassword();
    checkButtonHold();
  } else {
    checkButtonHold();
  }

  //Serial.print("fpsEnrolling= ");
  //Serial.println(fpsEnrolling);
  if(fpsEnrolling) {
    fpsEnroll();
  }

}