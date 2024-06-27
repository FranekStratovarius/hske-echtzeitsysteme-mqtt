package main

import (
	"os"
	"path/filepath"
)

// var MQTT_BROKER = "tcp://localhost:1883"
var MQTT_BROKER = "tcp://broker.hivemq.com:1883"
var LIDAR_TRIGGER_DISTANCE = 100

func check(e error) {
	if e != nil {
		panic(e)
	}
}

func main() {
	// create directory to save the images to
	directory_name := "ahm_traffic_light_data"
	os.Mkdir(directory_name, 0777)
	os.Mkdir(filepath.Join(directory_name, "drivers"), 0777)

	start_mqtt(directory_name)
}
