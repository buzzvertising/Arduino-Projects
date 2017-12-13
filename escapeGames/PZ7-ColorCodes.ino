/* Game steps
1: read keypad and cycle neopixel colors
2: when done play pattern on/off on laser barrier */

#define DEBUG
#include <Debug.h>
#include <modularCPU.h>

#include <escape3_1.h>
#define SELF_ID CODES_N_LASERS_ID

#include <Wire.h>

//NEOPIXELS
#include <Adafruit_NeoPixel.h>

#define PIN V3IO_6
Adafruit_NeoPixel strip = Adafruit_NeoPixel(30, PIN, NEO_GRB + NEO_KHZ800);


#define NUM_LEDS 8 // Number of leds to address
#define KEYS_MAX 8 // Maximum nr of keys on keypad

byte RedState = false;

// define Colors 
#define RED      0xFF0000
#define GREEN     0x008000
#define DEEPSKYBLUE   0x00BFFF
#define YELLOW      0xFFFF00

//I2C KEYPAD
#include <i2ckeypad.h>

#define ROWS 2
#define COLS 4

// With A0, A1 and A2 of PCF8574 to ground I2C address is 0x20
#define PCF8574_ADDR 0x21

i2ckeypad kpd = i2ckeypad(PCF8574_ADDR, ROWS, COLS);


//INPUTS & OUTPUTS
byte I2Cdata = 0;

//Define Inputs
//Inputs Local
#define GAME_ON 0

//Inputs I2C Sensors

#define SENSOR_1_ON 0
#define SENSOR_2_ON 1
#define SENSOR_3_ON 2
#define SENSOR_4_ON 3
#define SENSOR_5_ON 4
#define SENSOR_6_ON 5

//Define Outputs
#define LASER_LED_1 0
#define LASER_LED_2 1
#define DIR_N_LED 2 //B00000100
#define DIR_E_LED 3 //B00001000
#define DIR_S_LED 4 //B00010000
#define DIR_V_LED 5 //B00100000
#define PENALTY_LED 6

#define PENALTY_SIG V3SIG_1
#define ACTIVE HIGH
#define INACTIVE LOW

#include <PCF8574.h>
// adjust addresses if needed
// 8574  Address range is 0x20-0x27
// 8574A Address range is 0x38-0x3F
PCF8574 IN_BLK1(0x22);  // add leds to lines      (used as input)
PCF8574 OUT_BLK1(0x20);  // add leds to lines      (used as Output)

Inputs inA; //local Inputs
Inputs inB; //Laser Sensors on I2C

byte sensors; 

#define ON HIGH
#define OFF LOW

#define EM_LOCK V3OUT_0
#define SECOND_DOOR V3SIG_2

#define LOCKED LOW
#define UNLOCKED HIGH

//TIMING  Definitions and Variables  
#include <MsTimer2.h>
#define TIMER_MS 10

//TIMERS
#define MAX_TIMERS 6 //Modify accordingly with the number of used timers
typedef struct{
	volatile uint16_t ctr[MAX_TIMERS];
	volatile uint16_t preset[MAX_TIMERS];
	volatile uint8_t reps[MAX_TIMERS];
	volatile uint16_t active;
	volatile uint16_t done;
} ALL_TIMERS;

ALL_TIMERS  myTimers;

//timers number
#define KEYPAD_TIMEOUT 0
#define DIRECTIONS_TEMPO 1
#define PENALTY_TIMEOUT 2
#define TOGGLE_RED 3
#define LASERS_TIMER 4
#define LAST_DOOR_TIMER 5


void startTimer(byte _timer, int _value, byte _reps=0);

//COMM Variables
byte packet[PACHET_SIZE]={0,0,0,0,0};
byte messageID = 0;

//GAME
byte gameStep = 0;

//SOUNDS
#define ALARM_ACTIVE_SOUND SOUND_70
#define ALARM_SOUND SOUND_71
#define ALL_DONE_SOUND SOUND_72

//#######################
// DEFINE DATA FOR ARROWS
//#######################
#define CODE_LENGTH 8
/*Define code colors - 
0 Black
1 Red/Top
2 Blue/Right
3 Yellow/Bottom
4 Green/Left
*/
//RYBGYGRB
byte code[CODE_LENGTH] = {1, 3, 2, 4, 3, 4, 1, 2};

//User readings
char key;
byte keyIndex;
byte reading[NUM_LEDS]={1,1,1,1,1,1,1,1}; //Keep the input form user

#define DIRECTIONS 4
/* 0   1     2      3 */
/* top right bottom left */
byte controlPins[DIRECTIONS] = {B00000100, B00001000, B00010000, B00100000}; //Logic OR Mask on the Output
byte position;

#define INITIAL_DURATION_MS 200
#define FINAL_DURATION_MS 700
#define PAUSE_DURATION_MS 5000
#define LIGHT_ON_PERCENT 0.6

int currentDuration = INITIAL_DURATION_MS;
int currentPosition= 0;

int LedTiming = 0;
bool LEDOn = false;


//Simon says LASER Pattern
byte simonSays[] = { 1, 1, 1, 7, 1, 7, 13};
#define PATTERN_LEN sizeof(simonSays)

