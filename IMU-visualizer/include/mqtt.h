#ifndef MQTT_H
#define MQTT_H

#include <string>
#include "mqtt/async_client.h"
#include "mqtt_callback.h"

void test(mqtt::async_client& cli, mqtt::connect_options& connOpts, Callback& cb);

class MQTT {
	public:
		MQTT(mqtt::async_client& cli, mqtt::connect_options& connOpts, Callback& cb);
		~MQTT();
		void send_message(std::string topic, std::string payload);
	private:
		mqtt::async_client* client;
};


#endif