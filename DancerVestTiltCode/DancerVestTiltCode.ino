/*
C&C — Luda and Hiro
The Dancer — DF Pong vest controller

Controls:
- Lean left to move the paddle toward the top.
- Lean right to move the paddle toward the bottom.
- Straighten to return the paddle near the centre.

Hardware:
- Adafruit ESP32 Feather V2.
- LSM6DS3TR-C IMU connected through STEMMA QT.
- Three LEDs per shoulder.
- Left shoulder LEDs on pin 27; right shoulder LEDs on pin 33.
- Each LED has its own resistor; all share GND.

Feedback:
- The shoulder on the leaning side lights brightly.
- Both shoulders stay dim near upright.
- Paddle hit: both shoulders flash once.
- Scored: both shoulders flash twice.
- Conceded: both shoulders light for 400 milliseconds.
- Won: shoulders alternate for two seconds.
- The onboard status LED indicates connection status.

Paddle size:
- Start at 100%.
- Conceding enlarges the paddle to 200%.
- Scoring restores 100%.
- Winning, losing, or starting a new match restores 100%.
- Repeated concessions keep it at 200%; boosts do not stack.

Audio:
- Custom background music and paddle-hit sound hosted on GitHub.
- Set the game lobby's Music to "Players' choice".

Libraries:
- DFPongController 2.5.1.
- NimBLE-Arduino.
- Adafruit LSM6DS and its dependencies.
- MadgwickAHRS.

Serial Monitor: 9600 baud.

AI assistance:
- smoothTilt(), updateDirectionLeds(), startLedEffect(),
  updateLedFeedback(), and handleGameEvent() were developed
  with ChatGPT assistance.
 formatting, and comments assisted by ChatGPT.
*/

#include <DFPongController.h>
#include <Adafruit_LSM6DS3TRC.h>
#include <MadgwickAHRS.h>

// Create the controller, sensor, and orientation filter.
DFPongController controller;
Adafruit_LSM6DS3TRC imu;
Madgwick filter;

// Controller identity.
int controllerNumber = 11;

// Shoulder LED pins.
int leftLedPin = 27;
int rightLedPin = 33;

// Angles corresponding to the top and bottom of the screen.
int topAngle = -40;
int bottomAngle = 40;

// Both shoulders stay dim within 5 degrees of upright.
// This is only an LED threshold, not a paddle movement dead zone.
float neutralRange = 10.0;

// Normal and boosted paddle sizes, in percent.
int normalPaddleSize = 100;
int boostedPaddleSize = 200;

// Sensor readings and smoothed tilt angle.
sensors_event_t accel;
sensors_event_t gyro;
sensors_event_t temp;
float tiltAngle = 0;

// Sensor and Serial Monitor timing.
unsigned long lastUpdate = 0;
unsigned long lastPrint = 0;

// Temporary LED effect:
// 0 = none, 1 = hit, 2 = scored, 3 = conceded, 4 = won.
int activeLedEffect = 0;
unsigned long ledEffectStarted = 0;


// Smooth incoming angles to reduce small fluctuations.
float smoothTilt(float newAngle)
{
  static float smoothedAngle = 0;
  static bool firstReading = true;

  // Lower values smooth more; higher values respond faster.
  const float smoothingAmount = 0.2;

  // Begin at the first measured angle.
  if (firstReading)
  {
    smoothedAngle = newAngle;
    firstReading = false;
  }

  smoothedAngle = smoothedAngle
                + smoothingAmount * (newAngle - smoothedAngle);

  return smoothedAngle;
}


// Start a temporary LED effect.
// A new effect replaces the previous one.
void startLedEffect(int effectNumber)
{
  activeLedEffect = effectNumber;
  ledEffectStarted = millis();
}


// Show tilt direction, with dim steady light when upright.
void updateDirectionLeds(float sidewaysAngle)
{
  const int brightLevel = 255;
  const int dimLevel = 38;

  if (sidewaysAngle < -neutralRange)
  {
    analogWrite(leftLedPin, brightLevel);
    analogWrite(rightLedPin, 0);
  }
  else if (sidewaysAngle > neutralRange)
  {
    analogWrite(leftLedPin, 0);
    analogWrite(rightLedPin, brightLevel);
  }
  else
  {
    analogWrite(leftLedPin, dimLevel);
    analogWrite(rightLedPin, dimLevel);
  }
}


// Play an event effect without blocking the controller connection.
void updateLedFeedback(float sidewaysAngle)
{
  unsigned long elapsed = millis() - ledEffectStarted;

  int leftBrightness = 0;
  int rightBrightness = 0;
  const int flashBrightness = 255;

   // Paddle hit: short flash, dark gap, longer flash.
  if (activeLedEffect == 1)
  {
    if (elapsed >= 350)
    {
      activeLedEffect = 0;
    }
    else if (elapsed < 50 ||
             (elapsed >= 150 && elapsed < 350))
    {
      leftBrightness = flashBrightness;
      rightBrightness = flashBrightness;
    }
  }

  // Scored: two flashes, each 150 milliseconds long.
  else if (activeLedEffect == 2)
  {
    if (elapsed >= 600)
    {
      activeLedEffect = 0;
    }
    else if (elapsed < 150 ||
             (elapsed >= 300 && elapsed < 450))
    {
      leftBrightness = flashBrightness;
      rightBrightness = flashBrightness;
    }
  }

  // Conceded: both shoulders on for 400 milliseconds.
  else if (activeLedEffect == 3)
  {
    if (elapsed < 400)
    {
      leftBrightness = flashBrightness;
      rightBrightness = flashBrightness;
    }
    else
    {
      activeLedEffect = 0;
    }
  }

  // Won: alternate shoulders every 150 ms for two seconds.
  else if (activeLedEffect == 4)
  {
    if (elapsed >= 2000)
    {
      activeLedEffect = 0;
    }
    else if (elapsed % 300 < 150)
    {
      leftBrightness = flashBrightness;
    }
    else
    {
      rightBrightness = flashBrightness;
    }
  }

  // Restore tilt indication when no event effect is active.
  if (activeLedEffect == 0)
  {
    updateDirectionLeds(sidewaysAngle);
    return;
  }

  analogWrite(leftLedPin, leftBrightness);
  analogWrite(rightLedPin, rightBrightness);
}


