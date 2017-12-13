/* Game steps
1: read input and lit keypad
2: read i2c Capacitive keypad and lit 4 code validation lights
3: turn on 4 lasers and read 4 sensors.
4: if all sensors are on open Electomagnetic lock */

#define DEBUG
#include <Debug.h>
#include <modularCPU.h>

#include <escape3_1.h>
#define SELF_ID CODES_N_LASERS_ID


#include <Wire.h>
#include "Adafruit_MPR121.h" //MPR121 Calacitive Keypad

// CAPACITIVE KEYPAD
Adafruit_MPR121 cap = Adafruit_MPR121();
#define MAX_KEYS 9
char numbers[MAX_KEYS]={'1','2','3','4','5','6','7','8','9'};
//MPR121 treshholds 
//room London Blackwing 2nd
#define PRESS_THRESH 7
#define RELEASE_THRESH 3
//room London Blackwing 1st
// #PRESS_THRESH 220
// #RELEASE_THRESH 50


// Keeps track of the last pins touched
// so we know when buttons are 'released'
uint16_t lastTouched = 0;
uint16_t currTouched = 0;

uint8_t key = 0;
 
//CODES
#define NUMBER_OF_CODES 4
#define MAX_USER_CODE_LENGTH 4

byte index = 0;
byte foundCodes = 0;
byte validationLEDs[4]={B10000010,B10000110,B10001110,B10011110}; //See Outputs bits

String userCode = String();

bool codeFound[NUMBER_OF_CODES] = {
  false,
  false,
  false,
  false
};

const String KEYBOARD_CODES[NUMBER_OF_CODES] = {
  String("4678"),
  String("3453"),
  String("5678"),
  String("2345")
};

//INPUTS & OUTPUTS
byte I2Cdata = 0;

//Define Inputs

//Inputs
#define GAME_ON 0
#define SENSOR_1_ON 1
#define SENSOR_2_ON 2
#define SENSOR_3_ON 3
#define SENSOR_4_ON 4

//Define Outputs

#define KEYPAD_LEDS 0
#define CODE_1_LEDS 1
#define CODE_2_LEDS 2
#define CODE_3_LEDS 3
#define CODE_4_LEDS 4
#define HAPTIC_MOTOR 5
#define LASER_POWER 6


#include <PCF8574.h>
// adjust addresses if needed
// 8574  Address range is 0x20-0x27
// 8574A Address range is 0x38-0x3F
PCF8574 OUT_BLK1(0x20);  // add leds to lines      (used as Output)

Inputs inA; //local Inputs
Inputs keys; //keys pressed

#define ON HIGH
#define OFF LOW

byte keypadLEDState = OFF; //keypad LEDs State for blinking

#define EM_LOCK V3OUT_0
#define LOCKED LOW
#define UNLOCKED HIGH

//TIMING  Definitions and Variables  
#include <MsTimer2.h>
#define TIMER_MS 10

//Non-blocking TIMERS
#define MAX_TIMERS 3
typedef struct{
	volatile uint16_t ctr[MAX_TIMERS];
	volatile uint16_t preset[MAX_TIMERS];
	volatile uint8_t reps[MAX_TIMERS];
	volatile uint16_t active;
	volatile uint16_t done;
} ALL_TIMERS;

ALL_TIMERS  myTimers;

//timers number
#define HAPTIC_TIMER 0
#define KEYPAD_TIMEOUT 1
#define KEYPAD_BLINK 2


void startTimer(byte _timer, int _value, byte _reps=0);

//COMM Variables
byte packet[PACHET_SIZE]={0,0,0,0,0};
byte messageID = 0;

//GAME
byte gameStep = 0;

//SOUNDS
#define LIGHT_ON_SOUND SOUND_50
#define CORRECT_SOUND SOUND_51
#define CODES_COMPLETE_SOUND SOUND_52
#define MOVE_ROCK_SOUND SOUND_53
#define TOUCH_SOUND SOUND_54
#define ERROR_SOUND SOUND_55

