/*
 * DS1302TimeSync.cpp
 *
 *  Created on: Apr 7, 2024
 *      Author: mpand
 */

#include <Arduino.h>
#include <DS1302TimeSync.h>
#include <TimeLib.h>
#include <Wire.h>
#include <Arduino.h>
#include <stdio.h>

//#define DEBUG(...) { Serial.println(__VA_ARGS__); }
#ifndef DEBUG
#define DEBUG(...) {}
#endif

DS1302TimeSync *DS1302TimeSync::pTimeSync;
const int DS1302TimeSync::RTC_READ;
const int DS1302TimeSync::RTC_WRITE;
const int DS1302TimeSync::RTC_ENABLE;
const int DS1302TimeSync::RTC_DISABLE;

static byte decToBcd(byte val){
// Convert normal decimal numbers to binary coded decimal
  return ( (val/10*16) + (val%10) );
}

static byte bcdToDec(byte val)  {
// Convert binary coded decimal to normal decimal numbers
  return ( (val/16*10) + (val%16) );
}

static byte BcdToBin24Hour(byte bcdHour)
{
    byte hour;
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

void DS1302TimeSync::setDevice() {
	// set the date time
	tWire.beginTransmission(DS1302_REG_TIMEDATE_BURST);

	time_t _now = now();

	tWire.write(decToBcd(second(_now)));
	tWire.write(decToBcd(minute(_now)));
	tWire.write(decToBcd(hour(_now))); // 24 hour mode only
	tWire.write(decToBcd(day(_now)));
	tWire.write(decToBcd(month(_now)));
	tWire.write(decToBcd(dayOfWeek(_now)));
	tWire.write(decToBcd(year(_now) % 100));
	tWire.write(0); // no write protect, as all of this is ignored if it is protected

	tWire.endTransmission();
}

void DS1302TimeSync::setFromDevice() {
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
		::setTime(hour, minute, second, dayOfMonth, month, year + 2000);
		timeInitialized = true;
		_lastSyncFailed = false;
	} else {
		stats.failedCount = String(++numFailed);
		stats.lastFailedMessage = "Couldn't read time from DS1302";
		DEBUG(stats.lastFailedMessage);
	}

	// send it to the serial monitor
	DEBUG(hour, DEC);
	// convert the byte variable to a decimal number when displayed
	DEBUG(":");
	if (minute < 10) {
		DEBUG("0");
	}
	DEBUG(minute, DEC);
	DEBUG(":");
	if (second < 10) {
		DEBUG("0");
	}
	DEBUG(second, DEC);
	DEBUG(" ");
	DEBUG(dayOfMonth, DEC);
	DEBUG("/");
	DEBUG(month, DEC);
	DEBUG("/");
	DEBUG(year, DEC);
	DEBUG(" Day of week: ");
	switch (dayOfWeek) {
	case 1:
		DEBUG("Sunday");
		break;
	case 2:
		DEBUG("Monday");
		break;
	case 3:
		DEBUG("Tuesday");
		break;
	case 4:
		DEBUG("Wednesday");
		break;
	case 5:
		DEBUG("Thursday");
		break;
	case 6:
		DEBUG("Friday");
		break;
	case 7:
		DEBUG("Saturday");
		break;
	}
}

DS1302TimeSync::DS1302TimeSync(uint8_t ioPin, uint8_t clkPin, uint8_t cePin) : tWire(ioPin, clkPin, cePin) {
	pTimeSync = this;	// Yeuk
}

DS1302TimeSync::~DS1302TimeSync() {
	// TODO Auto-generated destructor stub
}

bool DS1302TimeSync::initialized() {
	return timeInitialized;
}

struct tm* DS1302TimeSync::getTimeWithTz(String tz, struct tm *pTm, suseconds_t *uSec) {
	return getLocalTime(pTm, uSec);
}

struct tm* DS1302TimeSync::getLocalTime(struct tm *pTm, suseconds_t *uSec) {
	time_t _now = ::now();
	if (pTm == NULL) {
		pTm = &cache;
	}

	pTm->tm_year = ::year(_now) % 100;
	pTm->tm_mon = ::month(_now);
	pTm->tm_mday = ::day(_now);
	pTm->tm_hour = ::hour(_now);
	pTm->tm_min = ::minute(_now);
	pTm->tm_sec = ::second(_now);
	pTm->tm_wday = ::weekday(_now);

	if (uSec != NULL) {
		*uSec = (::millis() % 1000) * 1000;
	}

	return pTm;
}

void DS1302TimeSync::setTz(String tz) {
}

void DS1302TimeSync::setTime(String s) {
	int intValues[6];
	grabInts(s, &intValues[0], ",");
    setTime(intValues[SYNC_HOURS], intValues[SYNC_MINS], intValues[SYNC_SECS], intValues[SYNC_DAY], intValues[SYNC_MONTH], intValues[SYNC_YEAR]);
}

void DS1302TimeSync::setTime(int hr, int min, int sec, int dy, int mnth, int yr) {
	::setTime(hr, min, sec, dy, mnth, yr);
	_lastSyncFailed = false;
#ifdef ESP32
	xTaskNotify(syncTimeTask, RTC_WRITE, eSetBits);
#endif
	timeInitialized = true;
}

TimeSync::SyncStats& DS1302TimeSync::getStats() {
	return stats;
}

#ifdef ESP8266
void DS1302TimeSync::init() {
	tWire.begin();
}

void DS1302TimeSync::enabled(bool flag) {
	_enabled = flag;
}

void DS1302TimeSync::sync() {
	if (_enabled) {
		DEBUG("Setting because READ")
		setFromDevice();
	}
}
#else
void DS1302TimeSync::init() {
    xTaskCreatePinnedToCore(
          syncTimeTaskFn, /* Function to implement the task */
          "RTC time task", /* Name of the task */
          4096,  /* Stack size in words */
          NULL,  /* Task input parameter */
		  tskIDLE_PRIORITY,  /* More than background tasks */
          &syncTimeTask,  /* Task handle. */
		  0
		  );
}

void DS1302TimeSync::enabled(bool flag) {
	xTaskNotify(syncTimeTask, flag ? RTC_ENABLE : RTC_DISABLE, eSetBits);
}

void DS1302TimeSync::sync() {
	xTaskNotify(syncTimeTask, RTC_READ, eSetBits);
}

// static method that calls instance method
void DS1302TimeSync::syncTimeTaskFn(void* pArg) {
	pTimeSync->taskFn(pArg);
}

// instance method
void DS1302TimeSync::taskFn(void* pArg) {
	tWire.begin();

	uint32_t waitValue = 0;	// First time through, wake immediately
	uint32_t notificationValue = 0;

	while (true) {
		while (xTaskNotifyWait(0xffffffff, 0xffffffff, &notificationValue, waitValue/portTICK_PERIOD_MS) == pdFALSE) {
			// The semaphore timed out, update time from RTC
			waitValue = 3600000;	// Delay at least 1 hour

			if (_enabled) {
				DEBUG("Setting after timeout")
				setFromDevice();
			}
		}

		// Take notification action, if any. Enable/disable always comes first
		if (notificationValue & RTC_DISABLE) {
			_enabled = false;
		}

		if (notificationValue & RTC_ENABLE) {
			_enabled = true;
		}

		if (notificationValue & RTC_READ) {
			if (_enabled) {
				DEBUG("Setting because READ")
				setFromDevice();
			}
		}

		if (notificationValue & RTC_WRITE) {
			setDevice();
		}
	}
}
#endif
