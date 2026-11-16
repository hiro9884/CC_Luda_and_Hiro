/*
// C&C Luda and Hiro - Vest Project
// Test comments for GITTTYHUB
//LUDA DF TILT CODE VERSION 
DF Pong: Tilt

Moves your paddle to a height set by how far you tip the 6DOF IMU forward or back:
tip it one way and the paddle goes to the top, the other way and it goes to the bottom.
The angle is the pitch from imuTilt: the Madgwick library turns the IMU's readings into degrees.

The IMU (LSM6DS3TR-C) plugs into the Feather's STEMMA QT port with a STEMMA QT / Qwiic cable.

Open the Serial Monitor at 9600 baud. Tip the sensor as far as you want to each way,
and put the two angles you see in topAngle and bottomAngle.
Needs the DFPongController library (2.4 or newer), NimBLE-Arduino, Adafruit LSM6DS and Madgwick,
all from the Library Manager.
*/


#include <DFPongController.h>
#include <Adafruit_LSM6DS3TRC.h>
#include <MadgwickAHRS.h>

DFPongController controller;
Adafruit_LSM6DS3TRC imu;
Madgwick filter;

//your controller number (1 - 242): every player has a different one
int controllerNumber = 11;



//IMU variables
sensors_event_t accel;
sensors_event_t gyro;
sensors_event_t temp;
// Keep decimal precision while smoothing the tilt.
float tiltAngle = 0;

//the angles for the top and bottom of the screen: change these to yours
int topAngle = -40;
int bottomAngle = 40;
// Allow a small amount of lean around upright without moving the paddle.
float neutralRange = 5.0;
// Assign the left and right LED pins.
int leftLedPin = 27;
int rightLedPin = 33;
//the filter gets a new reading every 10 ms, the Serial Monitor every 100 ms
unsigned long lastUpdate = 0;
unsigned long lastPrint = 0;

//New Function get tilt directikon
 int getTiltDirection(float sidewaysAngle, float neutralRange)
{
  // Leaning left beyond the neutral range means move up.
  if (sidewaysAngle < -neutralRange)
  {
    return UP;
  }

  // Leaning right beyond the neutral range means move down.
  if (sidewaysAngle > neutralRange)
  {
    return DOWN;
  }

  // Within the neutral range, stop moving.
  return NEUTRAL;
}
float smoothTilt(float newAngle)
{
  // Remember the smoothed angle between calls.
  static float smoothedAngle = 0;
  static bool firstReading = true;

  // Choose how strongly each new reading affects the result.
  const float smoothingAmount = 0.2;

  // Start from the actual angle to avoid drifting from zero at startup.
  if (firstReading)
  {
    smoothedAngle = newAngle;
    firstReading = false;
  }

  // Move partway toward the latest angle to reduce small fluctuations.
  smoothedAngle = smoothedAngle
                + smoothingAmount * (newAngle - smoothedAngle);

  return smoothedAngle;
}
void updateDirectionLeds(float sidewaysAngle)
{
  // Light the left LED when leaning left.
  if (sidewaysAngle < -neutralRange)
  {
    digitalWrite(leftLedPin, HIGH);
    digitalWrite(rightLedPin, LOW);
  }

  // Light the right LED when leaning right.
  else if (sidewaysAngle > neutralRange)
  {
    digitalWrite(leftLedPin, LOW);
    digitalWrite(rightLedPin, HIGH);
  }

  // Turn both LEDs off when upright.
  else
  {
    digitalWrite(leftLedPin, LOW);
    digitalWrite(rightLedPin, LOW);
  }
}
void setup()
{
  Serial.begin(9600);
  //start LEDs and configure them as output
  pinMode(leftLedPin, OUTPUT);
pinMode(rightLedPin, OUTPUT);
digitalWrite(leftLedPin, LOW);
digitalWrite(rightLedPin, LOW);

  //start the IMU on the I2C bus (address 0x6A) and the filter (100 readings a second)
  if (!imu.begin_I2C())
  {
    Serial.println("No IMU found: check the STEMMA QT cable");
  }
  filter.begin(100);

  controller.setControllerNumber(controllerNumber);
  controller.setStatusLED(LED_BUILTIN);

  //Customize your controller: remove the // at the start of a line to use it

  //your controller's name in the game (up to 20 characters)
  controller.setControllerName("TheTilter");

  //paddle color, as a web color code
  controller.setPlayerColor("#ff001a");

  //paddle speed: 1 - 100 (normal is 10)
  // controller.setMoveSpeed(14);

  //paddle size: 50 - 200 percent (normal is 100)
   controller.setPaddleSize(200);

  //sounds: a built-in name, or an https:// link straight to your own mp3 or wav file
  //built-in sounds: pingpong, claves, tick, bell, gong, bikebell, flourish, applause, pip, quindar,
  //clock, blip, boop, pluck, click, laser, drum, chirp, zap, thud, wood, glass, coin
  // controller.setHitSound("bikebell");    //when your paddle hits the puck
  // controller.setScoreSound("coin");      //when you score a point
  // controller.setWinSound("applause");    //when you win the match

  //background music: a built-in name, or an https:// link straight to your own mp3 file
  //built-in music: drift, tabletop, arcade, nightbus, gymnopedie, smallsteps, bounce, currents, palmleaf, spacemtv
  //(the Music tile in the game's lobby has to be set to "Players' choice" to hear it)
  // controller.setMusic("https://raw.githubusercontent.com/DigitalFuturesOCADU/df-pong/main/game/cc26/assets/music/palmleaf.mp3");

  controller.begin();
}

void loop()
{
  //keeps the connection to the game going: needed every loop
  controller.update();
 

// Every 10 ms, read the sensor and update the filter.
if (millis() - lastUpdate >= 10)
{
  lastUpdate = millis();
  imu.getEvent(&accel, &gyro, &temp);
  filter.updateIMU(degrees(gyro.gyro.x), degrees(gyro.gyro.y), degrees(gyro.gyro.z),
                   accel.acceleration.x, accel.acceleration.y, accel.acceleration.z);

  // Smooth the angle once per sensor update.
  tiltAngle = smoothTilt(-filter.getPitch());
  // Light the LED for the side you are leaning toward.
updateDirectionLeds(tiltAngle);
}
  //every 100 ms, print the angle, to find your top and bottom angles
  if (millis() - lastPrint >= 100)
  {
    lastPrint = millis();
    Serial.print("Tilt: ");
    Serial.println(tiltAngle);
  }
// Keep the tilt angle within the chosen movement range.
tiltAngle = constrain(tiltAngle, topAngle, bottomAngle);

// Map left tilt to the top and right tilt to the bottom.
controller.sendPosition(tiltAngle, topAngle, bottomAngle);
}