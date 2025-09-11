/*
   Firmware of MSXLinkCart

   Based on https://github.com/bouletmarc/BMBurner
            https://github.com/StormTrooper/eeprom_programmer
   and modified to fit for this project.

   Build a simple flashable MSX game cartridge.

   Use 64KB Winbond W27C512 (Electrically erasable) EEPROM as Game ROM chip, this cartridge can store two 32KB Games.
   Cartridge can be connected to a morden computer via USB cable and be programmed with game ROMs.

   For more information see https://geek-logic.com
*/
#include <EEPROM.h>
#include <Arduino.h>
#include <U8x8lib.h>

U8X8_SSD1306_128X32_UNIVISION_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);   // Adafruit ESP8266/32u4/ARM Boards + FeatherWing OLED

const byte bin_file[] PROGMEM = {"I AM WELLSWANG from geek-logic.com, It is a test string."};


//#define DEBUG

#define GAME_SELECTOR_ADDR 0
#define GAME_INFO_ADDR 100
#define GAME_OFFSET 100

// define the IO lines for the data - bus
#define D0 2
#define D1 3
#define D2 4
#define D3 5
#define D4 6
#define D5 7
#define D6 8
#define D7 9

// for high voltage programming supply
#define VH     12     //Erase Pin (A9)  12v
#define VPP    11    //OE Pin = VPP    12v

// for mode detect
#define A15PIN  13

// shiftOut part
#define DS     A0
#define LATCH  A1
#define CLOCK  A2

// define the IO lines for the eeprom control
#define CE     A3
#define OE     10

// direct access to port
#define STROBE_PORT PORTC
#define STROBE_DS      0
#define STROBE_LATCH   1
#define STROBE_CLOCK   2
#define STROBE_CE      3
#define STROBE_OE      4
#define STROBE_WE      5

//a buffer for bytes to burn
#define BUFFERSIZE 256
byte buffer[BUFFERSIZE + 1]; // +1 for checksum

//flag set if we are on the first write pass
boolean firstWritePass = true;

//Last Address (used for sending high/low output at only 1 specific location on the 74HC595
unsigned int Last_Address = 0;

int count = 0;  //Memory size
int mem_count;
int arraySize;
long BlockSize;
char mode;

long Start_Address;         //0x0000, 0x2000, 0x4000, 0x6000, 0x8000, 0xa000, 0xc000, 0xe000
int old_state, cur_state;

//###############################################################
// Setup
//###############################################################
void setup() {

  u8x8.begin();
  u8x8.setPowerSave(0);

  u8x8.setFont(u8x8_font_pressstart2p_r );
  u8x8.setInverseFont(1);
  u8x8.drawString(0, 0, "- MSXLinkCart - ");
  u8x8.setInverseFont(0);

  pinMode(A7, INPUT);
  if (analogRead(A7) > 700) {  //PLAY MODE
    pinMode(D0, INPUT);
    pinMode(D1, INPUT);
    pinMode(D2, INPUT);
    pinMode(D3, INPUT);
    pinMode(D4, INPUT);
    pinMode(D5, INPUT);
    pinMode(D6, INPUT);
    pinMode(D7, INPUT);
    //pinMode(OE, INPUT);
    //pinMode(VH, INPUT);
    //pinMode(VPP, INPUT);
    pinMode(A15PIN, OUTPUT);
    pinMode(A6, INPUT);
    digitalWrite(A15PIN, EEPROM.read(GAME_SELECTOR_ADDR) ? HIGH : LOW);
    
    updateGameInfo(EEPROM.read(GAME_SELECTOR_ADDR));
    
    old_state = analogRead(A6);
  } else { //PROGRAM MODE

    Serial.begin(115200);
    mode = 'C';
    updateInfo("C", "Wating for", "connection...");

    //arraySize = sizeof(bin_file) / sizeof(bin_file[0]);

    //define the shiftOut Pins as output
    pinMode(DS, OUTPUT);
    pinMode(LATCH, OUTPUT);
    pinMode(CLOCK, OUTPUT);

    digitalWrite(A15PIN, LOW);
    pinMode(A15PIN, OUTPUT);

    //define the boost pins as output (take care that they are LOW)
    digitalWrite(VH, LOW);
    pinMode(VH, OUTPUT);
    digitalWrite(VPP, LOW);
    pinMode(VPP, OUTPUT);

    //define the EEPROM Pins as output (take care that they are HIGH)
    digitalWrite(OE, HIGH);
    pinMode(OE, OUTPUT);
    digitalWrite(CE, HIGH);
    pinMode(CE, OUTPUT);
  }
}

//###############################################################
// Main
//###############################################################

