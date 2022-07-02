/*******************************************************************************
 * This file is part of PsxNewLib.                                             *
 *                                                                             *
 * Copyright (C) 2019-2020 by SukkoPera <software@sukkology.net>               *
 *                                                                             *
 * PsxNewLib is free software: you can redistribute it and/or                  *
 * modify it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation, either version 3 of the License, or           *
 * (at your option) any later version.                                         *
 *                                                                             *
 * PsxNewLib is distributed in the hope that it will be useful,                *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
 * GNU General Public License for more details.                                *
 *                                                                             *
 * You should have received a copy of the GNU General Public License           *
 * along with PsxNewLib. If not, see http://www.gnu.org/licenses.              *
 *******************************************************************************
 *
 * This example is very similar to DumpButtonsHwSpi, it only has some slight
 * modifications so that it can be used to test my PsxControllerShield
 * (https://github.com/SukkoPera/PsxControllerShield). It will dump to serial
 * whatever is done on a PSX controller and will light the available leds
 * according to various events.
 *
 * It is an excellent way to test that all buttons/sticks are read correctly,
 * and thus that both the controller and the shield are working fine.
 *
 * It's missing support for analog buttons, that will come in the future.
 *
 * Note that on Leonardo and other boards with a CDC USB serial port, all three
 * leds will blink at startup in the serial port is not connected. Just open
 * your serial monitor to go on :).
 */
#include <Arduino.h>
// PsxControllerShield connects controller to HW SPI port through ICSP connector
#include "./PsxControllerBitBang.h"

#include <avr/pgmspace.h>
typedef const __FlashStringHelper * FlashStr;
typedef const byte* PGM_BYTES_P;
#define PSTR_TO_F(s) reinterpret_cast<const __FlashStringHelper *> (s)

/** \brief Pin used for Controller Attention (ATTN)
 *
 * This pin makes the controller pay attention to what we're saying. The shield
 * has pin 10 wired for this purpose.
 */
const byte PIN_PS2_ATT = 10;
const byte PIN_PS2_CMD = 11;
const byte PIN_PS2_DAT = 12;
const byte PIN_PS2_CLK = 14;

/** \brief Dead zone for analog sticks
 *  
 * If the analog stick moves less than this value from the center position, it
 * is considered still.
 * 
 * \sa ANALOG_IDLE_VALUE
 */
const byte ANALOG_DEAD_ZONE = 50U;

PsxControllerBitBang<PIN_PS2_ATT, PIN_PS2_CMD, PIN_PS2_DAT, PIN_PS2_CLK> psx;

boolean haveController = false;

const unsigned long POLLING_INTERVAL = 1000U / 50U;

const char buttonSelectName[] PROGMEM = "Select";
const char buttonL3Name[] PROGMEM = "L3";
const char buttonR3Name[] PROGMEM = "R3";
const char buttonStartName[] PROGMEM = "Start";
const char buttonUpName[] PROGMEM = "Up";
const char buttonRightName[] PROGMEM = "Right";
const char buttonDownName[] PROGMEM = "Down";
const char buttonLeftName[] PROGMEM = "Left";
const char buttonL2Name[] PROGMEM = "L2";
const char buttonR2Name[] PROGMEM = "R2";
const char buttonL1Name[] PROGMEM = "L1";
const char buttonR1Name[] PROGMEM = "R1";
const char buttonTriangleName[] PROGMEM = "Triangle";
const char buttonCircleName[] PROGMEM = "Circle";
const char buttonCrossName[] PROGMEM = "Cross";
const char buttonSquareName[] PROGMEM = "Square";

const char* const psxButtonNames[PSX_BUTTONS_NO] PROGMEM = {
  buttonSelectName,
  buttonL3Name,
  buttonR3Name,
  buttonStartName,
  buttonUpName,
  buttonRightName,
  buttonDownName,
  buttonLeftName,
  buttonL2Name,
  buttonR2Name,
  buttonL1Name,
  buttonR1Name,
  buttonTriangleName,
  buttonCircleName,
  buttonCrossName,
  buttonSquareName
};

byte psxButtonToIndex (PsxButtons psxButtons) {
  byte i;

  for (i = 0; i < PSX_BUTTONS_NO; ++i) {
    if (psxButtons & 0x01) {
      break;
    }

    psxButtons >>= 1U;
  }

  return i;
}

FlashStr getButtonName (PsxButtons psxButton) {
  FlashStr ret = F("");
  
  byte b = psxButtonToIndex (psxButton);
  if (b < PSX_BUTTONS_NO) {
    PGM_BYTES_P bName = reinterpret_cast<PGM_BYTES_P> (pgm_read_ptr (&(psxButtonNames[b])));
    ret = PSTR_TO_F (bName);
  }

  return ret;
}

