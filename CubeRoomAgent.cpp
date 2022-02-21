
/*
  CubeRoomAgent.cpp - Arduino Library for Cube Ultimate Challenges rooms.
*/
#include "CubeRoomAgent.h"

CubeRoomAgent::CubeRoomAgent(char *_roomName) {
  roomName = _roomName;

  if (isBaseStation) {
    pinMode(redLEDPin, OUTPUT);
    pinMode(greenLEDPin, OUTPUT);
    pinMode(blueLEDPin, OUTPUT);

    pinMode(doorLock, OUTPUT);
    pinMode(dTrig, INPUT);

    pinMode(emergency, INPUT);
    pinMode(relay2, OUTPUT);
  }

  if (hasDoorFrame) {
    pinMode(doorFrameRedPin, OUTPUT);
    pinMode(doorFrameGreenPin, OUTPUT);
    pinMode(doorFrameBluePin, OUTPUT);
  }
}

void CubeRoomAgent::printRoomName() {
  String room = roomName;
  Serial.println("{\"type\":0,\"name\":\"" + room + "\"}");
}

int CubeRoomAgent::getRoomStatusFromSerial() {
  String resp = "";

  while (!resp.startsWith(">")) {
    Serial.println("{\"type\":1,\"mem\":" + String(freeMemory()) +
                   ",\"rt\":" + String(millis()) + "}");
    delay(100);
    resp = Serial.readStringUntil('\n');
    if (resp == "E") {
      agentReady = false;
      return inactiveStatus;
    }
  }
  resp.replace(">", "");

  return resp.toInt();
}

int CubeRoomAgent::getRoomDifficultyFromSerial() {
  String resp = "";

  while (!resp.startsWith(">")) {
    Serial.println("{\"type\":2}");
    delay(100);
    resp = Serial.readStringUntil('\n');
  }
  resp.replace(">", "");
  return resp.toInt();
}

int CubeRoomAgent::getNumberOfPlayersFromSerial() {
  String resp = "";

  while (!resp.startsWith(">")) {
    Serial.println("{\"type\":5}");
    delay(100);
    resp = Serial.readStringUntil('\n');
  }
  resp.replace(">", "");
  return resp.toInt();
}

// Game game time according to difficulty and number of players
int CubeRoomAgent::getGameTime() {
  String resp = "";

  while (!resp.startsWith(">")) {
    Serial.println("{\"type\":6}");
    delay(100);
    resp = Serial.readStringUntil('\n');
  }
  resp.replace(">", "");
  gameTime = resp.toInt();
  return gameTime;
}

void CubeRoomAgent::checkAgentStatus() {
  while (!agentReady) {
    printRoomName();
    delay(500);

    String str = Serial.readStringUntil('\n');
    if (str == "OK") {
      agentReady = true;
    }
  }
}

// Update db with new score, set hishscore if required, returns true when
// highscore
bool CubeRoomAgent::setNewScore(int score, int target, int bonus, int timeRem) {
  delay(500);
  String resp = "";
  while (!resp.startsWith(">")) {
    Serial.println("{\"type\":7,\"sc\":" + String(score) +
                   ",\"t\":" + String(target) + ",\"b\":" + String(bonus) +
                   ",\"tr\":" + String(timeRem) + "}");
    delay(500);
    resp = Serial.readStringUntil('\n');
  }
  resp.replace(">", "");
  return resp == String("true");
}

int CubeRoomAgent::setStatus(int newStatus) {
  delay(500);
  while (true) {
    Serial.println("{\"type\":4,\"st\":" + String(newStatus) + "}");
    delay(500);
    String str = Serial.readStringUntil('\n');
    if (str == "OK") {
      return 1;
    }
  }
}

void CubeRoomAgent::checkSerialIntegrity() {
  delay(500);
  while (true) {
    Serial.println("{\"type\":9,\"dif\":" + String(difficulty) +
                   ",\"nop\":" + String(numberOfPlayers) + "}");
    delay(500);
    String str = Serial.readStringUntil('\n');
    if (str == "OK") {
      return;
    }
  }
}

