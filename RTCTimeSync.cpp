/*
 * RTCTimeSync.cpp
 *
 *  Created on: Jul 23, 2020
 *      Author: mpand
 */

#include <RTCTimeSync.h>
#include <TimeLib.h>
#include <Wire.h>
#include <Arduino.h>

#ifndef DEBUG
#define DEBUG(...) {}
#endif

RTCTimeSync *RTCTimeSync::pTimeSync;

#define DS3231_I2C_ADDRESS 0x68
#define RTC_READ 0
#define RTC_WRITE 1

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
			}

			// send it to the serial monitor
			Serial.print(hour, DEC);
			// convert the byte variable to a decimal number when displayed
			Serial.print(":");
			if (minute < 10) {
				Serial.print("0");
			}
			Serial.print(minute, DEC);
			Serial.print(":");
			if (second < 10) {
				Serial.print("0");
			}
			Serial.print(second, DEC);
			Serial.print(" ");
			Serial.print(dayOfMonth, DEC);
			Serial.print("/");
			Serial.print(month, DEC);
			Serial.print("/");
			Serial.print(year, DEC);
			Serial.print(" Day of week: ");
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

void RTCTimeSync::getTimeWithTz(String tz) {
}

void RTCTimeSync::getLocalTime() {
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
	xTaskNotify(syncTimeTask, RTC_WRITE, eSetValueWithOverwrite);
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

void RTCTimeSync::sync() {
	xTaskNotify(syncTimeTask, RTC_READ, eSetValueWithOverwrite);
}

void RTCTimeSync::taskFn(void* pArg) {
	pinMode(SDApin, OUTPUT);
	pinMode(SCLpin, OUTPUT);

	Wire.begin(SDApin, SCLpin);

	uint32_t waitValue = 0;	// First time through, wake immediately
	while (true) {
		while (xTaskNotifyWait(0xffffffff, 0xffffffff, &waitValue, waitValue/portTICK_PERIOD_MS) == pdFALSE) {
			// The semaphore timed out, update time from RTC
			setFromDS3231();
		}

		// Take notification action, if any
		switch (waitValue) {
		case RTC_READ:
			setFromDS3231();
			break;
		case RTC_WRITE:
			setDS3231();
			break;
		}

		waitValue = 3600000;	// Delay at least 1 hour
	}
}
#endif
