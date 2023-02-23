// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Thomas Basler and others
 */
#include "SunPosition.h"
#include "Configuration.h"
#include "MessageOutput.h"
#include "Utils.h"

#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */

static RTC_DATA_ATTR int bootCount = 0;

SunPositionClass SunPosition;

SunPositionClass::SunPositionClass()
{
}

void SunPositionClass::init()
{
    // Fallback for first power on boot
    if (bootCount == 0)
        _bootDelay = millis() + DEEPSLEEP_BOOTDELAY;

    MessageOutput.print(F("SunsetClass init. bootCount = "));
    MessageOutput.printf("%u\n", bootCount++);
}

void SunPositionClass::loop()
{
    if (!_isValidInfo || (millis() - _lastUpdate > SUNPOS_UPDATE_INTERVAL)) {
        updateSunData();
        _lastUpdate = millis();
    }

    if (Configuration.get().Sunset_Deepsleep && !_isDayPeriod && (millis() > _bootDelay)) {
        esp_sleep_enable_timer_wakeup(Configuration.get().Sunset_Deepsleeptime * uS_TO_S_FACTOR);
        MessageOutput.print(F("SunsetClass - Going to sleep now for "));
        MessageOutput.printf("%u s\n", Configuration.get().Sunset_Deepsleeptime);
        delay(1000);
        MessageOutput.flush();
        esp_deep_sleep_start();
    }
}

bool SunPositionClass::isDayPeriod()
{
    return _isDayPeriod;
}

bool SunPositionClass::isValidInfo()
{
    return _isValidInfo;
}

void SunPositionClass::updateSunData()
{
    CONFIG_T const& config = Configuration.get();
    int offset = Utils::getTimezoneOffset() / 3600;
    _sun.setPosition(config.Ntp_Latitude, config.Ntp_Longitude, offset);

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 5)) {
        _isDayPeriod = false;
        _sunriseMinutes = 0;
        _sunsetMinutes = 0;
        _isValidInfo = false;
        return;
    }

    _sun.setCurrentDate(1900 + timeinfo.tm_year, timeinfo.tm_mon + 1, timeinfo.tm_mday);
    _sunriseMinutes = static_cast<int>(_sun.calcCustomSunrise(SunSet::SUNSET_NAUTICAL));
    _sunsetMinutes = static_cast<int>(_sun.calcCustomSunset(SunSet::SUNSET_NAUTICAL));
    uint minutesPastMidnight = timeinfo.tm_hour * 60 + timeinfo.tm_min;

    _isDayPeriod = (minutesPastMidnight >= _sunriseMinutes) && (minutesPastMidnight < _sunsetMinutes);
    _isValidInfo = true;
}

bool SunPositionClass::sunsetTime(struct tm* info)
{
    // Get today's date
    time_t aTime = time(NULL);

    // Set the time to midnight
    struct tm tm;
    localtime_r(&aTime, &tm);
    tm.tm_sec = 0;
    tm.tm_min = _sunsetMinutes;
    tm.tm_hour = 0;
    tm.tm_isdst = -1;
    time_t midnight = mktime(&tm);

    localtime_r(&midnight, info);
    return _isValidInfo;
}

bool SunPositionClass::sunriseTime(struct tm* info)
{
    // Get today's date
    time_t aTime = time(NULL);

    // Set the time to midnight
    struct tm tm;
    localtime_r(&aTime, &tm);
    tm.tm_sec = 0;
    tm.tm_min = _sunriseMinutes;
    tm.tm_hour = 0;
    tm.tm_isdst = -1;
    time_t midnight = mktime(&tm);

    localtime_r(&midnight, info);
    return _isValidInfo;
}