/*Reads an analog input from a microphone and triggers a response on a number of simmilar frequencies sounds */

#define DEBUG
#include <Debug.h>
#include <modularCPU.h>

#include <escaape1_1.h>

#include <Wire.h>

#define SELF_ID DING_ID

//INPUTS & OUTPUTS
byte I2Cdata = 0;

//Define Inputs and Outputs
#define ACTIVATE 0

Inputs inA; //local Inputs

#define ON HIGH
#define OFF LOW

#define EM_LOCK V3OUT_0
#define LOCKED LOW
#define UNLOCKED HIGH

//TIMING  Definitions and Variables  
#include <MsTimer2.h>
#define TIMER_MS 10

//TIMERS
#define MAX_TIMERS 5
typedef struct{
	volatile uint16_t ctr[MAX_TIMERS];
	volatile uint16_t preset[MAX_TIMERS];
	volatile uint8_t reps[MAX_TIMERS];
	volatile uint16_t active;
	volatile uint16_t done;
} ALL_TIMERS;

ALL_TIMERS  myTimers;

//timers number
#define MIN_BEAT 0
#define CHECK_BELL 1
#define REPLAY_DELAY 2
#define TIMEOUT 3
#define FLASH_SIG 4


void startTimer(byte _timer, int _value, byte _reps=0);


//COMM Variables
byte packet[PACHET_SIZE]={0,0,0,0,0};
byte messageID = 0;

//GAME
byte gameStep = 0;



//FREQUENCY Variables and Definitions
//clipping indicator variables
boolean clipping = 0;

//data storage variables
byte newData = 0;
byte prevData = 0;
unsigned int time = 0;//keeps time and sends vales to store in timer[] occasionally
int timer[10];//sstorage for timing of events
int slope[10];//storage for slope of events
unsigned int totalTimer;//used to calculate period
unsigned int period;//storage for period of wave
byte index = 0;//current storage index
float frequency;//storage for frequency calculations
int maxSlope = 0;//used to calculate max slope as trigger point
int newSlope;//storage for incoming slope data

//variables for decided whether you have a match
byte noMatch = 0;//counts how many non-matches you've received to reset variables if it's been too long
byte slopeTol = 3;//slope tolerance- adjust this if you need
int timerTol = 10;//timer tolerance- adjust this if you need

//variables for amp detection
unsigned int ampTimer = 0;
byte maxAmp = 0;
byte checkMaxAmp;
byte ampThreshold = 20;//raise if you have a very noisy signal

//Peek and Beat Detection

#define PEEK_AMP 110
bool peek = false;
float averageFrequency = 0;
byte samples = 0;
bool getBeat = true;
bool readBell = true;
bool getFreq = true;

byte beats = 0;

byte peekMax = PEEK_AMP;

//SOUNDS
#define SHERLOCK_RECORDING SOUND_30

void setup(){
	delay(2000);
	
	Serial.begin(115200);
	DPRINTLN(F("Starting"));

	
		DPRINT(F("ADC data aquisition setup..."));
	//Setup ADC data aquisition
	cli();//diable interrupts

	//set up continuous sampling of analog pin 0 at 38.5kHz

	//clear ADCSRA and ADCSRB registers
	ADCSRA = 0;
	ADCSRB = 0;

	ADMUX |= (1 << REFS0); //set reference voltage
	ADMUX |= (1 << ADLAR); //left align the ADC value- so we can read highest 8 bits from ADCH register only

	ADCSRA |= (1 << ADPS2) | (1 << ADPS0); //set ADC clock with 32 prescaler- 16mHz/32=500kHz
	ADCSRA |= (1 << ADATE); //enabble auto trigger
	ADCSRA |= (1 << ADIE); //enable interrupts when measurement complete
	ADCSRA |= (1 << ADEN); //enable ADC
	ADCSRA |= (1 << ADSC); //start ADC measurements

	sei();//enable interrupts
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
	
	//initialize inputs
	inA.setActiveOn(ACTIVATE,LOW); //input active on HIGH, default on LOW

	DPRINTLN(F(" DONE. ")); 
	
	DPRINT(F("Timer Two setup..."));
	MsTimer2::set(TIMER_MS,TimeKeeper);
	MsTimer2::start();
	DPRINTLN(F(" DONE. ")); 
	
	DPRINTLN(F("System Running!")); 	
}


