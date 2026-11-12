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
- Left LED on pin 27; right LED on pin 33.
- Each LED has its own resistor; both share GND.

Feedback:
- The LED on the leaning side lights up.
- Both direction LEDs turn off near upright.
- The onboard status LED indicates connection status.
- Game-event flashes are not implemented yet.

Serial Monitor: 9600 baud.

AI assistance:
- smoothTilt() and updateDirectionLeds() were developed
  with ChatGPT assistance.
- Formatting and comments were cleaned up with ChatGPT.
*/


// Include the controller, sensor, and orientation-filter libraries.
#include <DFPongController.h>
#include <Adafruit_LSM6DS3TRC.h>
#include <MadgwickAHRS.h>

// Create the objects used to communicate with the game and sensor.
DFPongController controller;
Adafruit_LSM6DS3TRC imu;
Madgwick filter;

// Identify this controller in the game's player list.
int controllerNumber = 11;

// Assign the two direction LED pins.
int leftLedPin = 27;
int rightLedPin = 33;

// Set the angles that place the paddle at the screen's endpoints.
int topAngle = -40;
int bottomAngle = 40;

// Keep both direction LEDs off within 5 degrees of upright.
// This does not create a neutral zone for paddle movement.
float neutralRange = 5.0;

// Store the sensor readings and the smoothed tilt angle.
sensors_event_t accel;
sensors_event_t gyro;
sensors_event_t temp;
float tiltAngle = 0;

// Remember when the sensor and Serial Monitor were last updated.
unsigned long lastUpdate = 0;
unsigned long lastPrint = 0;


// Smooth incoming angles to reduce small fluctuations.
float smoothTilt(float newAngle)
{
  // Preserve the previous result between function calls.
  static float smoothedAngle = 0;
  static bool firstReading = true;

  // Lower values smooth more; higher values respond faster.
  const float smoothingAmount = 0.2;

  // Begin at the first measured angle rather than easing from zero.
  if (firstReading)
  {
    smoothedAngle = newAngle;
    firstReading = false;
  }

  // Move partway from the previous result toward the new reading.
  smoothedAngle = smoothedAngle
                + smoothingAmount * (newAngle - smoothedAngle);

  // Return the updated angle in degrees.
  return smoothedAngle;
}


// Show the leaning direction using the two external LEDs.
void updateDirectionLeds(float sidewaysAngle)
{
  // Negative angles beyond the neutral range indicate a left lean.
  if (sidewaysAngle < -neutralRange)
  {
    digitalWrite(leftLedPin, HIGH);
    digitalWrite(rightLedPin, LOW);
  }

  // Positive angles beyond the neutral range indicate a right lean.
  else if (sidewaysAngle > neutralRange)
  {
    digitalWrite(leftLedPin, LOW);
    digitalWrite(rightLedPin, HIGH);
  }

  // Near upright, turn both direction LEDs off.
  else
  {
    digitalWrite(leftLedPin, LOW);
    digitalWrite(rightLedPin, LOW);
  }
}


// Initialize the hardware and controller once at startup.
void setup()
{
  // Start Serial Monitor communication.
  Serial.begin(9600);

  // Configure both direction LEDs and start them off.
  pinMode(leftLedPin, OUTPUT);
  pinMode(rightLedPin, OUTPUT);
  digitalWrite(leftLedPin, LOW);
  digitalWrite(rightLedPin, LOW);

  // Start the IMU at its default I2C address, 0x6A.
  // This reports a failure but does not stop the sketch.
  if (!imu.begin_I2C())
  {
    Serial.println("No IMU found: check the STEMMA QT cable");
  }

  // Tell the filter to expect 100 updates per second.
  filter.begin(100);

  // Set the controller identity and onboard connection-status LED.
  controller.setControllerNumber(controllerNumber);
  controller.setStatusLED(LED_BUILTIN);

  // Preserve the name, colour, and paddle size from the tested sketch.
  controller.setControllerName("TheTilter");
  controller.setPlayerColor("#ff001a");
  controller.setPaddleSize(200);

  // Start Bluetooth controller operation.
  controller.begin();
}


// Keep the connection, tilt readings, and paddle position updated.
void loop()
{
  // Service the controller connection on every pass through loop().
  controller.update();

  // Read and process the IMU approximately every 10 milliseconds.
  if (millis() - lastUpdate >= 10)
  {
    lastUpdate = millis();

    // Read acceleration, angular velocity, and temperature.
    imu.getEvent(&accel, &gyro, &temp);

    // Convert gyroscope readings to degrees per second.
    // Feed gyroscope and acceleration readings into the filter.
    filter.updateIMU(
      degrees(gyro.gyro.x),
      degrees(gyro.gyro.y),
      degrees(gyro.gyro.z),
      accel.acceleration.x,
      accel.acceleration.y,
      accel.acceleration.z
    );

    // Use the tested pitch direction: left negative, right positive.
    // Smooth it once per sensor update.
    tiltAngle = smoothTilt(-filter.getPitch());

    // Show the leaning direction using the smoothed angle.
    updateDirectionLeds(tiltAngle);
  }

  // Print the current angle every 100 milliseconds for testing.
  if (millis() - lastPrint >= 100)
  {
    lastPrint = millis();
    Serial.print("Tilt: ");
    Serial.println(tiltAngle);
  }

  // Limit the angle to the chosen paddle movement range.
  tiltAngle = constrain(tiltAngle, topAngle, bottomAngle);

  // Map the angle to paddle position rather than movement direction.
  controller.sendPosition(tiltAngle, topAngle, bottomAngle);
}