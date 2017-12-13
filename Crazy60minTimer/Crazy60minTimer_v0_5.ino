#include <avr/sleep.h>
#include <avr/wdt.h>

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

//constants
const int CoilA = 9;
const int CoilB = 10;

const int pinSpeaker = 7;


const int normal = 120;
const int fast = 144;
const int slow = 36;

const int pitch = 1200;
int beep = 0;

const unsigned int chargeTime = 50; //drive a coil for 50ms

//variables

volatile boolean f_wdt = 0;
boolean flag = true;

int run = 0;         //number of interupts in one hour
int tic = 0;            //number of interupts for on tac
int tac = 0;          //number of tac to do

int tictac = 1;         //define the speed of tac 1-normal 2-fast 3-slow

void setup()
{
  delay(2000);
  playTone(pitch, 1000); 
  delay(1000); //Allow a 30 sec delay before start.

  // CPU Sleep Modes 
  // SM2 SM1 SM0 Sleep Mode
  // 0    0  0 Idle
  // 0    0  1 ADC Noise Reduction
  // 0    1  0 Power-down
  // 0    1  1 Power-save
  // 1    0  0 Reserved
  // 1    0  1 Reserved
  // 1    1  0 Standby(1)

  cbi( SMCR,SE );      // sleep enable, power down mode
  cbi( SMCR,SM0 );     // power down mode
  sbi( SMCR,SM1 );     // power down mode
  cbi( SMCR,SM2 );     // power down mode


  pinMode(CoilA, OUTPUT);       // conect coil thrugh 330Ω
  pinMode(CoilB, OUTPUT);       // coil return
  pinMode(pinSpeaker, OUTPUT);     // piezoSpeaker
 
  digitalWrite(CoilA, LOW);      // reset coil
  digitalWrite(CoilB, LOW);      // reset coil
  
  run = 7200;         // start the timer
  tic = 2;            // number of interupts for on tac
  tac = 120;          // number of tac to do

  setup_watchdog(5); // Start WD timer at every 500ms

}

void loop()
{
  if (f_wdt==1)   // wait for timed out watchdog / flag is set when a watchdog timeout occurs
  {
    f_wdt=0;       // reset flag
    // Do Job

      if (run > 0) 
      {
        if (run%600 == 0) beep = 5;
        run --;
        tic --;
        if (tic == 0)
        {
          //prepare outputs to be used
          pinMode(CoilA, OUTPUT);  // set all ports into state before sleep
          pinMode(CoilB, OUTPUT);  // set all ports into state before sleep
          pinMode(pinSpeaker, OUTPUT);           
          
          doTac();
          tac --;
          changeSpeed();

          // Prepare to sleep
          pinMode(CoilA, INPUT);  // set all used port to intput to save power
          pinMode(CoilB, INPUT);  // set all used port to intput to save power
          pinMode(pinSpeaker, INPUT); 
        }
      }

    system_sleep();
  }  
}

void changeSpeed()
{
  if (tac == 0)
  {
    tictac ++;
    if (tictac == 4) tictac = 1;
    switch (tictac)
    {
      case 1:
        tac = normal;
        tic = 2;
        break;
      case 2:
        tac = fast;
        tic = 1;
        break;
      default:
        tac = slow;
        tic = 6;
    }
  }
  else 
  {
    switch (tictac)
    {
       case 1:
         tic = 2;
         break;
       case 2:
         tic = 1;
         break;
       default:
         tic = 6;
     }
  }
}

// Moves the seconds hand
void doTac() 
{
   flag = ! flag;
   if(flag == true)
   {
     digitalWrite(CoilA, HIGH);    // coil drive foward
     delay(chargeTime);            // wait
     digitalWrite(CoilA, LOW);     // coil drive end
   }
   else
   {
     digitalWrite(CoilB, HIGH);   // coil drive revers
     delay(chargeTime);           // wait
     digitalWrite(CoilB, LOW);    // coil drive end
   }
   PORTB ^= 32;  // toggle pin LED (13)
    
   if ( beep ) { playTone(pitch, 100); beep --; }
   if ( run <= 180 ) playTone(pitch, 100);
   if ( run == 0 ) playTone(pitch, 5000);
}

// duration in mSecs, frequency in hertz
void playTone(int freq, long duration) {
    duration *= 1000;
    int period = (1.0 / freq) * 1000000;
    long elapsed_time = 0;
    while (elapsed_time < duration) {
        digitalWrite(pinSpeaker,HIGH);
        delayMicroseconds(period / 2);
        digitalWrite(pinSpeaker, LOW);
        delayMicroseconds(period / 2);
        elapsed_time += (period);
    }
}

//****************************************************************  
// set system into the sleep state 
// system wakes up when wtchdog is timed out
void system_sleep()
{

  cbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter OFF

  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
  sleep_enable();

  sleep_mode();                        // System sleeps here

    sleep_disable();                     // System continues execution here when watchdog timed out 
    sbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter ON

}

//****************************************************************
// 0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms
// 6=1 sec,7=2 sec, 8=4 sec, 9= 8sec
void setup_watchdog(int ii)
{
  byte bb;
  int ww;
  if (ii > 9 ) ii=9;
  bb=ii & 7;
  if (ii > 7) bb|= (1<<5);
  bb|= (1<<WDCE);
  ww=bb;

  MCUSR &= ~(1<<WDRF);
  // start timed sequence
  WDTCSR |= (1<<WDCE) | (1<<WDE);
  // set new watchdog timeout value
  WDTCSR = bb;
  WDTCSR |= _BV(WDIE);
}
//****************************************************************  
// Watchdog Interrupt Service / is executed when  watchdog timed out
ISR(WDT_vect)
{
  if(f_wdt == 0)
  {
      f_wdt=1; // set global flag
  }
  else
  {
    //WDT Overrun!!!
  }  
}