void CubeRoomAgent::pingAgent() {
  Serial.println("{\"type\":8,\"mem\":" + String(freeMemory()) +
                 ",\"rt\":" + String(millis()) + "}");
  delay(100);
}

// Update data read from serial
void CubeRoomAgent::updateData() {
  active = false;

  checkAgentStatus();

  if (!hasMonitor && isBaseStation) {
    lightMidCube(false);
  }

  roomStatus = getRoomStatusFromSerial();

  if (roomStatus == activeStatus) {
    difficulty = getRoomDifficultyFromSerial();
    numberOfPlayers = getNumberOfPlayersFromSerial();
    checkSerialIntegrity();
    active = true;
  } else if ((roomStatus == activatedStatus || roomStatus == winStatus ||
              roomStatus == loseStatus || roomStatus == timeoutStatus ||
              roomStatus == emergencyStatus) &&
             isBaseStation) {
    int ret = updateRoomStatus(inactiveStatus);
    if (ret == -1) {
      return -1;
    }
  } else if (isBaseStation) {
    lightOff();
    active = false;
  }

  if (roomStatus == doorStatus && isBaseStation) {
    digitalWrite(doorLock, HIGH);
  } else {
    digitalWrite(doorLock, LOW);
  }
}

// Check if room is active
bool CubeRoomAgent::isActive() { return active; }

// Update room status
int CubeRoomAgent::updateRoomStatus(int newStatus) {
  return setStatus(newStatus);
}

bool CubeRoomAgent::checkEmergency() {
  if (digitalRead(emergency) == HIGH && isBaseStation) {
    digitalWrite(doorLock, HIGH);
    updateRoomStatus(emergencyStatus);
    Serial.println(F("Update Room Status: Emergency"));
    lightRed();
    delay(2000);
    updateRoomStatus(loseStatus);
    Serial.println(F("Update Room Status: Lose"));
    delay(2000);

    return true;
  } else {
    return false;
  }
}

// Get active team's selected difficulty
int CubeRoomAgent::getDifficulty() { return difficulty; }

// Get active team's ID number of players
int CubeRoomAgent::getNumberOfPlayers() { return numberOfPlayers; }

// Wait for door to open to set the game as activated. Sets inactive if timeout
// occurs
bool CubeRoomAgent::waitToRun(int doorChangeStateType) {
  // 0 - start instantly
  // 1 - wait for door to open
  // 2 - wait for door to open and close
  lightCube();
  if (!hasMonitor) {
    lightMidCube(true);
  }
  unsigned long activeTime = millis() + activeTimeout;

  if (doorChangeStateType != 0) {
    digitalWrite(doorLock, HIGH);
    Serial.println(F("Door Unlocked"));
    while (getDoorState() == 1) {
      delay(100);
      pingAgent();
      if (millis() > activeTime) {
        active = false;
        updateRoomStatus(inactiveStatus);
        delay(50);
        digitalWrite(doorLock, LOW);
        lightOff();
        Serial.println("Door timeout - didn't open, return to inactive");
        return false;
      }
    }
    Serial.println(F("Door Opened"));
  }
  if (doorChangeStateType == 2) {
    while (getDoorState() == 0) {
      delay(100);
      pingAgent();
      if (millis() > activeTime) {
        active = false;
        updateRoomStatus(inactiveStatus);
        delay(50);
        digitalWrite(doorLock, LOW);
        lightOff();
        Serial.println("Door timeout - didn't open, return to inactive");
        return false;
      }
    }
    Serial.println(F("Door Closed"));
  }

  digitalWrite(doorLock, LOW);
  updateRoomStatus(activatedStatus);

  return true;
}

