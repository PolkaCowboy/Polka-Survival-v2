#pragma once

#include <windows.h>
#include <string>

// The game's compass (minimap) setting, chosen by holding Alt. No native reports it.
enum class CompassType
{
	Regular,
	Expanded,
	Simple, // called "Compass" in the game's menu; INI value COMPASS
	Off,
};

// Where the exposure ring sits: centre and size in 1920x1080 pixels, whatever the real resolution.
struct RingPlacement
{
	float x = 0.0f;
	float y = 0.0f;
	float size = 0.0f;
};

struct ModConfig
{

	//Misc
	bool DisableEagleEye = true;
	bool DisableMod = false;

	//Deadeye
	float DeadEyeDurationCost = 10.0f; //[Game default ~4.27]
	float DeadEyeActivationCost = 10.0f;

	//Health Core
	int HealingThreshold = 50; //Percent of the health core that must be filled to start Hitpoint regeneration.
	float HealthCoreDrainsPerDay = 2.0f;

	//Stamina Core
	float StaminaCoreDrainsPerDay = 2.0f;
	bool StaminaHurtsHealth = true;
	float ImproperClothingStaminaDrainRate = 0.1f; //Additional stamina drain if outfit isn't proper for the weather. Float percentage between 0 - 1. 0.1 = 10%

	//Hurting
	float HurtingTimeToDieMin = 1.75f; //Minutes
	float HurtingTimeToDieMax = 6.0f; //Minutes
	bool HurtingIsLethal = true; //True: You will die when HP ticks to zero. False: Health will stop ticking when outer ring hits 0 and core will stop ticking at 1, leaving you very squishy until you can eat

	//Hypothermia
	bool EnableHypothermia = true;
	float TimeToHypothermiaMin = 30.0f; // Time to make Hypthermia is teh worst conditions. (Soaked, below freezing)
	float HypothermiaRecoverNominal = 60.0f; //Time to recover when warm, without a fire bonus.
	float MaxSweatAccumulation = 0.4f; //0-1 range. What percentage of max soak you can get purely by sweating.
	float NominalMaxSweatTime = 180.0f; //Seconds. How long to achieve MaxSweatAccumulation at max exertion at NominalTemperatureC. Too warm clothing and hot weather will accelerate.
	float MinTimeToGetSoaked = 30.0f; //Seconds. Time to get 100% soaked in a full rainstorm.
	float MinTimeToDry = 240.0f; //Seconds. Time to completely dry without a heat source in a dry hot climate. Campfires/heatsources will accelerate this. Other weather factors will slow this.


	//Temperature. Celsius. The .ini accepts F or C and converts.
	float NominalTemperatureC = 20.0f; //68F
	float ExtremeTemperatureC = 32.0f; //89.6F
	float FreezingTemperatureC = 0.0f; //32F
	float BitterColdTemperatureC = -15.0f; // 5F (Lowest I've seen in game is -1.6F)

	//HUD. The compass type the player starts with, and the ring's placement for each HUD layout:
	//regular (small); compass, which the Compass and Off options share; and expanded, which the
	//Expanded compass, an Alt tap and the weapon wheel all use.
	bool ShowExposureRing = true;
	CompassType Compass = CompassType::Regular;
	RingPlacement RingRegular{ 46.0f, 847.0f, 50.0f };
	RingPlacement RingCompass{ 87.0f, 986.0f, 50.0f };
	RingPlacement RingExpanded{ 47.0f, 777.0f, 65.0f };

	bool enableTextLogging = true;

};

ModConfig LoadConfig();
