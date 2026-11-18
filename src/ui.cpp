#include "ui.h"
#include "../inc/sdk.h"

#include <algorithm>
#include <format>

namespace
{
	// Drop shadow under debug text, so it stays readable on bright backgrounds like snow.
	constexpr float DEBUG_SHADOW_OFFSET_PX = 1.0f;
	const UI::Rgba DEBUG_SHADOW_COLOR{ 0, 0, 0, 220 };
	const UI::Rgba DEBUG_TEXT_COLOR{ 245, 245, 245, 255 };

	void DrawDebugTextOnce(const char* text, float pixelX, float pixelY, float scale, const UI::Rgba& color)
	{
		UIDEBUG::_BG_SET_TEXT_SCALE(scale, scale);
		UIDEBUG::_BG_SET_TEXT_COLOR(color.r, color.g, color.b, color.a);

		const char* varString = MISC::VAR_STRING(10, (char*)"LITERAL_STRING", (char*)text);
		UIDEBUG::_BG_DISPLAY_TEXT(varString, pixelX / 1920.0f, pixelY / 1080.0f);
	}
}

void UI::DisplaySubtitle(const char* text)
{
	UILOG::_UILOG_SET_CACHED_OBJECTIVE(text);
	UILOG::_UILOG_PRINT_CACHED_OBJECTIVE();
	UILOG::_UILOG_CLEAR_HAS_DISPLAYED_CACHED_OBJECTIVE();
	UILOG::_UILOG_CLEAR_CACHED_OBJECTIVE();
}

void UI::DrawAllDebug(const std::vector<std::string>& debugMessages)
{
	// An empty message leaves a blank row, to separate sections.
	int row = 0;
	for (const std::string& message : debugMessages) {
		if (!message.empty())
			UI::DrawDebugText(message.c_str(), 60.0f, 300.0f + row * 21.0f, 0.28f);
		row++;
	}
}

void UI::DrawDebugText(const char* text, float pixelX, float pixelY, float scale)
{
	// There's no text shadow native, so draw the text in black slightly offset first, then on top.
	DrawDebugTextOnce(text, pixelX + DEBUG_SHADOW_OFFSET_PX, pixelY + DEBUG_SHADOW_OFFSET_PX, scale, DEBUG_SHADOW_COLOR);
	DrawDebugTextOnce(text, pixelX, pixelY, scale, DEBUG_TEXT_COLOR);
}

void UI::DrawDebugText(const char* text) {
	UI::DrawDebugText(text, 100.0f, 100.0f, 0.30f);
}

UI::Rgba UI::HudColor(const char* name)
{
	Rgba c;
	HUD::_GET_COLOR_FROM_NAME(MISC::GET_HASH_KEY(name), &c.r, &c.g, &c.b, &c.a);
	return c;
}

bool UI::EnsureTextureDict(const char* dict)
{
	if (TXD::HAS_STREAMED_TEXTURE_DICT_LOADED(dict))
		return true;
	TXD::REQUEST_STREAMED_TEXTURE_DICT(dict, FALSE);
	return false;
}

void UI::DrawCoreRing(const CoreRing& ring)
{
	// Sizes are fractions of the screen on each axis, so dividing a pixel size by 1920 and 1080
	// keeps a square texture square on a 16:9 screen.
	float x = ring.centerX / 1920.0f;
	float y = ring.centerY / 1080.0f;
	float w = ring.sizePx / 1920.0f;
	float h = ring.sizePx / 1080.0f;

	// Drawn first so everything else sits on top of it.
	if (ring.backgroundDict && ring.backgroundName)
	{
		GRAPHICS::DRAW_SPRITE(ring.backgroundDict, ring.backgroundName, x, y, w * ring.backgroundScale, h * ring.backgroundScale, 0.0f, ring.backgroundColor.r, ring.backgroundColor.g, ring.backgroundColor.b, ring.backgroundColor.a, FALSE);
	}

	// The names must stay alive until DRAW_SPRITE returns; locals do.
	std::string trackName = std::format("rpg_meter_track_{}", std::clamp(ring.track, 0, 9));
	GRAPHICS::DRAW_SPRITE("rpg_meter_track", trackName.c_str(), x, y, w, h, 0.0f, ring.trackColor.r, ring.trackColor.g, ring.trackColor.b, ring.trackColor.a, FALSE);

	if (ring.percent > 0)
	{
		std::string fillName = std::format("rpg_meter_{}", std::clamp(ring.percent, 0, 99));
		GRAPHICS::DRAW_SPRITE("rpg_meter", fillName.c_str(), x, y, w, h, 0.0f, ring.fillColor.r, ring.fillColor.g, ring.fillColor.b, ring.fillColor.a, FALSE);
	}

	if (ring.iconDict && ring.iconName)
	{
		GRAPHICS::DRAW_SPRITE(ring.iconDict, ring.iconName, x, y, w * ring.iconScale, h * ring.iconScale, 0.0f, ring.iconColor.r, ring.iconColor.g, ring.iconColor.b, ring.iconColor.a, FALSE);
	}
}
