#pragma once

#include <windows.h>
#include <string>

struct ModConfig
{

	//Misc
	bool DisableEagleEye = true;
	bool DisableMod = false;

	//Deadeye
	float DeadEyeDurationCost = 10.0f; //[Game default ~4.27]
	float DeadEyeActivationCost = 10.0f;

	//Health Core
	int HealingThreshold = 50;
	float HealthCoreDrainsPerDay = 2.0f;

	//Stamina Core
	float StaminaCoreDrainsPerDay = 2.0f;
	bool StaminaHurtsHealth = true;

	//Hurting
	float HurtingTimeToDieMin = 1.5f; //Minutes
	float HurtingTimeToDieMax = 3.0f; //Minutes
	bool HurtingIsLethal = true; //Only drain the outer ring keeping the core HP as a reserve if false.

	//Hypothermia
	bool EnableHypothermia = true;
	float TimeToGetWet = 5.0f;
	float TimeToDry = 30.0f;

	bool enableTextLogging = true;

};

ModConfig LoadConfig();
