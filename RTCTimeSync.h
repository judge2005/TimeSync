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
	virtual void enabled(bool flag);
	virtual void sync();
	virtual bool initialized();

	virtual struct tm* getTimeWithTz(String tz, struct tm *pTm, suseconds_t *uSec);
	virtual struct tm* getLocalTime(struct tm *pTm, suseconds_t *uSec);
	virtual void setTz(String tz);

	virtual void setTime(String s);
	virtual void setTime(int hr,int min,int sec,int dy, int mnth, int yr);

	virtual TimeSync::SyncStats& getStats();

protected:
	virtual void setFromDS3231();
	virtual void setDS3231();

	bool timeInitialized = false;

	int numFailed = 0;
	SyncStats stats;
#ifdef ESP32
	TaskHandle_t syncTimeTask;
#endif

	static const int DS3231_I2C_ADDRESS = 0x68;
	static const int RTC_READ = 1;
	static const int RTC_WRITE = 2;
	static const int RTC_ENABLE = 4;
	static const int RTC_DISABLE = 8;

private:
	int SDApin;
	int SCLpin;

	struct tm cache;

	static RTCTimeSync *pTimeSync;

#ifdef ESP32
	static void syncTimeTaskFn(void *pArg);

	void taskFn(void *pArg);
#endif
};

#endif /* LIBRARIES_TIMESYNC_RTCTIMESYNC_H_ */
