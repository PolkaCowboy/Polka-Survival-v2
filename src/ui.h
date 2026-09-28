#pragma once

#include <string>
#include <vector>

namespace UI
{
	void DisplaySubtitle(const char* text);
	void DrawAllDebug(const std::vector<std::string>& debugMessages);
	void DrawDebugText(const char* text, float pixelX, float pixelY, float scale);
	void DrawDebugText(const char* text);

	// DRAW_SPRITE multiplies the texture by this colour, so white leaves it as it is.
	struct Rgba
	{
		int r = 255;
		int g = 255;
		int b = 255;
		int a = 255;
	};

	// A game HUD colour by name, e.g. "COLOR_METER_TRACK_DEFAULT".
	// Name list: https://github.com/femga/rdr3_discoveries/blob/master/useful_info_from_rpfs/colours/README.md
	Rgba HudColor(const char* name);

	// Streamed texture dictionaries must be loaded before DRAW_SPRITE shows anything. Requests the
	// dictionary on every call until it's loaded; returns whether it is loaded now.
	bool EnsureTextureDict(const char* dict);

	struct CoreRing
	{
		float centerX = 0.0f; // 1920x1080 pixels
		float centerY = 0.0f;
		float sizePx = 0.0f;
		// Dark disc behind everything, like the vanilla cores. nullptr = none.
		const char* backgroundDict = nullptr;
		const char* backgroundName = nullptr;
		Rgba backgroundColor;
		float backgroundScale = 1.0f; // relative to the ring
		int percent = 0; // fill, 0-100 (rpg_meter_N runs 0-99, N = percent full)
		int track = 0; // rpg_meter_track_N, 0-9
		Rgba trackColor;
		Rgba fillColor;
		const char* iconDict = nullptr; // nullptr = no icon
		const char* iconName = nullptr;
		Rgba iconColor;
		float iconScale = 1.0f; // icon size relative to the ring
	};

	// An HUD-style core ring from the game's own textures. Texture list:
	// https://github.com/femga/rdr3_discoveries/blob/master/useful_info_from_rpfs/textures/ui_hud____part_1/README.md
	// All texture dictionaries used must already be loaded (EnsureTextureDict).
	void DrawCoreRing(const CoreRing& ring);
}
