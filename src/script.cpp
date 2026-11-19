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

	// Logs every core change with the time since that core last changed. For checking drain timing;
	// off normally, since it writes a line for every tick of every core.
	constexpr bool LOG_CORE_CHANGES = false;

	const int CORE_IDS[3] = { ATTRIBUTE_CORE_HEALTH, ATTRIBUTE_CORE_STAMINA, ATTRIBUTE_CORE_DEADEYE };
	const char* const CORE_NAMES[3] = { "health", "stamina", "dead eye" };

	struct CoreChangeLog
	{
		int lastValue[3] = { -1, -1, -1 }; // -1 = not read yet
		int lastChangeAt[3] = { -1, -1, -1 }; // GET_GAME_TIMER, -1 = no change seen yet
	};

	void LogCoreChanges(CoreChangeLog& log, Ped playerPed, int now)
	{
		for (int i = 0; i < 3; i++) {
			int value = ATTRIBUTE::_GET_ATTRIBUTE_CORE_VALUE(playerPed, CORE_IDS[i]);
			if (log.lastValue[i] >= 0 && value != log.lastValue[i]) {
				std::string sinceLast = log.lastChangeAt[i] < 0 ? std::string("first change") : std::format("{:.3f} s since last change", (now - log.lastChangeAt[i]) / 1000.0);
				Vector3 pos = ENTITY::GET_ENTITY_COORDS(playerPed, TRUE, FALSE);
				Log::Write(std::format("{} {} {} -> {}  ({})  bodyheat {}  temp {:.1f}F",
					value < log.lastValue[i] ? "DROP" : "RISE", CORE_NAMES[i], log.lastValue[i], value, sinceLast,
					ATTRIBUTE::GET_ATTRIBUTE_POINTS(playerPed, SA_BODYHEAT),
					Utils::CelsiusToFahrenheit(MISC::_GET_TEMPERATURE_AT_COORDS(pos.x, pos.y, pos.z))));
				log.lastChangeAt[i] = now;
			}
			log.lastValue[i] = value;
		}
	}
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

	CoreChangeLog coreChangeLog;

	while (true)
	{
		debugMessages.clear();

		Player player = PLAYER::PLAYER_ID();
		Ped playerPed = PLAYER::PLAYER_PED_ID();

		int now = MISC::GET_GAME_TIMER();
		int rawDtMs = now - lastNow;
		int dtMs = std::clamp(rawDtMs, 0, MAX_FRAME_MS);
		lastNow = now;

		if (LOG_CORE_CHANGES)
			LogCoreChanges(coreChangeLog, playerPed, now);

		if (g_config.DisableEagleEye) {
			PLAYER::_ENABLE_EAGLEEYE(player, false);
		}

		float exertion = Exertion::GetExertion(playerPed);
		WetnessUpdate(wetnessState, playerPed, exertion, dtMs, g_config, debugMessages);
		HypothermiaUpdate(hypothermiaState, playerPed, wetnessState, dtMs, g_config, debugMessages);
		Hurt::Update(hurtState, player, playerPed, dtMs, hypothermiaState.effects, g_config, debugMessages);

		// Don't respawn soaked or frozen.
		if (PLAYER::IS_PLAYER_DEAD(player)) {
			WetnessInit(wetnessState, g_config);
			HypothermiaInit(hypothermiaState, g_config);
		}

		ExposureRing::Update(exposureRingState, wetnessState, hypothermiaState, dtMs, now, g_config);

		// F19 toggles the debug overlay.
		if (IsKeyJustUp(VK_F19))
			g_showOverlay = !g_showOverlay;

		if (g_showOverlay) {
			UI::DrawAllDebug(debugMessages);
		}

		WAIT(0);
	}
}
