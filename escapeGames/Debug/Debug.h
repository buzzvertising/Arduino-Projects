#ifdef DEBUG    //Macros are usually in all capital letters.
  #define DSERIALSTART(...)  Serial.begin(__VA_ARGS__) //DSERIALSTART is a macro, serial initialise.
  #define DPRINT(...)    Serial.print(__VA_ARGS__)     //DPRINT is a macro, debug print
  #define DPRINTLN(...)  Serial.println(__VA_ARGS__)   //DPRINTLN is a macro, debug print with new line
  #define DWRITE(...)  Serial.write(__VA_ARGS__)   //DWRITE is a macro, debug write
  
#else
  #define DSERIALSTART(...)  //now defines a blank line
  #define DPRINT(...)     //now defines a blank line
  #define DPRINTLN(...)   //now defines a blank line
  #define DWRITE(...)     //now defines a blank line
#endif


//print binary value of flags
void print16Bin(int);
void print8Bin(uint8_t);
int freeRam (void);