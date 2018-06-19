#define _DEBUG

#ifdef _DEBUG
  #include <SPI.h>
  #include "printf.h"
#endif _DEBUG

// ours.
#include "util.h"
#include "radio.h"
#include "input.h"
#include "settings.h"
#include "leds.h"

/********************** Setup *********************/
void setup(){

  delay(1000);      

  #ifdef _DEBUG    
    printf_begin();      
  #endif

  Serial.begin(115200);      
  LOGLN(F("[RESET]"));

  // Setup and configure rf radio
  radio.begin();       
  if( radio.isChipConnected() )
    LOGLN("Radio connected on SPI Bus.");
  else
    LOGLN("Radio not detected.");

  if( loadSettings())  
  {
    LOGLN("found some settings: ");
    printSettings();
    setRole(gSettings.role);    // this must be called after the radio has been started.
  }
  else
  {
    LOGLN("didn't find any settings, enterring config mode...");
    LOGLN("Please define role: (0 (config), 1 (sender), 2 (receiver 1), 3 (reciever 2), 4 (receiver 3)");     
  }    
  
  radio.setPALevel(RF24_PA_LOW);
 
  radio.enableAckPayload();                         // We will be using the Ack Payload feature, so please enable it
  radio.enableDynamicPayloads();                    // Ack payloads are dynamic payloads  
 
  delay(50);
  attachInterrupt(0, check_radio, FALLING);          // Attach interrupt handler to interrupt #0 (using pin 2) on BOTH the sender and receiver

  // Setup fast led.
  setupLeds();
}


/********************** Main Loop *********************/


void loop() 
{   
   handleInput();
   doLeds();				// Delays in here.
   
   switch(gSettings.role)
   {
    case eConfig:
      break;
      
    case eSender:
      doSender();
      break;
      
    case eReceiver1:
    case eReceiver2:
    case eReceiver3:
    case eReceiver4:
    case eReceiver5:
    case eReceiver6:
    case eReceiver7:
      doReciever();
      break;
      
    default:
      break;       
   }
}
  


