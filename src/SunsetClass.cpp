#include "MessageOutput.h"
#include "SunsetClass.h"
#include "Configuration.h"
#include "defaults.h"
#include <cstdlib>
#include <time.h>

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  5        /* Time ESP32 will go to sleep (in seconds) */

static RTC_DATA_ATTR int bootCount = 0;

SunsetClass::SunsetClass()
{
    _initialized = false;
}

void SunsetClass::init()
{
    setLocation();
    _isDayTime = true;
    _initialized = false;

    MessageOutput.print(F("SunsetClass init. bootCount = "));
    MessageOutput.printf("%u\n", bootCount++);
}

bool SunsetClass::loop()
{
    struct tm timeinfo;

    if (!getLocalTime(&timeinfo, 5)) { // Time is not valid
        _isDayTime = true;
        _currentDay = _currentMinute = -1;
        _sunriseMinutes = _sunsetMinutes = 0;
        _initialized = false;
        return false;
    }

    if (!Configuration.get().Sunset_Enabled) {
        if (_initialized) {
            _isDayTime = true;
            _currentDay = _currentMinute = -1;
            _sunriseMinutes = _sunsetMinutes = 0;
            _initialized = false;
        }
        return true;
    }
    
    if (!_initialized)
    {
        MessageOutput.println(F("SunsetClass - Initializing"));
        setLocation();
    }

    if (_currentDay != timeinfo.tm_mday) {
        _currentDay = timeinfo.tm_mday;
        _sunSet.setCurrentDate(1900 + timeinfo.tm_year, timeinfo.tm_mon + 1, timeinfo.tm_mday);

        // If you have daylight savings time, make sure you set the timezone appropriately as well
        _sunSet.setTZOffset(_timezoneOffset + (timeinfo.tm_isdst != 0 ? 1 : 0));
        _sunriseMinutes = static_cast<int>(_sunSet.calcSunrise());
        _sunsetMinutes = static_cast<int>(_sunSet.calcSunset());
    }

    if (_currentMinute != timeinfo.tm_min) {
        int minutesPastMidnight = timeinfo.tm_hour * 60 + timeinfo.tm_min;

        _currentMinute = timeinfo.tm_min;

        _isDayTime = (minutesPastMidnight >= (_sunriseMinutes + Configuration.get().Sunset_Sunriseoffset))
            && (minutesPastMidnight < (_sunsetMinutes + Configuration.get().Sunset_Sunsetoffset));
    }

    if (Configuration.get().Sunset_Deepsleep) {
        esp_sleep_enable_timer_wakeup(Configuration.get().Sunset_Deepsleeptime * uS_TO_S_FACTOR);
        MessageOutput.println(F("SunsetClass - Going to sleep now"));
        delay(1000);
        MessageOutput.flush(); 
        esp_deep_sleep_start();
    }

    return true;
}

void SunsetClass::setLocation()
{
    _latitude = std::stod(Configuration.get().Sunset_Latitude);
    _longitude = std::stod(Configuration.get().Sunset_Longitude);

    // Set default values
    _currentDay = _currentMinute = -1;
    _isDayTime = true;
    _sunriseMinutes = _sunsetMinutes = 0;

    // Get timezone offset
    struct tm dt;
    memset(&dt, 0, sizeof(struct tm));
    dt.tm_mday = 1;
    dt.tm_year = 70;
    time_t tzlag = mktime(&dt);
    _timezoneOffset = -tzlag / 3600;

    _sunSet.setPosition(_latitude, _longitude, static_cast<double>(_timezoneOffset));

    _initialized = true;
}

int SunsetClass::getTimezoneOffset()
{
    return _timezoneOffset;
}

int SunsetClass::getSunriseMinutes()
{
    return _sunriseMinutes;
}

int SunsetClass::getSunsetMinutes()
{
    return _sunsetMinutes;
}

bool SunsetClass::isDayTime()
{
    return _isDayTime;
}

SunsetClass SunsetClassInst;