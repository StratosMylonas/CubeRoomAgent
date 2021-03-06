/*
  CubeRoomAgent.h - Arduino Library for Cube Ultimate Challenges rooms.
*/

#ifndef CubeRoomAgent_h
#define CubeRoomAgent_h

#include <Adafruit_NeoPixel.h>
#include <MemoryFree.h>
#include <TimerEvent.h>

#include "Arduino.h"

// CubeRoomAgent class
class CubeRoomAgent {
 public:
  CubeRoomAgent(char *);

  void printRoomName();
  void updateData();
  void pingAgent();
  int updateRoomStatus(int);
  int getRoomStatus();

  int relay = 9;
  bool hasMonitor =
      true;  // true by default, change to false (recommended from game's file,
             // to keep library intact, eg. room.hasMonitor = false;) to light
             // up mid cube sign
  bool hasDoorFrame =
      false;  // false by default, change to true (recommended from game's file,
              // to keep library intact, eg. room.hasDoorFrame = true;) to light
              // up door frame sign
  bool isBaseStation =
      true;  // true by default, change to false (recommended from game's file,
             // to keep library intact, eg. room.isBaseStation = false;) to make
             // library not act as base_station

  //define functions
  void setup();

  int getDifficulty();
  int getNumberOfPlayers();
  int getGameTime();
  bool isActive();
  bool checkEmergency();
  void clear();

  bool waitToRun(int);
  void finishLose();
  void finishLose(int);
  void finishTimeout();
  void finishWin();
  void finishWin(int, int);
  void finishWin(int, int, int, int);
  void postScore(int, int);
  void postScore(int, int, int, int);

  void lightRed();
  void lightGreen();
  void lightBlue();
  void lightCube();
  void lightOff();
  void lightRGB(int, int, int);
  void lightMidCube(bool);
  void lightARGB(int, int, int, int, int);
  void lightARGBBubbleTrouble(int, int);

  int getDoorState();

  int getRoomStatusFromSerial();

  //define room status numbers
  const int activeStatus = 0;
  const int activatedStatus = 1;
  const int winStatus = 2;
  const int loseStatus = 3;
  const int timeoutStatus = 4;
  const int emergencyStatus = 5;
  const int inactiveStatus = 6;
  const int doorStatus = 7;
  const int maintenanceStatus = 8;


 private:
  //define default pins
  const int doorLock = 5;
  const int redLEDPin = 6;
  const int greenLEDPin = 7;
  const int blueLEDPin = 8;

  const int doorFrameRedPin = 22;
  const int doorFrameGreenPin = 24;
  const int doorFrameBluePin = 26;
  const int startPin = 9;
  const int dTrig = 14;
  const int emergency = 31;

  //define difficulty numbers
  const int easyDifficulty = 1;
  const int mediumDifficulty = 2;
  const int hardDifficulty = 3;
  const int juniorDifficulty = 4;

  char *roomName;

  bool active = false;
  bool agentReady = false;

  int roomStatus = inactiveStatus;
  int difficulty;

  int gameTime;
  int numberOfPlayers;

  void checkSerialIntegrity();
  bool setNewScore(int, int, int, int);
  int setStatus(int);
  int getRoomDifficultyFromSerial();
  int getNumberOfPlayersFromSerial();
  void checkAgentStatus();

  //define constants
  long activeTimeout = 80000; //time waiting if door won't open on active room_status, to return to inactive
  long endDelay = 20000;      //time for room to turn to inactive after a game has ended
};

#endif