void loop(){
	delay(10);
	checkClipping();
	
	if (checkMaxAmp > peekMax){
		peekMax = checkMaxAmp; 

		DPRINT(checkMaxAmp);
		DPRINTLN("dB");	

		if( readBell && getBeat ){
			beats++;
			DPRINT("ding! ");
			DPRINT(beats);
			DPRINTLN(".");
			
			startTimer(MIN_BEAT, 100);
			getBeat = false;
			
			startTimer(TIMEOUT, 1200);
		}
	}
	
	if(beats == 3){
		startTimer(CHECK_BELL, 150);
	}	

	if(timerDone(CHECK_BELL) ){
		getFreq = true;
	}

	if(getFreq && beats >= 3)	{
		if (checkMaxAmp>ampThreshold && checkMaxAmp < PEEK_AMP){
			frequency = 38462/float(period);//calculate frequency timer rate/period
			if ( (frequency < 2200) && (frequency > 2000) ){
				DPRINT(checkMaxAmp);
				DPRINT("dB - ");					
				DPRINT(frequency);
				DPRINTLN(" hz");
				// }
				DPRINTLN("All done!");
				
				//reinitialise
				beats = 0;
				digitalWrite(V3SIG_1, HIGH); //send Play signal
				startTimer(FLASH_SIG, 300);
				
				startTimer(REPLAY_DELAY, 7000);
				getFreq = false;
				readBell = false;
			}
		}
	}
	
	if(timerDone(MIN_BEAT)){
		getBeat = true;
		peekMax = PEEK_AMP;
	}
	
	if(timerDone(REPLAY_DELAY)){
		readBell = true;
		peekMax = PEEK_AMP; 
		DPRINTLN("Replay Ready! ");
	}
	
	if(timerDone(FLASH_SIG)){
		digitalWrite(V3SIG_1, LOW);
	}
	
	if(timerDone(TIMEOUT)){
		beats = 0;
		DPRINTLN("Timeout! ");
	}
}


//Handle data aquisition
ISR(ADC_vect) {//when new ADC value ready
  
  PORTB &= B11101111;//set pin 12 low
  prevData = newData;//store previous value
  newData = ADCH;//get value from A0
  if (prevData < 127 && newData >=127){//if increasing and crossing midpoint
    newSlope = newData - prevData;//calculate slope
    if (abs(newSlope-maxSlope)<slopeTol){//if slopes are ==
      //record new data and reset time
      slope[index] = newSlope;
      timer[index] = time;
      time = 0;
      if (index == 0){//new max slope just reset
        PORTB |= B00010000;//set pin 12 high
        noMatch = 0;
        index++;//increment index
      }
      else if (abs(timer[0]-timer[index])<timerTol && abs(slope[0]-newSlope)<slopeTol){//if timer duration and slopes match
        //sum timer values
        totalTimer = 0;
        for (byte i=0;i<index;i++){
          totalTimer+=timer[i];
        }
        period = totalTimer;//set period
        //reset new zero index values to compare with
        timer[0] = timer[index];
        slope[0] = slope[index];
        index = 1;//set index to 1
        PORTB |= B00010000;//set pin 12 high
        noMatch = 0;
      }
      else{//crossing midpoint but not match
        index++;//increment index
        if (index > 9){
          reset();
        }
      }
    }
    else if (newSlope>maxSlope){//if new slope is much larger than max slope
      maxSlope = newSlope;
      time = 0;//reset clock
      noMatch = 0;
      index = 0;//reset index
    }
    else{//slope not steep enough
      noMatch++;//increment no match counter
      if (noMatch>9){
        reset();
      }
    }
  }
    
  if (newData == 0 || newData == 1023){//if clipping
    PORTB |= B00100000;//set pin 13 high- turn on clipping indicator led
    clipping = 1;//currently clipping
  }
  
  time++;//increment timer at rate of 38.5kHz
  
  ampTimer++;//increment amplitude timer
  if (abs(127-ADCH)>maxAmp){
    maxAmp = abs(127-ADCH);
  }
  if (ampTimer==1000){
    ampTimer = 0;
    checkMaxAmp = maxAmp;
    maxAmp = 0;
  }
  
}

void reset(){//clea out some variables
  index = 0;//reset index
  noMatch = 0;//reset match couner
  maxSlope = 0;//reset slope
}


bool checkClipping(){//manage clipping indicator LED
  if (clipping){//if currently clipping
    PORTB &= B11011111;//turn off clipping indicator led
    clipping = 0;
	return true;
  }
  return false;
}

//read inputs

bool scanInputs(){
	byte localInputs = 0;
	for(byte i=0; i<LOCAL_INPUTS; i++){
		byte input = digitalRead(inputs[i]);
		bitWrite(localInputs, i, input);
	}
	return inA.update(localInputs); //return true if any input changed
} //END scanInputs()

//*************************
//BASE CODE for Modular CPU
//*************************
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


