package main

import (
	"log"
	"os"
	"os/exec"
	"path/filepath"
	"strings"

	MQTT "github.com/eclipse/paho.mqtt.golang"
)

func mqtt_topic_traffic_light_image(client MQTT.Client, payload []byte, directory_name string) {
	filename := filepath.Join(directory_name, "traffic_light.jpg")
	save_image(payload, filename)
	if check_traffic_light_image(filename) {
		// if red light was detected, send request for picture of driver
		mqtt_publish(client, "5/cam/get_single_image", []byte(""))
	}
}

func mqtt_topic_driver_image(payload []byte, directory_name string) {
	save_image(payload, filepath.Join(directory_name, "drivers", ".jpg"))
}

func save_image(image []byte, file_path string) {
	err := os.WriteFile(file_path, image, 0644)
	check(err)
}

func check_traffic_light_image(directory_name string) bool {
	out, err := exec.Command("python", "main.py", directory_name).Output()
	if err != nil {
		log.Printf("error %s", err)
		log.Fatal(err)
	}
	return strings.TrimRight(string(out), "\n") != "red"
}
