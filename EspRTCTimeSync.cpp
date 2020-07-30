/*
 * EspRTCTimeSync.cpp
 *
 *  Created on: Jul 27, 2020
 *      Author: mpand
 */

#if defined(ESP8266_2_7_3) || defined(ESP32)
#include <EspRTCTimeSync.h>
#include "time.h"
#include "sys/time.h"
#include <Wire.h>
#include <sntp_pt.h>
#include <stdio.h>

#ifndef DEBUG
#define DEBUG(...) {}
#endif

EspRTCTimeSync::~EspRTCTimeSync() {
	// TODO Auto-generated destructor stub
}

#define DS3231_I2C_ADDRESS 0x68

static uint8_t decToBcd(uint8_t val){
// Convert normal decimal numbers to binary coded decimal
  return ( (val/10*16) + (val%10) );
}

static uint8_t bcdToDec(uint8_t val)  {
// Convert binary coded decimal to normal decimal numbers
  return ( (val/16*10) + (val%16) );
}

void EspRTCTimeSync::setDS3231() {
	Wire.beginTransmission(DS3231_I2C_ADDRESS);
	if (Wire.endTransmission() == 0) {
		// sets time and date data to DS3231
		struct timeval _time;
		::gettimeofday(&_time, NULL);
		struct tm _now;
		::gmtime_r(&_time.tv_sec, &_now);

		DEBUG("Checking year");
		DEBUG(_now.tm_year);
		if (_now.tm_year >= 100) {
			DEBUG("Setting RTC");
			Wire.beginTransmission(DS3231_I2C_ADDRESS);
			Wire.write(0); // set next input to start at the seconds register
			Wire.write(decToBcd(_now.tm_sec)); // set seconds
			Wire.write(decToBcd(_now.tm_min)); // set minutes
			Wire.write(decToBcd(_now.tm_hour)); // set hours
			Wire.write(decToBcd(_now.tm_wday+1)); // set day of week (1=Sunday, 7=Saturday)
			Wire.write(decToBcd(_now.tm_mday)); // set date (1 to 31)
			Wire.write(decToBcd(_now.tm_mon+1)); // set month
			Wire.write(decToBcd(_now.tm_year % 100)); // set year (0 to 99)
			Wire.endTransmission();
			// Flip OSF bit. Set register to status
			Wire.beginTransmission(DS3231_I2C_ADDRESS);
			Wire.write(0x0F);  // set DS3231 register pointer to status
			Wire.endTransmission();

			// Read value
			Wire.requestFrom(DS3231_I2C_ADDRESS, 1);
			uint8_t status = Wire.read() & ~0x80; // flip OSF bit

			// Write to status
			Wire.beginTransmission(DS3231_I2C_ADDRESS);

			Wire.write(0x0F);
			Wire.write(status);
			Wire.endTransmission();
		}
	} else {
		stats.failedCount = String(++numFailed);
		stats.lastFailedMessage = "Couldn't find DS3231";
		DEBUG(stats.lastFailedMessage);
	}
}

void EspRTCTimeSync::setFromDS3231() {
	_lastSyncFailed = true;
	Wire.beginTransmission(DS3231_I2C_ADDRESS);
	if (Wire.endTransmission() == 0) {
		Wire.beginTransmission(DS3231_I2C_ADDRESS);
		Wire.write(0x0F);  // set DS3231 register pointer to status
		Wire.endTransmission();

		Wire.requestFrom(DS3231_I2C_ADDRESS, 1);
		uint8_t lostPower = Wire.read() >> 7;
		if (lostPower) {
			DEBUG("RTC lost power");
		}

		Wire.beginTransmission(DS3231_I2C_ADDRESS);
		Wire.write(0); // set DS3231 register pointer to 00h
		Wire.endTransmission();

		uint8_t readCount = Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
		if (readCount == 7) {
			// request seven uint8_ts of data from DS3231 starting from register 00h
			uint8_t second = bcdToDec(Wire.read() & 0x7f);
			uint8_t minute = bcdToDec(Wire.read());
			uint8_t hour = bcdToDec(Wire.read() & 0x3f);
			uint8_t dayOfWeek = bcdToDec(Wire.read());
			uint8_t dayOfMonth = bcdToDec(Wire.read());
			uint8_t month = bcdToDec(Wire.read());
			uint8_t year = bcdToDec(Wire.read());

			if (!lostPower) {
				DEBUG("Setting time from DS3231");
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
				char *curTZ = getenv("TZ");
				setenv("TZ", "UTC", 1);
				tzset();
				timeVal.tv_usec = 0;
				timeVal.tv_sec = ::mktime(&now);
				if (curTZ) {
					setenv("TZ", curTZ, 1);
				}

				::settimeofday(&timeVal, NULL);
				timeInitialized = true;
				_lastSyncFailed = false;
			}
		} else {
			char _msg[100];

			::sprintf(_msg, "Only read %d bytes from DS3231 - should have been 7\n", readCount);

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

void EspRTCTimeSync::setTime(int hr, int min, int sec, int dy, int mnth, int yr) {
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

#endif /* ESP8266_2_7_3 */

