/*
 * 4x Richwave RX5808 Video Receiver for use with a 
 * Video Splitscreen processor and optional HMDVR
 * 
 * 4 Buttons to trigger a channelsearch on each module.
 * Search is done by scanning channel 1-8 of each band one after the other
 * If valid signal is found, the tuner locks on to that channel.
 * 
 * Oled is used to display all 4 tunes Channels, Freq. and RSSI val.
 * 
 * requires a patched version of the Richwave lib by Timo Witte <timo.witte@gmail.com>
 * 
 * https://github.com/derFrickler/RichWave
 * 
 * 
 * 
 * http://der-Frickler.net
 * 
 * 
*/

//#define DEBUG

#include <avr/pgmspace.h>
#include <EEPROM.h>
#include <Wire.h>
#include <GOFi2cOLED.h>
#include <RichWave.h>

// Debug output on serial
#define DEBUG

// Choose if you wish to use 8 additional Channels
// 5362 MHz 5399 MHz 5436 MHz 5473 MHz 5510 MHz 5547 MHz 5584 MHz 5621 MHz
// Local laws may prohibit the use of these frequencies use at your own risk!
//#define USE_LBAND

// Recording Pin of the HMDVR/EachineDVR
#define P_REC A0

// RSSI default raw range
#define RSSI_REC_ON   160
#define RSSI_REC_OFF  140
#define RSSI_SEEK_TRESHOLD 160

#define rssiPin0 A1
#define rssiPin1 A3
#define rssiPin2 A6
#define rssiPin3 A7
//const uint8_t rssiPin[] = {rssiPin0, rssiPin1, rssiPin2, rssiPin3};

#define spiLatch0 7
#define spiLatch1 8
#define spiLatch2 9
#define spiLatch3 10
//const uint8_t slaveSelectPin[] = {spiLatch0, spiLatch1, spiLatch2, spiLatch3};
#define spiDataPin 11
#define spiClockPin 12

// DATA PIN; LATCH PIN; CLK PIN; RSSI PIN
RichWave rx0(spiDataPin,  spiLatch0,  spiClockPin, rssiPin0);
RichWave rx1(spiDataPin,  spiLatch1,  spiClockPin, rssiPin1);
RichWave rx2(spiDataPin,  spiLatch2,  spiClockPin, rssiPin2);
RichWave rx3(spiDataPin,  spiLatch3,  spiClockPin, rssiPin3);

// Pretuned Channels at startup.
uint8_t channel[] = {2, 5, 18, 0};

#define S_TUNED 0
#define S_SCAN  1
uint8_t state[]   = {S_TUNED, S_TUNED, S_TUNED, S_SCAN};
//uint16_t tuneTime[] = {0, 0, 0, 0};

#define BUTTON0 2
#define BUTTON1 4
#define BUTTON2 5
#define BUTTON3 6
const uint8_t buttons[]  =  {BUTTON0, BUTTON1, BUTTON2, BUTTON3};


// Channels with their Mhz Values
const uint16_t channelFreqTable[] PROGMEM = {
  // Channel 1 - 8
  5865, 5845, 5825, 5805, 5785, 5765, 5745, 5725, // Band A
  5733, 5752, 5771, 5790, 5809, 5828, 5847, 5866, // Band B
  5705, 5685, 5665, 5645, 5885, 5905, 5925, 5945, // Band E
  5740, 5760, 5780, 5800, 5820, 5840, 5860, 5880, // Band F / Airwave
#ifdef USE_LBAND
  5658, 5695, 5732, 5769, 5806, 5843, 5880, 5917, // Band C / Immersion Raceband
  5362, 5399, 5436, 5473, 5510, 5547, 5584, 5621  // Band D / 5.3
#else
  5658, 5695, 5732, 5769, 5806, 5843, 5880, 5917  // Band C / Immersion Raceband
#endif
};

// do coding as simpl#define P_REC 3e hex value to save memory.
const uint8_t channelNames[] PROGMEM = {
  0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, // Band A
  0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, // Band B
  0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, // Band E
  0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, // Band F / Airwave
#ifdef USE_LBAND
  0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, // Band C / Immersion Raceband
  0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8  // BAND D / 5.3
#else
  0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8  // Band C / Immersion Raceband
#endif
};

