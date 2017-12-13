#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <Debug.h>

//print binary value of flags
void print16Bin(int Flags){
	#ifdef DEBUG 
	DPRINTLN(F("\n5432109876543210"));
	for (unsigned int mask = 0x8000; mask; mask >>= 1) {
		if (mask & Flags) {
		   DPRINT('1');
		}
		else {
		   DPRINT('0');
		}
	}	
	DPRINTLN();
	#endif
}

void print8Bin(uint8_t Flags){
	#ifdef DEBUG 
	DPRINTLN(F("\n76543210"));
	for (unsigned int mask = 0x80; mask; mask >>= 1) {
		if (mask & Flags) {
		   DPRINT('1');
		}
		else {
		   DPRINT('0');
		}
	}	
	DPRINTLN();
	#endif
}

int freeRam () 
{
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}