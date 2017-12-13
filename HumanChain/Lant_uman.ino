const int analogPin = 0;
const int clockPin = 9;
const int doorPin = 12;
const int pinSpeaker = 3;

const int pitch = 1200;

int highRead = 0;
int lowRead = 0;

int totalCycles = 0;
int activeCycles = 0;
int waitingTime = 0;
int resetTime = 0;

void setup()
{
  Serial.begin(9600);             // Setup serial
  pinMode(clockPin, OUTPUT);
  pinMode(doorPin, OUTPUT);
  pinMode(13, OUTPUT);  // sets as output
  digitalWrite(clockPin, LOW);
  digitalWrite(doorPin, LOW);
  Serial.println("System running!");
  playTone(pitch, 2000);
}

void loop()
{
digitalWrite(clockPin, HIGH);
digitalWrite(13, HIGH);
highRead = analogRead(analogPin);
highRead = map(highRead, 0, 1023, 0, 100);
delay(5);
digitalWrite(clockPin, LOW);
digitalWrite(13, LOW);
lowRead = analogRead(analogPin);
lowRead = map(lowRead, 0, 1023, 0, 100);
totalCycles++;
if(highRead>5 && lowRead==0){
  activeCycles++;
}
delay(5);

if(totalCycles >25) {
  if ( resetTime != waitingTime ) {
    resetTime = waitingTime = 0;
    Serial.println("Clear waiting time!");
  }
  resetTime ++;
  if( totalCycles - activeCycles <= 5)
  {
    waitingTime ++;
    Serial.println("Eronnous Cycles: ");
    Serial.println(totalCycles - activeCycles);   
    if(waitingTime == 5){
      playTone(pitch, 2000);
      waitingTime = 0;
      Serial.println("Door Opened!"); 
      Serial.println("Wait for it....");
      digitalWrite(doorPin, HIGH);
      delay(10000);
      digitalWrite(doorPin, LOW);
      Serial.println("Door Closed!");
    }
    else {
      playTone(pitch, 200);
      Serial.println("Beep!");
    }
    delay(900);
  }
  totalCycles = 0;
  activeCycles = 0;
}


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