// Controller Type
const char ctrlTypeUnknown[] PROGMEM = "Unknown";
const char ctrlTypeDualShock[] PROGMEM = "Dual Shock";
const char ctrlTypeDsWireless[] PROGMEM = "Dual Shock Wireless";
const char ctrlTypeGuitHero[] PROGMEM = "Guitar Hero";
const char ctrlTypeOutOfBounds[] PROGMEM = "(Out of bounds)";

const char* const controllerTypeStrings[PSCTRL_MAX + 1] PROGMEM = {
  ctrlTypeUnknown,
  ctrlTypeDualShock,
  ctrlTypeDsWireless,
  ctrlTypeGuitHero,
  ctrlTypeOutOfBounds
};


// Controller Protocol
const char ctrlProtoUnknown[] PROGMEM = "Unknown";
const char ctrlProtoDigital[] PROGMEM = "Digital";
const char ctrlProtoDualShock[] PROGMEM = "Dual Shock";
const char ctrlProtoDualShock2[] PROGMEM = "Dual Shock 2";
const char ctrlProtoFlightstick[] PROGMEM = "Flightstick";
const char ctrlProtoNegcon[] PROGMEM = "neGcon";
const char ctrlProtoJogcon[] PROGMEM = "Jogcon";
const char ctrlProtoOutOfBounds[] PROGMEM = "(Out of bounds)";

const char* const controllerProtoStrings[PSPROTO_MAX + 1] PROGMEM = {
  ctrlProtoUnknown,
  ctrlProtoDigital,
  ctrlProtoDualShock,
  ctrlProtoDualShock2,
  ctrlProtoFlightstick,
  ctrlProtoNegcon,
  ctrlProtoJogcon,
  ctrlTypeOutOfBounds
};

void process() {
  uint8_t up, right, down, left;

  Joystick.button(1, psx.buttonPressed(PSB_SQUARE));
  Joystick.button(2, psx.buttonPressed(PSB_CROSS));
  Joystick.button(3, psx.buttonPressed(PSB_CIRCLE));
  Joystick.button(4, psx.buttonPressed(PSB_TRIANGLE));
  Joystick.button(5, psx.buttonPressed(PSB_L1));
  Joystick.button(6, psx.buttonPressed(PSB_R1));
  Joystick.button(7, psx.buttonPressed(PSB_L2));
  Joystick.button(8, psx.buttonPressed(PSB_R2));
  Joystick.button(9, psx.buttonPressed(PSB_SELECT));
  Joystick.button(10, psx.buttonPressed(PSB_START));
  Joystick.button(11, psx.buttonPressed(PSB_L3));
  Joystick.button(12, psx.buttonPressed(PSB_R3));

  up = psx.buttonPressed(PSB_PAD_UP) & 1u;
  right = psx.buttonPressed(PSB_PAD_RIGHT) & 1u;
  down = psx.buttonPressed(PSB_PAD_DOWN) & 1u;
  left = psx.buttonPressed(PSB_PAD_LEFT) & 1u;

  uint8_t state = 0;
  state |= up;
  state |= (right << 1);
  state |= (down << 2);
  state |= (left << 3);

  int16_t degree = -1;

  switch (state) {
  case 1:
    degree = 0;
    break;
  case 2:
    degree = 90;
    break;
  case 3:
    degree = 45;
    break;
  case 4:
    degree = 180;
    break;
  case 6:
    degree = 135;
    break;
  case 8:
    degree = 270;
    break;
  case 9:
    degree = 315;
    break;
  case 12:
    degree = 225;
    break;
  }

  Joystick.hat(degree);

  uint8_t x, y;
  psx.getLeftAnalog(x, y);
  Joystick.Y(y * 4);
  Joystick.X(x * 4);

  uint8_t zr, slider;
  psx.getRightAnalog(zr, slider);
  Joystick.slider(slider * 4);
  Joystick.Zrotate(zr * 4);

  Joystick.send_now();
}

void process_dengo() {
  Joystick.button(1, psx.buttonPressed(PSB_SQUARE));
  Joystick.button(2, psx.buttonPressed(PSB_CROSS));
  Joystick.button(3, psx.buttonPressed(PSB_CIRCLE));
  Joystick.button(4, psx.buttonPressed(PSB_TRIANGLE));
  Joystick.button(5, psx.buttonPressed(PSB_L1));
  Joystick.button(6, psx.buttonPressed(PSB_R1));

  Joystick.button(14, psx.buttonPressed(PSB_L2));
  Joystick.button(15, psx.buttonPressed(PSB_R2));

  Joystick.button(9, psx.buttonPressed(PSB_SELECT));
  Joystick.button(10, psx.buttonPressed(PSB_START));
  Joystick.button(11, psx.buttonPressed(PSB_L3));
  Joystick.button(12, psx.buttonPressed(PSB_R3));

  // Joystick.button(18, psx.buttonPressed(PSB_PAD_UP));
  // Joystick.button(19, psx.buttonPressed(PSB_PAD_DOWN));
  Joystick.button(20, psx.buttonPressed(PSB_PAD_LEFT));
  Joystick.button(21, psx.buttonPressed(PSB_PAD_RIGHT));

  Joystick.send_now();
}

