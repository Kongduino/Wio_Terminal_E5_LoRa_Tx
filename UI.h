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

#undef max
#undef min
#include <string>
#include <vector>
using namespace std;
template class basic_string<char>; // https://github.com/esp8266/Arduino/issues/1136
// Required or the code won't compile!
namespace std _GLIBCXX_VISIBILITY(default) {
_GLIBCXX_BEGIN_NAMESPACE_VERSION
//void __throw_bad_alloc() {}
}

struct myLabel {
  char label[33]; // Text of the label
  uint16_t px;
  uint16_t py;
  int color;
  const GFXfont *font;
};

struct myButton {
  void (*ptr)(); // Function pointer
  LGFX_Button button;
  const GFXfont *font;
};

struct mySlider {
  int minValue = -1;
  int maxValue = -1;
  int currentValue = -1;
  int step = 5;
};

struct myScreen {
  myLabel labels[6]; // Up to 6 labels per page
  uint8_t labelCount;
  myButton buttons[8]; // Up to 8 labels per page
  uint8_t buttonCount;
  int bgColor;
  int selectedIndex = -1;
  mySlider slider;
};

void handleLoRaSettings();
void handleReturnToMain(uint8_t);
void handleMain1();
void handleMain2();
void handleSF();
void handleBW();
void handleTx();
void handleFreq();
void handleLoRaReturn();
void handleRollover(myScreen, vector<string>);
void renderScreen(myScreen screen);
void setMenuLabels(myScreen thisScreen, vector<string>choices);

vector<string> menu1Choices;
vector<string> menuSFChoices;
vector<string> menuBWChoices;
vector<string> menuFreqChoices;
vector<string> menuFreqDecimalChoices;
vector<string> menuTxChoices;
float deg2rad = 0.0174533;

#define TXT_CENTERED 0xFFFF
#define TXT_RIGHT 0xFFFE
#define TXT_BOTTOM 0xFFFD
#define TXT_TOP 0xFFFC

myScreen mainScreen;
myScreen currentScreen;
myScreen screenLoRa, screen1, screen2, screenSF, screenBW, screenFreq, screenFreqDecimal, screenTx, screenLumi;

uint8_t luminosity = 128;
void drawLuminosity() {
  uint16_t ix, jx = 360 * luminosity / 255, cx = 10, cy = 10, r = 4;
  lcd.setColor(TFT_BLACK);
  for (ix = 0; ix < jx; ix++) {
    int d = ix - 90;
    if (d < 0) d = 360 + d;
    lcd.drawLine(cx, cy, cx + cos(deg2rad * d)*r, cy + sin(deg2rad * d)*r);
  }
  lcd.drawCircle(cx, cy, r);
  uint16_t px0, py0, px1, py1;
  for (ix = 0; ix < 360; ix += 45) {
    int d = ix - 45;
    if (d < 0) d = 360 + d;
    px0 = cx + cos(deg2rad * d) * r;
    py0 = cy + sin(deg2rad * d) * r;
    px1 = cx + cos(deg2rad * d) * (r + 3);
    py1 = cy + sin(deg2rad * d) * (r + 3);
    lcd.drawLine(px0, py0, px1, py1);
  }
}

void drawLoRa(uint8_t px = 18, uint8_t py = 240 - 27, uint8_t w = 16, int color = TFT_CYAN) {
  uint8_t x = w - 12, i;
  delay(100);
  for (i = 0; i < 3; i++) {
    lcd.drawArc(px, py, x, x, 45, 135, color);
    lcd.drawArc(px, py, x, x, 225, 315, color);
    x += 1;
    lcd.drawArc(px, py, x, x, 45, 135, color);
    lcd.drawArc(px, py, x, x, 225, 315, color);
    delay(100);
    x += 3;
  }
  lcd.setColor(TFT_BLACK);
  lcd.fillCircle(px, py, 2);
}

void setMenuLabels(myScreen thisScreen, vector<string>choices) {
  thisScreen.buttons[0].button.press(false);
  thisScreen.buttons[1].button.press(true);
  thisScreen.buttons[2].button.press(false);
  // SerialUSB.println("In setMenuLabels");
  // SerialUSB.printf("thisScreen.selectedIndex = %d, ie %s\n", thisScreen.selectedIndex, choices[thisScreen.selectedIndex].c_str());
  lcd.setFont(thisScreen.buttons[1].font);
  thisScreen.buttons[1].button.setLabel(choices[thisScreen.selectedIndex].c_str());
  uint8_t ix;
  if (thisScreen.selectedIndex == 0) ix = choices.size() - 1;
  else ix = thisScreen.selectedIndex - 1;
  // SerialUSB.printf("ix- = %d, ie %s\n", ix, choices[ix].c_str());
  lcd.setFont(thisScreen.buttons[0].font);
  thisScreen.buttons[0].button.setLabel(choices[ix].c_str());
  if (thisScreen.selectedIndex == choices.size() - 1) ix = 0;
  else ix = thisScreen.selectedIndex + 1;
  // SerialUSB.printf("ix+ = %d, ie %s\n", ix, choices[ix].c_str());
  lcd.setFont(thisScreen.buttons[2].font);
  thisScreen.buttons[2].button.setLabel(choices[ix].c_str());
}

void menuSFButtonDown() {
  // SerialUSB.println("In menuSFButtonDown");
  screenSF.selectedIndex += 1;
  if (screenSF.selectedIndex == menuSFChoices.size()) screenSF.selectedIndex = 0;
  setMenuLabels(screenSF, menuSFChoices);
}

