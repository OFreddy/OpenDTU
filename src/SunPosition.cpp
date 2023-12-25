// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Thomas Basler and others
 */
#include "SunPosition.h"
#include "Configuration.h"
#include "MessageOutput.h"
#include "Utils.h"
#include <Arduino.h>

#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */

static RTC_DATA_ATTR int bootCount = 0;

SunPositionClass SunPosition;

SunPositionClass::SunPositionClass()
{
}

void SunPositionClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setCallback(std::bind(&SunPositionClass::loop, this));
    _loopTask.setIterations(TASK_FOREVER);
    _loopTask.setInterval(5 * TASK_SECOND);
    _loopTask.enable();

    // Fallback for first power on boot
    if (bootCount == 0)
        _bootDelay = millis() + DEEPSLEEP_BOOTDELAY;

    MessageOutput.print(F("SunsetClass init. bootCount = "));
    MessageOutput.printf("%u\n", bootCount++);
}

void SunPositionClass::loop()
{
    if (_doRecalc || checkRecalcDayChanged()) {
        updateSunData();
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

bool SunPositionClass::isDayPeriod() const
{
    if (!_isValidInfo) {
        return true;
    }

    struct tm timeinfo;
    getLocalTime(&timeinfo, 5);
    const uint32_t minutesPastMidnight = timeinfo.tm_hour * 60 + timeinfo.tm_min;
    return (minutesPastMidnight >= _sunriseMinutes) && (minutesPastMidnight < _sunsetMinutes);
}

bool SunPositionClass::isSunsetAvailable() const
{
    return _isSunsetAvailable;
}

void SunPositionClass::setDoRecalc(const bool doRecalc)
{
    _doRecalc = doRecalc;
}

bool SunPositionClass::checkRecalcDayChanged() const
{
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo); // don't use getLocalTime() as there could be a delay of 10ms

    const uint32_t ymd = (timeinfo.tm_year << 9) | (timeinfo.tm_mon << 5) | timeinfo.tm_mday;

    return _lastSunPositionCalculatedYMD != ymd;
}

void SunPositionClass::updateSunData()
{
    struct tm timeinfo;
    const bool gotLocalTime = getLocalTime(&timeinfo, 5);

    _lastSunPositionCalculatedYMD = (timeinfo.tm_year << 9) | (timeinfo.tm_mon << 5) | timeinfo.tm_mday;
    setDoRecalc(false);

    if (!gotLocalTime) {
        _sunriseMinutes = 0;
        _sunsetMinutes = 0;
        _isSunsetAvailable = true;
        _isValidInfo = false;
        return;
    }

    CONFIG_T const& config = Configuration.get();

    double sunset_type;
    switch (config.Ntp.SunsetType) {
    case 0:
        sunset_type = SunSet::SUNSET_OFFICIAL;
        break;
    case 2:
        sunset_type = SunSet::SUNSET_CIVIL;
        break;
    case 3:
        sunset_type = SunSet::SUNSET_ASTONOMICAL;
        break;
    default:
        sunset_type = SunSet::SUNSET_NAUTICAL;
        break;
    }

    const int offset = Utils::getTimezoneOffset() / 3600;

    SunSet sun;
    sun.setPosition(config.Ntp.Latitude, config.Ntp.Longitude, offset);
    sun.setCurrentDate(1900 + timeinfo.tm_year, timeinfo.tm_mon + 1, timeinfo.tm_mday);

    const double sunriseRaw = sun.calcCustomSunrise(sunset_type);
    const double sunsetRaw = sun.calcCustomSunset(sunset_type);

    // If no sunset/sunrise exists (e.g. astronomical calculation in summer)
    // assume it's day period
    if (std::isnan(sunriseRaw) || std::isnan(sunsetRaw)) {
        _sunriseMinutes = 0;
        _sunsetMinutes = 0;
        _isSunsetAvailable = false;
        _isValidInfo = false;
        return;
    }

    _sunriseMinutes = static_cast<int>(sunriseRaw);
    _sunsetMinutes = static_cast<int>(sunsetRaw);

    _isDayPeriod = (minutesPastMidnight >= _sunriseMinutes) && (minutesPastMidnight < _sunsetMinutes);
    _isSunsetAvailable = true;
    _isValidInfo = true;
}

bool SunPositionClass::getSunTime(struct tm* info, const uint32_t offset) const
{
    // Get today's date
    time_t aTime = time(NULL);

    // Set the time to midnight
    struct tm tm;
    localtime_r(&aTime, &tm);
    tm.tm_sec = 0;
    tm.tm_min = offset;
    tm.tm_hour = 0;
    tm.tm_isdst = -1;
    const time_t midnight = mktime(&tm);

    localtime_r(&midnight, info);
    return _isValidInfo;
}

bool SunPositionClass::sunsetTime(struct tm* info) const
{
    return getSunTime(info, _sunsetMinutes);
}

bool SunPositionClass::sunriseTime(struct tm* info) const
{
    return getSunTime(info, _sunriseMinutes);
}
