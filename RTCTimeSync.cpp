/*
 * RTCTimeSync.cpp
 *
 *  Created on: Jul 23, 2020
 *      Author: mpand
 */

#include <Arduino.h>
#include <RTCTimeSync.h>
#include <TimeLib.h>
#include <Wire.h>
#include <Arduino.h>
#include <stdio.h>

//#define DEBUG(...) { Serial.println(__VA_ARGS__); }
#ifndef DEBUG
#define DEBUG(...) {}
#endif

RTCTimeSync *RTCTimeSync::pTimeSync;
const int RTCTimeSync::DS3231_I2C_ADDRESS;
const int RTCTimeSync::RTC_READ;
const int RTCTimeSync::RTC_WRITE;
const int RTCTimeSync::RTC_ENABLE;
const int RTCTimeSync::RTC_DISABLE;

static byte decToBcd(byte val){
// Convert normal decimal numbers to binary coded decimal
  return ( (val/10*16) + (val%10) );
}

static byte bcdToDec(byte val)  {
// Convert binary coded decimal to normal decimal numbers
  return ( (val/16*10) + (val%16) );
}

void RTCTimeSync::setDS3231() {
	Wire.beginTransmission(DS3231_I2C_ADDRESS);
	if (Wire.endTransmission() == 0) {
		// sets time and date data to DS3231
		time_t _now = now();

		Wire.beginTransmission(DS3231_I2C_ADDRESS);
		Wire.write(0); // set next input to start at the seconds register
		Wire.write(decToBcd(second(_now))); // set seconds
		Wire.write(decToBcd(minute(_now))); // set minutes
		Wire.write(decToBcd(hour(_now))); // set hours
		Wire.write(decToBcd(dayOfWeek(_now))); // set day of week (1=Sunday, 7=Saturday)
		Wire.write(decToBcd(day(_now))); // set date (1 to 31)
		Wire.write(decToBcd(month(_now))); // set month
		Wire.write(decToBcd(year(_now) % 100)); // set year (0 to 99)
		Wire.endTransmission();

		// Flip OSF bit. Set register to status
		Wire.beginTransmission(DS3231_I2C_ADDRESS);
		Wire.write(0x0F);  // set DS3231 register pointer to status
		Wire.endTransmission();

		// Read value
		Wire.requestFrom(DS3231_I2C_ADDRESS, 1);
		byte status = Wire.read() & ~0x80; // flip OSF bit

		// Write to status
		Wire.beginTransmission(DS3231_I2C_ADDRESS);

		Wire.write(0x0F);
		Wire.write(status);
		Wire.endTransmission();
	} else {
		stats.failedCount = String(++numFailed);
		stats.lastFailedMessage = "Couldn't find DS3231";
		DEBUG(stats.lastFailedMessage);
	}
}

