#ifndef MQTT_CONSTANTS_H
#define MQTT_CONSTANTS_H

#include <chrono>
#include <string>

const std::string SERVER_ADDRESS("mqtt://broker.hivemq.com:1883");
const std::string CLIENT_ID("IMU visualizer");
const std::string TOPIC("2/#");

const int QOS = 1;
const int N_RETRY_ATTEMPTS = 5;
const auto TIMEOUT = std::chrono::seconds(10);


typedef struct {
	float pos_x;
	float pos_y;
	float pos_z;

	float pitch;
	float roll;

	std::string temp;
} mqtt_data;

#endif