// All Channels of the above List ordered by Mhz
const uint8_t channelList[] PROGMEM = {
#ifdef USE_LBAND
  40, 41, 42, 43, 44, 45, 46, 47, 19, 18, 32, 17, 33, 16, 7, 34, 8, 24, 6, 9, 25, 5, 35, 10, 26, 4, 11, 27, 3, 36, 12, 28, 2, 13, 29, 37, 1, 14, 30, 0, 15, 31, 38, 20, 21, 39, 22, 23
#else
  19, 18, 32, 17, 33, 16, 7, 34, 8, 24, 6, 9, 25, 5, 35, 10, 26, 4, 11, 27, 3, 36, 12, 28, 2, 13, 29, 37, 1, 14, 30, 0, 15, 31, 38, 20, 21, 39, 22, 23
#endif
};


GOFi2cOLED oled;// default slave address is 0x3D

boolean recording = false;


// ----------------------------------------------------------------------------
// SETUP 
// ----------------------------------------------------------------------------
void setup() {
  oled.init(0x3C);  //initialze  OLED display
  oled.display(); // show splashscreen
  delay(2000);
  oled.clearDisplay();

  oled.drawFastHLine(0, 0, 128, WHITE);
  oled.drawFastHLine(0, 14, 128, WHITE);
  oled.setCursor(3, 4);
  oled.setTextSize(1);
  oled.print("4X Video Splitter RX");

  pinMode(LED_BUILTIN, OUTPUT);

  for (uint8_t i = 0; i <= sizeof(buttons); i++) {
    pinMode (buttons[i], INPUT_PULLUP);
  }


#ifdef DEBUG
  Serial.begin(115200);
  Serial.println(F("START:"));
#endif

for (int mod = 0; mod < 4; mod++) {
    // if modul is in tuned mode, tune to channel
    if (state[mod] == S_TUNED) {
      setChannelModule(mod, channel[mod]);
    }
}

  // Setup Done - LED ON
  digitalWrite(13, HIGH);

}

// ----------------------------------------------------------------------------
// MAIN LOOP 
// ----------------------------------------------------------------------------
void loop()
{

  // check Scan Buttons, and set module to scan if set.
  for (int mod = 0; mod < 4; mod++) {
    if (! digitalRead(buttons[mod])) {
      state[mod] = S_SCAN;
#ifdef DEBUG
      Serial.print(F("Setting Module" ));
      Serial.print(mod);
      Serial.println(F(" to Scan Mode"));
#endif
    }
  }
  
  // for each module, scan channels if in scan mode.
  int rssi = 0;
  int maxRSSI = 0;
  for (int mod = 0; mod < 4; mod++) {
    // if modul is in scan mode, skip to next ch.
    if (state[mod] == S_SCAN) {
      rssi = nextChannel(mod);
      // if something found on that ch, change mode to tuned.
      if (!hasChannel(mod) && rssi > RSSI_SEEK_TRESHOLD) {
        state[mod] = S_TUNED;
      }
    } else {
      rssi = readRSSI(mod);
    }
    displayChannel(mod, rssi);
    maxRSSI = max(maxRSSI, rssi);
  }

  // if highest RSSI of all Modules is over Treshhold -> Rec, else Rec Off
  if (maxRSSI < RSSI_REC_OFF) {
    rec(false);
  } else if (maxRSSI > RSSI_REC_ON) {
    rec(true);
  } 

 // delay(20);

}


/*###########################################################################*/
/*   SUB ROUTINES  
/*###########################################################################*/

// ----------------------------------------------------------------------------
// nextChannel   tune to next channel on given module
// ----------------------------------------------------------------------------
uint16_t nextChannel(uint8_t module) {
  uint8_t currChannel = channel[module];

  currChannel++;
  if (currChannel >= sizeof(channelList) ) {
    currChannel = 0;
  }

  setChannelModule(module, currChannel);
  channel[module] = currChannel;
//  wait_rssi_ready(module);
  int rssi = readRSSI(module);

#ifdef DEBUG
  Serial.print(F("Mod: ")); Serial.print(module);
  Serial.print(F(" tuned to CH: ")); Serial.print(currChannel);
  Serial.print(F(": ")); Serial.print(channelName(currChannel));
  Serial.print(F(": ")); Serial.print(pgm_read_word_near(channelFreqTable + currChannel));
  Serial.print(F(" RSSI: ")); Serial.println(rssi);
#endif

  return rssi;
}

// ----------------------------------------------------------------------------
// hasCHannel   check if current channel is already tuned on other module.
// ----------------------------------------------------------------------------
boolean hasChannel(uint8_t module) {
  int currChannel = channel[module];

#ifdef DEBUG
  Serial.print(F("Check CHANNEL: ")); Serial.print(currChannel);
  Serial.print(F(" IS IN ")); Serial.print(channel[0]);
  Serial.print(F(" ")); Serial.print(channel[1]);
  Serial.print(F(" ")); Serial.print(channel[2]);
  Serial.print(F(" ")); Serial.print(channel[3]);
#endif

  int mod = 0;
  for (mod = 0; mod <= 4; mod++) {
    //if (mod != module && (channel[mod] - 1 <= currChannel) && currChannel <= channel[mod] + 1) {
    
    if (mod != module && channel[mod] == currChannel) {
#ifdef DEBUG
      Serial.println(F(" -> TRUE"));
#endif
      return true;
    }
  }

#ifdef DEBUG
  Serial.println(F(" -> FALSE"));
#endif
  return false;
}