void RTCTimeSync::setFromDS3231() {
	_lastSyncFailed = true;
	Wire.beginTransmission(DS3231_I2C_ADDRESS);
	if (Wire.endTransmission() == 0) {
		Wire.beginTransmission(DS3231_I2C_ADDRESS);
		Wire.write(0x0F);  // set DS3231 register pointer to status
		Wire.endTransmission();

		Wire.requestFrom(DS3231_I2C_ADDRESS, 1);
		byte lostPower = Wire.read() >> 7;
		if (lostPower) {
			DEBUG("RTC lost power");
		}

		Wire.beginTransmission(DS3231_I2C_ADDRESS);
		Wire.write(0); // set DS3231 register pointer to 00h
		Wire.endTransmission();

		byte readCount = Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
		if (readCount == 7) {
			// request seven bytes of data from DS3231 starting from register 00h
			byte second = bcdToDec(Wire.read() & 0x7f);
			byte minute = bcdToDec(Wire.read());
			byte hour = bcdToDec(Wire.read() & 0x3f);
			byte dayOfWeek = bcdToDec(Wire.read());
			byte dayOfMonth = bcdToDec(Wire.read());
			byte month = bcdToDec(Wire.read());
			byte year = bcdToDec(Wire.read());

			if (!lostPower) {
				DEBUG("Setting time from DS3231");
				::setTime(hour, minute, second, dayOfMonth, month, year + 2000);
				timeInitialized = true;
				_lastSyncFailed = false;
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
		} else {
			char _msg[100];

			sprintf(_msg, "Only read %d bytes from DS3231 - should have been 7\n", readCount);

			stats.failedCount = String(++numFailed);
			stats.lastFailedMessage = _msg;
			DEBUG(stats.lastFailedMessage);
		}
	} else {
		stats.failedCount = String(++numFailed);
		stats.lastFailedMessage = "Couldn't find DS3231";
		DEBUG(stats.lastFailedMessage);
	}
}

RTCTimeSync::RTCTimeSync(int SDApin, int SCLpin) {
	pTimeSync = this;	// Yeuk

	this->SDApin = SDApin;
	this->SCLpin = SCLpin;
}

RTCTimeSync::~RTCTimeSync() {
	// TODO Auto-generated destructor stub
}

bool RTCTimeSync::initialized() {
	return timeInitialized;
}

struct tm* RTCTimeSync::getTimeWithTz(String tz, struct tm *pTm, suseconds_t *uSec) {
	return getLocalTime(pTm, uSec);
}

struct tm* RTCTimeSync::getLocalTime(struct tm *pTm, suseconds_t *uSec) {
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
		*uSec = 0;
	}

	return pTm;
}

void RTCTimeSync::setTz(String tz) {
}

void RTCTimeSync::setTime(String s) {
	int intValues[6];
	grabInts(s, &intValues[0], ",");
    setTime(intValues[SYNC_HOURS], intValues[SYNC_MINS], intValues[SYNC_SECS], intValues[SYNC_DAY], intValues[SYNC_MONTH], intValues[SYNC_YEAR]);
}

void RTCTimeSync::setTime(int hr, int min, int sec, int dy, int mnth, int yr) {
	::setTime(hr, min, sec, dy, mnth, yr);
	_lastSyncFailed = false;
#ifdef ESP32
	xTaskNotify(syncTimeTask, RTC_WRITE, eSetBits);
#endif
	timeInitialized = true;
}

TimeSync::SyncStats& RTCTimeSync::getStats() {
	return stats;
}

#ifdef ESP32
void RTCTimeSync::syncTimeTaskFn(void* pArg) {
	pTimeSync->taskFn(pArg);
}

void RTCTimeSync::init() {
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

void RTCTimeSync::enabled(bool flag) {
	xTaskNotify(syncTimeTask, flag ? RTC_ENABLE : RTC_DISABLE, eSetBits);
}

void RTCTimeSync::sync() {
	xTaskNotify(syncTimeTask, RTC_READ, eSetBits);
}

void RTCTimeSync::taskFn(void* pArg) {
	static bool enabled = true;
	pinMode(SDApin, OUTPUT);
	pinMode(SCLpin, OUTPUT);

	Wire.begin(SDApin, SCLpin);

	uint32_t waitValue = 0;	// First time through, wake immediately
	uint32_t notificationValue = 0;

	while (true) {
		while (xTaskNotifyWait(0xffffffff, 0xffffffff, &notificationValue, waitValue/portTICK_PERIOD_MS) == pdFALSE) {
			// The semaphore timed out, update time from RTC
			waitValue = 3600000;	// Delay at least 1 hour

			if (enabled) {
				DEBUG("Setting after timeout")
				setFromDS3231();
			}
		}

		// Take notification action, if any. Enable/disable always comes first
		if (notificationValue & RTC_DISABLE) {
			enabled = false;
		}

		if (notificationValue & RTC_ENABLE) {
			enabled = true;
		}

		if (notificationValue & RTC_READ) {
			if (enabled) {
				DEBUG("Setting because READ")
				setFromDS3231();
			}
		}

		if (notificationValue & RTC_WRITE) {
			setDS3231();
		}
	}
}
#endif