void menuSFButtonUp() {
  screenSF.selectedIndex -= 1;
  if (screenSF.selectedIndex == 0xFF) screenSF.selectedIndex = menuSFChoices.size() - 1;
  setMenuLabels(screenSF, menuSFChoices);
}

void menuSFButtonSelect() {
  char tmp[64];
  mySF = screenSF.selectedIndex;
  sprintf(tmp, "You selected %s, ie SF%d\n", menuSFChoices[screenSF.selectedIndex].c_str(), mySFs[mySF]);
  SerialUSB.print(tmp);
  notifyBLE(tmp);
  SerialUSB.println("Setting up SF...");
  initLoRaSettings();
  savePrefs();
  SerialUSB.println("................done!");
  currentScreen = screenLoRa;
  renderScreen(currentScreen);
}

void menuBWButtonDown() {
  // SerialUSB.println("In menuBWButtonDown");
  screenBW.selectedIndex += 1;
  if (screenBW.selectedIndex == menuBWChoices.size()) screenBW.selectedIndex = 0;
  setMenuLabels(screenBW, menuBWChoices);
}

void menuBWButtonUp() {
  screenBW.selectedIndex -= 1;
  if (screenBW.selectedIndex == 0xFF) screenBW.selectedIndex = menuBWChoices.size() - 1;
  setMenuLabels(screenBW, menuBWChoices);
}

void menuBWButtonSelect() {
  char tmp[64];
  myBW = screenBW.selectedIndex;
  sprintf(tmp, "You selected %s, ie BW%d\n", menuBWChoices[screenBW.selectedIndex].c_str(), myBWs[myBW]);
  SerialUSB.print(tmp);
  notifyBLE(tmp);
  SerialUSB.println("Setting up BW...");
  initLoRaSettings();
  savePrefs();

  SerialUSB.println("................done!");
  currentScreen = screenLoRa;
  renderScreen(currentScreen);
}

void restoreLoRa() {
  uint8_t i;
  for (i = 0; i < screenLoRa.buttonCount; i++) {
    if (i == screenLoRa.selectedIndex)screenLoRa.buttons[i].button.press(true);
    else screenLoRa.buttons[i].button.press(false);
  }
  renderScreen(screenLoRa);
}

void initMainScreen() {
  // Create the labels
  myLabel headerLabel = {
    "Main Screen", TXT_CENTERED, TXT_TOP, TFT_BLACK, FSS18
  };
  mainScreen.labels[0] = headerLabel;
  myLabel footerLabel = {
    "Select a menu...", TXT_RIGHT, TXT_BOTTOM, TFT_BLACK, FSS9
  };
  mainScreen.labels[1] = footerLabel;
  mainScreen.labelCount = 2;
  mainScreen.bgColor = TFT_WHITE;
  uint8_t bHeight = 32, bWidth = 150;
  uint16_t px, py;
  px = bWidth / 2 + 12;
  py = 60;

  lcd.setFont(FSS12);
  LGFX_Button btn0;
  btn0.initButton(&lcd, px, py, bWidth, bHeight, TFT_BLACK, TFT_WHITE, TFT_RED, "LoRa");
  btn0.press(false);
  myButton b0 = {handleLoRaSettings, btn0, FSS12};
  mainScreen.buttons[0] = b0;
  py += bHeight + 6;
  if (py + bHeight > lcd.height()) {
    py = 60;
    px += (bWidth + 12);
  }

  LGFX_Button btn1;
  btn1.initButton(&lcd, px, py, bWidth, bHeight, TFT_BLACK, TFT_WHITE, TFT_RED, "Listen");
  btn1.press(false);
  myButton b1 = {handleMain1, btn1, FSS12};
  mainScreen.buttons[1] = b1;
  py += bHeight + 6;
  if (py + bHeight > lcd.height()) {
    py = 60;
    px += (bWidth + 12);
  }

  LGFX_Button btn2;
  btn2.initButton(&lcd, px, py, bWidth, bHeight, TFT_BLACK, TFT_LIGHTGREY, TFT_BROWN, "Nothing!");
  btn2.press(false);
  myButton b2 = {handleMain2, btn2, FSS12};
  mainScreen.buttons[2] = b2;
  mainScreen.buttonCount = 3;
  py += bHeight + 6;
  if (py + bHeight > lcd.height()) {
    py = 60;
    px += (bWidth + 12);
  }
  mainScreen.selectedIndex = -1;
}

