#include "temperature.h"

namespace
{
	Temperature::Settings g_settings;
}

void Temperature::Init(const Settings& settings)
{
	g_settings = settings;
}

const Temperature::Settings& Temperature::Get()
{
	return g_settings;
}
