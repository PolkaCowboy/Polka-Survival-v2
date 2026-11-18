#pragma once
#include "../inc/sdk.h"

#include <string>
#include <cmath>

namespace Utils
{
	// Formats a millisecond count as "00h 00m 00.000s". Negative values show as zero.
	std::string FormatDuration(int ms);

	float ScaleToRange(float value, int minValue, int maxValue);
	int MapToRange(int value, int inMin, int inMax, int outMin, int outMax);

	// Maps value from [inMin, inMax] onto [outMin, outMax], clamped to the ends. Either range may
	// run backwards (e.g. 0-35C onto 1.0-0.0). inMin == inMax returns outMin.
	float MapToRange(float value, float inMin, float inMax, float outMin, float outMax);

	// The game works in Celsius; these are for config and display.
	inline float FahrenheitToCelsius(float f) { return (f - 32.0f) * 5.0f / 9.0f; }
	inline float CelsiusToFahrenheit(float c) { return c * 9.0f / 5.0f + 32.0f; }
	
	inline float Distance(const Vector3& a, const Vector3& b)
	{
		float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
		return std::sqrt(dx * dx + dy * dy + dz * dz);
	}

	inline float MoveTowards(float value, float target, float maxStep) {
		if (value < target) {
			return std::min(value + maxStep, target);
		}
		return std::max(value - maxStep, target);
	}
}
