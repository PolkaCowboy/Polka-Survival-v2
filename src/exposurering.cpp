#include "exposurering.h"
#include "../inc/sdk.h"
#include "utils.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace
{
	constexpr float FADE_SECONDS = 0.5f;
	constexpr float SHOW_THRESHOLD = 0.01f;
	constexpr float PULSES_PER_SECOND = 1.0f;
	constexpr float BACKGROUND_SCALE = 1.0f;
	const UI::Rgba BACKGROUND_TINT{ 0, 0, 0, 160 };

	
	const UI::Rgba WATER_BLUE{ 70, 160, 165, 255 };

	// The cold icon goes from white towards this as severity rises.
	const UI::Rgba ICE_BLUE{ 120, 190, 255, 255 };
	// The cold icon while there's no hypothermia: a dim placeholder.
	const UI::Rgba ICON_IDLE_GREY{ 70, 70, 70, 255 };
	// Hot color to the icon to go to when near a warming point.
	const UI::Rgba FIRE_ORANGE{ 255, 119, 0, 255 };
	

	// Swith to the expanded view layout for about 3 s of real time when the HUD is up.
	constexpr Hash INPUT_REVEAL_HUD = 0xCF8A4ECA;
	constexpr int REVEAL_MS = 3000;

	bool RevealPressed()
	{
		return PAD::IS_CONTROL_PRESSED(0, INPUT_REVEAL_HUD) || PAD::IS_DISABLED_CONTROL_PRESSED(0, INPUT_REVEAL_HUD);
	}

	// The weapon wheel also switches the HUD to the expanded layout, for as long as it's open.
	// It's open while INPUT_OPEN_WHEEL_MENU (Tab / LB) is held.
	// Control hashes: https://github.com/femga/rdr3_discoveries/blob/master/Controls/README.md
	constexpr Hash INPUT_OPEN_WHEEL_MENU = 0xAC4BD4F1;

	bool WeaponWheelOpen()
	{
		bool held = PAD::IS_CONTROL_PRESSED(0, INPUT_OPEN_WHEEL_MENU) || PAD::IS_DISABLED_CONTROL_PRESSED(0, INPUT_OPEN_WHEEL_MENU);
		return held && GRAPHICS::ANIMPOSTFX_IS_RUNNING("WheelHUDIn");
	}

	bool HudVisible()
	{
		return !HUD::IS_HUD_HIDDEN() && !HUD::IS_PAUSE_MENU_ACTIVE() && !CAMERA::IS_SCREEN_FADED_OUT();
	}

	UI::Rgba Lerp(const UI::Rgba& a, const UI::Rgba& b, float t)
	{
		return {
			static_cast<int>(std::lerp(static_cast<float>(a.r), static_cast<float>(b.r), t)),
			static_cast<int>(std::lerp(static_cast<float>(a.g), static_cast<float>(b.g), t)),
			static_cast<int>(std::lerp(static_cast<float>(a.b), static_cast<float>(b.b), t)),
			static_cast<int>(std::lerp(static_cast<float>(a.a), static_cast<float>(b.a), t)),
		};
	}

	UI::Rgba WithAlpha(UI::Rgba c, float alpha)
	{
		c.a = static_cast<int>(c.a * std::clamp(alpha, 0.0f, 1.0f));
		return c;
	}
}

void ExposureRing::Init(State& state)
{
	state = State{};
	state.trackColor = UI::HudColor("COLOR_METER_TRACK_DEFAULT");
}

void ExposureRing::Update(State& state, float wetness, float fireWeight, float severity, bool losingHp, int dtMs, int now, const ModConfig& cfg)
{
	int realNow = MISC::GET_SYSTEM_TIME();
	if (RevealPressed())
	{
		state.revealUntil = realNow + REVEAL_MS;
		state.revealActive = true;
	}
	if (state.revealActive && realNow >= state.revealUntil)
		state.revealActive = false;
	bool expandedHud = state.revealActive || WeaponWheelOpen();

	// Like the vanilla cores, the ring also shows while the HUD is revealed, even with nothing to report.
	bool relevant = wetness > SHOW_THRESHOLD || severity > SHOW_THRESHOLD || expandedHud;
	float target = (cfg.ShowExposureRing && relevant && HudVisible()) ? 1.0f : 0.0f;

	// Fade towards the target rather than popping on and off.
	float step = (dtMs / 1000.0f) / FADE_SECONDS;
	state.alpha = target > state.alpha ? std::min(target, state.alpha + step) : std::max(target, state.alpha - step);
	if (state.alpha <= 0.0f)
		return;

	// Requested every frame until loaded; draw only once all three are in.
	bool meter = UI::EnsureTextureDict("rpg_meter");
	bool track = UI::EnsureTextureDict("rpg_meter_track");
	bool icons = UI::EnsureTextureDict("rpg_textures");
	if (!meter || !track || !icons)
		return;

	// The Expanded compass, an Alt tap and the weapon wheel all use the expanded layout;
	const RingPlacement& place = (expandedHud || cfg.Compass == CompassType::Expanded) ? cfg.RingExpanded
		: cfg.Compass == CompassType::Regular ? cfg.RingRegular
		: cfg.RingCompass;

	UI::CoreRing ring;
	ring.centerX = place.x;
	ring.centerY = place.y;
	ring.sizePx = place.size;
	ring.backgroundDict = "rpg_textures";
	ring.backgroundName = "rpg_core_background";
	ring.backgroundColor = WithAlpha(BACKGROUND_TINT, state.alpha); // the texture is white: tint it dark
	ring.backgroundScale = BACKGROUND_SCALE;
	ring.percent = static_cast<int>(std::round(wetness * 100.0f));
	ring.trackColor = WithAlpha(state.trackColor, state.alpha);
	ring.fillColor = WithAlpha(WATER_BLUE, state.alpha);

	// The cold icon is always there: dark grey while there's no hypothermia, then white turning ice blue as it gets worse, pulsing when it costs health.
	float iconAlpha = state.alpha;
	UI::Rgba iconColor = ICON_IDLE_GREY;
	if (severity > SHOW_THRESHOLD)
	{
		iconColor = Lerp(UI::Rgba{}, ICE_BLUE, std::clamp(severity, 0.0f, 1.0f));
		if (losingHp)
		{
			// Pulse between 35% and 100%, like the vanilla core warning.
			float wave = 0.5f + 0.5f * std::sin(now / 1000.0f * PULSES_PER_SECOND * 2.0f * std::numbers::pi_v<float>);
			iconAlpha *= 0.35f + 0.65f * wave;
		}
	}
	else if (fireWeight > 0) {
		iconColor = Lerp(ICON_IDLE_GREY, FIRE_ORANGE, Utils::MapToRange(fireWeight, 0.0f, 2.0f, 0.0f, 1.0f)); //2.0f is half of MAX_HEAT_DRY_FACTOR in wetness.cpp to make flow glow happen before full potential glow. TODO: Move to config
	}
	ring.iconDict = "rpg_textures";
	ring.iconName = "rpg_cold";
	ring.iconColor = WithAlpha(iconColor, iconAlpha);

	UI::DrawCoreRing(ring);
}
