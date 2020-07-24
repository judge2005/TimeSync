/*
 * RTCTimeSync.h
 *
 *  Created on: Jul 23, 2020
 *      Author: mpand
 */

#ifndef LIBRARIES_TIMESYNC_RTCTIMESYNC_H_
#define LIBRARIES_TIMESYNC_RTCTIMESYNC_H_

#include <TimeSync.h>
#ifdef ESP32
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

class RTCTimeSync: public TimeSync {
public:
	RTCTimeSync(int SDApin, int SCLpin);
	virtual ~RTCTimeSync();

	virtual void init();
	virtual void sync();
	virtual bool initialized();

	virtual void getTimeWithTz(String tz);
	virtual void getLocalTime();
	virtual void setTz(String tz);

	virtual void setTime(String s);
	virtual void setTime(int hr,int min,int sec,int dy, int mnth, int yr);

	virtual TimeSync::SyncStats& getStats();

private:
	int SDApin;
	int SCLpin;

	bool timeInitialized = false;
	int numFailed = 0;
	SyncStats stats;

	void setFromDS3231();
	void setDS3231();

	static RTCTimeSync *pTimeSync;

#ifdef ESP32
	static void syncTimeTaskFn(void *pArg);

	void taskFn(void *pArg);
	TaskHandle_t syncTimeTask;
#endif
};

#endif /* LIBRARIES_TIMESYNC_RTCTIMESYNC_H_ */
