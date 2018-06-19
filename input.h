#pragma once

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

