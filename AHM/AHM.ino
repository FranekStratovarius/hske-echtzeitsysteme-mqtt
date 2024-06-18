// This example uses an ESP32 Development Board
// to connect to shiftr.io.
//
// You can check on your device after a successful
// connection here: https://www.shiftr.io/try.
//
// by Joël Gähwiler
// https://github.com/256dpi/arduino-mqtt

#include <Adafruit_MPU6050.h>
#include <Wire.h>

#include <WiFi.h>
#include <PubSubClient.h>
#include "esp_camera.h"

// select camera model
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

#define I2C_SDA 14
#define I2C_SCL 15

#define calibrationSamples 30

TwoWire I2CSensors = TwoWire(0);
Adafruit_MPU6050 mpu;

char imuTemp[5];
char imuGyro[20];
char imuAccel[20];

const char ssid[] = "iPhone von Louis";
const char pass[] = "einsbisacht";
//const char ssid[] = "Ellharter WGs";
//const char pass[] = "$M6_aPQs";

WiFiClient net;
PubSubClient client(net);
//IPAddress server(192, 168, 178, 41);
const char* server = "broker.hivemq.com";

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

bool imu_active = 0;

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] >");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println("<");
  Serial.printf("topic comparison: %i\n", strcmp(topic, "2/set_imu"));
  if(strcmp(topic, "2/take_picture") == 0) {
    Serial.println("taking picture...");
    camera_fb_t *pic = esp_camera_fb_get();
    if (pic) {
      // use pic->buf to access the image
      bool success = client.publish("2/cam", pic->buf, pic->len, false);
      // free memory
      esp_camera_fb_return(pic);
    }
  }
  if(strcmp(topic, "2/set_imu") == 0) {
    imu_active = payload[0] & 0b1;
    Serial.printf("| imu_active: %d\n", imu_active);
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("esp32")) {
      Serial.println("connected");
      client.publish("2/status","bin da.");
      client.subscribe("2/take_picture");
      client.subscribe("2/set_imu");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }

  client.publish("2/status", "connected with broker!");
}

void setup_camera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  //config.frame_size = FRAMESIZE_UXGA;
  config.frame_size = FRAMESIZE_QVGA;
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  if(config.pixel_format == PIXFORMAT_JPEG){
    if(psramFound()){
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }
  /*
  // drop down frame size for higher initial frame rate
  if(config.pixel_format == PIXFORMAT_JPEG){
    s->set_framesize(s, FRAMESIZE_QVGA);
  }
  */
}

void calibrate() {
  client.publish("2/status", "calibrating IMU...");
  client.publish("2/status", "lay board on a bench...");

  delay(1000);

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  client.publish("2/status", "entering loop");
  for(int i=0; i<calibrationSamples; i++) {
    gyro_x_cal += g.gyro.x;
    gyro_y_cal += g.gyro.y;
    gyro_z_cal += g.gyro.z;
    delay(100);
  }

  client.publish("2/status", "calculating offset");
  gyro_x_cal = gyro_x_cal/calibrationSamples;
  gyro_y_cal = gyro_y_cal/calibrationSamples;
  gyro_z_cal = gyro_z_cal/calibrationSamples;
}

void imu(sensors_event_t a, sensors_event_t g, sensors_event_t temp) {
  // MPU-6050-9250-I2C-CompFilter by MarkSherstan
  sprintf(imuTemp, "%f", temp.temperature);
  client.publish("2/imu/temp", imuTemp);

  freq = 1/((micros() - loopTimer2) * 1e-6);
  loopTimer2 = micros();
  dt = 1/freq;

  // Send out acceleration data
  accel_x = a.acceleration.x;
  accel_y = a.acceleration.y;
  accel_z = a.acceleration.z;
  sprintf(imuAccel, "%f", accel_x);
  client.publish("2/imu/acc/x", imuAccel);
  sprintf(imuAccel, "%f", accel_y);
  client.publish("2/imu/acc/y", imuAccel);
  sprintf(imuAccel, "%f", accel_z);
  client.publish("2/imu/acc/z", imuAccel);

  // Subtract the offset calibration value
  gyro_x = g.gyro.x - gyro_x_cal;
  gyro_y = g.gyro.y - gyro_y_cal;
  gyro_z = g.gyro.z - gyro_z_cal;

  // Send out gyro data
  sprintf(imuGyro, "%f", gyro_x);
  client.publish("2/imu/gyro/x", imuGyro);
  sprintf(imuGyro, "%f", gyro_x);
  client.publish("2/imu/gyro/y", imuGyro);
  sprintf(imuGyro, "%f", gyro_x);
  client.publish("2/imu/gyro/z", imuGyro);

  // Calculate Pitch and Roll from Accelerometer
  accelPitch = atan2(accel_y, accel_z) * RAD_TO_DEG;
  accelRoll = atan2(accel_x, accel_z) * RAD_TO_DEG;

  // Complementary filter
  pitch = (tau)*(pitch + gyro_x*dt) + (1-tau)*(accelPitch);
  roll = (tau)*(roll - gyro_y*dt) + (1-tau)*(accelRoll);

  sprintf(imuGyro, "%f", pitch);
  client.publish("2/imu/pitch", imuGyro);
  sprintf(imuGyro, "%f", roll);
  client.publish("2/imu/roll", imuGyro);

  // Wait until the loopTimer reaches 4000us (250Hz) before next loop
  while (micros() - loopTimer <= 4000);
  loopTimer = micros();
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  I2CSensors.begin(I2C_SDA, I2C_SCL, 100000);
  while (!Serial) delay(10); // will pause Zero, Leonardo, etc until serial console opens

  // Note: Local domain names (e.g. "Computer.local" on OSX) are not supported
  // by Arduino. You need to set the IP address directly.

  client.setServer(server, 1883);
  client.setCallback(callback);
  client.setBufferSize(10000);
  
  Serial.print("\nconnecting to broker...");
  if (!client.connected()) {
    reconnect();
  }
  Serial.print("\nconneced to broker.\n");

  boolean sent = client.publish("2/status", "ESP ist da.");
  Serial.printf("sent: %d\n", sent);

  client.publish("status", "initializing IMU...");

  // Try to initialize!
  if (!mpu.begin(0x68, &I2CSensors)){
  client.publish("2/status", "failed to find IMU!");
    while (1) {
      delay(10);
    }
  }
  client.publish("2/status", "IMU found!");

  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);

  // Digital Low-Pass Filter
  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);

  calibrate();

  setup_camera();

  client.publish("2/status", "ESP setup finished");

  delay(100);
}

void loop() {
  //delay(10);  // <- fixes some issues with WiFi stability
  if (!client.connected()) {
    reconnect();
  }

  if(imu_active) {
    /* Get new sensor events with the readings */
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    imu(a, g, temp);
    delay(50);
  }
  client.loop();
}
