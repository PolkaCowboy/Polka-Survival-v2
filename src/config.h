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
	float ImproperClothingStaminaDrainRate = 0.1f; //Addiditional stamina drain if outfit isn't proper for the weather. Float percentage betweem 0 - 1. 0.1 = 10%

	//Hurting
	float HurtingTimeToDieMin = 1.75f; //Minutes
	float HurtingTimeToDieMax = 6.0f; //Minutes
	bool HurtingIsLethal = true; //True: You will die when HP ticks to zero. False: Health will stop ticking when outer ring hits 0 and core will stop ticking at 1, leaving you very squishy until you can eat

	//Hypothermia
	bool EnableHypothermia = true;
	float TimeToGetWet = 5.0f;
	float TimeToDry = 30.0f;

	bool enableTextLogging = true;

};

ModConfig LoadConfig();
