// This example uses an ESP32 Development Board
// to connect to shiftr.io.
//
// You can check on your device after a successful
// connection here: https://www.shiftr.io/try.
//
// by Joël Gähwiler
// https://github.com/256dpi/arduino-mqtt

#include <WiFi.h>
#include <MQTT.h>

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

#define I2C_SDA 14
#define I2C_SCL 15

#define calibrationSamples 30

TwoWire I2CSensors = TwoWire(0);
Adafruit_MPU6050 mpu;

char imuTemp[5];
char imuGyro[45];
char imuAccel[45];

const char ssid[] = "Handy von Louis";
const char pass[] = "einsbisacht";
// const char ssid[] = "Ellharter WGs";
// const char pass[] = "$M6_aPQs";

WiFiClient net;
MQTTClient client;

unsigned long lastMillis = 0;

long loopTimer, loopTimer2;
float accelPitch, accelRoll;
float freq, dt;
float tau = 0.98;

float roll = 0;
float pitch = 0;

int gyro_x, gyro_y, gyro_z;
float gyro_x_cal, gyro_y_cal, gyro_z_cal;
float accel_x, accel_y, accel_z;

void connect() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.print("\nconnecting to broker...");
  while (!client.connect("arduino", "public", "public")) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nconnected to broker!");

  client.subscribe("/hello");
  // client.unsubscribe("/hello");
}

void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

  // Note: Do not use the client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `client.loop()`.
}

void calibrate() {
  client.publish("/hello", "calibrating IMU...");
  client.publish("/hello", "lay board on a bench...");

  delay(1000);

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  client.publish("/hello", "entering loop");
  for(int i=0; i<calibrationSamples; i++) {
    gyro_x_cal += g.gyro.x;
    gyro_y_cal += g.gyro.y;
    gyro_z_cal += g.gyro.z;
    delay(100);
  }

  client.publish("/hello", "calculating offset");
  gyro_x_cal = gyro_x_cal/calibrationSamples;
  gyro_y_cal = gyro_y_cal/calibrationSamples;
  gyro_z_cal = gyro_z_cal/calibrationSamples;
}

float imu(sensors_event_t a, sensors_event_t g, sensors_event temp) {
  // MPU-6050-9250-I2C-CompFilter by MarkSherstan
  sprintf(imuTemp, "%f", temp.temperature);
  client.publish("/temperature", imuTemp);

  freq = 1/((micros() - loopTimer2) * 1e-6);
  loopTimer2 = micros();
  dt = 1/freq;

  // Send acceleration data
  accel_x = a.acceleration.x;
  accel_y = a.acceleration.y;
  accel_z = a.acceleration.z;
  sprintf(imuAccel, "X: %f Y: %f Z: %f", accel_x, accel_y, accel_z);
  client.publish("/acceleration", imuAccel);

  // Subtract the offset calibration value
  gyro_x = g.gyro.x - gyro_x_cal;
  gyro_y = g.gyro.y - gyro_y_cal;
  gyro_z = g.gyro.z - gyro_z_cal;

  // Calculate Pitch and Roll from Accelerometer
  accelPitch = atan2(accel_y, accel_z) * RAD_TO_DEG;
  accelRoll = atan2(accel_x, accel_z) * RAD_TO_DEG;

  // Complementary filter
  pitch = (tau)*(pitch + gyro_x*dt) + (1-tau)*(accelPitch);
  roll = (tau)*(roll - gyro_y*dt) + (1-tau)*(accelRoll);

  sprintf(imuGyro, "Pitch %f Roll %f", pitch, roll);
  client.publish("/rotation", imuGyro);
  client.publish("/rotation", " ");

  // Wait until the loopTimer reaches 4000us (250Hz) before next loop
  while (micros() - loopTimer <= 4000);
  loopTimer = micros();

  return diff;
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, pass);
  I2CSensors.begin(I2C_SDA, I2C_SCL, 100000);
  while (!Serial) delay(10); // will pause Zero, Leonardo, etc until serial console opens

  // Note: Local domain names (e.g. "Computer.local" on OSX) are not supported
  // by Arduino. You need to set the IP address directly.

  // client.begin("172.20.10.5", net);
  client.begin("192.168.93.109", net);
  client.onMessage(messageReceived);

  connect();

  client.publish("/hello", "initializing IMU...");

  // Try to initialize!
  if (!mpu.begin(0x68, &I2CSensors)){
  client.publish("/hello", "failed to find IMU!");
    while (1) {
      delay(10);
    }
  }
  client.publish("/hello", "IMU found!");

  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);

  // Digital Low-Pass Filter
  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);

  calibrate();

  delay(100);
}

void loop() {
  client.loop();
  delay(10);  // <- fixes some issues with WiFi stability

   /* Get new sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  if (!client.connected()) {
    connect();
  }
  imu(a, g, temp);
}
