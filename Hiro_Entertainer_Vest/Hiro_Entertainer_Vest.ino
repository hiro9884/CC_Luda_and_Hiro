/*
DF Pong: Tilt

Moves your paddle to a height set by how far you tip the 6DOF IMU forward or back:
tip it one way and the paddle goes to the top, the other way and it goes to the bottom.
The angle is the pitch from 01_imuTilt: the Madgwick library turns the IMU's readings into degrees.

The IMU (LSM6DS3TR-C) plugs into the Feather's STEMMA QT port with a STEMMA QT / Qwiic cable.

Open the Serial Monitor at 9600 baud. Tip the sensor as far as you want to each way,
and put the two angles you see in topAngle and bottomAngle.
Needs the DFPongController library (2.4 or newer), NimBLE-Arduino, Adafruit LSM6DS and Madgwick,
all from the Library Manager.
*/

#include <DFPongController.h>
#include <Adafruit_LSM6DS3TRC.h>
#include <MadgwickAHRS.h>
#include <ESP32Servo.h>

DFPongController controller;
Adafruit_LSM6DS3TRC imu;
Madgwick filter;
Servo servo;

// Your controller number (1 - 242): every player has a different one
int controllerNumber = 12;

// IMU variables
sensors_event_t accel;
sensors_event_t gyro;
sensors_event_t temp;
int tiltAngle;

// The angles for the top and bottom of the screen: change these to yours
int topAngle = -20;
int bottomAngle = 20;

// The filter gets a new reading every 10 ms, the Serial Monitor every 100 ms
unsigned long lastUpdate = 0;
unsigned long lastPrint = 0;

// Servo settings
const int servoPin = 13;
const int openThreshold = -10;
const int closeThreshold = -5;

bool servoOpen = false;

void setup()
{
  Serial.begin(9600);

  servo.setPeriodHertz(50);
servo.attach(servoPin, 1000, 2000);

  // Start the IMU on the I2C bus (address 0x6A)
  // and the filter (100 readings a second)
  if (!imu.begin_I2C())
  {
    Serial.println("No IMU found: check the STEMMA QT cable");
  }

  filter.begin(100);

  controller.setControllerNumber(controllerNumber);
  //controller.setStatusLED(LED_BUILTIN);

  // Customize your controller: remove the // at the start of a line to use it

  // Your controller's name in the game (up to 20 characters)
  controller.setControllerName("Samurai");

  // Paddle color, as a web color code
  controller.setPlayerColor("#00BFFF");

  // Paddle speed: 1 - 100 (normal is 10)
  // controller.setMoveSpeed(10);

  // Paddle size: 50 - 200 percent (normal is 100)
  controller.setPaddleSize(200);

  // Sounds: a built-in name, or an https:// link straight to your own mp3 or wav file
  // Built-in sounds: pingpong, claves, tick, bell, gong, bikebell, flourish,
  // applause, pip, quindar, clock, blip, boop, pluck, click, laser, drum,
  // chirp, zap, thud, wood, glass, coin
  controller.setHitSound("applause"); // When your paddle hits the puck
  controller.setScoreSound("Wood");  // When you score a point
  controller.setWinSound("drum");    // When you win the match

  // Background music: a built-in name, or an https:// link straight to your own mp3 file
  // Built-in music: drift, tabletop, arcade, nightbus, gymnopedie,
  // smallsteps, bounce, currents, palmleaf, spacemtv
  // The Music tile in the game's lobby has to be set to "Players' choice" to hear it
  controller.setMusic("https://raw.githubusercontent.com/DigitalFuturesOCADU/df-pong/main/game/cc26/assets/music/palmleaf.mp3");

  controller.begin();

    // ゲーム側の初期化が終わってからサーボを設定
  servo.setPeriodHertz(50);
  servo.attach(servoPin, 1000, 2000);
  servo.write(0);
  servoOpen = false;
}

void loop()
{
  // Keeps the connection to the game going: needed every loop
  controller.update();

  // Every 10 ms, give the filter a new reading
  if (millis() - lastUpdate >= 10)
  {
    lastUpdate = millis();

    imu.getEvent(&accel, &gyro, &temp);

    filter.updateIMU(
      degrees(gyro.gyro.x),
      degrees(gyro.gyro.y),
      degrees(gyro.gyro.z),
      accel.acceleration.x,
      accel.acceleration.y,
      accel.acceleration.z
    );
  }

  // The tilt forward and back, in degrees
  tiltAngle = filter.getPitch();

  // Every 100 ms, print the angle, to find your top and bottom angles
  if (millis() - lastPrint >= 100)
  {
    lastPrint = millis();

    Serial.print("Tilt: ");
    Serial.println(tiltAngle);
  }

  // Keep the value inside its range
  tiltAngle = constrain(tiltAngle, topAngle, bottomAngle);

  // topAngle puts the paddle at the top, bottomAngle at the bottom
  controller.sendPosition(tiltAngle, topAngle, bottomAngle);

  if (!servoOpen && tiltAngle <= openThreshold)
{
  Serial.println("OPEN command");
  servo.write(180);
  servoOpen = true;

}
else if (servoOpen && tiltAngle > closeThreshold)
{
  Serial.println("CLOSE command");
  servo.write(0);
  servoOpen = false;

}
}