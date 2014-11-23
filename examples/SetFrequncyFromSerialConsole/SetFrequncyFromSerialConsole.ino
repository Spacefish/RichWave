#include <RichWave.h>

// if you use a BOSCAM Module, you need to enable SPI by removing a Resistor!!
// CH1-CH3 pins are used as SPI Bus pins then! CH1 = DATA; CH2 = LATCH; CH3 = CLOCK

// DATA PIN; LATCH PIN; CLK PIN
RichWave myModule(2,3,4);

void setup() {
  Serial.begin(9600);
}


void setFrequency(int freq) {
  myModule.setFrequency(freq);
  
  // calculate for serial output (not required)
  int n = freq/64;
  int a = (freq%64)/2;
  Serial.print("Setting Frequeny: ");
  Serial.print(2*(n*32+a));
  Serial.print("\n");
}

String inBuffer;

// the loop function runs over and over again forever
void loop() {
  if(Serial.available()) {
    int c = Serial.read();
    if(c == '\n') {
      int freq = inBuffer.toInt();
      setFrequency(freq);
      inBuffer = "";
    }
    else  {
      inBuffer += (char)c;
    }
  }
}
