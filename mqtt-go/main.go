package main

import (
	"log"
	"os"
	"os/exec"

	MQTT "github.com/eclipse/paho.mqtt.golang"
)

func check(e error) {
	if e != nil {
		panic(e)
	}
}

func checkTrafficLightImage(client MQTT.Client) {
	out, err := exec.Command("python", "main.py").Output()
	if err != nil {
		log.Fatal(err)
	}
	log.Printf("detected traffic light: %s", out)
	switch string(out) {
	case "green":
		mqtt_publish(client, "drive", []byte("go"))
	case "red":
		mqtt_publish(client, "drive", []byte("stop"))
	}
}

func mqtt_publish(client MQTT.Client, topic string, payload []byte) {
	log.Println("Sample Publisher Started")
	for i := 0; i < 1; i++ {
		log.Println("---- doing publish ----")
		token := client.Publish(topic, byte(0), false, payload)
		token.Wait()
	}
}

func mqtt_topic_image(client MQTT.Client, payload []byte) {
	//log.Printf("%s", string(payload))
	log.Print("[/image]")
	err := os.WriteFile("./light/img.jpg", payload, 0644)
	check(err)
	checkTrafficLightImage(client)
}

func mqtt_topic_default(topic string, payload string) {
	log.Printf("[%s] %s", topic, payload)
}

func main() {
	// myApp := app.New()
	// w := myApp.NewWindow("Image")
	// image := canvas.NewImageFromResource(theme.FyneLogo())
	// image.FillMode = canvas.ImageFillOriginal
	// w.SetContent(image)

	// w.ShowAndRun()

	opts := MQTT.NewClientOptions()
	opts.AddBroker("tcp://localhost:1883")

	choke := make(chan bool)

	opts.SetDefaultPublishHandler(func(client MQTT.Client, msg MQTT.Message) {
		switch msg.Topic() {
		case "/image":
			mqtt_topic_image(client, msg.Payload())
		default:
			mqtt_topic_default(msg.Topic(), string(msg.Payload()))
		}
		choke <- true
	})

	client := MQTT.NewClient(opts)
	if token := client.Connect(); token.Wait() && token.Error() != nil {
		panic(token.Error())
	}

	if token := client.Subscribe("#", byte(0), nil); token.Wait() && token.Error() != nil {
		log.Fatal(token.Error())
		os.Exit(1)
	}

	// publish
	mqtt_publish(client, "test", []byte("testpayload von go"))

	// subscribe
	for {
		<-choke
	}
}
