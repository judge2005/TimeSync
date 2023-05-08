/*
 * TimeServerTimeSync.cpp
 *
 *  Created on: Jul 23, 2020
 *      Author: mpand
 */

#include <TimeServerTimeSync.h>
#include <TimeLib.h>
#include <core_version.h>

#ifdef ESP8266
#ifdef ARDUINO_ESP8266_RELEASE_2_3_0
extern "C" {
#include "lwip/timers.h"
}
#else
extern "C" {
void sys_timeout_LWIP2(unsigned int, void (*)(void *), void *);
}
#endif
#include <ESP8266WiFi.h>
#endif

//#define DEBUG(...) { Serial.println(__VA_ARGS__); }
#define DEBUG(...) { }

#define TSTS_NOOP 0
#define TSTS_DISABLE 1
#define TSTS_ENABLE 2

TimeServerTimeSync *TimeServerTimeSync::pTimeSync;

TimeServerTimeSync::TimeServerTimeSync(String url, SetCb setCb, ErrorCb errorCb) {
	pTimeSync = this;	// Yeuk
	this->url = url;
	this->setCb = setCb;
	this->errorCb = errorCb;
}

TimeServerTimeSync::~TimeServerTimeSync() {
	// TODO Auto-generated destructor stub
}

void TimeServerTimeSync::enabled(bool flag) {

}

bool TimeServerTimeSync::initialized() {
	return timeInitialized;
}

struct tm* TimeServerTimeSync::getTimeWithTz(String tz, struct tm *pTm, suseconds_t *uSec) {
	return getLocalTime(pTm, uSec);
}

struct tm* TimeServerTimeSync::getLocalTime(struct tm *pTm, suseconds_t *uSec) {
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

void TimeServerTimeSync::setTz(String tz) {
	url = tz;
	httpClient.initialize(url);
}

void TimeServerTimeSync::setTime(String s) {
	int intValues[6];
	grabInts(s, &intValues[0], ",");
    setTime(intValues[SYNC_HOURS], intValues[SYNC_MINS], intValues[SYNC_SECS], intValues[SYNC_DAY], intValues[SYNC_MONTH], intValues[SYNC_YEAR]);
}

void TimeServerTimeSync::setTime(int hr, int min, int sec, int dy, int mnth, int yr) {
	::setTime(hr, min, sec, dy, mnth, yr);
	timeInitialized = true;
}

TimeSync::SyncStats& TimeServerTimeSync::getStats() {
	return stats;
}

// Only called from HTTPClientResponse
void TimeServerTimeSync::setTimeFromInternet() {
#ifdef ESP8266
	sys_timeout(3600000, syncTimeTaskFn, (void*)NULL);
#endif
	String body = httpClient.getBody();
	DEBUG(String("Got response ") + body);
	int intValues[6];
	grabInts(body, &intValues[0], ",");

	char _lastUpdateTime[24];

	sprintf(_lastUpdateTime, "%02d:%02d:%02d %4d-%02d-%02d",
			intValues[SYNC_HOURS],
			intValues[SYNC_MINS],
			intValues[SYNC_SECS],
			intValues[SYNC_YEAR],
			intValues[SYNC_MONTH],
			intValues[SYNC_DAY]
					  );

	_lastSyncFailed = false;
	stats.lastUpdateTime = _lastUpdateTime;
    setTime(intValues[SYNC_HOURS], intValues[SYNC_MINS], intValues[SYNC_SECS], intValues[SYNC_DAY], intValues[SYNC_MONTH], intValues[SYNC_YEAR]);
    if (setCb != NULL) {
    	setCb(body);
    }
}

void TimeServerTimeSync::setTimeFromInternetCb() {
	pTimeSync->setTimeFromInternet();
}

void TimeServerTimeSync::readTimeFailed(String msg) {
	_lastSyncFailed = true;
	stats.failedCount = String(++numFailed);
	stats.lastFailedMessage = msg;
	DEBUG(msg);
    if (errorCb != NULL) {
    	errorCb(msg);
    }

	// Try again in 10 seconds
#ifdef ESP32
	xTaskNotify(syncTimeTask, 10000, eSetValueWithOverwrite);
#else
	sys_timeout(10000, syncTimeTaskFn, NULL);
#endif
}

void TimeServerTimeSync::readTimeFailedCb(String msg) {
	pTimeSync->readTimeFailed(msg);
}

void TimeServerTimeSync::syncTimeTaskFn(void *pArg) {
	pTimeSync->taskFn(pArg);
}

#ifdef ESP8266
void TimeServerTimeSync::taskFn(void *pArg) {
	sync();
}

void TimeServerTimeSync::sync() {
    DEBUG(url);
    // There may already be a timeout queued, but not much we can do about that
	if (WiFi.status() == WL_CONNECTED) {
		httpClient.makeRequest(setTimeFromInternetCb, readTimeFailedCb);
	} else {
		sys_timeout(10000, syncTimeTaskFn, NULL);	// Try again in 10 seconds
	}
}

void TimeServerTimeSync::init() {
	httpClient.initialize(url);
	sync();
}
#endif

#ifdef ESP32
void TimeServerTimeSync::init() {
    xTaskCreatePinnedToCore(
          syncTimeTaskFn, /* Function to implement the task */
          "Sync time task", /* Name of the task */
          4096,  /* Stack size in words */
          NULL,  /* Task input parameter */
		  tskIDLE_PRIORITY,  /* More than background tasks */
          &syncTimeTask,  /* Task handle. */
		  0
		  );
}

void TimeServerTimeSync::taskFn(void *pArg) {
	static bool enabled = true;

	httpClient.initialize(url);

	uint32_t waitValue = 0;
	uint32_t notificationValue = 0;

	while (true) {
		while (xTaskNotifyWait(0xffffffff, 0xffffffff, &notificationValue, waitValue/portTICK_PERIOD_MS) == pdFALSE) {
			// The semaphore timed out, send the request

			waitValue = 600000;	// Default wait for 1 quarter
//			if (WiFi.status() == WL_CONNECTED) {
			if (enabled) {
				DEBUG("Getting time");
				httpClient.makeRequest(setTimeFromInternetCb, readTimeFailedCb);
			}
//			}
		}
		switch (notificationValue) {
		case TSTS_DISABLE:
			enabled = false;
			break;
		case TSTS_ENABLE:
			enabled = true;
			break;
		default:
			// The task was notified, so loop and sleep for the time requested
			break;
		}
	}
}

void TimeServerTimeSync::sync() {
	xTaskNotify(syncTimeTask, TSTS_NOOP, eSetValueWithOverwrite);
}


#endif
