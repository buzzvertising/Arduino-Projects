// For Arduino 1.0 and earlier
#include "Arduino.h"

#include <ModularCPU.h>


//define Functions of Inputs class
Inputs::Inputs(){
	inputsCurrent = 0;
	inputsChanged = 0;
	inputsPrevious = 0;
	activeOn = 0;
}

uint8_t Inputs::update(uint8_t inputsValue){
	inputsCurrent = inputsValue;
	inputsChanged = inputsPrevious ^ inputsCurrent;
	inputsPrevious = inputsCurrent;
	return inputsChanged;
}

uint8_t Inputs::updateOne(uint8_t _pin, uint8_t _value){
	if(_value) {
		bitSet(inputsCurrent, _pin);
	}
	else{
		bitClear(inputsCurrent, _pin);		
	}
	inputsChanged = inputsPrevious ^ inputsCurrent;
	inputsPrevious = inputsCurrent;
	return inputsChanged;
}

void Inputs::setActiveOn(uint8_t pin, uint8_t state){
	bitWrite(activeOn,pin,state);
}

bool Inputs::read(uint8_t pin){
	return ((byte)( bitRead(inputsCurrent, pin)) == ((byte)bitRead(activeOn, pin)));
}

bool Inputs::changed(uint8_t pin){
	return bitRead(inputsChanged, pin);
}