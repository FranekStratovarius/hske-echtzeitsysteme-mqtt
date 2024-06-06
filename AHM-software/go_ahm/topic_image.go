package main

import (
	"log"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
)

func mqtt_topic_image(payload []byte, dname string) {
	log.Print("[image]")
	err := os.WriteFile(filepath.Join(dname, "img.jpg"), payload, 0644)
	check(err)
}

func checkTrafficLightImage(dname string) bool {
	out, err := exec.Command("python", "main.py", dname).Output()
	if err != nil {
		log.Printf("error %s", err)
		log.Fatal(err)
	}
	// log.Printf("detected traffic light: %s", out)
	return strings.TrimRight(string(out), "\n") != "red"
}
