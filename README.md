# CubeRoomAgent

This sketch is a refactored version of the Crystal Maze sketch which was previously handling all communication with Cube's back-end database directly,  through an Ethernet connection. The refactoring changes the communication architecture which now relies on getting status information and posting  updates about the room through the Arduino Agent, an application that is installed on the room's computer which is connected with the "master" Arduino through a USB connection. The Arduino Agent:
- acts as a "smart" proxy for all communication between the back-end database (accessed through an API) and the "master" Arduino
- offloads many computational tasks from the Arduino
- provides a UI for all logs sent from the Arduino to the serial port
- checks for "heartbeat" messages sent from the Arduino, to decide whether it is still working. If a heartbeat is not received within a specific period of time, the agent assumes a problem with the Arduino and resets it automatically.

Other features of the Arduino Agent include:
- Detailed logging of any errors during communication with the back-end. This feature, combined with the logging of all communication between the Arduino and the agent, is expected to considerably decrease problem resolution times.
- Agents will be keeping a copy of the latest status of the room and the players. In case of communication problems with the back-end, the agents will be able to continue serving the Arduinos with cached information and will also keep track of updated information that hasn't been sent to the database yet (to retry sending it when connectivity is resumed).

The new architecture has a couple of additional advantages:
- Arduinos will be "protected" from any accidental structural changes in the database of Cube.
- The cost and complexity of "master" Arduinos will be decreased, since they won't need to support communication through Ethernet and accessing a database.

# Reusing the refactored code for other rooms

For any room that will be upgraded to the new architecture, the following changes should be applied in the code of its sketch:
- **Replace `CubeRoom.cpp` and `CuberRoom.h`**  
The previous `CubeRoom.cpp` and `CuberRoom.h` files should be replaced with the corresponding files from this repository.
- **Update all `updateRoomStatus()` calls and any other code which handles room statuses to use the new `int` values**  
The initial version used strings for the different statuses of a room, so all code that handles statuses should be adjusted to use int values instead of strings. The values are defined in CubeRoom.h as follows:
    ```c
    const int activeStatus = 0;
    const int activatedStatus = 1;
    const int winStatus = 2;
    const int loseStatus = 3;
    const int timeoutStatus = 4;
    const int emergencyStatus = 5;
    const int inactiveStatus = 6;
    const int doorStatus = 7;
    const int maintenanceStatus = 8;
    ```
    > **Example**  
    `room.updateRoomStatus("inactive");` should change to: `room.updateRoomStatus(room.inactiveStatus);`
- **Include the "heartbeat" in every long running loop**  
Once a game starts, the Arduino enters a "game loop" and it is important that the loop doesn't block the Arduino from sending the heartbeat message. To enable the heartbeat:
   - Use the TimerEvent library: `#include <TimerEvent.h>`
   - Initialise the timer and define the callback function for the heartbeat:
        ```
        void setup() {
        ......
        agentLoop.set(1000, updateDataFromAgent);
        ......
        }

        void updateDataFromAgent() {
        room.pingAgent();
        }
        ```
   - In every long-running/game loop, include a check whether the heartbeat should be sent : `agentLoop.update();`
- **Replace every all `Serial.print()` calls with `Serial.println()`**
