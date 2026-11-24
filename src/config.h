#pragma once

#include "camp.h"
#include "exposurering.h"
#include "hurt.h"
#include "hypothermia.h"
#include "movement.h"
#include "temperature.h"
#include "wetness.h"

#include <windows.h>
#include <string>

struct ModConfig
{

	//Misc
	bool DisableEagleEye = true;
	bool DisableMod = false;

	//Deadeye
	float DeadEyeDurationCost = 10.0f; //Dead eye used per second while it is on. 0 = the game default (~4.6)
	float DeadEyeActivationCost = 10.0f; //Dead eye taken each time it is turned on

	Hurt::Settings hurt;
	Hypothermia::Settings hypothermia;
	Wetness::Settings wetness;
	Temperature::Settings temperature;
	Camp::Settings camp;
	Movement::Settings movement;
	ExposureRing::Settings exposureRing;

	bool enableTextLogging = false;

};

ModConfig LoadConfig();
