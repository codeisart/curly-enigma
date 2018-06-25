#pragma once

#include "radio.h"
#include <EEPROM.h>

#define BUILDSTRING "Sausages"

static const uint32_t kCurrentVer = 1;
static const uint32_t kMagicVal = 'JIMMY';
static const uint32_t EEPROM_settingsAddress= 0x123;  // Random place in the EEPROM to save our state, that's likely unused.

enum Mode {
  eConfig,         //0
  eSender,         //1
  eReceiver1,      //2
  eReceiver2,      //3
  eReceiver3,      //4
  eMaxReceiver
  //eReceiver4,      //5
  //eReceiver5,      //6
  //eReceiver6,      //7
  //eReceiver7,      //8  
};

struct SettingsStruct {
  uint32_t magic;
  int role;
  uint32_t ver;
  int power;

  SettingsStruct()
    : magic(kMagicVal)
    , role(0)   
	, power(RF24_PA_LOW)
  {}
} gSettings;

#ifdef _DEBUG

const char* rolename[]
{
  "Config",         //0
  "Sender",         //1
  "Receiver1",      //2
  "Receiver2",      //3
  "Receiver3",      //4
};

const char* power_levels[] = { "min", "high", "max" };


#endif// _DEBUG

void printSettings()
{
	LOGF("%s, %d\n", rolename[gSettings.role], power_levels[gSettings.power]);
}
bool loadSettings()
{
  //LOGF("Reading from EEPROM... ");
 
  // load settings.
  SettingsStruct s;
  EEPROM.get(EEPROM_settingsAddress, s);
  if (s.magic == kMagicVal)
  {
	  //LOGF("Found settings version %d\n", s.ver);
	  	  
	  // different version, migrate...
	  if (gSettings.ver < kCurrentVer)
	  {
		  //LOGF("Migrating settings...");
		  switch (gSettings.ver)
		  {
		  default:
		  case 0: 
			  s.power = RF24_PA_LOW;
			  break;
		  }
		  s.ver = kCurrentVer;
	  }
	  
	  // all good.
	  gSettings = s;
	  return true;

  }
  return false;
}
void writeSettings()
{
  //LOGF("Writing settings to EEPROM... ");
  gSettings.magic = kMagicVal;
  gSettings.ver = kCurrentVer;
  EEPROM.put(EEPROM_settingsAddress,gSettings);
  LOGF("done\n");
}
