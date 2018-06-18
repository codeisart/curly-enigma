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
  //radio.setPayloadSize(1);                // Here we are sending 1-byte payloads to test the call-response speed

  radio.enableAckPayload();                         // We will be using the Ack Payload feature, so please enable it
  radio.enableDynamicPayloads();                    // Ack payloads are dynamic payloads  
  //radio.openWritingPipe(address[0]);             // communicate back and forth.  One listens on it, the other talks to it.
  //radio.openReadingPipe(1,address[0]); 
  //radio.startListening();

/*
                                                    // Open pipes to other node for communication
  if ( role == role_sender ) {                      // This simple sketch opens a pipe on a single address for these two nodes to 
     radio.openWritingPipe(address[0]);             // communicate back and forth.  One listens on it, the other talks to it.
     radio.openReadingPipe(1,address[1]); 
  }else{
    radio.openWritingPipe(address[1]);
    radio.openReadingPipe(1,address[0]);
    radio.startListening();
    radio.writeAckPayload( 1, &message_count, sizeof(message_count) );  // Add an ack packet for the next time around.  This is a simple
    ++message_count;        
  }
*/

  delay(50);
  attachInterrupt(0, check_radio, LOW);             // Attach interrupt handler to interrupt #0 (using pin 2) on BOTH the sender and receiver
}


/********************** Main Loop *********************/


void loop() 
{   
   handleInput();
   
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
  