// Respond to incoming game events.
// The library calls this function; value is unused here.
void handleGameEvent(int event, int value)
{
  if (event == EVENT_HIT)
  {
    startLedEffect(1);
  }
  else if (event == EVENT_SCORED)
  {
    startLedEffect(2);
    controller.setPaddleSize(normalPaddleSize);
  }
  else if (event == EVENT_CONCEDED)
  {
    startLedEffect(3);
    controller.setPaddleSize(boostedPaddleSize);
  }
  else if (event == EVENT_WON)
  {
    startLedEffect(4);
    controller.setPaddleSize(normalPaddleSize);
  }
  else if (event == EVENT_LOST)
  {
    controller.setPaddleSize(normalPaddleSize);
  }
  else if (event == EVENT_MATCH_START)
  {
    activeLedEffect = 0;
    controller.setPaddleSize(normalPaddleSize);
  }
}
// Change paddle colour only when the tilt direction changes.
void updatePaddleColor(float sidewaysAngle)
{
  static int previousSide = 99;
  int currentSide = 0;

  if (sidewaysAngle < -neutralRange)
  {
    currentSide = -1;
  }
  else if (sidewaysAngle > neutralRange)
  {
    currentSide = 1;
  }

  // Avoid sending the same colour repeatedly.
  if (currentSide == previousSide)
  {
    return;
  }

  if (currentSide == -1)
  {
    controller.setPlayerColor("#0088ff"); // Left: blue.
  }
  else if (currentSide == 1)
  {
    controller.setPlayerColor("#ff001a"); // Right: red.
  }
  else
  {
    controller.setPlayerColor("#ffffff"); // Upright: white.
  }

  previousSide = currentSide;
}


// Initialize hardware and controller once at startup.
void setup()
{
  Serial.begin(9600);

  pinMode(leftLedPin, OUTPUT);
  pinMode(rightLedPin, OUTPUT);
  digitalWrite(leftLedPin, LOW);
  digitalWrite(rightLedPin, LOW);

  // Start the IMU at its default I2C address, 0x6A.
  // Preserves the earlier sketch's report-only failure handling.
  // Detect and initialize the IMU.
    // Explicitly enable power to the Feather V2's STEMMA QT port.
  pinMode(NEOPIXEL_I2C_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_I2C_POWER, HIGH);
  delay(100);

  if (imu.begin_I2C(0x6A))
  {
    Serial.println("IMU found at 0x6A");
  }
  else if (imu.begin_I2C(0x6B))
  {
    Serial.println("IMU found at 0x6B");
  }
  else
  {
    Serial.println("IMU not found at 0x6A or 0x6B");

    // Stop instead of processing readings from an uninitialized IMU.
    while (true)
    {
      delay(100);
    }
  }

  // The filter expects 100 sensor updates per second.
  filter.begin(100);

  controller.setControllerNumber(controllerNumber);
  controller.setStatusLED(LED_BUILTIN);
  controller.setControllerName("The Dancer");
  controller.setPlayerColor("#ff001a");
  controller.setPaddleSize(normalPaddleSize);

  // Custom background music and paddle-hit sound.
  controller.setMusic(
    "https://raw.githubusercontent.com/hiro9884/CC_Luda_and_Hiro/main/mariomusic.mp3"
  );

  controller.setHitSound(
    "https://raw.githubusercontent.com/hiro9884/CC_Luda_and_Hiro/main/sound.mp3"
  );

  // Built-in score and win sounds discussed yesterday.
  controller.setScoreSound("bell");
  controller.setWinSound("applause");

  // Connect game events to LED effects and paddle-size changes.
  controller.onGameEvent(handleGameEvent);

  controller.begin();
}


// Keep the connection, tilt readings, and feedback updated.
void loop()
{
  controller.update();

  // Read and process the IMU approximately every 10 milliseconds.
  if (millis() - lastUpdate >= 10)
  {
    lastUpdate = millis();

    imu.getEvent(&accel, &gyro, &temp);

    // Convert gyroscope readings to degrees per second.
    filter.updateIMU(
      degrees(gyro.gyro.x),
      degrees(gyro.gyro.y),
      degrees(gyro.gyro.z),
      accel.acceleration.x,
      accel.acceleration.y,
      accel.acceleration.z
    );

    // Tested orientation: left negative, right positive.
    tiltAngle = smoothTilt(-filter.getPitch());
  }

  // Update effects on every loop, outside the sensor timing block.
  updateLedFeedback(tiltAngle);
  updatePaddleColor(tiltAngle);

  // Print the angle every 100 milliseconds.
  if (millis() - lastPrint >= 100)
  {
    lastPrint = millis();
    Serial.print("Tilt: ");
    Serial.println(tiltAngle);
  }

  tiltAngle = constrain(tiltAngle, topAngle, bottomAngle);
  controller.sendPosition(tiltAngle, topAngle, bottomAngle);
}