void setup(){
	Serial.begin(4800);
	DPRINTLN(F("Starting"));

	DPRINT(F("Clear Outputs ..."));	
	I2Cdata = 128;
	updateOutputs(&OUT_BLK1, I2Cdata);
	DPRINTLN(F(" DONE. "));
	
	delay(5000);
	DPRINT(F("RS485 setup..."));
	pinMode(TX_ENABLE, OUTPUT);
	pinMode(RX_ENABLE, OUTPUT);
	enableReceiving();	
	DPRINTLN(F(" DONE. "));
	
	DPRINT(F("Inputs & Outputs setup..."));
	//initialize Outputs
	for (byte i=0; i< LOCAL_OUTPUTS; i++){
		pinMode(outputs[i] ,OUTPUT);
	}
	//initialize Signals
	for (byte i=0; i< LOCAL_SIGNALS; i++){
		pinMode(signals[i] ,OUTPUT);
	}
	
	Wire.begin();                                  // Start Wire (I2C)
	Wire.setClock(400000);                         // Restart the I2C bus at 400kHz
	
	
	// initialize I2C Outputs
	OUT_BLK1.write8(128); //initialize all Outputs on LOW

	//initialize inputs
	inA.setActiveOn(GAME_ON,LOW); //input active on HIGH, default on LOW
	inA.setActiveOn(SENSOR_1_ON,HIGH); //input active on HIGH, default on LOW
	inA.setActiveOn(SENSOR_2_ON,HIGH); //input active on HIGH, default on LOW
	inA.setActiveOn(SENSOR_3_ON,HIGH); //input active on HIGH, default on LOW
	inA.setActiveOn(SENSOR_4_ON,HIGH); //input active on HIGH, default on LOW

	DPRINTLN(F(" DONE. ")); 
	
	DPRINT(F("Timer Two setup..."));
	MsTimer2::set(TIMER_MS,TimeKeeper);
	MsTimer2::start();
	DPRINTLN(F(" DONE. "));
	
	DPRINT(F("MPR121 setup..."));
	// if (!MPRinit()) {
		// DPRINTLN(F("MPR121 not found, check wiring?"));
		// while (1);
	// }
	
	
	// Default address is 0x5A, if tied to 3.3V its 0x5B
	// If tied to SDA its 0x5C and if SCL then 0x5D
	if (!cap.begin(0x5A)) {
		DPRINTLN(F("MPR121 not found, check wiring?"));
		while (1);
	}
	DPRINT(F("MPR121 found ..."));	
	cap.setThreshholds(PRESS_THRESH,RELEASE_THRESH);
	DPRINTLN(F(" DONE. "));
	
	DPRINTLN(F("System Running!")); 	
}


