#ifndef POKEBW2_FIELD_WEATHER_WEATHER_SANDSTORM_H
#define POKEBW2_FIELD_WEATHER_WEATHER_SANDSTORM_H

#include "types.h"
#include "field/weather/field_weather.h"

// weather_sandstorm.c, overlay 77: the sandstorm weathers, 3 and 12, which ConvFieldWeatherToBtl makes a sandstorm in
// battle and Sand Veil makes rarer encounters in. The ROM doesn't name the file; the name is a guess. Overlay 36's
// weather table (WEATHER_OVERLAYS) loads it

// Weather 3
extern WeatherData SANDSTORM_WEATHER;
// Weather 12, the same but for unk00[2]
extern WeatherData SANDSTORM_WEATHER_2;

#endif // POKEBW2_FIELD_WEATHER_SANDSTORM_H
