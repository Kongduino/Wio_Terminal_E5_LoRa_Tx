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

#include <rpcBLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
char deviceName[32] = "WioE5_0123456789abcde";
LGFX lcd;
unsigned char inBuffer[512] = {0};
uint32_t pingCount = 0;

#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
#define BLUETOOTH_COLOR 0x12BE

uint16_t prevx, prevy;
uint8_t mySFs[6] = {7, 8, 9, 10, 11, 12};
uint8_t mySF = 5;
uint8_t myBWs[3] = {125, 250, 500};
uint8_t myBW = 0;
float myFreq = 868.0;
uint8_t myTx = 20, myFreqIndex = 5;

struct myDetails {
  char magic[5]; // @love 5
  uint8_t sfbwtx[3]; // 3
  float freq; // 4
  // Total: 12
};

static void lineTo(uint16_t x, uint16_t y, uint16_t color) {
  lcd.drawLine( prevx, prevy, x, y, color);
  prevx = x;
  prevy = y;
}
static void drawBluetoothLogo(uint16_t x, uint16_t y, uint8_t height = 10, uint16_t color = TFT_WHITE, uint16_t bgcolor = BLUETOOTH_COLOR) {
  if (height < 10) height = 10; // low cap
  if (height % 2 != 0) height++; // lame centering
  lcd.fillRoundRect(x + height / 4, y + height * 0.05, height / 2, height - height * 0.1, height / 4, bgcolor);
  x += height * .1;
  y += height * .1;
  height *= .8;
  float y1 = height * 0.05;
  float y2 = height * 0.25;
  prevx = x + y2;
  prevy = y + y2;
  lineTo(x + height - y2, y + height - y2, color);
  lineTo(x + height / 2, y + height - y1, color);
  lineTo(x + height / 2, y + y1, color);
  lineTo(x + height - y2, y + y2, color);
  lineTo(x + y2, y + height - y2, color);
}

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      SerialUSB.println("MyServerCallbacks onConnect ");
      deviceConnected = true;
      uint16_t py = 11;
      uint16_t px = 298;
      // lcd.setColor(TFT_BLUE);
      // lcd.fillRoundRect(px, 2, 20, 26, 4);
      // lcd.setTextColor(TFT_WHITE);
      // lcd.setFont(FSS9);
      // lcd.drawString("B", px + 5, py - 4);
      // lcd.setColor(TFT_WHITE);
      // lcd.drawLine(px + 2, py, px + 5, py + 3);
      // lcd.drawLine(px + 5, py + 3, px + 2, py + 6);
      // lcd.drawLine(px + 2, py + 1, px + 5, py + 4);
      // lcd.drawLine(px + 5, py + 4, px + 2, py + 7);
      drawBluetoothLogo(px, 2, 20);
    };
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      lcd.setColor(TFT_WHITE);
      lcd.fillRoundRect(298, 2, 20, 26, 4);
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();
      if (rxValue.length() > 0) {
        SerialUSB.println("*********");
        SerialUSB.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++) SerialUSB.print(rxValue[i]);
        SerialUSB.println();
        SerialUSB.println("*********");
      }
    }
};

void hexDump(uint8_t* buf, uint16_t len) {
  // Something similar to the Unix/Linux hexdump -C command
  // Pretty-prints the contents of a buffer, 16 bytes a row
  char alphabet[17] = "0123456789abcdef";
  uint16_t i, index;
  SerialUSB.print(F("   +------------------------------------------------+ +----------------+\n"));
  SerialUSB.print(F("   |.0 .1 .2 .3 .4 .5 .6 .7 .8 .9 .a .b .c .d .e .f | |      ASCII     |\n"));
  for (i = 0; i < len; i += 16) {
    if (i % 128 == 0) SerialUSB.print(F("   +------------------------------------------------+ +----------------+\n"));
    char s[] = "|                                                | |                |\n";
    // pre-formated line. We will replace the spaces with text when appropriate.
    uint8_t ix = 1, iy = 52, j;
    for (j = 0; j < 16; j++) {
      if (i + j < len) {
        uint8_t c = buf[i + j];
        // fastest way to convert a byte to its 2-digit hex equivalent
        s[ix++] = alphabet[(c >> 4) & 0x0F];
        s[ix++] = alphabet[c & 0x0F];
        ix++;
        if (c > 31 && c < 128) s[iy++] = c;
        else s[iy++] = '.'; // display ASCII code 0x20-0x7F or a dot.
      }
    }
    index = i >> 4;
    // display line number then the text
    if (i < 256) Serial.write(' ');
    SerialUSB.print(index, HEX); Serial.write('.');
    SerialUSB.print(s);
  }
  SerialUSB.print(F("   +------------------------------------------------+ +----------------+\n"));
}

