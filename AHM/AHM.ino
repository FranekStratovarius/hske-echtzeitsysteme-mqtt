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
char imuGyro[45];
char imuAccel[45];

//const char ssid[] = "Handy von Louis";
//const char pass[] = "einsbisacht";
const char ssid[] = "Ellharter WGs";
const char pass[] = "$M6_aPQs";

WiFiClient net;
PubSubClient client(net);
IPAddress server(192, 168, 178, 41);

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

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("esp32")) {
      Serial.println("connected");
      client.publish("/hello","bin da.");
      client.subscribe("/hello");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }

  client.publish("/hello", "connected with broker!");
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
  config.frame_size = FRAMESIZE_UXGA;
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
  // drop down frame size for higher initial frame rate
  if(config.pixel_format == PIXFORMAT_JPEG){
    s->set_framesize(s, FRAMESIZE_QVGA);
  }
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

void imu(sensors_event_t a, sensors_event_t g, sensors_event_t temp) {
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
  while (!client.connect("arduino", "public", "public")) {
    Serial.print(".");
    delay(1000);
  }

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

  setup_camera();

  delay(100);
}

void loop() {
  //delay(10);  // <- fixes some issues with WiFi stability
  if (!client.connected()) {
    reconnect();
  }

  camera_fb_t *pic = esp_camera_fb_get();

  if (pic) {
    // use pic->buf to access the image
    //Serial.printf("Picture taken! Its size was: %zu bytes\n", pic->len);
    bool success = client.publish("/image", pic->buf, pic->len, false);
    //Serial.printf("picture send success: %i\n", success);
    // free memory
    esp_camera_fb_return(pic);

    vTaskDelay(1000 / portTICK_RATE_MS);
  }

   /* Get new sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  imu(a, g, temp);
  client.loop();
}
