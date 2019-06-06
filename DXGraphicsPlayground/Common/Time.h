#pragma once

class Win32App;
class Time {
	friend class Win32App;

public:
	Time() = delete;
	~Time() = delete;

	static float getDeltaTime();
	static float getTimeSinceStartup();

private:
	static float _deltaTime;
	static float _timeSinceStartup;
};