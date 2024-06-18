add_requires("pahomqttcpp", "raylib")

add_rules("mode.debug", "mode.release")

target("imu-visualizer") do
	set_license("LGPL-2.0")
	set_kind("binary")
	set_warnings("all", "error")

	add_files("src/*.cpp")
	add_includedirs("include")

	add_packages("pahomqttcpp", "raylib")
	add_defines("PLATFORM_DESKTOP")
end