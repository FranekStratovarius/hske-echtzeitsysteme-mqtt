#include <fstream>
#include "mqtt_callback.h"
#include "mqtt_constants.h"

void Callback::reconnect() {
	std::this_thread::sleep_for(std::chrono::milliseconds(2500));
	try {
		cli_.connect(connOpts_, nullptr, *this);
	}
	catch (const mqtt::exception& exc) {
		std::cerr << "Error: " << exc.what() << std::endl;
		exit(1);
	}
}

void Callback::on_failure(const mqtt::token& tok) {
	std::cout << "Connection attempt failed" << std::endl;
	if (++nretry_ > N_RETRY_ATTEMPTS)
		exit(1);
	reconnect();
}

void Callback::connected(const std::string& cause) {
	std::cout << "\nConnection success" << std::endl;
	std::cout << "\nSubscribing to topic '" << TOPIC << "'\n"
		<< "\tfor client " << CLIENT_ID
		<< " using QoS" << QOS << "\n"
		<< "\nPress Q<Enter> to quit\n" << std::endl;

	cli_.subscribe(TOPIC, QOS, nullptr, subListener_);
}

void Callback::connection_lost(const std::string& cause) {
	std::cout << "\nConnection lost" << std::endl;
	if (!cause.empty())
		std::cout << "\tcause: " << cause << std::endl;

	std::cout << "Reconnecting..." << std::endl;
	nretry_ = 0;
	reconnect();
}

void Callback::message_arrived(mqtt::const_message_ptr msg) {
	std::cout << "Message arrived" << std::endl;
	std::cout << "\ttopic: '" << msg->get_topic() << "'" << std::endl;
	std::cout << "\tpayload: '" << msg->to_string() << "'\n" << std::endl;
	if(msg->get_topic() == "2/status") {
		md->status = msg->to_string();
	}
	if(msg->get_topic() == "2/imu/temp") {
		md->temp = msg->to_string();
	}
	if(msg->get_topic() == "2/imu/roll") {
		md->roll = std::stof(msg->to_string());
	}
	if(msg->get_topic() == "2/imu/pitch") {
		md->pitch = std::stof(msg->to_string());
	}
	if(msg->get_topic() == "2/imu/acc/x") {
		md->pos_x = std::stof(msg->to_string());
	}
	if(msg->get_topic() == "2/imu/acc/y") {
		md->pos_y = std::stof(msg->to_string());
	}
	if(msg->get_topic() == "2/cam") {
		std::ofstream file;
		file.open("image.jpeg", std::ios::binary | std::ios::out);
		file.write(msg->get_payload().data(), msg->get_payload().length());
		file.close();
	}
}