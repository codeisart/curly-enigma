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
	eChangeHue		= 10,			// Tell other radios to change base hue.
	eChangePattern  = 20,	
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

	EVERY_N_SECONDS(5)
	{
		//radio.startListening();
		//delay(10);
		//radio.stopListening();
		
		static int TickCount = 0;
		TickCount++;
		LOGF("Tick %s %d\n", rolename[gSettings.role], TickCount);		
	}
}

void sendCmd(byte cmd, uint16_t param, int addressIdx=0 )
{
	radio.openWritingPipe(address[addressIdx]);	
	uint32_t data = (uint32_t) cmd | (uint32_t) param << 8;	
	LOGF("C{%u} P{%u} A{%s}\n", cmd, param, address[addressIdx]);	
	radio.stopListening();
	radio.startWrite( &data, sizeof(data) ,0);		// Non-blocking write.
}

void setupSender()
{ 
  LOGF("Sender... ");  
  radio.stopListening();
  
  LOGF("+w A=[%s]", (char*)address[0]);
  radio.openWritingPipe(address[0]);					// writing to broadcast address.
  
  for( int i = 1; i < ARRAY_SIZE(address); ++i )
  {
	radio.writeAckPayload(i,&ping_count, sizeof(ping_count));			
	radio.openReadingPipe(i,address[i]);								  // open reading pipes to all suits.
    LOGF("\t+r %d=A=%s", i, (const char*)address[i]);  
  }
  LOGF(" done.\n");
 
  
#ifdef _DEBUG
  radio.printDetails();                             // Dump the configuration of the rf unit for debugging
#endif// _DEBUG    
}

void setupReciever()
{ 
  LOGF("Rcrv...");  
  radio.stopListening();
  
  auto writeAddress = role_to_address(gSettings.role);
  radio.openWritingPipe(writeAddress);
    
  radio.setAutoAck(1, false);						// Turn off Ack on broadcast address.
  radio.openReadingPipe(1,address[0]);              // Broadcast.
  
  LOGF("\t+w=%s, +r=%s. \n", (const char*)writeAddress, (const char*)address[0]);
  radio.startListening();  
  
  ++ping_count;

#ifdef _DEBUG
  radio.printDetails();                             // Dump the configuration of the rf unit for debugging
#endif// _DEBUG    
}

void doReciever()
{ 	
	EVERY_N_SECONDS(5) 
	{ 
		//// Ping server to say we are alive.
		//radio.stopListening();
		//auto writeAddress = role_to_address(gSettings.role);
		//radio.openWritingPipe(writeAddress);
		//radio.setAutoAck(1, false);
		//unsigned long t = millis();
		//radio.startWrite(&t, sizeof(t), false);

		// Print tick log.
		static int TickCount = 0;
		TickCount++;
		LOGF("Tick %s %d\n", rolename[gSettings.role], TickCount);

		//radio.startListening();	//?
	} 
}

void setRole(int role)
{
  gSettings.role = role;  
  LOGF("Role=%d [%s]\n", gSettings.role, rolename[gSettings.role]);

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

  //radio.stopListening();
  
  if (gSettings.role == eConfig)
  {
	  LOGF("CfgMode Wtf?\n");
	  return;
  }

  if ( tx ) 
  {                                         // Have we successfully transmitted?
      if( gSettings.role == eSender )
	  {   
		  LOGF("Send:OK\n");
	  }
      if ( gSettings.role >= eReceiver1)
	  { 
		  LOGF("Ack Payload:Sent\n");
	  }
  }
  
  if ( fail ) 
  {                                       // Have we failed to transmit?
      if ( gSettings.role == eSender )
	  {   
		  //LOGF("Send:Failed\n");
	  }
      if( gSettings.role >= eReceiver1)
	  { 
		  LOGF("Ack Payload:Failed\n");
	  }
  }
 
  // If data is available, handle it accordingly
  if (rx)
  {
	  if (radio.getDynamicPayloadSize() < 1)
	  {
		  LOGF("Flushed\n");

		  // Corrupt payload has been flushed
		  return;
	  }

	  uint8_t pipe = 0xff;
	  uint32_t len = radio.getDynamicPayloadSize();
	  
	  byte payload[32];

	  // See if any other data is available.
	  for ( bool avl = radio.available(&pipe); avl; avl = radio.available(&pipe))
	  {
		  LOGF("%u bytes on %u\n", (int)len, (int)pipe );

		  if (gSettings.role == eSender)
		  {
			  // If we're the sender, we've received an ack payload	
			  radio.read(&ping_count, sizeof(ping_count));
			  LOGF("Ping: %s,\n", pipe >= 1 ? address[pipe - 1] : "Unknown");
			  radio.writeAckPayload(pipe, &ping_count, sizeof(ping_count));  // This will be queued and sent out next time a packet is received on this pipe.		
		  }
		  else if (gSettings.role >= eReceiver1)
		  {                    // If we're the receiver, we've received a time message						
			  if (pipe == 1)	// Broadcast message? 
			  {
				  radio.read(payload, len);
				  uint32_t* cmd = (uint32_t*) payload;

				  uint16_t param = *cmd >> 8;
				  *cmd &= 0xff;

				  LOGF("Got Cmd %u, %u\n", (int)*cmd, (int)param);

				  // handle cmd.
				  switch (*cmd)
				  {
				  default:
					  LOGF("Unknwn cmd\n");
					  break;
				  case eChangePattern:
					  LOGF("Cmd -> Pattern to %u\n", (int)param);
					  changePattern(param);
					  break;
				  case eChangeHue:
					  LOGF("Cmd -> HUE to %u\n", (int)param);
					  setHue(param);
					  break;
				  }

				  // We dont want to send Acks on the broadbast address as multiple listeners will stomp each other.
				  //radio.writeAckPayload( pipe, &ping_count, sizeof(ping_count) );  // Add an ack packet for the next time around.  This is a simple
				  ++ping_count;                                // packet counter
			  }
		  }
		  else
		  {			 
			  radio.read(payload, len);
		  }
	  }	  
  }
  
  // Put us back in listen mode.
  //radio.startListening();
}
 

