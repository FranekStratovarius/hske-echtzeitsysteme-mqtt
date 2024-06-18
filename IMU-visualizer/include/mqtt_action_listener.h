#ifndef MQTT_ACTION_LISTENER_H
#define MQTT_ACTION_LISTENER_H

#include "mqtt/iaction_listener.h"
#include "mqtt/token.h"

/**
 * Callbacks for the success or failures of requested actions.
 * This could be used to initiate further action, but here we just log the
 * results to the console.
 */
class Action_listener : public virtual mqtt::iaction_listener {
	private:
		std::string name_;
		void on_failure(const mqtt::token& tok) override;
		void on_success(const mqtt::token& tok) override;
	public:
		Action_listener(const std::string& name) : name_(name) {}
};

#endif