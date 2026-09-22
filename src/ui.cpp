#include "ui.h"
#include "../inc/sdk.h"

void UI::DisplaySubtitle(const char* text)
{
	UILOG::_UILOG_SET_CACHED_OBJECTIVE(text);
	UILOG::_UILOG_PRINT_CACHED_OBJECTIVE();
	UILOG::_UILOG_CLEAR_HAS_DISPLAYED_CACHED_OBJECTIVE();
	UILOG::_UILOG_CLEAR_CACHED_OBJECTIVE();
}

void UI::DrawAllDebug(const std::vector<std::string>& debugMessages)
{
	int row = 0;
	for (const std::string& message : debugMessages){
		UI::DrawDebugText(message.c_str(), 60.0f, 300.0f + row * 21.0f, 0.28f);
		row++;
	}
}

void UI::DrawDebugText(const char* text, float pixelX, float pixelY, float scale)
{
	UIDEBUG::_BG_SET_TEXT_SCALE(scale, scale);
	UIDEBUG::_BG_SET_TEXT_COLOR(245, 245, 245, 255);

	const char* varString = MISC::VAR_STRING(10, (char*)"LITERAL_STRING", (char*)text);
	UIDEBUG::_BG_DISPLAY_TEXT(varString, pixelX / 1920.0f, pixelY / 1080.0f);
}

void UI::DrawDebugText(const char* text) {
	UI::DrawDebugText(text, 100.0f, 100.0f, 0.30f);
}