void loop(){
	delay(10);
	switch (gameStep){
		case 0:
			if ( scanInputs() ){
				DPRINTLN(F("Input Changed!"));
				if( inA.read(GAME_ON) && inA.changed(GAME_ON)){
					gameStep++;
					PlaySound(LIGHT_ON_SOUND);
					bitWrite(I2Cdata,  KEYPAD_LEDS, ON);
					DPRINT(F("\nI2Cdata: "));
					DPRINT(I2Cdata);
					keypadLEDState = ON;
					updateOutputs(&OUT_BLK1, I2Cdata);
				}
			}
		break;	
		case 1:
			currTouched = cap.touched(); //Read Keypad touches
			for (key=0; key<MAX_KEYS; key++) {
				// it if *is* touched and *wasnt* touched before, alert!
				if ((currTouched & _BV(key)) && !(lastTouched & _BV(key)) ) {
					#ifdef DEBUG
						// debugging info
						DPRINT("\nKey : ");
						for (uint8_t i=0; i<12; i++) {
						DPRINT(i); DPRINT("  \t");
						}
						DPRINT("\nFilt: ");
						for (uint8_t i=0; i<12; i++) {
						DPRINT(cap.filteredData(i)); DPRINT("\t");
						}
						DPRINTLN(".");
						DPRINT("Base: ");
						for (uint8_t i=0; i<12; i++) {
						DPRINT(cap.baselineData(i)); DPRINT("\t");
						}
						DPRINTLN(".");
					#endif
					
					DPRINT(F("\n")); DPRINT(key); DPRINTLN(" touched");
					break;
				}
			}			
			// reset our state
			lastTouched = currTouched;
			
			if( key < MAX_KEYS ){
				bitWrite(I2Cdata, HAPTIC_MOTOR, ON);
				bitWrite(I2Cdata, KEYPAD_LEDS, OFF);
				PlaySound(TOUCH_SOUND);
				startTimer(HAPTIC_TIMER, 100); //activate haptic motor for 600 miliseconds
				updateOutputs(&OUT_BLK1, I2Cdata);
				startTimer(KEYPAD_TIMEOUT, 2000); //set a timeout for keypad.

				if( addToUserCode(numbers[key]) ){
					DPRINT(userCode);
					if ( newCode() ){
						clearTimer(KEYPAD_TIMEOUT); //clear error timeout
						I2Cdata = I2Cdata | validationLEDs[foundCodes];
						updateOutputs(&OUT_BLK1, I2Cdata);					
						foundCodes++;
					}
				}
			}

			//Handle timeut of timers
			if( timerDone(KEYPAD_TIMEOUT) ){
				//reinitialise keypad
				DPRINTLN("reinitialise keypad");
				cap.begin(0x5A);
				cap.setThreshholds(PRESS_THRESH,RELEASE_THRESH);		
				
				PlaySound(ERROR_SOUND);
				startTimer(KEYPAD_BLINK, 300, 6);
				userCode.remove(0);
			}

			if( timerDone(KEYPAD_BLINK) ){
				//blink keypad lights on error or timeout
				keypadLEDState = !keypadLEDState;
				if(myTimers.reps[KEYPAD_BLINK] == 0){
					keypadLEDState = ON;
				}
				bitWrite(I2Cdata,  KEYPAD_LEDS, keypadLEDState);
				updateOutputs(&OUT_BLK1, I2Cdata);
				DPRINT(F("\nI2Cdata: "));
				DPRINT(I2Cdata);				
			}
			
			if( timerDone(HAPTIC_TIMER) ){
				//run haptic feedback
				bitWrite(I2Cdata, HAPTIC_MOTOR, OFF);
				bitWrite(I2Cdata, KEYPAD_LEDS, ON);
				updateOutputs(&OUT_BLK1, I2Cdata);
			}
			
			if(foundCodes == NUMBER_OF_CODES){
				gameStep++;
				delay(500);
				PlaySound(CODES_COMPLETE_SOUND);
				I2Cdata = B11011111; //all Validation LEDs ON keypad LEDs OFF - LASERS ON
				updateOutputs(&OUT_BLK1, I2Cdata);
				delay(1000);	
			}
		break;
		case 2:
			//read laser sensors and open door if both active
			if ( scanInputs() ){
				DPRINTLN(F("Input Changed!"));
				byte activeLasers = inA.read(SENSOR_1_ON) && inA.read(SENSOR_2_ON);
				if( activeLasers ){
					delay(6000);
					PlaySound(MOVE_ROCK_SOUND);
					digitalWrite(EM_LOCK, UNLOCKED);
					I2Cdata = B10011111; //all Validation LEDs ON keypad LEDs OFF - LASERS ON
					updateOutputs(&OUT_BLK1, I2Cdata);
					delay(1000);					
					while(1){};
				}
			}
		break;
	}	
}

//CODE functions
bool newCode(){
	for(index = 0; index < NUMBER_OF_CODES; index++) {
		if (KEYBOARD_CODES[index].compareTo(userCode) == 0) {
		  break;
		}
	}	
	userCode.remove(0); //reset user code
	if (index != NUMBER_OF_CODES && !codeFound[index]) {
		codeFound[index] = true;
		return true;
	}
	return false;
}

bool addToUserCode(char code){
    userCode += code;
	if (userCode.length() == MAX_USER_CODE_LENGTH) {
		return true;
	}
	return false;
}

bool MPRinit() {
  // soft reset
  cap.writeRegister(MPR121_SOFTRESET, 0x63);
  delay(10);
  cap.writeRegister(MPR121_ECR, 0x0);

  uint8_t c = cap.readRegister8(MPR121_CONFIG2);
  
  if (c != 0x24) return false;

  cap.writeRegister(MPR121_MHDR, 0x01);
  cap.writeRegister(MPR121_NHDR, 0x01);
  cap.writeRegister(MPR121_NCLR, 0x0E);
  cap.writeRegister(MPR121_FDLR, 0x00);

  cap.writeRegister(MPR121_MHDF, 0x01);
  cap.writeRegister(MPR121_NHDF, 0x05);
  cap.writeRegister(MPR121_NCLF, 0x01);
  cap.writeRegister(MPR121_FDLF, 0x00);

  cap.writeRegister(MPR121_NHDT, 0x00);
  cap.writeRegister(MPR121_NCLT, 0x00);
  cap.writeRegister(MPR121_FDLT, 0x00);

  cap.writeRegister(MPR121_DEBOUNCE, 0);
  cap.writeRegister(MPR121_CONFIG1, 0x20); // 0x10 for 16uA  (default) and  0x20 for 32uA charge current
  cap.writeRegister(MPR121_CONFIG2, 0x3A); // 0x20 for 0.5uS encoding, 1ms period; 0x3A for 0.5us encoding, 18 samples, 4ms period

  return true;
}