byte index = 0;
#define ON_BASE 350
#define OFF_BASE 130


bool lasersON = true;
int delayLaser = 0;

#define SENSOR_MASK B00111111

void setup(){
	delay(2000);
	
	Serial.begin(4800);
	DPRINT(F("Starting"));
	
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
	

	DPRINT(F("Clear Outputs ..."));	
	I2Cdata = 128;
	updateOutputs(&OUT_BLK1, I2Cdata);
	DPRINTLN(F(" DONE. "));

	//initialize inputs
	inA.setActiveOn(GAME_ON,LOW); //input active on HIGH, default on LOW

	inB.setActiveOn(0,LOW); //input active on HIGH, default on LOW
	inB.setActiveOn(1,LOW); //input active on HIGH, default on LOW
	inB.setActiveOn(2,LOW); //input active on HIGH, default on LOW
	inB.setActiveOn(3,LOW); //input active on HIGH, default on LOW
	inB.setActiveOn(4,LOW); //input active on HIGH, default on LOW
	inB.setActiveOn(5,LOW); //input active on HIGH, default on LOW

	DPRINTLN(F(" DONE. ")); 

	DPRINT(F("NeoPixels setup..."));
	strip.begin();
	strip.show(); // Initialize all pixels to 'off'	
	DPRINTLN(F(" DONE. "));
	
	DPRINT(F("Timer Two setup..."));
	MsTimer2::set(TIMER_MS,TimeKeeper);
	MsTimer2::start();
	DPRINTLN(F(" DONE. "));
	
	digitalWrite(PENALTY_SIG, INACTIVE);

	DPRINTLN(F("System Running!")); 	
}


void loop(){
	delay(5);
	switch (gameStep){
		case 0:
			if ( scanInputs() ){
				DPRINTLN(F("Input Changed!"));
				if( inA.read(GAME_ON) && inA.changed(GAME_ON)){
					PlaySound(ALARM_ACTIVE_SOUND);
					startTimer(DIRECTIONS_TEMPO, 10000);
					I2Cdata = B11000011; //turn on LASER LEDs and Penalty message
					updateOutputs(&OUT_BLK1, I2Cdata);
					delay(200); //wait to stabilize before reading sensors
					sensors = scanI2CInputs(&IN_BLK1); //initialize laser sensors state.
					sensors &= B00111111; //mask only used sensors;
					inB.update(sensors);
					
					// initialize neo pixels and user readings 
					for (int i=0; i<KEYS_MAX; i++){
						strip.setPixelColor(i, RED);
						reading[i] = 1;
					}
					strip.show();
 					
					gameStep++;
				}
			}	
		break;	
		case 1:
			key = kpd.get_key();
			if(key != '\0') {
				startTimer(KEYPAD_TIMEOUT, 3000);
				
				keyIndex = convertKey(key);
				DPRINTLN(keyIndex);
								
				reading[keyIndex]++;
				if (reading[keyIndex] > 4) reading[keyIndex] = 0;
				changeLED(KEYS_MAX - keyIndex -1,reading[keyIndex]); //Change the color of corresponding LED 
				//(!! ledurile sunt montte invers. pentru leduri normale, linia este)
				// changeLED(keyIndex,reading[keyIndex]); //Change the color of 	corresponding LED 			
				if( validCode() ){
					for (int i=0; i<KEYS_MAX; i++){ //turn all leds GREEN
						strip.setPixelColor(i, GREEN);
					}  
					strip.show();
					
					digitalWrite(SECOND_DOOR, UNLOCKED);
					
					I2Cdata = B11000011; //turn directions LEDs off
					updateOutputs(&OUT_BLK1, I2Cdata);
					delay(20);
					PlaySound(ALL_DONE_SOUND);
          
					DPRINTLN(F("Access Granted!"));
					startTimer(LAST_DOOR_TIMER, 25000);
					startTimer(LASERS_TIMER, 1000);
				
					gameStep++;
					
					break;
				}
			}
			
			sensors = scanI2CInputs(&IN_BLK1);
			sensors &= SENSOR_MASK; //mask only used sensors;
			if ( inB.update(sensors) ) { //if inputs changed
				//DPRINTLN();DPRINTLN(sensors,BIN);
				if (sensors){
					PlaySound(ALARM_SOUND);
					startTimer(PENALTY_TIMEOUT, 1000);
					digitalWrite(PENALTY_SIG, ACTIVE);
				}
			}
			
			if( timerDone(PENALTY_TIMEOUT) ){			
				digitalWrite(PENALTY_SIG, INACTIVE);
			}
			
			if( timerDone(KEYPAD_TIMEOUT) ){
				//adjust Arrows display speed
				//adjustSpeed();
				
				startTimer(TOGGLE_RED, 300, 7); //Blink RED for timeout
				RedState = ON;
			}
			
			//Toogle Red on error.	
			if( timerDone(TOGGLE_RED) ){
				RedState = !RedState;
				if(myTimers.reps[TOGGLE_RED] == 0){
					RedState = OFF;
					for (int i=0; i<KEYS_MAX; i++){
						reading[i] = 0;
					}					
				}
				if (RedState){
					for (int i=0; i<KEYS_MAX; i++){
						strip.setPixelColor(i, RED);
					} 	
				}
				else{
					for (int i=0; i<KEYS_MAX; i++){
						strip.setPixelColor(i, 0);
					} 	
				}
				strip.show();
			}			
			
			
			if( timerDone(DIRECTIONS_TEMPO) ){
				if (LEDOn) {
					I2Cdata &= B11000011; //All leds off
					updateOutputs(&OUT_BLK1, I2Cdata);
					LedTiming = currentDuration * (1-LIGHT_ON_PERCENT);
					LEDOn = false;
					currentPosition++;
					if (currentPosition == CODE_LENGTH){
						currentPosition = 0;
						LedTiming = PAUSE_DURATION_MS;
					}

				} 
				else {
					position = code[currentPosition]-1;
					I2Cdata|= controlPins[position]; //Next led  on
					updateOutputs(&OUT_BLK1, I2Cdata);
					LedTiming = currentDuration;
					LEDOn = true;
				}
				startTimer(DIRECTIONS_TEMPO, LedTiming);
				//DPRINTLN(I2Cdata, BIN);
			} 
			
		break;
		case 2:
			if( timerDone(LAST_DOOR_TIMER) )
				digitalWrite(EM_LOCK, UNLOCKED);

			if( timerDone(LASERS_TIMER) ){
				lasersON = !lasersON;
				if (lasersON){
					I2Cdata = B11000011; //turn on LASER LEDs
					updateOutputs(&OUT_BLK1, I2Cdata);

					startTimer(LASERS_TIMER, ON_BASE);
					delay(50); //stabilize lasers
				}
				else{
					I2Cdata = B11000000; //turn off LASER LEDs
					updateOutputs(&OUT_BLK1, I2Cdata);

					delayLaser = OFF_BASE * simonSays[index];
					startTimer(LASERS_TIMER, delayLaser);

					index++;
					if (index == PATTERN_LEN) index = 0;
				}
			}
			delay(50);
			if (lasersON){
				sensors = scanI2CInputs(&IN_BLK1);
				sensors &= SENSOR_MASK; //mask only used sensors;
				if ( inB.update(sensors) ) { //if inputs changed
					if ( sensors ){
						PlaySound(ALARM_SOUND);
						startTimer(PENALTY_TIMEOUT, 2000);
						digitalWrite(PENALTY_SIG, ACTIVE);
					}
				}
			}

			if( timerDone(PENALTY_TIMEOUT) ){			
				digitalWrite(PENALTY_SIG, INACTIVE);
			}
		break;
	}	
}

