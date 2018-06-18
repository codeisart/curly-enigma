#pragma once

#include "radio.h"
#include <EEPROM.h>

static const uint32_t kMagicVal = 'JIMMY';
static const uint32_t EEPROM_settingsAddress= 0x123;  // Random place in the EEPROM to save our state, that's likely unused.

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

struct SettingsStruct {
  uint32_t magic;
  int role;
  SettingsStruct()
    : magic(kMagicVal)
    , role(0)   
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
  "Receiver4",      //5
  "Receiver5",      //6
  "Receiver6",      //7
  "Receiver7",      //8
};

#endif// _DEBUG

void printSettings()
{
  LOG("role = " );    
  LOGLN(rolename[gSettings.role]);
}
bool loadSettings()
{
  LOG(F("Reading from EEPROM... "));
 
  // load settings.
  EEPROM.get(EEPROM_settingsAddress, gSettings);
  return gSettings.magic == kMagicVal;
}
void writeSettings()
{
  LOG("Writing settings to EEPROM... ");
  gSettings.magic = kMagicVal;
  EEPROM.put(EEPROM_settingsAddress,gSettings);
  LOGLN("done.");
}
