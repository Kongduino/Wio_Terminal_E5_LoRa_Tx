/*
  Wio_Terminal_E5_LoRa_Tx. A demonstration of two-way LoRa communication
  between two (or more) Wio Terminal devices equipped with Wio-E5.
  Copyright (C) 2023 by Kongduino
  kongduino@protonmail.com https://github.com/Kongduino

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

  For commercial and/or closed-source usage and licensing, please contact the author.
*/

#include <KLoRaWan.h> // https://github.com/Kongduino/WioE5_LoRaWAN
#define LGFX_USE_V1
#define LGFX_AUTODETECT // LGFX_WIO_TERMINAL // LGFX_AUTODETECT
#define PING_DELAY 30000

#include <LovyanGFX.hpp>
#include <LGFX_AUTODETECT.hpp>
#include "fonts.h"
#include "Helper.h"
#include "UI.h"
#include "GPS_Helper.h"

uint32_t sendTimer;

void setup(void) {
  SerialUSB.begin(115200);
  Serial1.begin(9600);
  initGPS();
  for (uint8_t ix = 0; ix < 3; ix++) {
    delay(1000);
    SerialUSB.print(3 - ix);
    SerialUSB.print(", ");
  }
  delay(1000);
  SerialUSB.println("0!");
  // RNG
  lora.initRandom();

  /*
    Setting are broken. Will fix ASAP.
    2023/02/19
  */
  savePrefs();
  uint8_t prefs[16];
  memset(prefs, 0xFF, 16);
  for (ix = 0; ix < 16; ix++)
    prefs[ix] = lora.getEEPROM(ix + 240);
  hexDump(prefs, 12);
  float fq;
  if (memcmp(prefs, "@love", 5) == 0) {
    SerialUSB.println("Magic word found!");
    mySF = prefs[5];
    myBW = prefs[6];
    myTx = prefs[7];
    myFreqIndex = prefs[12];
    memcpy(&myFreq, (prefs + 8), 4);
  } else savePrefs();
  SerialUSB.printf("Freq: %.3f\n", myFreq);
  SerialUSB.printf("SF: %d\n", mySF);
  SerialUSB.printf("BW: %d\n", myBW);
  SerialUSB.printf("TX: %d\n", myTx);
  SerialUSB.printf("myFreqIndex: %d\n", myFreqIndex);
  // LoRa
  initLoRaSettings();

  // Navigation
  pinMode(WIO_KEY_A, INPUT_PULLUP);
  pinMode(WIO_KEY_B, INPUT_PULLUP);
  pinMode(WIO_KEY_C, INPUT_PULLUP);
  pinMode(WIO_5S_UP, INPUT_PULLUP);
  pinMode(WIO_5S_DOWN, INPUT_PULLUP);
  pinMode(WIO_5S_LEFT, INPUT_PULLUP);
  pinMode(WIO_5S_RIGHT, INPUT_PULLUP);
  pinMode(WIO_5S_PRESS, INPUT_PULLUP);
  // LCD
  lcd.init();
  lcd.setBrightness(luminosity);
  lcd.setRotation(1);
  initMainScreen();
  initScreenLoRa();
  initScreen1();
  initScreenSF();
  initScreenBW();
  initScreenFreq();
  initScreenFreqDecimal();
  initScreenTx();
  initScreenLumi();
  mainScreen.selectedIndex = 0;
  renderScreen(mainScreen);
  // BLE
  ble_setup();
  sendTimer = millis();
}

