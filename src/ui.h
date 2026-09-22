#pragma once

#include <string>
#include <vector>

namespace UI
{
	void DisplaySubtitle(const char* text);
	void DrawAllDebug(const std::vector<std::string>& debugMessages);
	void DrawDebugText(const char* text, float pixelX, float pixelY, float scale);
	void DrawDebugText(const char* text);
}
