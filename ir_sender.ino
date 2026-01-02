// marantz wired remote control web server
// inspired by https://github.com/Arduino-IRremote/Arduino-IRremote
// Ported code by samm-git for esp8266.  Added mDNS capability, some code modifications.
// Module used here was a Wemos D1 Mini Pro. 
// see https://smallhacks.wordpress.com/2021/07/07/controlling-marantz-amplifier-using-arduino-via-remote-socket/

/* some definitions from the IRremote Arduino Library */
#define RC5_ADDRESS_BITS 5
#define RC5_COMMAND_BITS 6
#define RC5_EXT_BITS 6
#define RC5_COMMAND_FIELD_BIT 1
#define RC5_TOGGLE_BIT 1
#define RC5_BITS (RC5_COMMAND_FIELD_BIT + RC5_TOGGLE_BIT + RC5_ADDRESS_BITS + RC5_COMMAND_BITS)  // 13
#define RC5X_BITS (RC5_BITS + RC5_EXT_BITS) // 19
#define RC5_UNIT 889  // (32 cycles of 36 kHz)
#define RC5_DURATION (15L * RC5_UNIT)  // 13335
#define RC5_REPEAT_PERIOD (128L *RC5_UNIT)  // 113792
#define RC5_REPEAT_SPACE (RC5_REPEAT_PERIOD - RC5_DURATION)  // 100 ms

#// define TRACE // print binary commands to serial

uint8_t sLastSendToggleValue1 = 0;

/* 
 *  normal Philips RC-5 as in the https://en.wikipedia.org/wiki/RC-5
 *  code taken from IRremote with some changes
 */

int sendRC5(uint8_t aAddress, uint8_t aCommand, uint_fast8_t aNumberOfRepeats) {
  digitalWrite(IR_TX_PIN, LOW);

  uint16_t tIRData = ((aAddress & 0x1F) << RC5_COMMAND_BITS);

  if (aCommand < 0x40) {
    // set field bit to lower field / set inverted upper command bit
    tIRData |= 1 << (RC5_TOGGLE_BIT + RC5_ADDRESS_BITS + RC5_COMMAND_BITS);
  } else {
    // let field bit zero
    aCommand &= 0x3F;
  }

  tIRData |= aCommand;
  tIRData |= 1 << RC5_BITS;

  if (sLastSendToggleValue1 == 0) {
    sLastSendToggleValue1 = 1;
    // set toggled bit
    tIRData |= 1 << (RC5_ADDRESS_BITS + RC5_COMMAND_BITS);
  } else {
    sLastSendToggleValue1 = 0;
  }

  uint_fast8_t tNumberOfCommands = aNumberOfRepeats;

  while (tNumberOfCommands > 0) {
    for (int i = 13; 0 <= i; i--) {
      #ifdef TRACE
      Serial.print((tIRData &(1 << i)) ? '1' : '0');
      #endif
      (tIRData &(1 << i)) ? send_1() : send_0();
    }
    tNumberOfCommands--;
    if (tNumberOfCommands > 0) {
      // send repeated command in a fixed raster
      delay(RC5_REPEAT_SPACE / 1000);
    }
    #ifdef TRACE
    Serial.print("\n");
    #endif
  }

  digitalWrite(IR_TX_PIN, LOW);
  return 0;
}

/* 
 *  Marantz 20 bit RC5 extension, see 
 *  http://lirc.10951.n7.nabble.com/Marantz-RC5-22-bits-Extend-Data-Word-possible-with-lircd-conf-semantic-td9784.html 
 *  could be combined with sendRC5, but ATM split to simplify debugging
 */
 
int sendRC5_X(uint8_t aAddress, uint8_t aCommand, uint8_t aExt, uint_fast8_t aNumberOfRepeats) {

  uint32_t tIRData = (uint32_t)(aAddress & 0x1F) << (RC5_COMMAND_BITS + RC5_EXT_BITS);

  digitalWrite(IR_TX_PIN, LOW);

  if (aCommand < 0x40) {
    // set field bit to lower field / set inverted upper command bit
    tIRData |= (uint32_t) 1 << (RC5_TOGGLE_BIT + RC5_ADDRESS_BITS + RC5_COMMAND_BITS + RC5_EXT_BITS);
  } else {
    // let field bit zero
    aCommand &= 0x3F;
  }

  tIRData |= (uint32_t)(aExt & 0x3F);
  tIRData |= (uint32_t) aCommand << RC5_EXT_BITS;
  tIRData |= (uint32_t) 1 << RC5X_BITS;

  if (sLastSendToggleValue1 == 0) {
    sLastSendToggleValue1 = 1;
    // set toggled bit
    tIRData |= (uint32_t) 1 << (RC5_ADDRESS_BITS + RC5_COMMAND_BITS + RC5_EXT_BITS);
  } else {
    sLastSendToggleValue1 = 0;
  }

  uint_fast8_t tNumberOfCommands = aNumberOfRepeats + 1;

  while (tNumberOfCommands > 0) {
    for (int i = 19; 0 <= i; i--) {
      #ifdef TRACE
      Serial.print((tIRData &((uint32_t) 1 << i)) ? '1' : '0');
      #endif
      
      (tIRData &((uint32_t) 1 << i)) ? send_1() : send_0();
      if (i == 12) {
        #ifdef TRACE
        Serial.print("<p>");
        #endif
        // space marker for marantz rc5 extension
        digitalWrite(IR_TX_PIN, LOW);
        delayMicroseconds(RC5_UNIT *2 *2);
      }
    }
    #ifdef TRACE
    Serial.print("\n");
    #endif
    tNumberOfCommands--;
    if (tNumberOfCommands > 0) {
      // send repeated command in a fixed raster
      delay(RC5_REPEAT_SPACE / 1000);
    }
  }
  digitalWrite(IR_TX_PIN, LOW);
  return 0;
}

void send_0() {
  digitalWrite(IR_TX_PIN, HIGH);
  delayMicroseconds(RC5_UNIT);
  digitalWrite(IR_TX_PIN, LOW);
  delayMicroseconds(RC5_UNIT);
}

void send_1() {
  digitalWrite(IR_TX_PIN, LOW);
  delayMicroseconds(RC5_UNIT);
  digitalWrite(IR_TX_PIN, HIGH);
  delayMicroseconds(RC5_UNIT);
}