void initScreenLoRa() {
  // Create the labels
  myLabel headerLabel = {
    "Settings", TXT_CENTERED, TXT_TOP, TFT_BLACK, FSS18
  };
  screenLoRa.labels[0] = headerLabel;
  myLabel footerLabel = {
    "Select a setting", TXT_CENTERED, TXT_BOTTOM, TFT_BLACK, FSS9
  };
  screenLoRa.labels[1] = footerLabel;
  screenLoRa.labelCount = 2;
  screenLoRa.bgColor = TFT_WHITE;

  uint8_t bHeight = 30, bWidth = 150;
  uint16_t px, py;
  px = (320 - bWidth) / 2;
  py = 40;

  screenLoRa.selectedIndex = 0;
  lcd.setFont(FMB12);
  LGFX_Button btn0;
  btn0.initButton(&lcd, px, py, bWidth, bHeight, TFT_BLACK, TFT_WHITE, TFT_BLUE, "SF");
  btn0.press(true);
  myButton b0 = {handleSF, btn0, FMB12};
  screenLoRa.buttons[0] = b0;
  py += bHeight + 4;

  LGFX_Button btn1;
  btn1.initButton(&lcd, px, py, bWidth, bHeight, TFT_BLACK, TFT_WHITE, TFT_BLUE, "BW");
  btn1.press(true);
  myButton b1 = {handleBW, btn1, FMB12};
  screenLoRa.buttons[1] = b1;
  py += bHeight + 4;

  LGFX_Button btn2;
  btn2.initButton(&lcd, px, py, bWidth, bHeight, TFT_BLACK, TFT_WHITE, TFT_BLUE, "Freq");
  btn2.press(true);
  myButton b2 = {handleFreq, btn2, FMB12};
  screenLoRa.buttons[2] = b2;
  py += bHeight + 4;

  LGFX_Button btn3;
  btn3.initButton(&lcd, px, py, bWidth, bHeight, TFT_BLACK, TFT_WHITE, TFT_BLUE, "Tx");
  btn3.press(true);
  myButton b3 = {handleTx, btn3, FMB12};
  screenLoRa.buttons[3] = b3;
  py += bHeight + 4;

  LGFX_Button btn4;
  btn4.initButton(&lcd, px, py, bWidth, bHeight, TFT_BLACK, TFT_WHITE, TFT_BLUE, "BACK");
  btn4.press(false);
  myButton b4 = {handleLoRaReturn, btn4, FMB12};
  screenLoRa.buttons[4] = b4;

  screenLoRa.buttonCount = 5;
}

void initScreen1() {
  // Create the labels
  myLabel headerLabel = {
    "Listening...", TXT_CENTERED, TXT_TOP, TFT_BLACK, FSS18
  };
  screen1.labels[0] = headerLabel;
  myLabel footerLabel = {
    "Press a key to return", TXT_CENTERED, TXT_BOTTOM, TFT_BLACK, FSS9
  };
  screen1.labels[1] = footerLabel;

  screen1.labelCount = 2;
  screen1.buttonCount = 0;
  screen1.bgColor = TFT_WHITE;
}

void initScreenSF() {
  //  SerialUSB.println("initScreenSF");
  // Create the labels
  myLabel headerLabel = {
    "SF", TXT_CENTERED, TXT_TOP, TFT_BLACK, FSS18
  };
  screenSF.labels[0] = headerLabel;
  myLabel footerLabel = {
    "Select an SF value", TXT_CENTERED, TXT_BOTTOM, TFT_BLACK, FSS9
  };
  screenSF.labels[1] = footerLabel;
  screenSF.labelCount = 2;
  screenSF.bgColor = TFT_WHITE;
  menuSFChoices.push_back("7");
  menuSFChoices.push_back("8");
  menuSFChoices.push_back("9");
  menuSFChoices.push_back("10");
  menuSFChoices.push_back("11");
  menuSFChoices.push_back("12");
  screenSF.selectedIndex = mySF;
  uint8_t bHeight = 32, bSmallWidth = 100, bLargeWidth = 150, overallHeight = 40;
  uint16_t px, py;
  px = (320 - bSmallWidth) / 2;
  py = 120 - (bHeight * 2);
  screenSF.selectedIndex = 5;

  lcd.setFont(FMB9);
  LGFX_Button btn0;
  if (screenSF.selectedIndex > 0) btn0.initButton(&lcd, px, py, bSmallWidth, bHeight, TFT_DARKGREY, TFT_WHITE, TFT_DARKGREY, menuSFChoices[screenSF.selectedIndex - 1].c_str());
  else btn0.initButton(&lcd, px, py, bSmallWidth, bHeight, TFT_DARKGREY, TFT_WHITE, TFT_DARKGREY, menuSFChoices[5].c_str());
  btn0.press(false);
  myButton b0 = {menuSFButtonUp, btn0, FMB9};
  screenSF.buttons[0] = b0;

  py = 145;
  LGFX_Button btn2;
  if (screenSF.selectedIndex < 5) btn2.initButton(&lcd, px, py, bSmallWidth, bHeight, TFT_DARKGREY, TFT_WHITE, TFT_DARKGREY, menuSFChoices[screenSF.selectedIndex + 1].c_str());
  else btn2.initButton(&lcd, px, py, bSmallWidth, bHeight, TFT_DARKGREY, TFT_WHITE, TFT_DARKGREY, menuSFChoices[0].c_str());
  btn2.press(false);
  myButton b2 = {menuSFButtonDown, btn2, FMB9};
  screenSF.buttons[2] = b2;

  px = (320 - bLargeWidth) / 2;
  py = 100;
  lcd.setFont(FMB12);
  LGFX_Button btn1;
  btn1.initButton(&lcd, px, py, bLargeWidth, bHeight, TFT_BLACK, TFT_WHITE, TFT_BLUE, menuSFChoices[screenSF.selectedIndex].c_str());
  btn1.press(true);
  myButton b1 = {menuSFButtonSelect, btn1, FMB12};
  screenSF.buttons[1] = b1;

  screenSF.buttonCount = 3;
  screenSF.bgColor = TFT_WHITE;
}