void loop() {
  if (analogRead(A7) > 700){ // PLAY MODE
    cur_state = analogRead(A6);
    
    if ( cur_state - old_state > 500 ) {
      EEPROM.write(GAME_SELECTOR_ADDR, (EEPROM.read(GAME_SELECTOR_ADDR) + 1) % 2);
      updateInfo((EEPROM.read(GAME_SELECTOR_ADDR) ? "2" : "1"), "Game switched", "Please RESTART");
    }
    old_state = cur_state;
    delay(200);

  } else { // PROGRAM MODE


    if (Serial.available() > 0) {
      readSerialCommand();
    }


    if (mode == 'W') {
      delay(500);
      updateInfo("W", "Erasing...", " ");
      erase();
      Serial.print("W");
      delay(300);

      writeGameName(0);
      Serial.print("W");
      delay(300);

      writeGameName(1);
      EEPROM.write(GAME_SELECTOR_ADDR, 0x0);
      Serial.print("W");
      delay(300);

      writeFromSerial();
      resetVariables();
    }
  }
}

//###############################################################
// Functions
//###############################################################

//###############################################################

void updateInfo(String cap, String line1, String line2){
  u8x8.setFont(u8x8_font_lucasarts_scumm_subtitle_o_2x2_r);
  u8x8.drawString(0, 2, cap.c_str());	
  u8x8.setFont(u8x8_font_pressstart2p_r );
  u8x8.drawString(2, 2, padRight(line1, 14, ' ').c_str());
  u8x8.drawString(2, 3, padRight(line2, 14, ' ').c_str());
}

void updateGameInfo(int id){
  String game_name = readStringFromEEPROM(GAME_INFO_ADDR + GAME_OFFSET * id);
  String part1 = "";
  String part2 = "";

  if (game_name.length()>14){
    part1 = game_name.substring(0, 14);
    part2 = game_name.substring(14, (game_name.length() > 28) ? 28 : game_name.length() );
  } else
    part1 = game_name;
  updateInfo(id > 0 ? "2" : "1", part1, part2);
}

void writeGameName(int id) {
  String game_name ="";
  while (!Serial.available()){
  }
  while (Serial.available()){
    game_name += char(Serial.read());
  }
  if (game_name.length() > 28)
    game_name = game_name.substring(0,28);
  game_name += '\0';
  writeStringToEEPROM(GAME_INFO_ADDR + GAME_OFFSET * id, game_name);
}


void writeFromSerial() {
  long unsigned addr = 0;
  updateInfo("W", "Programming...", " "); 
  while (addr <= 65535){
    if (Serial.available()){
      char rx_byte = Serial.read(); 
      writeStart();
      fastWrite(addr, byte(rx_byte));
      writeEnd();
      addr++;
      if ( addr % 655 == 1){
        String percent = String(addr / 655) + "%";
        updateInfo("W", "Programming...", percent); 
      }
      if (addr > 0 && addr % 32 == 0 ) {
        Serial.print("N");
      }
    }
  }
}

//###############################################################

void erase() {
  setCE(HIGH);
  setOE(HIGH);
  setVH(HIGH);
  setVPP(HIGH);
  delay(1);
  fastWrite(0, 0xff);

  //erase pulse
  setCE(LOW);
  delay(350);
  setCE(HIGH);
  delayMicroseconds(1);

  //Turning Off
  setVH(LOW);
  setVPP(LOW);
  delayMicroseconds(1);
}

//###############################################################
// COMMANDS SUBS functions
//###############################################################
void writeStringToEEPROM(int addrOffset, const String &strToWrite){
  byte len = strToWrite.length();
  updateInfo("W", strToWrite.substring(0,10), "   ");
  for (int i = 0; i < len; i++) {
    EEPROM.write(addrOffset + i, strToWrite[i]);
  }
}

String readStringFromEEPROM(int addrOffset){
  int i = 0;
  char c= ' ';
  String data="";

  do{
    c = EEPROM.read(addrOffset + i);
    data += ' ';
    data[i] = c;
    i++;
  } while( c != '\0' && i < 28);
  return data;
}

void readStart() {
  dataBusInput();
  //enable chip select
  setCE(LOW);
  //enable output
  setOE(LOW);
}

void readEnd() {
  //disable output
  setOE(HIGH);
  //disable chip select
  setCE(HIGH);
}

inline byte readByte(unsigned int address)
{
  setAddressBus(address);
  return readDataBus();
}

void writeStart() {
  firstWritePass = true;
  //disable output
  setOE(HIGH);

  setVPP(HIGH);

  delayMicroseconds(5);
  dataBusOutput();
}

void writeEnd() {
  setVPP(LOW);
  dataBusInput();

}

