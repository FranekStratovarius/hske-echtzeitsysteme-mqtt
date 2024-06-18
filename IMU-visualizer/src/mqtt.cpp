#include "mqtt.h"
#include "mqtt/async_client.h"
#include "mqtt_callback.h"
#include "mqtt_constants.h"

void test(mqtt::async_client& cli, mqtt::connect_options& connOpts, Callback& cb) {
	cli.set_callback(cb);

	// Start the connection.
	// When completed, the callback will subscribe to topic.
	std::cout << "Connecting to the MQTT server..." << std::flush;
	mqtt::token_ptr conntok = cli.connect(connOpts, nullptr, cb);

	std::cout << "Waiting for the connection..." << std::endl;
	conntok->wait();
	std::cout << "  ...OK" << std::endl;
}

MQTT::MQTT(mqtt::async_client& cli, mqtt::connect_options& connOpts, Callback& cb) {
	this->client = &cli;
	cli.set_callback(cb);

	// Start the connection.
	// When completed, the callback will subscribe to topic.
	std::cout << "Connecting to the MQTT server..." << std::flush;
	mqtt::token_ptr conntok = cli.connect(connOpts, nullptr, cb);

	std::cout << "Waiting for the connection..." << std::endl;
	conntok->wait();
	std::cout << "  ...OK" << std::endl;
}

MQTT::~MQTT() {
	std::cout << "\nDisconnecting from the MQTT server..." << std::flush;
	client->disconnect()->wait();
	std::cout << "OK" << std::endl;
}

void MQTT::send_message(std::string topic, std::string payload) {
	mqtt::message_ptr pubmsg = mqtt::make_message(topic, payload);
	pubmsg->set_qos(QOS);
	client->publish(pubmsg);
}