void process_negcon() {
  uint8_t up, right, down, left;

  Joystick.button(1, psx.buttonPressed(PSB_SQUARE));
  Joystick.button(2, psx.buttonPressed(PSB_CROSS));
  Joystick.button(3, psx.buttonPressed(PSB_CIRCLE));
  Joystick.button(4, psx.buttonPressed(PSB_TRIANGLE));
  Joystick.button(5, psx.buttonPressed(PSB_L1));
  Joystick.button(6, psx.buttonPressed(PSB_R1));
  Joystick.button(7, psx.buttonPressed(PSB_L2));
  Joystick.button(8, psx.buttonPressed(PSB_R2));
  Joystick.button(9, psx.buttonPressed(PSB_SELECT));
  Joystick.button(10, psx.buttonPressed(PSB_START));
  Joystick.button(11, psx.buttonPressed(PSB_L3));
  Joystick.button(12, psx.buttonPressed(PSB_R3));

  up = psx.buttonPressed(PSB_PAD_UP) & 1u;
  right = psx.buttonPressed(PSB_PAD_RIGHT) & 1u;
  down = psx.buttonPressed(PSB_PAD_DOWN) & 1u;
  left = psx.buttonPressed(PSB_PAD_LEFT) & 1u;

  uint8_t state = 0;
  state |= up;
  state |= (right << 1);
  state |= (down << 2);
  state |= (left << 3);

  int16_t degree = -1;

  switch (state) {
  case 1:
    degree = 0;
    break;
  case 2:
    degree = 90;
    break;
  case 3:
    degree = 45;
    break;
  case 4:
    degree = 180;
    break;
  case 6:
    degree = 135;
    break;
  case 8:
    degree = 270;
    break;
  case 9:
    degree = 315;
    break;
  case 12:
    degree = 225;
    break;
  }

  Joystick.hat(degree);

  uint8_t x, y;
  psx.getLeftAnalog(x, y);
  Joystick.X(x * 4);
  Joystick.Y(y * 4);

  uint8_t accel, brake;
  accel = psx.getAnalogButton(PSAB_CROSS);
  brake = psx.getAnalogButton(PSAB_SQUARE);

  Joystick.Z(accel * 4);
  Joystick.Zrotate(brake * 4);

  uint8_t slider = psx.getAnalogButton(PSAB_L1);
  Joystick.slider(slider * 4);

  Joystick.send_now();
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("Ready!"));

  Joystick.useManualSend(1);
}

void loop() {
  PsxControllerProtocol proto;
  static unsigned long last = 0;
  if (millis() - last >= POLLING_INTERVAL) {
    last = millis();
  } else {
    return;
  }

  if (!haveController) {
    if (psx.begin()) {
      Serial.println (F("Controller found!"));
      delay (300);
      if (!psx.enterConfigMode ()) {
        Serial.println (F("Cannot enter config mode"));
      } else {
        PsxControllerType ctype = psx.getControllerType ();
        PGM_BYTES_P cname = reinterpret_cast<PGM_BYTES_P> (pgm_read_ptr (&(controllerTypeStrings[ctype < PSCTRL_MAX ? static_cast<byte> (ctype) : PSCTRL_MAX])));
        Serial.print (F("Controller Type is: "));
        Serial.println (PSTR_TO_F (cname));

        if (!psx.enableAnalogSticks ()) {
          Serial.println (F("Cannot enable analog sticks"));
        }
        
        if (!psx.enableAnalogButtons ()) {
          Serial.println (F("Cannot enable analog buttons"));
        }
        
        if (!psx.exitConfigMode ()) {
          Serial.println (F("Cannot exit config mode"));
        }
      }

      psx.read ();    // Make sure the protocol is up to date
      proto = psx.getProtocol ();
      PGM_BYTES_P pname = reinterpret_cast<PGM_BYTES_P> (pgm_read_ptr (&(controllerProtoStrings[proto < PSPROTO_MAX ? static_cast<byte> (proto) : PSPROTO_MAX])));
      Serial.print (F("Controller Protocol is: "));
      Serial.println (PSTR_TO_F (pname));

      haveController = true;
    }
  } else {
    if (!psx.read ()) {
      Serial.println (F("Controller lost :("));
      haveController = false;
    } else {
      if (psx.getProtocol() == PSPROTO_NEGCON) {
        process_negcon();
      }
      else
        process_dengo();
    }
  }
}
