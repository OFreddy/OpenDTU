// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <mutex>
#include <sunset.h>

#define SUNPOS_UPDATE_INTERVAL 60000l
#define DEEPSLEEP_BOOTDELAY 120000l

class SunPositionClass {
public:
    SunPositionClass();
    void init();
    void loop();

    bool isDayPeriod();
    bool isSunsetAvailable();
    bool sunsetTime(struct tm* info);
    bool sunriseTime(struct tm* info);
    void setDoRecalc(bool doRecalc);

private:
    void updateSunData();
    bool checkRecalcDayChanged();
    bool getDoRecalc();

    SunSet _sun;
    bool _isSunsetAvailable = true;
    uint32_t _sunriseMinutes = 0;
    uint32_t _sunsetMinutes = 0;

    bool _isValidInfo = false;
    uint32_t _bootDelay = 0;
    bool _doRecalc = true;
    std::mutex _recalcLock;
    uint32_t _lastSunPositionCalculatedYMD = 0;
};

extern SunPositionClass SunPosition;