void CubeRoomAgent::finishLose() {
  Serial.println(F("Update Room Status: Lose"));
  updateRoomStatus(loseStatus);
  lightRed();
  unsigned long endDelayTime = millis() + endDelay;
  while (millis() < endDelayTime) {
    pingAgent();
    delay(1000);
  }
  lightOff();
}

void CubeRoomAgent::finishLose(int timeRem) {
  Serial.println(F("Update Room Status: Lose"));
  lightRed();
  postScore(0, timeRem);
  updateRoomStatus(loseStatus);
  unsigned long endDelayTime = millis() + endDelay;
  while (millis() < endDelayTime) {
    pingAgent();
    delay(1000);
  }
  updateRoomStatus(inactiveStatus);
  lightOff();
}

void CubeRoomAgent::finishTimeout() {
  Serial.println(F("Update Room Status: Timeout"));
  lightRed();
  updateRoomStatus(timeoutStatus);
  postScore(0, 0);
  unsigned long endDelayTime = millis() + endDelay;
  while (millis() < endDelayTime) {
    pingAgent();
    delay(1000);
  }
  updateRoomStatus(inactiveStatus);
  lightOff();
}

void CubeRoomAgent::finishWin() {
  Serial.println(F("Update Room Status: Win"));
  lightGreen();
  unsigned long endDelayTime = millis() + endDelay;
  while (millis() < endDelayTime) {
    pingAgent();
    delay(1000);
  }
  lightOff();
}

void CubeRoomAgent::finishWin(int score, int timeRem) {
  Serial.println(F("Update Room Status: Win"));
  lightGreen();
  postScore(score, timeRem);
  updateRoomStatus(winStatus);
  unsigned long endDelayTime = millis() + endDelay;
  while (millis() < endDelayTime) {
    pingAgent();
    delay(1000);
  }
  updateRoomStatus(inactiveStatus);
  lightOff();
}

void CubeRoomAgent::finishWin(int score, int target, int bonus, int timeRem) {
  Serial.println(F("Update Room Status: Win"));
  lightGreen();
  postScore(score, target, bonus, timeRem);
  if (bonus == 0) {
    updateRoomStatus(timeoutStatus);
  } else {
    updateRoomStatus(winStatus);
  }
  unsigned long endDelayTime = millis() + endDelay;
  while (millis() < endDelayTime) {
    pingAgent();
    delay(1000);
  }
  updateRoomStatus(inactiveStatus);
  lightOff();
}

void CubeRoomAgent::postScore(int score, int timeRem) {
  Serial.println("Post Score: " + String(score));
  bool highScore = setNewScore(score, 0, 0, timeRem);
  if (highScore) {
    Serial.println(F("New Highscore"));
  }
}

void CubeRoomAgent::postScore(int score, int target, int bonus, int timeRem) {
  Serial.println("Post Score: " + String(score));
  bool highScore = setNewScore(score, target, bonus, timeRem);
  if (highScore) {
    Serial.println(F("New Highscore"));
  }
}

int CubeRoomAgent::getDoorState() { return digitalRead(dTrig); }

//------------------RGB States------------------
// Red
void CubeRoomAgent::lightRed() {
  if (strcmp(roomName, "BUBBLE_TROUBLE") == 0) {
    lightARGB(22, 138, 255, 0, 0);
  } else if (isBaseStation) {
    digitalWrite(redLEDPin, HIGH);
    digitalWrite(greenLEDPin, LOW);
    digitalWrite(blueLEDPin, LOW);
    if (hasDoorFrame) {
      digitalWrite(doorFrameRedPin, HIGH);
      digitalWrite(doorFrameGreenPin, LOW);
      digitalWrite(doorFrameBluePin, LOW);
    }
  }
}

