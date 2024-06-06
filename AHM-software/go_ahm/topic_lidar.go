package main

import (
	"log"
	"strconv"

	MQTT "github.com/eclipse/paho.mqtt.golang"
)

func mqtt_topic_lidar(client MQTT.Client, payload []byte, dname string) {
	distance, err := strconv.Atoi(string(payload))
	check(err)
	log.Printf("[lidar] %d %d", distance, LIDAR_TRIGGER_DISTANCE)

	if distance <= LIDAR_TRIGGER_DISTANCE {
		if !checkTrafficLightImage(dname) {
			mqtt_publish(client, "ahm_ticket", []byte("Fahrer erwischt! Zahltag :)"))
		}
	}
}
