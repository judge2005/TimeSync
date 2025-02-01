/*
 * EspSNTPTimeSync.cpp
 *
 *  Created on: Jul 25, 2020
 *      Author: mpand
 */
#include <Arduino.h>
#include <EspSNTPTimeSync.h>
#if !defined(ARDUINO_ESP8266_RELEASE_2_3_0) || defined(ESP32)

#include <ESPPerfectTime.h>
#include <sys/time.h>
#include <lwip/tcpip.h>
#include <sntp_pt.h>
#include <stdio.h>

// https://github.com/espressif/arduino-esp32/issues/10526
#ifdef CONFIG_LWIP_TCPIP_CORE_LOCKING
  #define TCP_MUTEX_LOCK()                                \
    if (!sys_thread_tcpip(LWIP_CORE_LOCK_QUERY_HOLDER)) { \
      LOCK_TCPIP_CORE();                                  \
    }

  #define TCP_MUTEX_UNLOCK()                             \
    if (sys_thread_tcpip(LWIP_CORE_LOCK_QUERY_HOLDER)) { \
      UNLOCK_TCPIP_CORE();                               \
    }
#else // CONFIG_LWIP_TCPIP_CORE_LOCKING
  #define TCP_MUTEX_LOCK()
  #define TCP_MUTEX_UNLOCK()
#endif // CONFIG_LWIP_TCPIP_CORE_LOCKING


//#define DEBUG(...) { Serial.println(__VA_ARGS__); }
#ifndef DEBUG
#define DEBUG(...) { }
#endif
EspSNTPTimeSync *EspSNTPTimeSync::pTimeSync;

EspSNTPTimeSync::EspSNTPTimeSync(String timezone, SetCb setCb, ErrorCb errorCb) {
	pTimeSync = this;	// Yeuk
	this->timezone = timezone;
	this->setCb = setCb;
	this->errorCb = errorCb;
}

EspSNTPTimeSync::~EspSNTPTimeSync() {
}

void EspSNTPTimeSync::init() {
	pftime_sntp::setsynccallback(setTimeFromInternetCb);
	pftime_sntp::setfailcallback(readTimeFailedCb);

#ifdef ESP32
	TCP_MUTEX_LOCK();
	pftime::configTzTime(PSTR(timezone.c_str()), "pool.ntp.org");
	TCP_MUTEX_UNLOCK();
#else
	pftime::configTzTime(timezone.c_str(), "pool.ntp.org");
#endif
	// setenv leaks memory, so make it allocate a buffer and then update that instead
	// See https://github.com/espressif/esp-idf/issues/3046
	if (getenv("TZ") == NULL) {
		setenv("TZ", "012345678901234567890123456789012345678901234567890123456789", 1);
	}
	setTz(timezone);
}

void EspSNTPTimeSync::enabled(bool flag) {
	if (!flag) {
		pftime_sntp::stop();
	} else {
		init();
	}
}

void EspSNTPTimeSync::sync() {
}

bool EspSNTPTimeSync::initialized() {
	return timeInitialized;
}

struct tm* EspSNTPTimeSync::getTimeWithTz(String tz, struct tm* pTm, suseconds_t* uSec) {
	String curTZ = getenv("TZ");	// Copy contents of TZ string onto heap
	setTz(tz);				// Overwrite existing value
	struct tm* tm = getLocalTime(pTm, uSec);	// Get time
	setTz(curTZ);			// Copy old TZ into environment

	return tm;
}

struct tm* EspSNTPTimeSync::getLocalTime(struct tm* pTm, suseconds_t* uSec) {
	suseconds_t usec;
	struct tm *tm = pftime::localtime(nullptr, &usec);

//	DEBUG(tm->tm_year);

	if (pTm != NULL && tm != NULL) {
		*pTm = *tm;
	} else {
		pTm = tm;
	}

//	DEBUG(pTm->tm_year);
	pTm->tm_mon += 1;
	pTm->tm_year = pTm->tm_year % 100;
//	DEBUG(pTm->tm_year);
	if (uSec != NULL) {
		*uSec = usec;
	}

	return pTm;
}

/**
 * setenv() leaks like a sieve. So this is how you do it.
 * See https://github.com/espressif/esp-idf/issues/3046
 */
void EspSNTPTimeSync::setTz(String tz) {
	strcpy(getenv("TZ"), tz.c_str());
	tzset();
}

void EspSNTPTimeSync::setTime(String s) {
	int intValues[6];
	grabInts(s, &intValues[0], ",");
    setTime(intValues[SYNC_HOURS], intValues[SYNC_MINS], intValues[SYNC_SECS], intValues[SYNC_DAY], intValues[SYNC_MONTH], intValues[SYNC_YEAR]);
}

void EspSNTPTimeSync::setTime(int hr, int min, int sec, int dy, int mnth, int yr) {
	struct tm _time;
	_time.tm_hour = hr;
	_time.tm_min = min;
	_time.tm_sec = sec;
	_time.tm_mday = dy;
	_time.tm_mon = mnth - 1;
	_time.tm_year = yr - 1900;
	_time.tm_isdst = -1;

	timeval timeVal;
	timeVal.tv_usec = 0;
	timeVal.tv_sec = mktime(&_time);
	::settimeofday(&timeVal, NULL);

	timeInitialized = true;
}

TimeSync::SyncStats& EspSNTPTimeSync::getStats() {
	return stats;
}

void EspSNTPTimeSync::setTimeFromInternet() {
	timeInitialized = true;
	_lastSyncFailed = false;

	DEBUG(String("Got response"));

	char _lastUpdateTime[24];

	struct tm *_time = pftime::localtime(nullptr);

//	failedCount = 0;
	::sprintf(_lastUpdateTime, "%02d:%02d:%02d %4d-%02d-%02d",
			_time->tm_hour,
			_time->tm_min,
			_time->tm_sec,
			_time->tm_year + 1900,
			_time->tm_mon + 1,
			_time->tm_mday
					  );

	stats.lastUpdateTime = _lastUpdateTime;

	// time = "YYYY,MM,DD,HH,mm,SS[,ms]", e.g.: "2020,04,02,14,25,59"
	::sprintf(_lastUpdateTime, "%4d,%02d,%02d,%02d,%02d,%02d",
			_time->tm_year + 1900,
			_time->tm_mon + 1,
			_time->tm_mday,
			_time->tm_hour,
			_time->tm_min,
			_time->tm_sec
			);

    if (setCb != NULL) {
    	setCb(_lastUpdateTime);
    }
}

void EspSNTPTimeSync::setTimeFromInternetCb() {
	pTimeSync->setTimeFromInternet();
}

void EspSNTPTimeSync::readTimeFailed(String msg) {
	_lastSyncFailed = true;

	stats.failedCount = String(++numFailed);
	stats.lastFailedMessage = msg;
	DEBUG(msg);
    if (errorCb != NULL) {
    	errorCb(msg);
    }
}

void EspSNTPTimeSync::readTimeFailedCb(const char* msg) {
	pTimeSync->readTimeFailed(msg);
}

#endif /* !ARDUINO_ESP8266_RELEASE_2_3_0 */