void initScreenBW() {
  //  SerialUSB.println("initScreenBW");
  // Create the labels
  myLabel headerLabel = {
    "BW", TXT_CENTERED, TXT_TOP, TFT_BLACK, FSS18
  };
  screenBW.labels[0] = headerLabel;
  myLabel footerLabel = {
    "Select a BW value", TXT_CENTERED, TXT_BOTTOM, TFT_BLACK, FSS9
  };
  screenBW.labels[1] = footerLabel;
  screenBW.labelCount = 2;
  screenBW.bgColor = TFT_WHITE;
  menuBWChoices.push_back("125");
  menuBWChoices.push_back("250");
  menuBWChoices.push_back("500");
  screenBW.selectedIndex = myBW;
  uint8_t bHeight = 32, bSmallWidth = 100, bLargeWidth = 150, overallHeight = 40;
  uint16_t px, py;
  px = (320 - bSmallWidth) / 2;
  py = 120 - (bHeight * 2);
  screenBW.selectedIndex = myBW;

  lcd.setFont(FMB9);
  LGFX_Button btn0;
  if (screenBW.selectedIndex > 0) btn0.initButton(&lcd, px, py, bSmallWidth, bHeight, TFT_DARKGREY, TFT_WHITE, TFT_DARKGREY, menuBWChoices[screenBW.selectedIndex - 1].c_str());
  else btn0.initButton(&lcd, px, py, bSmallWidth, bHeight, TFT_DARKGREY, TFT_WHITE, TFT_DARKGREY, menuBWChoices[5].c_str());
  btn0.press(false);
  myButton b0 = {menuBWButtonUp, btn0, FMB9};
  screenBW.buttons[0] = b0;

  py = 145;
  LGFX_Button btn2;
  if (screenBW.selectedIndex < 5) btn2.initButton(&lcd, px, py, bSmallWidth, bHeight, TFT_DARKGREY, TFT_WHITE, TFT_DARKGREY, menuBWChoices[screenBW.selectedIndex + 1].c_str());
  else btn2.initButton(&lcd, px, py, bSmallWidth, bHeight, TFT_DARKGREY, TFT_WHITE, TFT_DARKGREY, menuBWChoices[0].c_str());
  btn2.press(false);
  myButton b2 = {menuBWButtonDown, btn2, FMB9};
  screenBW.buttons[2] = b2;

  px = (320 - bLargeWidth) / 2;
  py = 100;
  lcd.setFont(FMB12);
  LGFX_Button btn1;
  btn1.initButton(&lcd, px, py, bLargeWidth, bHeight, TFT_BLACK, TFT_WHITE, TFT_BLUE, menuBWChoices[screenBW.selectedIndex].c_str());
  btn1.press(true);
  myButton b1 = {menuBWButtonSelect, btn1, FMB12};
  screenBW.buttons[1] = b1;

  screenBW.buttonCount = 3;
  screenBW.bgColor = TFT_WHITE;
}

void renderScreen(myScreen screen) {
  lcd.fillScreen(screen.bgColor);
  uint8_t i, j;
  uint16_t px, py;
  j = screen.labelCount;
  // SerialUSB.printf("There are %d label%s\n", j, j > 1 ? "s" : "");
  if (j > 0) {
    for (i = 0; i < j; i++) {
      myLabel lb = screen.labels[i];
      lcd.setTextColor(lb.color);
      lcd.setFont(lb.font);
      px = lb.px;
      py = lb.py;
      if (px == TXT_CENTERED) px = (lcd.width() - lcd.textWidth(lb.label, lb.font)) >> 1;
      else if (px == TXT_RIGHT) px = lcd.width() - lcd.textWidth(lb.label, lb.font);
      if (py == TXT_BOTTOM) py = lcd.height() - lcd.fontHeight(lb.font) - 2;
      else if (py == TXT_TOP) py = 2;
      lcd.drawString(lb.label, px, py);
    }
  }
  j = screen.buttonCount;
  // SerialUSB.printf("There are %d button%s; selectedIndex = %d\n", j, j > 1 ? "s" : "", screen.selectedIndex);
  if (j > 0) {
    for (i = 0; i < j; i++) {
      // SerialUSB.printf("Drawing button #%d ", i);
      lcd.setFont(screen.buttons[i].font);
      screen.buttons[i].button.drawButton(i == screen.selectedIndex, NULL);
      // SerialUSB.println(" done!");
    }
  }
  // SerialUSB.println("Screen rendered!");
  currentScreen = screen;
  // SerialUSB.printf("selectedIndex = %d\n", currentScreen.selectedIndex);
  drawLuminosity();
}

void menuFreqButtonDown() {
  // SerialUSB.println("In menuFreqButtonDown");
  screenFreq.selectedIndex += 1;
  myFreqIndex = screenFreq.selectedIndex;
  if (screenFreq.selectedIndex == menuFreqChoices.size()) {
    screenFreq.selectedIndex = 0;
    myFreqIndex = screenFreq.selectedIndex;
  }
  setMenuLabels(screenFreq, menuFreqChoices);
}

void menuFreqButtonUp() {
  screenFreq.selectedIndex -= 1;
  myFreqIndex = screenFreq.selectedIndex;
  if (screenFreq.selectedIndex == 0xFF) {
    screenFreq.selectedIndex = menuFreqChoices.size() - 1;
    myFreqIndex = screenFreq.selectedIndex;
  }
  setMenuLabels(screenFreq, menuFreqChoices);
}

void menuFreqButtonSelect() {
  char tmp[64];
  uint8_t ix = screenFreq.selectedIndex;
  myFreq = stof(menuFreqChoices[ix]);
  sprintf(tmp, "You selected option #%d, ie %.3f MHz\n", ix, myFreq);
  SerialUSB.print(tmp);
  sprintf(tmp, "%d.", (uint16_t)myFreq);
  strcpy(screenFreqDecimal.labels[2].label, tmp);
  handleRollover(screenFreqDecimal, menuFreqDecimalChoices);
  SerialUSB.println("Setting up Freq...");
  initLoRaSettings();
  savePrefs();
  SerialUSB.println("................done!");
  currentScreen = screenLoRa;
  renderScreen(currentScreen);
}

