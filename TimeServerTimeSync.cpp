/*
 * TimeServerTimeSync.cpp
 *
 *  Created on: Jul 23, 2020
 *      Author: mpand
 */

#include <TimeServerTimeSync.h>
#include <TimeLib.h>

TimeServerTimeSync *TimeServerTimeSync::pTimeSync;

// Only called from HTTPClientResponse
void TimeServerTimeSync::setTimeFromInternet() {
	String body = httpClient.getBody();
	DEBUG(String("Got response") + body);
	int intValues[6];
	grabInts(body, &intValues[0], ",");

	char _lastUpdateTime[24];

//	failedCount = 0;
	sprintf(_lastUpdateTime, "%02d:%02d:%02d %4d-%02d-%02d",
			intValues[SYNC_HOURS],
			intValues[SYNC_MINS],
			intValues[SYNC_SECS],
			intValues[SYNC_YEAR],
			intValues[SYNC_MONTH],
			intValues[SYNC_DAY]
					  );

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
	stats.failedCount = String(++numFailed);
	stats.lastFailedMessage = msg;
	DEBUG(msg);
    if (errorCb != NULL) {
    	errorCb(msg);
    }

	// Try again in 10 seconds
	xTaskNotify(syncTimeTask, 10000, eSetValueWithOverwrite);
}

void TimeServerTimeSync::readTimeFailedCb(String msg) {
	pTimeSync->readTimeFailed(msg);
}

TimeServerTimeSync::TimeServerTimeSync(String url, SetCb setCb, ErrorCb errorCb) {
	pTimeSync = this;	// Yeuk
	this->url = url;
	this->setCb = setCb;
	this->errorCb = errorCb;
}

TimeServerTimeSync::~TimeServerTimeSync() {
	// TODO Auto-generated destructor stub
}

bool TimeServerTimeSync::initialized() {
	return timeInitialized;
}

void TimeServerTimeSync::getTimeWithTz(String tz) {
}

void TimeServerTimeSync::getLocalTime() {
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

TimeSync::SyncStats& TimeServerTimeSync::getStats() {
	return stats;
}

void TimeServerTimeSync::taskFn(void *pArg) {
	httpClient.initialize(url);

	uint32_t waitValue = 0;
	while (true) {
		while (xTaskNotifyWait(0xffffffff, 0xffffffff, &waitValue, waitValue/portTICK_PERIOD_MS) == pdFALSE) {
			// The semaphore timed out, send the request

			waitValue = 600000;	// Default wait for 1 quarter
//			if (WiFi.status() == WL_CONNECTED) {
				DEBUG("Getting time");
				httpClient.makeRequest(setTimeFromInternetCb, readTimeFailedCb);
//			}
		}
		// The task was notified, so loop and sleep for the time requested
	}
}

void TimeServerTimeSync::syncTimeTaskFn(void *pArg) {
	pTimeSync->taskFn(pArg);
}

void TimeServerTimeSync::sync() {
	xTaskNotify(syncTimeTask, 0, eSetValueWithOverwrite);
}

#endif
