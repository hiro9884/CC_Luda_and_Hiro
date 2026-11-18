The Dancer
A wearable DF Pong controller by Walid Idris (Luda), developed alongside Hiromune Kubayashi’s The Entertainer for Experiment 3, Creation & Computation, OCAD University, Fall 2026.
How to play
Open DF Pong and connect the battery-powered vest through the game’s Bluetooth interface.
- Lean left to move the paddle toward the top.
- Lean right to move it toward the bottom.
- Straighten to return near the centre.
Shoulder LEDs indicate leaning direction and flash for game events. Select Music → Players’ choice in the lobby to hear the custom background music.
Hardware and code
- Adafruit ESP32 Feather V2 and LSM6DS3TR-C IMU, wired through header pins using I2C.
- Three white LEDs per shoulder, each with its own resistor: left GPIO 27, right GPIO 33.
- Battery and electronics housed inside a second-hand red vest; IMU mounted in the small red hanging flap near the collar.
The sketch builds on the DF Pong tilt example. Madgwick estimates tilt from acceleration and angular velocity; smoothing reduces fluctuations before the angle is mapped to paddle position over BLE. LED effects use nonblocking timers.

Libraries: DFPongController, NimBLE-Arduino, Adafruit LSM6DS, and MadgwickAHRS. Controller number: 11. Serial Monitor: 9600 baud.
AI attribution
OpenAI ChatGPT assisted with smoothTilt(), updateDirectionLeds(), startLedEffect(), updateLedFeedback(), handleGameEvent(), and updatePaddleColor(), as well as debugging, formatting, and comments. ChatGPT also assisted with documentation and zine text, layout, and illustration. Interfaces: ChatGPT chat and Work Mode. The exact model is GPT Astra 6.1 (Medium and Light).
References
- DF Pong game and examples
- DFPongController library
- Adafruit LSM6DS3TR-C guide
- MadgwickAHRS