void initScreenFreq() {
  // SerialUSB.println("initScreenFreq");
  // Create the labels
  myLabel headerLabel = {
    "Freq", TXT_CENTERED, TXT_TOP, TFT_BLACK, FSS18
  };
  screenFreq.labels[0] = headerLabel;
  myLabel footerLabel = {
    "Select a frequency", TXT_CENTERED, TXT_BOTTOM, TFT_BLACK, FSS9
  };
  screenFreq.labels[1] = footerLabel;
  screenFreq.labelCount = 2;
  screenFreq.bgColor = TFT_WHITE;
  uint16_t i;
  uint8_t ix;
  menuFreqChoices.push_back("863");
  menuFreqChoices.push_back("864");
  menuFreqChoices.push_back("865");
  menuFreqChoices.push_back("866");
  menuFreqChoices.push_back("867");
  menuFreqChoices.push_back("868");
  menuFreqChoices.push_back("869");
  menuFreqChoices.push_back("870");
  menuFreqChoices.push_back("902");
  menuFreqChoices.push_back("903");
  menuFreqChoices.push_back("904");
  menuFreqChoices.push_back("905");
  menuFreqChoices.push_back("906");
  menuFreqChoices.push_back("907");
  menuFreqChoices.push_back("908");
  menuFreqChoices.push_back("909");
  menuFreqChoices.push_back("910");
  menuFreqChoices.push_back("911");
  menuFreqChoices.push_back("912");
  menuFreqChoices.push_back("913");
  menuFreqChoices.push_back("914");
  menuFreqChoices.push_back("915");
  menuFreqChoices.push_back("916");
  menuFreqChoices.push_back("917");
  menuFreqChoices.push_back("918");
  menuFreqChoices.push_back("919");
  menuFreqChoices.push_back("920");
  menuFreqChoices.push_back("921");
  menuFreqChoices.push_back("922");
  menuFreqChoices.push_back("923");
  screenFreq.selectedIndex = myFreqIndex;
  // SerialUSB.printf("screenFreq.selectedIndex = %d\n", screenFreq.selectedIndex);
  uint8_t bHeight = 32, bSmallWidth = 100, bLargeWidth = 150, overallHeight = 40;
  uint16_t px, py;
  px = (320 - bSmallWidth) / 2;
  py = 120 - (bHeight * 2);

  lcd.setFont(FMB9);
  LGFX_Button btn0;
  if (screenFreq.selectedIndex > 0) ix = screenFreq.selectedIndex - 1;
  else ix = menuFreqChoices.size() - 1;
  btn0.initButton(&lcd, px, py, bSmallWidth, bHeight, TFT_DARKGREY, TFT_WHITE, TFT_DARKGREY, menuFreqChoices[ix].c_str());
  btn0.press(false);
  myButton b0 = {menuFreqButtonUp, btn0, FMB9};
  screenFreq.buttons[0] = b0;

  py = 145;
  LGFX_Button btn2;
  if (screenFreq.selectedIndex < (menuFreqChoices.size() - 1)) ix = screenFreq.selectedIndex + 1;
  else ix = 0;
  btn2.initButton(&lcd, px, py, bSmallWidth, bHeight, TFT_DARKGREY, TFT_WHITE, TFT_DARKGREY, menuFreqChoices[ix].c_str());
  btn2.press(false);
  myButton b2 = {menuFreqButtonDown, btn2, FMB9};
  screenFreq.buttons[2] = b2;

  px = (320 - bLargeWidth) / 2;
  py = 100;
  lcd.setFont(FMB12);
  LGFX_Button btn1;
  btn1.initButton(&lcd, px, py, bLargeWidth, bHeight, TFT_BLACK, TFT_WHITE, TFT_BLUE, menuFreqChoices[screenFreq.selectedIndex].c_str());
  btn1.press(true);
  myButton b1 = {menuFreqButtonSelect, btn1, FMB12};
  screenFreq.buttons[1] = b1;

  screenFreq.buttonCount = 3;
  screenFreq.bgColor = TFT_WHITE;
}

void menuFreqDecimalButtonDown() {
  // SerialUSB.println("In menuFreqDecimalButtonDown");
  screenFreqDecimal.selectedIndex += 1;
  if (screenFreqDecimal.selectedIndex == menuFreqDecimalChoices.size()) screenFreqDecimal.selectedIndex = 0;
  setMenuLabels(screenFreqDecimal, menuFreqDecimalChoices);
}

void menuFreqDecimalButtonUp() {
  screenFreqDecimal.selectedIndex -= 1;
  if (screenFreqDecimal.selectedIndex == 0xFF) screenFreqDecimal.selectedIndex = menuFreqDecimalChoices.size() - 1;
  setMenuLabels(screenFreqDecimal, menuFreqDecimalChoices);
}

void menuFreqDecimalButtonSelect() {
  char tmp[64];
  uint8_t ix = screenFreqDecimal.selectedIndex;
  myFreq += (stof(menuFreqDecimalChoices[ix]) / 1e3);
  sprintf(tmp, "You selected option #%d, ie %.3f MHz\n", ix, myFreq);
  SerialUSB.print(tmp);
  notifyBLE(tmp);
}