void loop(void) {
  if (digitalRead(WIO_5S_UP) == LOW) {
    while (digitalRead(WIO_5S_UP) == LOW) ; // debounce
    char tmp[32];
    sprintf(tmp, "selectedIndex: %d\n", currentScreen.selectedIndex);
    SerialUSB.print(tmp);
    currentScreen.buttons[currentScreen.selectedIndex].button.press(false);
    if (currentScreen.selectedIndex == 0) currentScreen.selectedIndex = currentScreen.buttonCount - 1;
    else currentScreen.selectedIndex -= 1;
    sprintf(tmp, "selectedIndex: %d\n", currentScreen.selectedIndex);
    currentScreen.buttons[currentScreen.selectedIndex].button.press(true);
    SerialUSB.print(tmp);
    renderScreen(currentScreen);
  } else if (digitalRead(WIO_5S_DOWN) == LOW) {
    while (digitalRead(WIO_5S_DOWN) == LOW) ; // debounce
    char tmp[32];
    sprintf(tmp, "selectedIndex: %d\n", currentScreen.selectedIndex);
    SerialUSB.print(tmp);
    currentScreen.buttons[currentScreen.selectedIndex].button.press(false);
    if (currentScreen.selectedIndex == currentScreen.buttonCount - 1) currentScreen.selectedIndex = 0;
    else currentScreen.selectedIndex += 1;
    sprintf(tmp, "selectedIndex: %d\n", currentScreen.selectedIndex);
    SerialUSB.print(tmp);
    currentScreen.buttons[currentScreen.selectedIndex].button.press(true);
    renderScreen(currentScreen);
  } else if (digitalRead(WIO_5S_PRESS) == LOW) {
    while (digitalRead(WIO_5S_PRESS) == LOW) ; // debounce
    char tmp[32];
    sprintf(tmp, "selectedIndex: %d\n", currentScreen.selectedIndex);
    SerialUSB.print(tmp);
    if (currentScreen.selectedIndex > -1) {
      myButton b0 = currentScreen.buttons[currentScreen.selectedIndex];
      b0.ptr();
    }
  } else if (digitalRead(WIO_KEY_C) == LOW) {
    while (digitalRead(WIO_KEY_C) == LOW) ; // debounce
    handleLuminosity();
  }
  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    std::string hello = "Oh hello there!\r\n";
    pTxCharacteristic->setValue(hello);
    pTxCharacteristic->notify();
    oldDeviceConnected = deviceConnected;
  }
  if (millis() - sendTimer > PING_DELAY) {
    drawLoRa(); // draws the regular LoRa logo in cyan
    SerialUSB.println("Send hex.");
    sprintf((char*)inBuffer, "PING #%d.", pingCount++);
    uint8_t ln = strlen((char*)inBuffer);
    hexDump(inBuffer, ln);
    lora.transferPacketP2PMode(inBuffer, ln);
    lcd.setColor(TFT_WHITE);
    lcd.fillRect(0, 240 - 40, 36, 40);
    sendTimer = millis();
  }
  if (millis() - lastGPS > GPS_DELAY) {
    gps.flush();
    uint32_t t0 = millis();
    while (millis() - t0 < GPS_DURATION) {
      if (gps.available()) {
        char c = gps.read();
        if (waitForDollar && c == '$') {
          waitForDollar = false;
          buffer[0] = '$';
          ix = 1;
        } else if (waitForDollar == false) {
          if (c == 13) {
            buffer[ix] = 0;
            c = gps.read();
            delay(50);
            string nextLine = string(buffer);
            userStrings.push_back(nextLine.substr(0, nextLine.size() - 3));
            waitForDollar = true;
          } else {
            buffer[ix++] = c;
          }
        }
      }
    }
    if (userStrings.size() > 0) {
      string nextLine = userStrings[0];
      userStrings.erase(userStrings.begin());
      if (nextLine.substr(0, 1) != "$") {
        // Serial.print("Not an NMEA string!\n>> ");
        // Serial.println(nextLine.c_str());
        return;
      } else {
        vector<string>result = parseNMEA(nextLine);
        if (result.size() == 0) return;
        string verb = result.at(0);
        if (verb.substr(3, 3) == "RMC") {
          parseGPRMC(result);
        } else if (verb.substr(3, 3) == "GSV") {
          parseGPGSV(result);
        } else if (verb.substr(3, 3) == "GGA") {
          parseGPGGA(result);
        } else if (verb.substr(3, 3) == "GLL") {
          parseGPGLL(result);
        } else if (verb.substr(3, 3) == "GSA") {
          parseGPGSA(result);
        } else if (verb.substr(3, 3) == "VTG") {
          parseGPVTG(result);
        } else if (verb.substr(3, 3) == "TXT") {
          parseGPTXT(result);
        } else {
          Serial.println(nextLine.c_str());
        }
      }
    }
    lastGPS = millis();
  }
}
