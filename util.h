#pragma once

#ifdef _DEBUG

#include <stdarg.h>
void printf_local(char *fmt, ...) 
{
	char buf[128]; // resulting string limited to 128 chars
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, 128, fmt, args);
	va_end(args);
	Serial.print(buf);
}

  //#define LOG(x) //(Serial.print(x))
  //#define LOGLN(x) //(Serial.println(x))
  #define LOGF(...) (printf_local(__VA_ARGS__))
#else 
  #define LOG(x) 
  #define LOGLN(x) 
  #define LOGF(...)
#endif 

#define ARRAY_SIZE(A) (sizeof(A)/sizeof(A[0]))

void software_Reset() // Restarts program from beginning but does not reset the peripherals and registers
{
  asm volatile ("  jmp 0");  
}  