void initScreenFreqDecimal() {
  // SerialUSB.println("initScreenFreqDecimal");
  // Create the labels
  myLabel headerLabel = {
    "Freq: Decimal", TXT_CENTERED, TXT_TOP, TFT_BLACK, FSS18
  };
  screenFreqDecimal.labels[0] = headerLabel;
  myLabel footerLabel = {
    "Select a Decimal", TXT_CENTERED, TXT_BOTTOM, TFT_BLACK, FSS9
  };
  screenFreqDecimal.labels[1] = footerLabel;
  myLabel freqLabel = {
    "868", 15, 104, TFT_BLUE, FSSB18
  };
  screenFreqDecimal.labels[2] = freqLabel;
  screenFreqDecimal.labelCount = 3;
  screenFreqDecimal.bgColor = TFT_WHITE;
  menuFreqDecimalChoices.push_back("000");
  menuFreqDecimalChoices.push_back("125");
  menuFreqDecimalChoices.push_back("250");
  menuFreqDecimalChoices.push_back("500");
  uint16_t fq = myFreq;
  uint16_t dec = (myFreq - fq) * 100;
  SerialUSB.printf("Decimal = %d\n", dec);
  if (dec == 0) screenFreqDecimal.selectedIndex = 0;
  else if (dec == 125) screenFreqDecimal.selectedIndex = 1;
  else if (dec == 250) screenFreqDecimal.selectedIndex = 2;
  else screenFreqDecimal.selectedIndex = 3;
  char tmp[12];
  uint8_t bHeight = 32, bSmallWidth = 90, bLargeWidth = 130, overallHeight = 40, ix;
  uint16_t px, py;
  px = (320 - bSmallWidth) / 2;
  py = 120 - (bHeight * 2);

  lcd.setFont(FMB9);
  LGFX_Button btn0;
  if (screenFreqDecimal.selectedIndex > 0) ix = screenFreqDecimal.selectedIndex - 1;
  else ix = menuFreqDecimalChoices.size() - 1;
  btn0.initButton(&lcd, px, py, bSmallWidth, bHeight, TFT_DARKGREY, TFT_WHITE, TFT_DARKGREY, menuFreqDecimalChoices[ix].c_str());
  btn0.press(false);
  myButton b0 = {menuFreqDecimalButtonUp, btn0, FMB9};
  screenFreqDecimal.buttons[0] = b0;

  py = 145;
  LGFX_Button btn2;
  if (screenFreqDecimal.selectedIndex < (menuFreqDecimalChoices.size() - 1)) ix = screenFreqDecimal.selectedIndex + 1;
  else ix = 0;
  btn2.initButton(&lcd, px, py, bSmallWidth, bHeight, TFT_DARKGREY, TFT_WHITE, TFT_DARKGREY, menuFreqDecimalChoices[ix].c_str());
  btn2.press(false);
  myButton b2 = {menuFreqDecimalButtonDown, btn2, FMB9};
  screenFreqDecimal.buttons[2] = b2;

  px = (320 - bLargeWidth) / 2;
  py = 100;
  lcd.setFont(FMB12);
  LGFX_Button btn1;
  btn1.initButton(&lcd, px, py, bLargeWidth, bHeight, TFT_BLACK, TFT_WHITE, TFT_BLUE, menuFreqDecimalChoices[screenFreqDecimal.selectedIndex].c_str());
  btn1.press(true);
  myButton b1 = {menuFreqDecimalButtonSelect, btn1, FMB12};
  screenFreqDecimal.buttons[1] = b1;

  screenFreqDecimal.buttonCount = 3;
  screenFreqDecimal.bgColor = TFT_WHITE;
}

void menuTxButtonDown() {
  // SerialUSB.println("In menuTxButtonDown");
  screenTx.selectedIndex += 1;
  if (screenTx.selectedIndex == menuTxChoices.size()) screenTx.selectedIndex = 0;
  setMenuLabels(screenTx, menuTxChoices);
}

void menuTxButtonUp() {
  if (screenTx.selectedIndex == 0) screenTx.selectedIndex = menuTxChoices.size() - 1;
  else screenTx.selectedIndex -= 1;
  setMenuLabels(screenTx, menuTxChoices);
}

void menuTxButtonSelect() {
  char tmp[64];
  uint8_t ix = screenTx.selectedIndex;
  myTx = stoi(menuTxChoices[ix]);
  sprintf(tmp, "You selected option #%d, ie %d MHz\n", ix, myTx);
  SerialUSB.print(tmp);
  notifyBLE(tmp);
  SerialUSB.println("Setting up Tx...");
  initLoRaSettings();
  savePrefs();
  SerialUSB.println("................done!");
  currentScreen = screenLoRa;
  renderScreen(currentScreen);
}