inline void fastWrite(unsigned int address, byte data)
{
  setAddressBus(address);
  writeDataBus(data);
  delayMicroseconds(1);

  //programming pulse
  setCE(LOW);
  delayMicroseconds(100); // for W27E512, works for 27SF512 also (but 27SF512 should be 20ms)
  setCE(HIGH);
  delayMicroseconds(1);
}

//###############################################################
// DATA BUS functions
//###############################################################

void dataBusInput() {
  pinMode(D0, INPUT);
  pinMode(D1, INPUT);
  pinMode(D2, INPUT);
  pinMode(D3, INPUT);
  pinMode(D4, INPUT);
  pinMode(D5, INPUT);
  pinMode(D6, INPUT);
  pinMode(D7, INPUT);
}

void dataBusOutput() {
  pinMode(D0, OUTPUT);
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D3, OUTPUT);
  pinMode(D4, OUTPUT);
  pinMode(D5, OUTPUT);
  pinMode(D6, OUTPUT);
  pinMode(D7, OUTPUT);
}

byte readDataBus()
{
  byte b = 0;
  if (digitalRead(D0) == HIGH) b |= 1;
  if (digitalRead(D1) == HIGH) b |= 2;
  if (digitalRead(D2) == HIGH) b |= 4;
  if (digitalRead(D3) == HIGH) b |= 8;
  if (digitalRead(D4) == HIGH) b |= 16;
  if (digitalRead(D5) == HIGH) b |= 32;
  if (digitalRead(D6) == HIGH) b |= 64;
  if (digitalRead(D7) == HIGH) b |= 128;
  return (b);
}

inline void writeDataBus(byte data)
{
  digitalWrite(D0, data & 1);
  digitalWrite(D1, data & 2);
  digitalWrite(D2, data & 4);
  digitalWrite(D3, data & 8);
  digitalWrite(D4, data & 16);
  digitalWrite(D5, data & 32);
  digitalWrite(D6, data & 64);
  digitalWrite(D7, data & 128);
}

//###############################################################
// FAST SWIFT functions
//###############################################################
inline void setAddressBus(unsigned int address)
{
  byte hi, low;
  hi = (address >> 8);
  low = address & 0xff;

  applyShiftAt(hi, low);
  digitalWrite(A15PIN, (address & 32768) ? HIGH : LOW); // A15 
  delayMicroseconds(1);
}

inline void setOutputAt(unsigned int Position, bool IsHigh)
{
  byte hi, low;
  hi = (Last_Address >> 8);
  low = Last_Address & 0xff;
  if (Position >= 8) {
    if (IsHigh) bitSet(hi, Position - 8);
    else  bitClear(hi, Position - 8);
  }
  else {
    if (IsHigh) bitSet(low, Position);
    else  bitClear(low, Position);
  }
  applyShiftAt(hi, low);
}

void applyShiftAt(byte hi, byte low)
{
  fastShiftOut(hi);
  fastShiftOut(low);
  //strobe latch line
  bitSet(STROBE_PORT, STROBE_LATCH);
  bitClear(STROBE_PORT, STROBE_LATCH);
  delayMicroseconds(1);
}

void fastShiftOut(byte data) {
  //clear
  bitClear(STROBE_PORT, STROBE_DS);

  //Loop for the 8x outputs
  for (int i = 7; i >= 0; i--) {
    //clear clock pin
    bitClear(STROBE_PORT, STROBE_CLOCK);

    //Enable/Disable pin Output
    if (bitRead(data, i) == 1) bitSet(STROBE_PORT, STROBE_DS);
    else  bitClear(STROBE_PORT, STROBE_DS);

    //register shifts bits on upstroke of clock pin
    bitSet(STROBE_PORT, STROBE_CLOCK);
    //clear after shift to prevent bleed through
    bitClear(STROBE_PORT, STROBE_DS);
  }

  //stop shifting
  bitClear(STROBE_PORT, STROBE_CLOCK);
}

//###############################################################
// PINS functions
//###############################################################

//**attention, this line is LOW - active**
inline void setOE (byte state)
{
  digitalWrite(OE, state);
}

//**attention, this line is LOW - active**
inline void setCE (byte state)
{
  digitalWrite(CE, state);
}

//Boost VPP 12V
void setVPP (byte state)
{
  digitalWrite(VPP, state); //only for W27C512
}

//Boost Erase 12V
void setVH (byte state)
{
  digitalWrite(VH, state);
}

//==========================================================================

String padRight(String str, int totalLength, char padChar) {
  while (str.length() < totalLength) {
    str += padChar;
  }
  return str;
}

void readSerialCommand() {

  if (mode == 'C'){
    char rx_byte = Serial.read();       // get the character
    if ((rx_byte == 'W')) {
      mode = 'W';
    }
  }
}

void resetVariables() {
  mode = 'C';
  updateInfo("C", "Program", "finished.");
}