void ble_setup() {
  SerialUSB.println("============");
  SerialUSB.println(" BLE Setup");
  SerialUSB.println("============");
  BLEDevice::init(deviceName);
  uint8_t ix = 1;
  SerialUSB.printf(" %d.\n", ix++);
  // Create the BLE Server
  pServer = BLEDevice::createServer();
  SerialUSB.printf(" %d.\n", ix++);
  pServer->setCallbacks(new MyServerCallbacks());
  SerialUSB.printf(" %d.\n", ix++);
  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  SerialUSB.printf(" %d.\n", ix++);
  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ);
  SerialUSB.printf(" %d.\n", ix++);
  pTxCharacteristic->setAccessPermissions(GATT_PERM_READ);
  SerialUSB.printf(" %d.\n", ix++);
  pTxCharacteristic->addDescriptor(new BLE2902());
  SerialUSB.printf(" %d.\n", ix++);
  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
  SerialUSB.printf(" %d.\n", ix++);
  pRxCharacteristic->setAccessPermissions(GATT_PERM_READ | GATT_PERM_WRITE);
  SerialUSB.printf(" %d.\n", ix++);
  pRxCharacteristic->setCallbacks(new MyCallbacks());
  SerialUSB.printf(" %d.\n", ix++);
  // Start the service
  pService->start();
  SerialUSB.printf("!%d.\n", ix++);
  // Start advertising
  pServer->getAdvertising()->start();
  SerialUSB.printf("%d.\n", ix++);
  SerialUSB.println("Waiting a client connection to notify...");
}

void initLoRaSettings() {
  SerialUSB.println("=============");
  SerialUSB.println(" LoRa Setup");
  SerialUSB.println("=============");
  lora.initP2PMode(myFreq, (_spreading_factor_t)mySFs[mySF], (_band_width_t)myBWs[myBW], 8, 8, myTx);
  delay(100);
}

void hex2array(char *src, size_t sLen, char *dst) {
  size_t i, n = 0;
  for (i = 0; i < sLen; i += 2) {
    uint8_t x, c;
    c = src[i];
    if (c > 0x39) c -= 55;
    else c -= 0x30;
    x = c << 4;
    c = src[i + 1];
    if (c > 0x39) c -= 55;
    else c -= 0x30;
    dst[n++] = (x + c);
  }
  dst[n] = 0;
}

void notifyBLE(char* myString) {
  if (deviceConnected) {
    std::string txt = myString;
    pTxCharacteristic->setValue(txt);
    pTxCharacteristic->notify();
  }
}

void setEEPROM(uint8_t addr, uint8_t v) {
  Serial1.flush();
  delay(10);
  while (Serial1.available()) {
    char c = Serial1.read();
    delay(5);
  }
  uint8_t n;
  char tmp[32];
  sprintf(tmp, "AT+EEPROM=%02x,%02x\r\n", addr, v);
  Serial1.print(tmp);
  delay(100);
  memset(tmp, 0, 32);
  char c = 0;
  n = 0;
  while (c != 10 && n < 32) {
    c = Serial1.read();
    if (c > 31) tmp[n++] = c;
    delay(10);
  }
  SerialUSB.println(tmp);
  return;
}

uint8_t getEEPROM(uint8_t addr) {
  /*
    AT+EEPROM=00,40
    AT+EEPROM=01,6C
    AT+EEPROM=02,6F
    AT+EEPROM=03,76
    AT+EEPROM=04,65
  */
  Serial1.flush();
  delay(10);
  while (Serial1.available()) {
    char c = Serial1.read();
    delay(5);
  }
  uint8_t n;
  char tmp[32];
  sprintf(tmp, "AT+EEPROM=%02x\r\n", addr);
  Serial1.print(tmp);
  delay(100);
  memset(tmp, 0, 32);
  char c = 0;
  n = 0;
  bool done = false;
  while (c != 10 && n < 32) {
    c = Serial1.read();
    if (c > 31) tmp[n++] = c;
    done = true;
    delay(10);
  }
  if (done == false) {
    tmp[n] = 0;
    SerialUSB.printf("Failed to get full sentence! []\n", tmp);
    return 0xFF;
  }
  // SerialUSB.println(tmp);
  char *ptr = strstr(tmp, "+EEPROM: ");
  // +EEPROM: 01, 00
  if (ptr) {
    // set value
    n = 9;
    tmp[0] = ptr[n++];
    tmp[1] = ptr[n];
    n += 3;
    tmp[2] = ptr[n++];
    tmp[3] = ptr[n];
    hex2array(tmp, 4, tmp + 4);
    // hexDump((uint8_t*)tmp, 6);
    uint8_t k, v;
    k = tmp[4]; v = tmp[5];
    // SerialUSB.printf(" --> buffer[%02x] = buffer[%02x]\n", k, v);
    return v;
  } else return 0xFF;
}

void savePrefs(float freq = myFreq, uint8_t sf = mySF, uint8_t bw = myBW, uint8_t tx = myTx) {
  SerialUSB.println("Saving Prefs...");
  uint8_t prefs[16] = {
    '@', 'l', 'o', 'v', 'e',
    mySF, myBW, myTx,
    0, 0, 0, 0,
    myFreqIndex, 0, 0, 0
  };
  memcpy(prefs + 8, (uint8_t*)&myFreq, 4);
  hexDump(prefs, 16);
  for (uint8_t ix = 0; ix < 16; ix++) {
    lora.setEEPROM(240 + ix, prefs[ix]);
    delay(100);
  }
}
