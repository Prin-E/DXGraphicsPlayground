#include "pch.h"
#include "Time.h"

float Time::_deltaTime = 0;
float Time::_timeSinceStartup = 0;

float Time::getDeltaTime() {
	return _deltaTime;
}

float Time::getTimeSinceStartup() {
	return _timeSinceStartup;
}