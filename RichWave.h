#ifndef _RICHWAVE_H_INCLUDED
#define _RICHWAVE_H_INCLUDED

#include <stdio.h>
#include <Arduino.h>

class RichWave
{
	public:
	  RichWave(uint8_t dataPin, uint8_t lePin, uint8_t clkPin, uint8_t rssiPin);
	  void setFrequency(int frequency);
	  void setRegister(byte address, unsigned long data);
          uint16_t readRSSI();
	private:
		uint8_t dataPin;
		uint8_t lePin;
		uint8_t clkPin;
                uint8_t rssiPin;
                uint16_t lastTuned;
		void sendBit(boolean isOne);
};

#endif
