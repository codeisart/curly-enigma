/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 
 Update 2014 - TMRh20
 */

/**
 * Example of using interrupts
 *
 * This is an example of how to user interrupts to interact with the radio, and a demonstration
 * of how to use them to sleep when receiving, and not miss any payloads. 
 * The pingpair_sleepy example expands on sleep functionality with a timed sleep option for the transmitter.
 * Sleep functionality is built directly into my fork of the RF24Network library
 */

#include <SPI.h>
#include <EEPROM.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

static const uint32_t kMagicVal = 'JIMMY';

struct SettingsStruct {
  uint32_t magic;
  int role;
  SettingsStruct()
    : magic(kMagicVal)
    , role(0)   
  {}
} gSettings;

// Hardware configuration
RF24 radio(7,8);                                      // Set up nRF24L01 radio on SPI bus plus pins 7 & 8
static const uint32_t EEPROM_settingsAddress= 0x123;  // Random place in the EEPROM to save our state, that's likely unused.

char* rolename[]
{
  "Config",         //0
  "Sender",         //1
  "Receiver1",      //2
  "Receiver2",      //3
  "Receiver3",      //4
  "Receiver4",      //5
  "Receiver5",      //6
  "Receiver6",      //7
  "Receiver7",      //8
};

enum Mode {
  eConfig,         //0
  eSender,         //1
  eReceiver1,      //2
  eReceiver2,      //3
  eReceiver3,      //4
  eReceiver4,      //5
  eReceiver5,      //6
  eReceiver6,      //7
  eReceiver7,      //8  
};
enum Pipe
{
  eBroadcast = 1,
  eIndividual,
};

byte address[][5] = 
{ 
  {'b','d','c','s','t'},  // Broadcast address.                   0 (all clients are bound to this).
  {'S','u','i','t','1'},  // Sender                               1
  {'S','u','i','t','2'},  // Reciever1                            2
  {'S','u','i','t','3'},  // Reciever2                            3
  {'S','u','i','t','4'},  // Reciever3                            4
  {'S','u','i','t','5'},  // Reciever4                            5
  {'S','u','i','t','6'},  // Reciever5                            6
  {'S','u','i','t','7'},  // Reciever6                            7
  {'S','u','i','t','8'},  // Reciever7                            8
};

void software_Reset() // Restarts program from beginning but does not reset the peripherals and registers
{
  asm volatile ("  jmp 0");  
}  

/********************** Setup *********************/

void setup(){

  delay(1000);      
  
  Serial.begin(115200);
  printf_begin();  
  Serial.println(F("[RESET]"));

  if( loadSettings())  
  {
    Serial.println("found some settings: ");
    printSettings();
    //setRole(gSettings.role);
  }
  else
  {
    Serial.println("didn't find any settings, enterring config mode...");
    Serial.println("Please define role: (0 (config), 1 (sender), 2 (receiver 1), 3 (reciever 2), 4 (receiver 3)");     
  }

  radio.begin();  
      
  // Setup and configure rf radio
  if( radio.isChipConnected() )
    Serial.println("Radio connected on SPI Bus.");
  else
    Serial.println("Radio not detected.");
    
  
  radio.setPALevel(RF24_PA_LOW);
  radio.setPayloadSize(1);                // Here we are sending 1-byte payloads to test the call-response speed

  //radio.enableAckPayload();                         // We will be using the Ack Payload feature, so please enable it
  //radio.enableDynamicPayloads();                    // Ack payloads are dynamic payloads

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
  
  radio.printDetails();                             // Dump the configuration of the rf unit for debugging
  delay(50);
  //attachInterrupt(0, check_radio, LOW);             // Attach interrupt handler to interrupt #0 (using pin 2) on BOTH the sender and receiver
}

 #define ARRAY_SIZE(A) (sizeof(A)/sizeof(A[0]))

/********************** Main Loop *********************/

void printSettings()
{
  Serial.print("role = " );    
  Serial.println(rolename[gSettings.role]);
}
bool loadSettings()
{
  Serial.print(F("Reading from EEPROM... "));
 
  // load settings.
  EEPROM.get(EEPROM_settingsAddress, gSettings);
  return gSettings.magic == kMagicVal;
}
void writeSettings()
{
  Serial.print("Writing settings to EEPROM... ");
  EEPROM.put(EEPROM_settingsAddress,gSettings);
  Serial.println("done.");
}
void setRole(int role)
{
  Serial.print("Setting role to ");
  gSettings.role = role;  
  Serial.println(rolename[gSettings.role]);

  // do setup.
  if( gSettings.role == eSender )
    setupSender();
  else if( gSettings.role >= eReceiver1 && gSettings.role <= eReceiver7 )
    setupReciever();
 }

void handleInput()
{
  if (Serial.available() > 0) 
   {
    int inByte = Serial.read();
    switch (inByte) 
    {
      // numbers define the role.
      case '0':      
      case '1':      
      case '2':      
      case '3':      
      case '4':      
      case '5':      
      case '6':      
      case '7':      
      case '8':                 
        setRole(inByte - '0');      
        break;      
      case 'l':
        loadSettings();
        setRole(gSettings.role);
        break;
      case 'w':
        writeSettings();
        break;      
      case 'r':
        software_Reset();
        break;
        
      default: 
      {}       
     }
   }     
}

void setupSender()
{ 
  Serial.print("Setting up sender... ");
  radio.stopListening();
  Serial.println("done. ");
}

bool bShouldPing = true;

void doSender()
{  
  // send broadcast ping.
  radio.openReadingPipe(1,address[0]);
  radio.openWritingPipe(address[0]);  
  
  byte bytes[] = {'p','i','n','g'};
  if( bShouldPing )
  {
    Serial.print("Sending ping...");
    radio.write(bytes, ARRAY_SIZE(bytes), true);
    bShouldPing = false;
    radio.startListening();
  } 
}


void setupReciever()
{  
  Serial.print("Setting up receiver... ");
  
  radio.openReadingPipe(eBroadcast,  address[0]);               // pipe 1 broadcast.
  radio.setAutoAck(eBroadcast, false);
  
  radio.openReadingPipe(eIndividual, address[gSettings.role]);  // pipe 2 our address.
  radio.setAutoAck(eIndividual, true);
  
  radio.startListening();
  Serial.println("done.");

}

void doReciever()
{ 
  byte bytes[32];
  if( radio.available((int)eBroadcast) )
  {    
    radio.read( bytes, ARRAY_SIZE(bytes));
    Serial.print("Got ping");
  }
  if( radio.available((int)eIndividual) )
  {    
    radio.read( bytes, ARRAY_SIZE(bytes));
  }
}

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
  
  


/********************** Interrupt *********************/

