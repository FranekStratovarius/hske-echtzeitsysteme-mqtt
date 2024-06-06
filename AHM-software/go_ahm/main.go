package main

import (
	"log"
	"os"
)

// var MQTT_BROKER = "tcp://localhost:1883"
var MQTT_BROKER = "tcp://broker.hivemq.com:1883"
var LIDAR_TRIGGER_DISTANCE = 100

func check(e error) {
	if e != nil {
		panic(e)
	}
}

func mqtt_topic_default(topic string, payload string) {
	log.Printf("[%s] %s", topic, payload)
}

func main() {
	// create temp directory to save the images to
	// dname, err := os.MkdirTemp("", "ahm_traffic_light_data")
	dname := "ahm_traffic_light_data"
	os.Mkdir(dname, 0777)

	start_mqtt(dname)
}
