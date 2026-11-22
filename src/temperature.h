#pragma once

// Shared by the wetness and hypothermia systems.
namespace Temperature
{
	//Celsius. The .ini accepts F or C and converts.
	struct Settings
	{
		float nominalC = 20.0f; //68F
		float extremeC = 32.0f; //89.6F
		float freezingC = 0.0f; //32F
		float bitterColdC = -17.8f; // 0F (Lowest I've seen in game is -4.0F)
	};

	void Init(const Settings& settings);
	const Settings& Get();
}
