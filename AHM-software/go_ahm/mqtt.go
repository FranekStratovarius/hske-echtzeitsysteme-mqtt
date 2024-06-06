package main

import (
	"log"
	"os"

	MQTT "github.com/eclipse/paho.mqtt.golang"
)

func mqtt_publish(client MQTT.Client, topic string, payload []byte) {
	for i := 0; i < 1; i++ {
		// log.Printf("publishing [%s], %s\n", topic, string(payload))
		token := client.Publish(topic, byte(0), false, payload)
		token.Wait()
	}
}

func start_mqtt(dname string) {
	opts := MQTT.NewClientOptions()
	opts.AddBroker(MQTT_BROKER)
	opts.ClientID = "ahm_server"

	choke := make(chan bool)

	opts.SetDefaultPublishHandler(func(client MQTT.Client, msg MQTT.Message) {
		switch msg.Topic() {
		case "lidar":
			mqtt_topic_lidar(client, msg.Payload(), dname)
		case "image":
			mqtt_topic_image(msg.Payload(), dname)
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

	if token := client.Subscribe("status", byte(0), nil); token.Wait() && token.Error() != nil {
		log.Fatal(token.Error())
		os.Exit(1)
	}

	// publish
	mqtt_publish(client, "status", []byte("testpayload von go"))

	// subscribe
	for {
		<-choke
	}
}
