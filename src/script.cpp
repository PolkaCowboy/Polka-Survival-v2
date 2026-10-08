#include "script.h"
#include "keyboard.h"
#include "log.h"
#include "config.h"
#include "ui.h"
#include "hurt.h"
#include "hypothermia.h"
#include "wetness.h"
#include "utils.h"
#include "exertion.h"
#include "exposurering.h"


#include <format>
#include <vector>
#include <string>
#include <algorithm>

namespace
{
	constexpr int MAX_FRAME_MS = 1000;
}


void ScriptMain()
{
	// All the running timers and flags
	Hurt::State hurtState;
	HypothermiaState hypothermiaState;
	WetnessState wetnessState;
	ExposureRing::State exposureRingState;

	ModConfig g_config = LoadConfig();
	std::vector<std::string> debugMessages;
	bool g_showOverlay = true;

	//Set up states
	Hurt::Init(hurtState, g_config);
	HypothermiaInit(hypothermiaState, g_config);
	WetnessInit(wetnessState, g_config);
	ExposureRing::Init(exposureRingState);

	if (g_config.DisableMod)
		return;
	else
		UI::DisplaySubtitle("Polka Survival Activated");

	int lastNow = MISC::GET_GAME_TIMER();

	if (g_config.enableTextLogging) {
		Log::Init();
		Log::Write(std::format("ScriptMain started"));
	}
	
	//Debug thing to remove: start the vanilla drain test from a full health core.
	ATTRIBUTE::_SET_ATTRIBUTE_CORE_VALUE(PLAYER::PLAYER_PED_ID(), ATTRIBUTE_CORE_HEALTH, 62);
	ATTRIBUTE::_SET_ATTRIBUTE_CORE_VALUE(PLAYER::PLAYER_PED_ID(), ATTRIBUTE_CORE_STAMINA, 62);

	// DEBUG: core drain test. Checks the three cores every tick and logs whenever one drops,
	// with game time (pauses with the game) and the time since that core's previous drop.
	const int coreIds[3] = { ATTRIBUTE_CORE_HEALTH, ATTRIBUTE_CORE_STAMINA, ATTRIBUTE_CORE_DEADEYE };
	const char* const coreNames[3] = { "health", "stamina", "deadeye" };
	int coreTestStart = MISC::GET_GAME_TIMER();
	int lastCoreValue[3];
	int lastCoreDropAt[3];
	for (int i = 0; i < 3; i++) {
		lastCoreValue[i] = ATTRIBUTE::_GET_ATTRIBUTE_CORE_VALUE(PLAYER::PLAYER_PED_ID(), coreIds[i]);
		lastCoreDropAt[i] = -1; // no drop seen yet
	}
	Log::Write(std::format("core drain test: mod drains per day H {:g} S {:g} (0 = off)  start h {}  s {}  d {}",
		g_config.HealthCoreDrainsPerDay, g_config.StaminaCoreDrainsPerDay, lastCoreValue[0], lastCoreValue[1], lastCoreValue[2]));

	while (true)
	{
		debugMessages.clear();

		Player player = PLAYER::PLAYER_ID();
		Ped playerPed = PLAYER::PLAYER_PED_ID();

		int now = MISC::GET_GAME_TIMER();
		int rawDtMs = now - lastNow;
		int dtMs = std::clamp(rawDtMs, 0, MAX_FRAME_MS);
		lastNow = now;

		/* Monitor health */
		debugMessages.push_back(std::format("h: {}, s: {}, d: {}",
			ATTRIBUTE::_GET_ATTRIBUTE_CORE_VALUE(playerPed, ATTRIBUTE_CORE_HEALTH),
			ATTRIBUTE::_GET_ATTRIBUTE_CORE_VALUE(playerPed, ATTRIBUTE_CORE_STAMINA),
			ATTRIBUTE::_GET_ATTRIBUTE_CORE_VALUE(playerPed, ATTRIBUTE_CORE_DEADEYE)
		));

		for (int i = 0; i < 3; i++) {
			int value = ATTRIBUTE::_GET_ATTRIBUTE_CORE_VALUE(playerPed, coreIds[i]);
			if (value != lastCoreValue[i]) {
				std::string sinceLast = lastCoreDropAt[i] < 0 ? std::string("first change") : std::format("{:.3f} s since last change", (now - lastCoreDropAt[i]) / 1000.0);
				Vector3 pos = ENTITY::GET_ENTITY_COORDS(playerPed, TRUE, FALSE);
				Log::Write(std::format("{} {} {} -> {}  at {:.3f} s  ({})  bodyheat {}  temp {:.1f}F",
					value < lastCoreValue[i] ? "DROP" : "RISE",
					coreNames[i], lastCoreValue[i], value, (now - coreTestStart) / 1000.0, sinceLast,
					ATTRIBUTE::GET_ATTRIBUTE_POINTS(playerPed, SA_BODYHEAT),
					Utils::CelsiusToFahrenheit(MISC::_GET_TEMPERATURE_AT_COORDS(pos.x, pos.y, pos.z))));
				lastCoreDropAt[i] = now; // time of the last change, drop or rise
			}
			lastCoreValue[i] = value;
		}

		if (g_config.DisableEagleEye) {
			PLAYER::_ENABLE_EAGLEEYE(player, false);
		}



		float exertion = Exertion::GetExertion(playerPed);
		WetnessUpdate(wetnessState, playerPed, exertion, dtMs, g_config, debugMessages);
		//HypothermiaUpdate(hypothermiaState, playerPed, wetnessState.wetness.Value(), dtMs, g_config, debugMessages);
		Hurt::Update(hurtState, player, playerPed, dtMs, hypothermiaState.effects, g_config, debugMessages);

		bool losingHpToCold = hypothermiaState.effects.hitPointsDrainModifier > 0.0;
		ExposureRing::Update(exposureRingState, wetnessState.wetness.Value(), wetnessState.fireWeight, hypothermiaState.severity, losingHpToCold, dtMs, now, g_config);
		
		

		if (IsKeyJustUp(VK_F19))
			g_showOverlay = !g_showOverlay;

		if (g_showOverlay) {
			UI::DrawAllDebug(debugMessages);
		}

		WAIT(0);
	}
}
