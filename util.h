#pragma once

#ifdef _DEBUG
  #define LOG(x) (Serial.print(x))
  #define LOGLN(x) (Serial.println(x))
#else 
  #define LOG(x) 
  #define LOGLN(x) 
#endif 

#define ARRAY_SIZE(A) (sizeof(A)/sizeof(A[0]))

void software_Reset() // Restarts program from beginning but does not reset the peripherals and registers
{
  asm volatile ("  jmp 0");  
}  