void initScreenTx() {
  // SerialUSB.println("initScreenTx");
  // Create the labels
  myLabel headerLabel = {
    "Tx", TXT_CENTERED, TXT_TOP, TFT_BLACK, FSS18
  };
  screenTx.labels[0] = headerLabel;
  myLabel footerLabel = {
    "Select a Tx Power", TXT_CENTERED, TXT_BOTTOM, TFT_BLACK, FSS9
  };
  screenTx.labels[1] = footerLabel;
  screenTx.labelCount = 2;
  screenTx.bgColor = TFT_WHITE;
  menuTxChoices.push_back("10");
  menuTxChoices.push_back("11");
  menuTxChoices.push_back("12");
  menuTxChoices.push_back("13");
  menuTxChoices.push_back("14");
  menuTxChoices.push_back("15");
  menuTxChoices.push_back("16");
  menuTxChoices.push_back("17");
  menuTxChoices.push_back("18");
  menuTxChoices.push_back("19");
  menuTxChoices.push_back("20");
  menuTxChoices.push_back("21");
  menuTxChoices.push_back("22");
  screenTx.selectedIndex = myTx - 10;
  // SerialUSB.printf("screenTx.selectedIndex = %d\n", screenTx.selectedIndex);
  // SerialUSB.printf("menuTxChoices.size = %d\n", menuTxChoices.size());
  uint8_t bHeight = 32, bSmallWidth = 100, bLargeWidth = 150, overallHeight = 40;
  uint16_t px, py;
  px = (320 - bSmallWidth) / 2;
  py = 120 - (bHeight * 2);
  screenTx.selectedIndex = myTx - 10;

  lcd.setFont(FMB9);
  LGFX_Button btn0;
  uint8_t ix;
  if (screenTx.selectedIndex > 0) ix = screenTx.selectedIndex - 1;
  else ix = menuTxChoices.size() - 1;
  btn0.initButton(&lcd, px, py, bSmallWidth, bHeight, TFT_DARKGREY, TFT_WHITE, TFT_DARKGREY, menuTxChoices[ix].c_str());
  btn0.press(false);
  myButton b0 = {menuTxButtonUp, btn0, FMB9};
  screenTx.buttons[0] = b0;

  py = 145;
  LGFX_Button btn2;
  if (screenTx.selectedIndex < (menuTxChoices.size() - 1)) ix = +1;
  else ix = 0;
  btn2.initButton(&lcd, px, py, bSmallWidth, bHeight, TFT_DARKGREY, TFT_WHITE, TFT_DARKGREY, menuTxChoices[ix].c_str());
  btn2.press(false);
  myButton b2 = {menuTxButtonDown, btn2, FMB9};
  screenTx.buttons[2] = b2;

  px = (320 - bLargeWidth) / 2;
  py = 100;
  lcd.setFont(FMB12);
  LGFX_Button btn1;
  btn1.initButton(&lcd, px, py, bLargeWidth, bHeight, TFT_BLACK, TFT_WHITE, TFT_BLUE, menuTxChoices[screenTx.selectedIndex].c_str());
  btn1.press(true);
  myButton b1 = {menuTxButtonSelect, btn1, FMB12};
  screenTx.buttons[1] = b1;

  screenTx.buttonCount = 3;
  screenTx.bgColor = TFT_WHITE;
}

void handleReturnToMain(uint8_t ix) {
  mainScreen.selectedIndex = ix;
  mainScreen.buttons[0].button.press(false);
  mainScreen.buttons[1].button.press(false);
  mainScreen.buttons[2].button.press(false);
  mainScreen.buttons[ix].button.press(true);
  renderScreen(mainScreen);
}

void handleLoRaSettings() {
  // SerialUSB.println("handleLoRaSettings");
  mainScreen.selectedIndex = 0;
  mainScreen.buttons[0].button.press(true);
  mainScreen.buttons[1].button.press(false);
  mainScreen.buttons[2].button.press(false);
  screenLoRa.buttons[0].button.press(true);
  renderScreen(screenLoRa);
  currentScreen = screenLoRa;
}

void handleMain2() {
  // SerialUSB.println("Button 2");
}

void handleRollover(myScreen thisScreen, vector<string>choices) {
  // SerialUSB.println("handleRollover");
  renderScreen(thisScreen);
  setMenuLabels(thisScreen, choices);
  while (true) {
    if (digitalRead(WIO_5S_UP) == LOW) {
      while (digitalRead(WIO_5S_UP) == LOW) ; // debounce
      // SerialUSB.println("UP");
      thisScreen.buttons[0].ptr();
    } else if (digitalRead(WIO_5S_DOWN) == LOW) {
      while (digitalRead(WIO_5S_DOWN) == LOW) ; // debounce
      // SerialUSB.println("DOWN");
      thisScreen.buttons[2].ptr();
      SerialUSB.println("Back from DOWN");
    } else if (digitalRead(WIO_5S_PRESS) == LOW) {
      while (digitalRead(WIO_5S_PRESS) == LOW) ; // debounce
      // SerialUSB.println("SELECT");
      thisScreen.buttons[1].ptr();
      return;
    }
  }
}

void handleMain1() {
  // SerialUSB.println("handleMain1");
  renderScreen(screen1);
  lcd.setTextWrap(true, true);
  short length = 0;
  short rssi, snr;
  bool leaving = false;
  Serial1.print("AT+TEST=RXLRPKT\r\n");
  delay(100);

  while (1) {
    if (digitalRead(WIO_KEY_A) == LOW) {
      while (digitalRead(WIO_KEY_A) == LOW) ; // debounce
      leaving = true;
    }
    if (digitalRead(WIO_KEY_B) == LOW) {
      while (digitalRead(WIO_KEY_B) == LOW) ; // debounce
      leaving = true;
    }
    if (digitalRead(WIO_KEY_C) == LOW) {
      while (digitalRead(WIO_KEY_C) == LOW) ; // debounce
      leaving = true;
    }
    if (leaving) {
      handleReturnToMain(1);
      return;
    }
    if (Serial1.available()) {
      SerialUSB.println("Incoming!");
      memset(inBuffer, 0, 512);
      uint16_t ix = 0;
      while (Serial1.available()) {
        char c = Serial1.read();
        delay(5);
        if (c != 13) inBuffer[ix++] = c;
        if (ix == 512) ix = 0; // protection against overflow.
      }
      hexDump(inBuffer, ix);
      char *ptr = strstr((char*)inBuffer, "+TEST: LEN:");
      short number = 0, rssi = 255, snr = 255;
      if (ptr) number = atoi(ptr + 11);
      if (number > 0) {
        drawLoRa(18, 18, 16, TFT_GREEN); // draws the regular LoRa logo in green
        delay(500);
        ptr = strstr((char*)inBuffer, "RSSI:");
        if (ptr)rssi = atoi(ptr + 5);
        ptr = strstr((char*)inBuffer, "SNR:");
        if (ptr)snr = atoi(ptr + 4);
        ptr = strstr((char*)inBuffer, "RX \"");
        if (ptr) {
          ptr += 4;
          lcd.setColor(TFT_WHITE);
          lcd.fillRect(0, 50, 319, 220);
          char tmp[128];
          sprintf(tmp, "Length: %d bytes, RSSI: %d, SNR: %d\n", number, rssi, snr);
          SerialUSB.print(tmp);
          notifyBLE(tmp);
          lcd.drawString(tmp, 4, 50, FSS9);
          hex2array(ptr, number * 2, (char*)inBuffer);
          SerialUSB.println((char*)inBuffer);
          uint16_t py = 72, ix = 0, nChar = 28;
          notifyBLE((char*)inBuffer);
          while (strlen((char*)inBuffer + ix) > nChar) {
            char c = inBuffer[ix + nChar];
            inBuffer[ix + nChar] = 0;
            lcd.drawString((char*)inBuffer + ix, 2, py, FM9);
            ix += nChar;
            inBuffer[ix] = c;
            py += 18;
          }
          lcd.drawString((char*)inBuffer + ix, 2, py, FM9);
        }
        lcd.setColor(TFT_WHITE);
        lcd.fillRect(0, 0, 32, 32);
      }
    }
  }
}