void MPRStart(){
  cap.setThreshholds(PRESS_THRESH,RELEASE_THRESH);
  cap.writeRegister(MPR121_ECR, 0x8F);  // start with first 5 bits of baseline tracking
}
//*************************
//BASE CODE for Modular CPU
//*************************
bool scanInputs(){
	byte localInputs = 0;
	for(byte i=0; i<LOCAL_INPUTS; i++){
		byte input = digitalRead(inputs[i]);
		bitWrite(localInputs, i, input);
	}
	return inA.update(localInputs); //return true if any input changed
} //END scanInputs()

void updateOutputs(PCF8574 *_OUT_BLK, byte _I2Cdata){
	uint8_t tmp = TWBR;
	//TWBR = 2; // speed up I2C;
	_OUT_BLK->write8(_I2Cdata);
	byte _currentData = _OUT_BLK->read8();
	if( _currentData != _I2Cdata ){
		delay(5);
		_OUT_BLK->write8(_I2Cdata);
	}
	//TWBR = tmp;
}

void TimeKeeper (){
	for(byte i = 0; i < MAX_TIMERS; i++){
		if(bitRead(myTimers.active, i)){             // if key countdown activated
			if(myTimers.ctr[i]){      // if countdown in progress
				myTimers.ctr[i]--;      // count down
			}
			if(myTimers.ctr[i] == 0){
				if(myTimers.reps[i]){
					myTimers.reps[i]--;
					myTimers.ctr[i] = myTimers.preset[i];
				}
				if (myTimers.reps[i] == 0 ){
					bitClear(myTimers.active, i);	// count down reached 0, disable
				}
				bitSet(myTimers.done, i);   	// call user's function
			}
		}
	}
} 
 
 bool timerDone(byte _timer){ //timer of (_timer * TIMER_MS) miliseconds.
	bool _timerState;
	_timerState = bitRead(myTimers.done, _timer);
	bitClear(myTimers.done, _timer);
	return _timerState;
}

void startTimer(byte _timer, int _value, byte _reps){
	_value = _value/TIMER_MS;
	myTimers.ctr[_timer] = _value;
	myTimers.preset[_timer] = _value;
	myTimers.reps[_timer] = _reps;
	bitClear(myTimers.done, _timer);
	bitSet(myTimers.active, _timer);
}

void clearTimer(byte _timer){
	bitClear(myTimers.active, _timer);
	bitClear(myTimers.done, _timer);
}
 
 //***************************************
//Communication Functions with SLAVE Arduinos
//***************************************

void enableTrasmission(){
	digitalWrite(TX_ENABLE, TRASNSMIT);
	digitalWrite(RX_ENABLE, TRASNSMIT);	
}

void enableReceiving(){
	digitalWrite(TX_ENABLE, RECEIVE);
	digitalWrite(RX_ENABLE, RECEIVE);	
}

void PlaySound(byte sound){
	buildPacket(sound, MP3_ID);
	enableTrasmission();
	sendMsg (fWrite, packet, sizeof packet);
	while (!(UCSR0A & (1 << UDRE0)))  // Wait for empty transmit buffer
		UCSR0A |= 1 << TXC0;  // mark transmission not complete
	while (!(UCSR0A & (1 << TXC0)));   // Wait for the transmission to complete
	enableReceiving();
	messageID++;
}

void buildPacket(byte _data, byte _destination){
	packet[DESTINATION_ID] = _destination;
	packet[SOURCE_ID] = SELF_ID;
	packet[TYPE] = CMD;	
	packet[MESSAGE_ID] = messageID;
	packet[PAYLOAD] = _data;
}

void fWrite (const byte what){
	Serial.write (what);  
}

int fAvailable (){
	return Serial.available ();  
}

int fRead (){
	return Serial.read ();  
} 

//****************************
// END Communication Functions
//****************************





