#include <string>
#include "raylib.h"
#include "rlgl.h"
#include "mqtt_callback.h"
#include "mqtt.h"
#include "mqtt_constants.h"

int main(void) {
	std::string log = "";
	mqtt_data md = {
		.pos_x = 0.0,
		.pos_y = 0.0,
		.pos_z = 0.0,
		
		.pitch = 0.0,
		.roll = 0.0,
		.temp = "",
	};
	//*

	mqtt::async_client cli(SERVER_ADDRESS, CLIENT_ID);

	mqtt::connect_options connOpts;
	connOpts.set_clean_session(false);

	// Install the callback(s) before connecting.
	Callback cb(cli, connOpts, &md);

	// test(cli, connOpts, cb);
	
	MQTT* mqtt;

	try {
		mqtt = new MQTT(cli, connOpts, cb);
	}
	catch (const mqtt::exception& exc) {
		std::cerr << "\nERROR: Unable to connect to MQTT server: '"
			<< SERVER_ADDRESS << "'" << exc << std::endl;
		return 1;
	}
	//*/

	
	const int screenWidth = 800;
	const int screenHeight = 450;

	InitWindow(screenWidth, screenHeight, "IMU visualizer");

	Camera camera = { { 0.0f, 10.0f, 10.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 45.0f, 0 };

	// Vector3 spherePos = { 0.0f, 0.0f, 0.0f };
	float sphereSize = 1.5f;

	SetTargetFPS(60);
	
	while (!WindowShouldClose()) {
		if(IsKeyDown(KEY_A)) {
			mqtt->send_message("2/imu/acc/x", "-0.5");
		}
		if(IsKeyDown(KEY_D)) {
			mqtt->send_message("2/imu/acc/x", "0.5");
		}
		if(IsKeyPressed(KEY_X)) {
			log = "stopping IMU\n" + log;
			mqtt->send_message("2/set_imu", "0");
		}
		if(IsKeyPressed(KEY_C)) {
			log = "starting IMU\n" + log;
			mqtt->send_message("2/set_imu", "1");
		}
		if(IsKeyPressed(KEY_V)) {
			log = "taking picture\n" + log;
			mqtt->send_message("2/take_picture", "");
		}
		BeginDrawing();
			ClearBackground(RAYWHITE);
			BeginMode3D(camera);

				rlPushMatrix();
				float scaling = 0.1;
				rlTranslatef(
					md.pos_x * scaling,
					md.pos_z * scaling,
					md.pos_y * scaling
				);
				rlRotatef(md.roll, 0, 0, 1);
				rlRotatef(md.pitch, 1, 0, 0);

				DrawSphere(Vector3{0, 0, 0}, sphereSize, GRAY);
				DrawSphereWires(Vector3{0, 0, 0}, sphereSize, 16, 16, DARKGRAY);

				rlPopMatrix();

				DrawGrid(10, 1.0f);

			EndMode3D();
			DrawText(
				TextFormat("%s °C", md.temp.c_str()),
				10, 20, 20, GRAY
			);
			DrawText(
				md.status.c_str(),
				10, 40, 20, GRAY
			);
			DrawText(
				log.c_str(),
				10, 60, 20, GRAY
			);

			// DrawTexture(texture, screenWidth/2 - texture.width/2, screenHeight/2 - texture.height/2, WHITE);
		EndDrawing();
	}
	CloseWindow();

	return 0;
}