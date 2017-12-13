#ifndef ModularCPU_h
#define ModularCPU_h

//define R-CPU V3 pins
#define V3IO_1 A2
#define V3IO_2 A3
#define V3IO_3 8
#define V3IO_4 9
#define V3IO_5 10
#define V3IO_6 11

#define V3OUT_0 4
#define V3OUT_1 5
#define V3OUT_2 6

#define V3SIG_1 7
#define V3SIG_2 12
#define V3SIG_3 13
#define V3SIG_4 2
#define V3SIG_5 3

//define MP3 Pins
#define MP3IO_1 A0
#define MP3IO_2 A1
#define MP3IO_3 A2
#define MP3IO_4 A3
#define MP3IO_5 6
#define MP3IO_6 7



//define inputs
#define IN_1 0
#define IN_2 1 
#define IN_3 2
#define IN_4 3
#define IN_5 4
#define IN_6 5
#define IN_7 6
#define IN_8 7

//define inputs
#define OUT_1 0
#define OUT_2 1 
#define OUT_3 2
#define OUT_4 3
#define OUT_5 4
#define OUT_6 5
#define OUT_7 6
#define OUT_8 7


//define Game FLAGS
#define FLAG_1 0
#define FLAG_2 1
#define FLAG_3 2
#define FLAG_4 3
#define FLAG_5 4
#define FLAG_6 5
#define FLAG_7 6
#define FLAG_8 7
#define FLAG_9 8
#define FLAG_10 9
#define FLAG_11 10
#define FLAG_12 11
#define FLAG_13 12
#define FLAG_14 13
#define FLAG_15 14
#define FLAG_16 15

//define macros for FLAGS manipulation
#define setF(BIT) bitSet(gameFlags,BIT)
#define clearF(BIT) bitClear(gameFlags,BIT)
#define getF(BIT) bitRead(gameFlags,BIT)
#define writeF(BIT,STATE) bitWrite(gameFlags,BIT,STATE)


//define class to work with the inputs
class Inputs {
public:
	Inputs();
	
	uint8_t update(uint8_t);
	bool read(uint8_t);
	bool changed(uint8_t);
	void setActiveOn(uint8_t, uint8_t);
	uint8_t updateOne(uint8_t, uint8_t);	
	
private:	
	uint8_t inputsCurrent;
	uint8_t inputsChanged;
	uint8_t inputsPrevious;
	uint8_t activeOn;
};

#endif
