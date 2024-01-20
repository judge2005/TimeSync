/*
 * TimeServerTimeSync.h
 *
 *  Created on: Jul 23, 2020
 *      Author: mpand
 */

#ifndef LIBRARIES_TIMESYNC_TIMESERVERTIMESYNC_H_
#define LIBRARIES_TIMESYNC_TIMESERVERTIMESYNC_H_

#include <TimeSync.h>
#include "ESPAsyncHTTPClient.h"
#ifdef ESP32
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

class TimeServerTimeSync: public TimeSync {
public:
	TimeServerTimeSync(String url, SetCb setCb, ErrorCb errorCb);
	virtual ~TimeServerTimeSync();

	virtual void init();
	virtual void enabled(bool flag);
	virtual void sync();
	virtual bool initialized();

	virtual struct tm* getTimeWithTz(String tz, struct tm *pTm, suseconds_t *uSec);
	virtual struct tm* getLocalTime(struct tm *pTm, suseconds_t *uSec);
	virtual void setTz(String tz);

	virtual void setTime(String s);
	virtual void setTime(int hr,int min,int sec,int dy, int mnth, int yr);

	virtual TimeSync::SyncStats& getStats();

private:
	void setTimeFromInternet();
	void readTimeFailed(String msg);
	void taskFn(void *pArg);

	AsyncHTTPClient httpClient;
	bool timeInitialized = false;
	int numFailed = 0;
	SyncStats stats;
	String url;
	SetCb setCb = NULL;
	ErrorCb errorCb = NULL;

	struct tm cache;

	static TimeServerTimeSync *pTimeSync;

	static void setTimeFromInternetCb();
	static void readTimeFailedCb(String msg);
	static void syncTimeTaskFn(void *pArg);

#ifdef ESP32
	TaskHandle_t syncTimeTask;
#endif
};

#endif /* LIBRARIES_TIMESYNC_TIMESERVERTIMESYNC_H_ */
