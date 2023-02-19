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
#include "SoftwareSerial1.h"

SoftwareSerial gps(3, 2);
float latitude, longitude;
char buffer[256];
char gpsBuff[128];
char timeBuff[128];
uint8_t ix = 0;
vector<string> userStrings;
char UTC[7] = {0};
uint8_t SIV = 0;
double lastRefresh = 0;
bool waitForDollar = true, hasFix = false;

#define GPS_DELAY 5000
#define GPS_DURATION 800
uint32_t lastGPS;

float parseDegrees(const char *term) {
  float value = (float)(atof(term) / 100.0);
  uint16_t left = (uint16_t)value;
  value = (value - left) * 1.66666666666666;
  value += left;
  return value;
}

vector<string> parseNMEA(string nmea) {
  vector<string>result;
  if (nmea.at(0) != '$') {
    Serial.println("Not an NMEA sentence!");
    return result;
  }
  size_t lastFound = 0;
  size_t found = nmea.find(",", lastFound);
  while (found < nmea.size() && found != string::npos) {
    string token = nmea.substr(lastFound, found - lastFound);
    result.push_back(token);
    lastFound = found + 1;
    found = nmea.find(",", lastFound);
  }
  string token = nmea.substr(lastFound, found - lastFound);
  result.push_back(token);
  lastFound = found + 1;
  found = nmea.find(",", lastFound);
  return result;
}

void parseGPRMC(vector<string>result) {
  if (result.at(1) != "") {
    sprintf(timeBuff, "%s:%s:%s UTC", result.at(1).substr(0, 2).c_str(), result.at(1).substr(2, 2).c_str(), result.at(1).substr(4, 2).c_str());
    Serial.println(timeBuff);
  }
  if (result.at(2) == "V") {
    Serial.printf("Invalid fix! [%c]", result.at(2));
    hasFix = false;
  } else {
    Serial.printf("Valid fix! [%c]", result.at(2));
    hasFix = true;
  }
  if (result.at(3) != "") {
    float newLatitude, newLongitude;
    int8_t signLat = 1, signLong = 1;
    if (result.at(4).c_str()[0] == 'W') signLat = -1;
    if (result.at(6).c_str()[0] == 'S') signLong = -1;
    newLatitude = signLat * parseDegrees(result.at(3).c_str());
    newLongitude = signLong * parseDegrees(result.at(5).c_str());
    if (newLatitude != latitude || newLongitude != longitude) {
      latitude = newLatitude;
      longitude = newLongitude;
      sprintf(gpsBuff, "Coordinates:\n%3.8f, %3.8f\n", latitude, longitude);
      Serial.print(gpsBuff);
      // refreshDisplay();
    }
  }
}

void parseGPGGA(vector<string>result) {
  if (result.at(1) != "") {
    sprintf(timeBuff, "UTC Time: %s:%s:%s\n", result.at(1).substr(0, 2).c_str(), result.at(1).substr(2, 2).c_str(), result.at(1).substr(4, 2).c_str());
    Serial.print(timeBuff);
  }
  //  if (result.at(6) == "0") Serial.println("Invalid fix!");
  //  else Serial.println("Valid fix!");
  if (result.at(2) != "") {
    latitude = parseDegrees(result.at(2).c_str());
    longitude = parseDegrees(result.at(4).c_str());
    sprintf(timeBuff, "Coordinates: %3.8f %c, %3.8f %c\n", latitude, result.at(3).c_str()[0], longitude, result.at(5).c_str()[0]);
    Serial.print(timeBuff);
  }
}

void parseGPGLL(vector<string>result) {
  if (result.at(1) != "") {
    latitude = parseDegrees(result.at(1).c_str());
    longitude = parseDegrees(result.at(3).c_str());
    sprintf(gpsBuff, "Coordinates:\n%3.8f %c, %3.8f %c\n", latitude, result.at(2).c_str()[0], longitude, result.at(4).c_str()[0]);
    Serial.print(gpsBuff);
  }
}

void parseGPGSV(vector<string>result) {
  if (result.at(1) != "") {
    uint8_t newSIV = atoi(result.at(3).c_str());
    if (SIV != newSIV) {
      sprintf(buffer, "Message %s / %s. SIV: %s\n", result.at(2).c_str(), result.at(1).c_str(), result.at(3).c_str());
      Serial.print(buffer);
      SIV = newSIV;
    }
  }
}

void parseGPTXT(vector<string>result) {
  //$GPTXT, 01, 01, 02, ANTSTATUS = INIT
  if (result.at(1) != "") {
    sprintf(buffer, " . Message %s / %s. Severity: %s\n . Message text: %s\n",
            result.at(2).c_str(), result.at(1).c_str(), result.at(3).c_str(), result.at(4).c_str(), result.at(5).c_str());
    Serial.print(buffer);
  }
}

void parseGPVTG(vector<string>result) {
  Serial.println("Track Made Good and Ground Speed.");
  if (result.at(1) != "") {
    sprintf(buffer, " . True track made good %s [%s].\n", result.at(1).c_str(), result.at(2).c_str());
    Serial.print(buffer);
  }
  if (result.at(3) != "") {
    sprintf(buffer, " . Magnetic track made good %s [%s].\n", result.at(3).c_str(), result.at(4).c_str());
    Serial.print(buffer);
  }
  if (result.at(5) != "") {
    sprintf(buffer, " . Speed: %s %s.\n", result.at(5).c_str(), result.at(6).c_str());
    Serial.print(buffer);
  }
  if (result.at(7) != "") {
    sprintf(buffer, " . Speed: %s %s.\n", result.at(7).c_str(), result.at(8).c_str());
    Serial.print(buffer);
  }
}

void parseGPGSA(vector<string>result) {
  // $GPGSA,A,3,15,29,23,,,,,,,,,,12.56,11.96,3.81
  Serial.println("GPS DOP and active satellites");
  if (result.at(1) == "A") Serial.println(" . Mode: Automatic");
  else if (result.at(1) == "M") Serial.println(" . Mode: Manual");
  else Serial.println(" . Mode: ???");
  if (result.at(2) == "1") {
    Serial.println(" . Fix not available.");
    return;
  } else if (result.at(2) == "2") Serial.println(" . Fix: 2D");
  else if (result.at(2) == "3") Serial.println(" . Fix: 3D");
  else {
    Serial.println(" . Fix: ???");
    return;
  }
  Serial.print(" . PDOP: "); Serial.println(result.at(result.size() - 3).c_str());
  Serial.print(" . HDOP: "); Serial.println(result.at(result.size() - 2).c_str());
  Serial.print(" . VDOP: "); Serial.println(result.at(result.size() - 1).c_str());
}

void initGPS() {
  SerialUSB.println("============");
  SerialUSB.println(" GPS Setup");
  SerialUSB.println("============");
  gps.begin(9600);
  gps.listen();
  lastGPS = millis();
}
