#pragma once

#include "nRF24L01.h"
#include "RF24.h"
#include "settings.h"

// Hardware configuration
RF24 radio(7,8);                                      // Set up nRF24L01 radio on SPI bus plus pins 7 & 8

enum Pipe
{
  eBroadcast = 1,
  eIndividual,
};

// Simple messages to represent a 'ping' and 'pong'
static const uint32_t ping = 111;
static const uint32_t pong = 222;
static uint32_t message_count = 0;

volatile uint32_t round_trip_timer = 0;

//byte addresses[][6] = {"1Node","2Node"};

const byte address[][6] = 
{ 
  "bdcst",  // Broadcast address.                   0 (all clients are bound to this).
  "suit1",  // Sender                               1
  "suit2",  // Sender                               1
  "suit3",  // Sender                               1
  "suit4",  // Sender                               1
  "suit5",  // Sender                               1
};

void doSender()
{  
  // Only allow 1 transmission per 45ms to prevent overlapping IRQ/reads/writes
  // Default retries = 5,15 = ~20ms per transmission max
  //while(micros() - round_trip_timer < 45000){
    //delay between writes 
  //}
  //LOGLN("Sending ping...");
  //radio.stopListening();                
  //round_trip_timer = micros();
  //radio.startWrite( &ping, sizeof(ping),0 );

  unsigned long time = millis();                   // Take the time, and send it.
  Serial.print(F("Now sending "));
  Serial.println(time);
  radio.startWrite( &time, sizeof(unsigned long) ,0);
  delay(2000);                                     // Try again soon
}


void setupSender()
{ 
  LOG("Setting up Sender... ");  
  radio.stopListening();
  
  LOG("Writing");
  LOG((char*)address[0]);
  radio.openWritingPipe(address[0]);
  
  for( int i = 0; i < ARRAY_SIZE(address); ++i )
  {
    radio.openReadingPipe(1+i,address[i]);            // open reading pipes to all suits.
    LOG("\tReading ");  
    LOG(i);  
    LOG("=");
    LOG((const char*)address[i]);  
    LOG("\t");    
  }
  LOGLN("");
  LOGLN(" done. ");

#ifdef _DEBUG
  radio.printDetails();                             // Dump the configuration of the rf unit for debugging
#endif// _DEBUG    
}

void setupReciever()
{ 
  LOG("Setting up receiver... ");  
  radio.stopListening();
  
  radio.openWritingPipe(address[gSettings.role]);             
  LOG("\tWriting");
  LOG((const char*)address[gSettings.role]);  
  
  radio.openReadingPipe(1,address[0]);              // Broadcast.
  LOG("\tReading ");  
  LOG((const char*)address[0]);
   
  radio.startListening();
  radio.writeAckPayload( 1, &message_count, sizeof(message_count) );  // Add an ack packet for the next time around.  This is a simple
  ++message_count;
    
  LOGLN(", done.");

#ifdef _DEBUG
  radio.printDetails();                             // Dump the configuration of the rf unit for debugging
#endif// _DEBUG    
}

void doReciever()
{ 
}

void setRole(int role)
{
  LOG("Setting role to ");
  gSettings.role = role;  
  LOGLN(rolename[gSettings.role]);

  // do setup.
  if( gSettings.role == eSender )
    setupSender();
  else if( gSettings.role >= eReceiver1 && gSettings.role <= eReceiver7 )
    setupReciever();
 }


void check_radio(void)                                // Receiver role: Does nothing!  All the work is in IRQ
{
  LOGLN("int" );
  
  bool tx,fail,rx;
  radio.whatHappened(tx,fail,rx);                     // What happened?
   
  if ( tx ) {                                         // Have we successfully transmitted?
      if ( gSettings.role == eSender ){   Serial.println(F("Send:OK")); }
      if ( gSettings.role != eSender  ){ Serial.println(F("Ack Payload:Sent")); }
  }
  
  if ( fail ) {                                       // Have we failed to transmit?
      if ( gSettings.role == eSender ){   Serial.println(F("Send:Failed"));  }
      if ( gSettings.role != eSender ){ Serial.println(F("Ack Payload:Failed"));  }
  }
 
  // If data is available, handle it accordingly
  if ( rx ){
    
    if(radio.getDynamicPayloadSize() < 1){
      // Corrupt payload has been flushed
      return; 
    }

    byte pipe= 0xff;
    bool avl = radio.available(&pipe);
    LOG("data on pipe " );
    LOGLN(pipe);

    if ( gSettings.role == eSender ) {                      // If we're the sender, we've received an ack payload
        radio.read(&message_count,sizeof(message_count));
        Serial.print(F("Ack: "));
        Serial.println(message_count);
    }
    
    if ( gSettings.role != eSender ) {                    // If we're the receiver, we've received a time message
      static unsigned long got_time;                  // Get this payload and dump it
      radio.read( &got_time, sizeof(got_time) );
      Serial.print(F("Got payload "));
      Serial.println(got_time);
      radio.writeAckPayload( pipe, &message_count, sizeof(message_count) );  // Add an ack packet for the next time around.  This is a simple
      ++message_count;                                // packet counter
    }
  }
        
    // Read in the data
    /*
    uint32_t received;
    radio.read(&received,sizeof(received));

    byte cmd = received & 0xff; // byte is the cmd.
    uint32_t data = received >> 8;
    switch(cmd)
    {
      case ping:
      {
        radio.stopListening();
        // Normal delay will not work here, so cycle through some no-operations (16nops @16mhz = 1us delay)
        for(uint32_t i=0; i<130;i++){
           __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");
        }
        radio.startWrite(&pong,sizeof(pong),0);
        Serial.print("pong");
        break;
      }
      case pong:
      {
        round_trip_timer = micros() - round_trip_timer;
        Serial.print(F("Received Pong, Round Trip Time: "));
        Serial.println(round_trip_timer);
        break;
      }
    }
  }

  // Start listening if transmission is complete
  if( tx || fail ){
     radio.startListening(); 
     Serial.println(tx ? F(":OK") : F(":Fail"));     
  }  
  */
}
 

