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
	float HurtingTimeToDieMin = 1.5f;
	float HurtingTimeToDieMax = 3.0f;

	//Hypothermia
	bool EnableHypothermia = true;
	float TimeToGetWet = 5.0f;
	float TimeToDry = 30.0f;

	bool enableTextLogging = true;

};

ModConfig LoadConfig();
