#pragma once

#include "nRF24L01.h"
#include "RF24.h"
#include "settings.h"
#include "FastLED.h"

// Hardware configuration
RF24 radio(7,8);                                      // Set up nRF24L01 radio on SPI bus plus pins 7 & 8

enum Pipe
{
  eBroadcast = 1,
  eIndividual,
};

enum RadioCommands
{
	eChangeHue,			// Tell other radios to change base hue.
	eChangePattern,	
};

// Simple messages to represent a 'ping' and 'pong'
static const uint32_t ping = 111;
static const uint32_t pong = 222;
static uint32_t ping_count = 0;

volatile uint32_t round_trip_timer = 0;

//byte addresses[][6] = {"1Node","2Node"};

const byte address[][6] = 
{ 
  "bdcst",  // Broadcast address / Sender Suit1     0 (all clients are bound to this).
  "suit2",  // Receiver1							1
  "suit3",  // Receiver2							2
  "suit4",  // Receiver3							3
};

#ifdef _DEBUG
	const char* power_levels[] = { "min", "high", "max" };
#endif// _DEBUG

const byte* role_to_address(int r)
{
	//eConfig,         //0 -> default to bdcst.
	//eSender,         //1 -> bdcst
	//eReceiver1,      //2 -> suit2
	//eReceiver2,      //3 -> suit3
	//eReceiver3,      //4 -> suit4
	//eMaxReceiver
	
	if( r >= eSender && r < eMaxReceiver)
	{
		return address[r - 1];
	}
	
	// default to bdcst.
	return address[0];
}

void setRadioPower(int power)
{
	if (power == RF24_PA_LOW || power == RF24_PA_HIGH || power == RF24_PA_MAX)
	{
		LOGF("Setting radio power to [%s]\n", power_levels[power] );
		radio.setPALevel(power);
		gSettings.power = power;		
	}
}

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

  //unsigned long time = millis();                   // Take the time, and send it.
  //LOG(F("Now sending "));
  //LOGLN(time);
  //radio.startWrite( &time, sizeof(unsigned long) ,0);
  //delay(2000);                                     // Try again soon
}

void sendCmd(byte cmd, uint16_t param, int addressIdx=0 )
{
	radio.openWritingPipe(address[addressIdx]);
	
	uint32_t data = (uint32_t) cmd | (uint32_t) param << 8;	
	LOGF("Now sending cmd {%u} and param {%u} to address %s\n", cmd, param, address[addressIdx]);	
	radio.stopListening();
	radio.startWrite( &data, sizeof(data) ,0);		// Non-blocking write.
}

void setupSender()
{ 
  LOG("Setting up Sender... ");  
  radio.stopListening();
  
  LOG("Writing");
  LOG((char*)address[0]);
  radio.openWritingPipe(address[0]);		// writing to broadcast address.
  
  for( int i = 1; i < ARRAY_SIZE(address); ++i )
  {
	radio.writeAckPayload(i,&ping_count, sizeof(ping_count));			
	radio.openReadingPipe(i,address[i]);								  // open reading pipes to all suits.
    LOG("\tReading ");  
    LOG(i);  
    LOG("=");
    LOG((const char*)address[i]);  
    LOG("\t");    
  }
  LOGLN("");
  LOGLN(" done. ");

  
  
#ifdef _DEBUG
  //radio.printDetails();                             // Dump the configuration of the rf unit for debugging
#endif// _DEBUG    
}

void setupReciever()
{ 
  LOG("Setting up receiver... ");  
  radio.stopListening();
  
  radio.openWritingPipe(role_to_address(gSettings.role));
  LOG("\tWriting");
  LOG((const char*)role_to_address(gSettings.role));
  
  radio.setAutoAck(1, false);						// Turn off Ack on broadcast address.
  radio.openReadingPipe(1,address[0]);              // Broadcast.
  LOG("\tReading ");  
  LOG((const char*)address[0]);
    
  LOG("\tStart listening");
  radio.startListening();
  
  
  
  ++ping_count;
    
  LOGLN(", done.");

#ifdef _DEBUG
  //radio.printDetails();                             // Dump the configuration of the rf unit for debugging
#endif// _DEBUG    
}

void doReciever()
{ 
	// Ping server to say we are alive.
	EVERY_N_SECONDS(5) 
	{ 
		// WIP.
		// Send ping to server every X
		//radio.stopListening();
		//radio.write
	} 
}

void setRole(int role)
{
  LOG("Setting role to ");
  gSettings.role = role;  
  LOGLN(rolename[gSettings.role]);

  // do setup.
  if( gSettings.role == eSender )
    setupSender();
  else if( gSettings.role >= eReceiver1 && gSettings.role < eMaxReceiver )
    setupReciever();
 }

void setHue(uint8_t hue);
void nextPattern();
void changePattern(int x);

void check_radio(void)                                // Receiver role: Does nothing!  All the work is in IRQ
{ 
  bool tx,fail,rx;
  radio.whatHappened(tx,fail,rx);                     // What happened?
   
  if ( tx ) 
  {                                         // Have we successfully transmitted?
      if( gSettings.role == eSender )
	  {   
		  LOGLN(F("Send:OK"));
	  }
      if ( gSettings.role >= eReceiver1)
	  { 
		  LOGLN(F("Ack Payload:Sent"));
	  }
  }
  
  if ( fail ) 
  {                                       // Have we failed to transmit?
      if ( gSettings.role == eSender )
	  {   
		  LOGLN(F("Send:Failed"));
	  }
      if( gSettings.role >= eReceiver1)
	  { 
		  LOGLN(F("Ack Payload:Failed"));
	  }
  }
 
  // If data is available, handle it accordingly
  if ( rx )
  {    
    if(radio.getDynamicPayloadSize() < 1)
	{
	   LOGLN("Corrupt payload has been flushed");

      // Corrupt payload has been flushed
      return; 
    }

    byte pipe= 0xff;
    bool avl = radio.available(&pipe);
    LOGF("data on pipe %d\n", pipe );

    if ( gSettings.role == eSender ) 
	{                      // If we're the sender, we've received an ack payload
        radio.read(&ping_count,sizeof(ping_count));
        LOGF("Got Ping from: %s, accking...\n", pipe >= 1 ? address[pipe-1] : "Unknown" );
		radio.writeAckPayload( pipe, &ping_count, sizeof(ping_count) );  // This will be queued and sent out next time a packet is received on this pipe.		
    }
    
    if ( gSettings.role >= eReceiver1 ) 
	{                    // If we're the receiver, we've received a time message
		uint32_t cmd;	
		radio.read( &cmd, sizeof(cmd) );

		uint16_t param = cmd >> 8;
		cmd &= 0xff;

		LOGF("Received Cmd %u, %u\n", cmd, param);
		
		// handle cmd.
		switch (cmd)
		{
			case eChangePattern: 
				LOGF("Cmd -> Change Pattern to %u\n", param);
				changePattern(param);
				break;
			case eChangeHue:
				LOGF("Cmd -> Change HUE to %u\n", param);
				setHue(param);
				break;
		}

		// We dont want to send Acks on the broadbast address as multiple listeners will stomp each other.
		//radio.writeAckPayload( pipe, &ping_count, sizeof(ping_count) );  // Add an ack packet for the next time around.  This is a simple
		++ping_count;                                // packet counter
    }
  }           
}
 

