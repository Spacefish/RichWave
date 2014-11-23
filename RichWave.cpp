/*
Created by Timo Witte <timo.witte@gmail.com>

DO WHATEVER YOU WANT PUBLIC LICENSE 
 Everyone is permitted to copy and distribute verbatim or modified 
 copies of this software and license, changing it is allowed.

            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE 
   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION 

  0. You just DO WHAT THE FUCK YOU WANT TO.
  
Version 1.0:
  - initial version
  - set Register and setFrequency support
*/

#include "RichWave.h"

// SPIDATA = CS0; SPILE = CS1; SPICLK = CS2
RichWave::RichWave(uint8_t spiDataPin, uint8_t spiLePin, uint8_t spiClkPin)
{
	dataPin = spiDataPin;
	lePin = spiLePin;
	clkPin = spiClkPin;
	
	pinMode(spiDataPin, OUTPUT);
	pinMode(spiLePin, OUTPUT);
	digitalWrite(spiLePin, HIGH);
	pinMode(spiClkPin, OUTPUT);
	digitalWrite(spiClkPin, LOW);
}

void RichWave::setFrequency(int freq) {
	// calculatute F_LO
	freq = freq - 479;
	// N Part
	int n = freq/64;
	// A Part
	int a = (freq%64)/2;
	setRegister(0x01, a | (n << 7));
}

void RichWave::setRegister(byte address, unsigned long data) {
	// transmission begin (set latch to low)
	digitalWrite(lePin, LOW);

	// address (4 bits)
	for(int c = 0; c < 4; c++) {
		sendBit(bitRead(address, c));
	}

	// write mode
	sendBit(true);

	// 20 data bits (LSB FISRST)
	for(int c = 0; c < 20; c++) {
		sendBit(bitRead(data, c));
	}

	// end transmission and shift to register (set latch to high)
	digitalWrite(lePin, HIGH);
}

void RichWave::sendBit(boolean isOne) {
	// pull dataline to level
	digitalWrite(dataPin, isOne ? HIGH : LOW);
	// generate clock pulse
	digitalWrite(clkPin, HIGH);
	digitalWrite(clkPin, LOW);
}