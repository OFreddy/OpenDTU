// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <TaskSchedulerDeclarations.h>
#include <atomic>
#include <sunset.h>

#define SUNPOS_UPDATE_INTERVAL 60000l
#define DEEPSLEEP_BOOTDELAY 120000l

class SunPositionClass {
public:
    SunPositionClass();
    void init(Scheduler& scheduler);

    bool isDayPeriod() const;
    bool isSunsetAvailable() const;
    bool isValidInfo() const;
    bool sunsetTime(struct tm* info) const;
    bool sunriseTime(struct tm* info) const;
    void setDoRecalc(const bool doRecalc);

private:
    void loop();
    void updateSunData();
    bool checkRecalcDayChanged() const;
    bool getSunTime(struct tm* info, const uint32_t offset) const;

    Task _loopTask;

    bool _isSunsetAvailable = true;
    uint32_t _sunriseMinutes = 0;
    uint32_t _sunsetMinutes = 0;

    bool _isValidInfo = false;
    uint32_t _bootDelay = 0;
    std::atomic_bool _doRecalc = true;
    uint32_t _lastSunPositionCalculatedYMD = 0;
};

extern SunPositionClass SunPosition;