// ----------------------------------------------------------------------------
// displayChannel   display the given channel, freq and rssi on the oled
// ----------------------------------------------------------------------------
void displayChannel(int module, int rssi) {

  if (state[module] == S_SCAN) {
    oled.setTextColor(BLACK, WHITE); // 'inverted' text
  } else {
    oled.setTextColor(WHITE);
  }

  int ch = channel[module];
  switch (module) {
    case 0:
      oled.fillRect(5, 16, 63, 26, BLACK);
      oled.setCursor(5, 16);
      oled.setTextSize(3);
      oled.print(channelName(ch));
      oled.setTextColor(WHITE);
      oled.setTextSize(1);
      oled.print(pgm_read_word_near(channelFreqTable + ch));
      oled.setCursor(40, 27);
      break;
    case 1:
      oled.fillRect(68, 16, 63, 26, BLACK);
      oled.setCursor(68, 16);
      oled.setTextSize(3);
      oled.print(channelName(ch));
      oled.setTextColor(WHITE);
      oled.setTextSize(1);
      oled.print(pgm_read_word_near(channelFreqTable + ch));
      oled.setCursor(103, 27);
      break;
    case 2:
      oled.fillRect(5, 42, 63, 26, BLACK);
      oled.setCursor(5, 42);
      oled.setTextSize(3);
      oled.print(channelName(ch));
      oled.setTextColor(WHITE);
      oled.setTextSize(1);
      oled.print(pgm_read_word_near(channelFreqTable + ch));
      oled.setCursor(40, 53);
      break;
    case 3:
      oled.fillRect(68, 42, 63, 26, BLACK);
      oled.setCursor(68, 42);
      oled.setTextSize(3);
      oled.print(channelName(ch));
      oled.setTextColor(WHITE);
      oled.setTextSize(1);
      oled.print(pgm_read_word_near(channelFreqTable + ch));
      oled.setCursor(103, 53);
      break;

    default:
      break;
  }
  oled.print(rssi);
  oled.display();
}

// ----------------------------------------------------------------------------
// readRSSI  read rssi of given module.
// ----------------------------------------------------------------------------
uint16_t readRSSI(uint8_t module)
{
  switch (module) {
    case 0:
      return rx0.readRSSI();
    case 1:
      return rx1.readRSSI();
    case 2:
      return rx2.readRSSI();
    case 3:
      return rx3.readRSSI();
  }
  return 0;
}


// ----------------------------------------------------------------------------
// channelName  return channel name from list.
// ----------------------------------------------------------------------------
String channelName(uint8_t ch) {
  String name = String(pgm_read_byte_near(channelNames + ch), HEX);
  name.toUpperCase();
  if (name.charAt(0) == 'C') {
    name.setCharAt(0, 'R');
  }
  return name;
}


// ----------------------------------------------------------------------------
// channel_from_index  
// ----------------------------------------------------------------------------
uint8_t channel_from_index(uint8_t channelIndex)
{
  uint8_t loop = 0;
  uint8_t channel = 0;
  for (loop = 0; loop <= sizeof(channelList); loop++)
  {
    if (pgm_read_byte_near(channelList + loop) == channelIndex)
    {
      channel = loop;
      break;
    }
  }
  return (channel);
}


// ----------------------------------------------------------------------------
// setChannelModule   tune given module to given channel
// ----------------------------------------------------------------------------
void setChannelModule(uint8_t module, uint8_t channel) {
  
  uint16_t channelFreq = pgm_read_word_near(channelFreqTable + channel);

  switch (module) {
    case 0:
      rx0.setFrequency(channelFreq);
      break;
    case 1:
      rx1.setFrequency(channelFreq);
      break;
    case 2:
      rx2.setFrequency(channelFreq);
      break;
    case 3:
      rx3.setFrequency(channelFreq);
      break;
    default:
      break;
  }
  
//  tuneTime[module] = millis();
}

void rec(boolean rec) {
  if (rec == recording)
    return
    
  digitalWrite(P_REC, HIGH);
  delay(1000);
  digitalWrite(P_REC, LOW);
  recording = !recording;
}

