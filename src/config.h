#pragma once

#include <windows.h>
#include <string>

struct ModConfig
{
	//Reference examples for various data types
	std::string message = "Hello from the template";
	float modifier = 0.5f;
	int maxCount = 10;
	bool enabled = true;
	DWORD toggleKey = VK_F19;

	bool enableTextLogging = true;

};

ModConfig LoadConfig();
