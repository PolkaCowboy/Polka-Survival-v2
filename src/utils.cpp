#include "utils.h"

#include <format>

std::string Utils::FormatDuration(int ms)
{
	if (ms < 0)
		ms = 0;

	int totalSeconds = ms / 1000;
	int hours = totalSeconds / 3600;
	int minutes = (totalSeconds / 60) % 60;
	int seconds = totalSeconds % 60;
	int millis = ms % 1000;

	return std::format("{:02}h {:02}m {:02}.{:03}s", hours, minutes, seconds, millis);
}