void handleSF() {
  // SerialUSB.println("In handleSF");
  screenSF.selectedIndex = mySF;
  handleRollover(screenSF, menuSFChoices);
  screenLoRa.selectedIndex = 0;
  restoreLoRa();
}

void handleBW() {
  // SerialUSB.println("In handleBW");
  screenBW.selectedIndex = myBW;
  handleRollover(screenBW, menuBWChoices);
  screenLoRa.selectedIndex = 1;
  restoreLoRa();
}

void handleFreq() {
  // SerialUSB.println("In handleFreq");
  if (screenFreq.selectedIndex == -1 || screenFreq.selectedIndex > menuFreqChoices.size() - 1) {
    screenFreq.selectedIndex = 5;
    myFreqIndex = 5;
  }
  handleRollover(screenFreq, menuFreqChoices);
  screenLoRa.selectedIndex = 2;
  restoreLoRa();
}

void handleTx() {
  // SerialUSB.printf("In handleTx. myTx = %d\n", myTx);
  screenTx.selectedIndex = myTx - 10;
  handleRollover(screenTx, menuTxChoices);
  screenLoRa.selectedIndex = 3;
  restoreLoRa();
}

void handleLoRaReturn() {
  handleReturnToMain(0);
}

void drawSlider(myScreen screen) {
  lcd.fillRoundRect(106, 98, 108, 32, 8, TFT_BLUE);
  uint16_t start, width;
  width = 104 - (108 * screen.slider.currentValue / screen.slider.maxValue);
  start = 106 + 108 - width;
  SerialUSB.printf("current: %d, min: %d, max: %d\n", screen.slider.currentValue, screen.slider.minValue, screen.slider.maxValue);
  SerialUSB.printf("width: %d, start: %d\n", width, start);
  lcd.fillRect(start, 100, width, 28, TFT_WHITE);
  lcd.drawRoundRect(106, 98, 108, 32, 8, TFT_BLUE);
  lcd.drawRoundRect(107, 99, 106, 30, 8, TFT_BLUE);
}

void handleSlider(myScreen screen) {
  renderScreen(screen);
  drawSlider(screen);
  while (true) {
    uint32_t lastSlide = millis();
    if (digitalRead(WIO_5S_LEFT) == LOW) {
      while (digitalRead(WIO_5S_LEFT) == LOW && millis() - lastSlide < 100) ; // debounce
      int newValue = screen.slider.currentValue - screen.slider.step;
      if (newValue < screen.slider.minValue) newValue = screen.slider.minValue;
      screen.slider.currentValue = newValue;
      lcd.setBrightness(screen.slider.currentValue);
      drawSlider(screen);
      lastSlide = millis();
    } else if (digitalRead(WIO_5S_RIGHT) == LOW) {
      while (digitalRead(WIO_5S_RIGHT) == LOW && millis() - lastSlide < 100) ; // debounce
      int newValue = screen.slider.currentValue + screen.slider.step;
      if (newValue > screen.slider.maxValue) newValue = screen.slider.maxValue;
      screen.slider.currentValue = newValue;
      lcd.setBrightness(screen.slider.currentValue);
      drawSlider(screen);
      lastSlide = millis();
    } else if (digitalRead(WIO_5S_PRESS) == LOW) {
      while (digitalRead(WIO_5S_PRESS) == LOW) ; // debounce
      return;
    }
  }
}

void handleLuminosity() {
  myScreen previousScreen = currentScreen;
  handleSlider(screenLumi);
  currentScreen = previousScreen;
  renderScreen(currentScreen);
}

void initScreenLumi() {
  // Create the labels
  myLabel headerLabel = {
    "Luminosity", TXT_CENTERED, TXT_TOP, TFT_BLACK, FSS18
  };
  screenLumi.labels[0] = headerLabel;
  myLabel footerLabel = {
    "Adjust screen brightness", TXT_CENTERED, TXT_BOTTOM, TFT_BLACK, FSS9
  };
  screenLumi.labels[1] = footerLabel;
  screenLumi.labelCount = 2;
  screenLumi.bgColor = TFT_WHITE;
  screenLumi.slider.minValue = 10;
  screenLumi.slider.maxValue = 255;
  screenLumi.slider.currentValue = luminosity;
  screenLumi.slider.step = 5;
}
