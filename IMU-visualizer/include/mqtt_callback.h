#ifndef MQTT_CALLBACK_H
#define MQTT_CALLBACK_H

#include "mqtt/async_client.h"
#include "mqtt_action_listener.h"
#include "mqtt_constants.h"

/**
 * Local callback & listener class for use with the client connection.
 * This is primarily intended to receive messages, but it will also monitor
 * the connection to the broker. If the connection is lost, it will attempt
 * to restore the connection and re-subscribe to the topic.
 */
class Callback :
	public virtual mqtt::callback,
	public virtual mqtt::iaction_listener
{
	public:
		Callback(mqtt::async_client& cli, mqtt::connect_options& connOpts, mqtt_data* md)
			: nretry_(0), cli_(cli), connOpts_(connOpts), subListener_("Subscription") {
				this->md = md;
			}
	private:
		// Counter for the number of connection retries
		int nretry_;
		// The MQTT client
		mqtt::async_client& cli_;
		// Options to use if we need to reconnect
		mqtt::connect_options& connOpts_;
		// An action listener to display the result of actions.
		Action_listener subListener_;
		mqtt_data* md;
		/**
		 * This deomonstrates manually reconnecting to the broker by calling
		 * connect() again. This is a possibility for an application that keeps
		 * a copy of it's original connect_options, or if the app wants to
		 * reconnect with different options.
		 * Another way this can be done manually, if using the same options, is
		 * to just call the async_client::reconnect() method.
		 */
		void reconnect();
		// Re-connection failure
		void on_failure(const mqtt::token& tok) override;

		/*
		 * (Re)connection success
		 * Either this or connected() can be used for callbacks.
		  */
		void on_success(const mqtt::token& tok) override {}
		// (Re)connection success
		void connected(const std::string& cause) override;
		/*
		 * Callback for when the connection is lost.
		 * This will initiate the attempt to manually reconnect.
		 */
		void connection_lost(const std::string& cause) override;
		// Callback for when a message arrives.
		void message_arrived(mqtt::const_message_ptr msg) override;
		void delivery_complete(mqtt::delivery_token_ptr token) override {}
};

#endif