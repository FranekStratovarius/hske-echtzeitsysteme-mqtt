package main

import (
	"log"
	"os"

	MQTT "github.com/eclipse/paho.mqtt.golang"
)

func mqtt_publish(client MQTT.Client, topic string, payload []byte) {
	for i := 0; i < 1; i++ {
		token := client.Publish(topic, byte(0), false, payload)
		token.Wait()
	}
}

func mqtt_topic_default(topic string, payload string) {
	log.Printf("[%s] %s", topic, payload)
}

func start_mqtt(directory_name string) {
	opts := MQTT.NewClientOptions()
	opts.AddBroker(MQTT_BROKER)
	opts.ClientID = "ahm_server"

	choke := make(chan bool)

	opts.SetDefaultPublishHandler(func(client MQTT.Client, msg MQTT.Message) {
		switch msg.Topic() {
		case "1/lidar/trigger_15cm":
			mqtt_topic_lidar(client, msg.Payload())
		case "2/cam":
			mqtt_topic_traffic_light_image(client, msg.Payload(), directory_name)
		case "5/cam/image":
			mqtt_topic_driver_image(msg.Payload(), directory_name)
		default:
			mqtt_topic_default(msg.Topic(), string(msg.Payload()))
		}
		choke <- true
	})

	client := MQTT.NewClient(opts)
	if token := client.Connect(); token.Wait() && token.Error() != nil {
		panic(token.Error())
	}
	log.Print("connected to broker")

	// subscribe to all needed topics
	if token := client.Subscribe("1/lidar/trigger_15cm", byte(0), nil); token.Wait() && token.Error() != nil {
		log.Fatal(token.Error())
		os.Exit(1)
	}

	if token := client.Subscribe("2/cam", byte(0), nil); token.Wait() && token.Error() != nil {
		log.Fatal(token.Error())
		os.Exit(1)
	}

	if token := client.Subscribe("5/cam/image", byte(0), nil); token.Wait() && token.Error() != nil {
		log.Fatal(token.Error())
		os.Exit(1)
	}

	// publish
	mqtt_publish(client, "status", []byte("AHM software is online"))

	// subscribe
	for {
		<-choke
	}
}
