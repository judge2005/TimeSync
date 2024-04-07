/*
 * EspDS1302TimeSync.cpp
 *
 *  Created on: Apr 7, 2024
 *      Author: mpand
 */

#include <EspDS1302TimeSync.h>
#if !defined(ARDUINO_ESP8266_RELEASE_2_3_0) || defined(ESP32)
#include <Arduino.h>
#include "time.h"
#include "sys/time.h"
#include <Wire.h>
#include <sntp_pt.h>
#include <stdio.h>

//#define DEBUG(...) { Serial.println(__VA_ARGS__); }
#ifndef DEBUG
#define DEBUG(...) {}
#endif

EspDS1302TimeSync::~EspDS1302TimeSync() {
	// TODO Auto-generated destructor stub
}

static uint8_t decToBcd(uint8_t val){
// Convert normal decimal numbers to binary coded decimal
  return ( (val/10*16) + (val%10) );
}

static uint8_t bcdToDec(uint8_t val)  {
// Convert binary coded decimal to normal decimal numbers
  return ( (val/16*10) + (val%16) );
}

static uint8_t BcdToBin24Hour(uint8_t bcdHour)
{
    uint8_t hour;
    if (bcdHour & 0x40)
    {
        // 12 hour mode, convert to 24
        bool isPm = ((bcdHour & 0x20) != 0);

        hour = bcdToDec(bcdHour & 0x1f);
        if (isPm)
        {
           hour += 12;
        }
    }
    else
    {
        hour = bcdToDec(bcdHour);
    }

    return hour;
}

void EspDS1302TimeSync::setDevice() {
	// sets time and date data to DS1302
	struct timeval _time;
	::gettimeofday(&_time, NULL);
	struct tm _now;
	::gmtime_r(&_time.tv_sec, &_now);

	DEBUG("Checking year");
	DEBUG(_now.tm_year);
	if (_now.tm_year >= 100) {
		DEBUG("Setting RTC");
		// set the date time
		tWire.beginTransmission(DS1302_REG_TIMEDATE_BURST);

		tWire.write(decToBcd(_now.tm_sec));
		tWire.write(decToBcd(_now.tm_min));
		tWire.write(decToBcd(_now.tm_hour));
		tWire.write(decToBcd(_now.tm_mday));  // set date (1 to 31)
		tWire.write(decToBcd(_now.tm_mon+1)); // set month (1 to 12)
		tWire.write(decToBcd(_now.tm_wday+1)); // set day of week (1=Sunday, 7=Saturday)
		tWire.write(decToBcd(_now.tm_year % 100)); // set year (0 to 99)
		tWire.write(0); // no write protect, as all of this is ignored if it is protected

		tWire.endTransmission();
	}
}

void EspDS1302TimeSync::setFromDevice() {
	_lastSyncFailed = true;
	bool lostPower = true;

	tWire.beginTransmission(DS1302_REG_TIMEDATE_BURST | THREEWIRE_READFLAG);

	uint8_t second = bcdToDec(tWire.read() & 0x7F);
	uint8_t minute = bcdToDec(tWire.read());
	uint8_t hour = BcdToBin24Hour(tWire.read());
	uint8_t dayOfMonth = bcdToDec(tWire.read());
	uint8_t month = bcdToDec(tWire.read());
	uint8_t dayOfWeek = bcdToDec(tWire.read());
	uint8_t year = bcdToDec(tWire.read());

	tWire.read();  // throwing away write protect flag

	tWire.endTransmission();

	if (second > 59 || minute > 59 || hour > 23 || dayOfMonth > 31 || dayOfWeek > 7 || year > 99) {
		lostPower = true;
	}

	if (!lostPower) {
		DEBUG("Setting time from DS1302");
		struct tm now;
		now.tm_hour = hour;
		now.tm_min = minute;
		now.tm_sec = second;
		now.tm_mday = dayOfMonth;
		now.tm_mon = month - 1;
		now.tm_year = year + 100;
		now.tm_isdst = -1;

		DEBUG(now.tm_year);
		struct timeval timeVal;
		// Yeuk. Not nice and not thread safe and not fast
		char *tzBuf = getenv("TZ");
		String ctz;

		if (tzBuf == NULL) {
			// Allocate a big buffer to TZ
			setenv("TZ", "012345678901234567890123456789012345678901234567890123456789", 1);
			tzBuf = getenv("TZ");
		} else {
			ctz = tzBuf;	// Save current TZ
		}
		strcpy(tzBuf, "UTC-0"); // See https://github.com/espressif/esp-idf/issues/3046
		tzset();
		timeVal.tv_usec = 0;
		timeVal.tv_sec = ::mktime(&now);

		if (ctz.length() > 0) {
			// Restore previous timezone
			strcpy(tzBuf, ctz.c_str()); // See https://github.com/espressif/esp-idf/issues/3046
			tzset();
		}

		::settimeofday(&timeVal, NULL);
		timeInitialized = true;
		_lastSyncFailed = false;
	} else {
		stats.failedCount = String(++numFailed);
		stats.lastFailedMessage = "Couldn't read time from DS1302";
		DEBUG(stats.lastFailedMessage);
	}
}

void EspDS1302TimeSync::setTime(int hr, int min, int sec, int dy, int mnth, int yr) {
	struct tm now;
	now.tm_hour = hr;
	now.tm_min = min;
	now.tm_sec = sec;
	now.tm_mday = dy;
	now.tm_mon = mnth - 1;
	now.tm_year = yr - 1900;
	now.tm_isdst = -1;

	struct timeval timeVal;
	timeVal.tv_usec = 0;
	timeVal.tv_sec = ::mktime(&now);
	::settimeofday(&timeVal, NULL);

	_lastSyncFailed = false;
#ifdef ESP32
	xTaskNotify(syncTimeTask, RTC_WRITE, eSetBits);
#endif
	timeInitialized = true;
}

#endif /* !ARDUINO_ESP8266_RELEASE_2_3_0 */

