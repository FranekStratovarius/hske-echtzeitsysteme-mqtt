package main

import (
	"log"
	"strconv"

	MQTT "github.com/eclipse/paho.mqtt.golang"
)

func mqtt_topic_lidar(client MQTT.Client, payload []byte) {
	trigger, err := strconv.Atoi(string(payload))
	check(err)
	log.Printf("[lidar] triggered: %d", trigger)

	if trigger == 1 {
		// when triggered, send request for picture of traffic light
		mqtt_publish(client, "2/take_picture", []byte(""))
	}
}
