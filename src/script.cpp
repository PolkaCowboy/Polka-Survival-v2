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

	ModConfig g_config = LoadConfig();
	std::vector<std::string> debugMessages;
	bool g_showOverlay = true;

	//Set up states
	Hurt::Init(hurtState, g_config);
	HypothermiaInit(hypothermiaState, g_config);
	WetnessInit(wetnessState, g_config);

	if (g_config.DisableMod)
		return;
	else
		UI::DisplaySubtitle("Polka Survival Activated");

	int lastNow = MISC::GET_GAME_TIMER();

	if (g_config.enableTextLogging) {
		Log::Init();
		Log::Write(std::format("ScriptMain started"));
	}
	
	//Debug thing to remove
	ATTRIBUTE::_SET_ATTRIBUTE_CORE_VALUE(PLAYER::PLAYER_PED_ID(), ATTRIBUTE_CORE_HEALTH, 17);

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

		if (g_config.DisableEagleEye) {
			PLAYER::_ENABLE_EAGLEEYE(player, false);
		}

		float exertion = Exertion::GetExertion(playerPed);
		WetnessUpdate(wetnessState, playerPed, exertion, dtMs, g_config, debugMessages);
		//HypothermiaUpdate(hypothermiaState, playerPed, wetnessState.wetness.Value(), dtMs, g_config, debugMessages);
		Hurt::Update(hurtState, player, playerPed, dtMs, hypothermiaState.effects, g_config, debugMessages);
		

		if (IsKeyJustUp(VK_F19))
			g_showOverlay = !g_showOverlay;

		if (g_showOverlay) {
			UI::DrawAllDebug(debugMessages);
		}

		WAIT(0);
	}
}