// Green
void CubeRoomAgent::lightGreen() {
  if (strcmp(roomName, "BUBBLE_TROUBLE") == 0) {
    lightARGB(22, 138, 0, 255, 0);
  } else if (isBaseStation) {
    digitalWrite(redLEDPin, LOW);
    digitalWrite(greenLEDPin, HIGH);
    digitalWrite(blueLEDPin, LOW);
    if (hasDoorFrame) {
      digitalWrite(doorFrameRedPin, LOW);
      digitalWrite(doorFrameGreenPin, HIGH);
      digitalWrite(doorFrameBluePin, LOW);
    }
  }
}

// Blue
void CubeRoomAgent::lightBlue() {
  if (strcmp(roomName, "BUBBLE_TROUBLE") == 0) {
    lightARGB(22, 138, 0, 0, 255);
  } else if (isBaseStation) {
    digitalWrite(redLEDPin, LOW);
    digitalWrite(greenLEDPin, LOW);
    digitalWrite(blueLEDPin, HIGH);
    if (hasDoorFrame) {
      digitalWrite(doorFrameRedPin, LOW);
      digitalWrite(doorFrameGreenPin, LOW);
      digitalWrite(doorFrameBluePin, HIGH);
    }
  }
}

// Cyan
void CubeRoomAgent::lightCube() {
  if (strcmp(roomName, "BUBBLE_TROUBLE") == 0) {
    lightARGBBubbleTrouble(22, 138);
  } else if (isBaseStation) {
    digitalWrite(redLEDPin, LOW);
    digitalWrite(greenLEDPin, HIGH);
    digitalWrite(blueLEDPin, HIGH);
    if (hasDoorFrame) {
      digitalWrite(doorFrameRedPin, LOW);
      digitalWrite(doorFrameGreenPin, HIGH);
      digitalWrite(doorFrameBluePin, HIGH);
    }
  }
}

// Off
void CubeRoomAgent::lightOff() {
  if (strcmp(roomName, "BUBBLE_TROUBLE") == 0) {
    lightARGB(22, 138, 0, 0, 0);
  } else if (isBaseStation) {
    digitalWrite(redLEDPin, LOW);
    digitalWrite(greenLEDPin, LOW);
    digitalWrite(blueLEDPin, LOW);
    if (hasDoorFrame) {
      digitalWrite(doorFrameRedPin, LOW);
      digitalWrite(doorFrameGreenPin, LOW);
      digitalWrite(doorFrameBluePin, LOW);
    }
  }
}

// Custom
void CubeRoomAgent::lightRGB(int r, int g, int b) {
  if (isBaseStation) {
    analogWrite(redLEDPin, r);
    analogWrite(greenLEDPin, g);
    analogWrite(blueLEDPin, b);
  }
}

// Light Mid Cube
void CubeRoomAgent::lightMidCube(bool state) { digitalWrite(relay2, state); }

void CubeRoomAgent::lightARGB(int pin, int pixels, int red, int green,
                              int blue) {
  Adafruit_NeoPixel ledTape(pixels, pin, NEO_RGB + NEO_KHZ800);
  ledTape.begin();

  for (int i = 0; i < pixels; i++) {
    ledTape.setPixelColor(i, ledTape.Color(red, green, blue));
    ledTape.show();
  }
}

void CubeRoomAgent::lightARGBBubbleTrouble(int pin, int pixels) {
  Adafruit_NeoPixel ledTape(pixels, pin, NEO_RGB + NEO_KHZ800);
  ledTape.begin();

  for (int i = 0; i < 16; i++) {
    ledTape.setPixelColor(i, ledTape.Color(0, 0, 255));
  }
  for (int i = 16; i < 35; i++) {
    ledTape.setPixelColor(i, ledTape.Color(0, 255, 0));
  }
  for (int i = 35; i < 100; i++) {
    ledTape.setPixelColor(i, ledTape.Color(105, 10, 85));
  }
  for (int i = 100; i < 117; i++) {
    ledTape.setPixelColor(i, ledTape.Color(0, 255, 0));
  }
  for (int i = 117; i < 138; i++) {
    ledTape.setPixelColor(i, ledTape.Color(0, 0, 255));
  }
  ledTape.show();
}