//CODE functions
char convertKey(char _key){
	switch(_key){
		case '1': return 0;
		case '2': return 1;
		case '3': return 2;
		case 'A': return 3;
		case '4': return 4;
		case '5': return 5;
		case '6': return 6;
		case 'B': return 7;
	}
}

bool validCode(){
  for (int i=0; i<KEYS_MAX; i++) {   // Scan the whole key list.
       if ( reading[i] != (code[i]) ) return false;
  }
  return true;
} 

void changeLED(byte LEDno, byte Color){
 switch (Color) {
  case 0:
   strip.setPixelColor(LEDno, 0);
   break;
  case 1:
  strip.setPixelColor(LEDno, RED);
   break;
  case 2:
  strip.setPixelColor(LEDno, DEEPSKYBLUE);
   break;
  case 3:
  strip.setPixelColor(LEDno, YELLOW);
   break;
  case 4:
  strip.setPixelColor(LEDno, GREEN);
   break;
 }
 strip.show();
}

//############################
// DEFINE FUNCTIONS for ARROWS
//############################
void adjustSpeed() {
  if( currentDuration < FINAL_DURATION_MS){
	clearTimer(DIRECTIONS_TEMPO);
    currentDuration += 50;
	I2Cdata &= B11000011; //All leds off
	updateOutputs(&OUT_BLK1, I2Cdata);
	LEDOn = false;
	currentPosition = 0;
	LedTiming = PAUSE_DURATION_MS;
	startTimer(DIRECTIONS_TEMPO, LedTiming);
  }
}

void led(byte position) {

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

byte scanI2CInputs(PCF8574 *_IN_BLK){
	uint8_t tmp = TWBR;
	TWBR = 2; // speed up I2C;
	//Read Inputs Block 1 
	byte _sensors = _IN_BLK->read8();
	TWBR = tmp;	
	return _sensors;
}

void writeOutput(byte pin, byte state){
	bitWrite(I2Cdata, pin, state);
}


void updateOutputs(PCF8574 *_OUT_BLK, byte _I2Cdata){
	uint8_t tmp = TWBR;
	TWBR = 2; // speed up I2C;
	_OUT_BLK->write8(_I2Cdata);
	byte _currentData = _OUT_BLK->read8();
	if( _currentData != _I2Cdata ){
		delay(5);
		_OUT_BLK->write8(_I2Cdata);
	}
	TWBR = tmp;
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
//Communication Functions with SLAVE CPUs
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

