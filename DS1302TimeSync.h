/*
 * DS1302TimeSync.h
 *
 *  Created on: Apr 7, 2024
 *      Author: mpand
 */

#ifndef LIBRARIES_TIMESYNC_DS1302TIMESYNC_H_
#define LIBRARIES_TIMESYNC_DS1302TIMESYNC_H_

#include <TimeSync.h>
#include <ThreeWire.h>
#ifdef ESP32
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

class DS1302TimeSync: public TimeSync {
public:
	DS1302TimeSync(uint8_t ioPin, uint8_t clkPin, uint8_t cePin);
	virtual ~DS1302TimeSync();

	virtual void init();
	virtual void enabled(bool flag);
	virtual void sync();
	virtual bool initialized();

	virtual struct tm* getTimeWithTz(String tz, struct tm *pTm, suseconds_t *uSec);
	virtual struct tm* getLocalTime(struct tm *pTm, suseconds_t *uSec);
	virtual void setTz(String tz);

	virtual void setTime(String s);
	virtual void setTime(int hr,int min,int sec,int dy, int mnth, int yr);

	virtual void setFromDevice();
	virtual void setDevice();

	virtual TimeSync::SyncStats& getStats();

protected:

	ThreeWire tWire;

	bool timeInitialized = false;
	bool _enabled = true;

	int numFailed = 0;
	SyncStats stats;
#ifdef ESP32
	TaskHandle_t syncTimeTask;
#endif

	static const uint8_t DS1302_REG_TIMEDATE_BURST = 0xBE;
	static const int RTC_READ = 1;
	static const int RTC_WRITE = 2;
	static const int RTC_ENABLE = 4;
	static const int RTC_DISABLE = 8;

private:

	struct tm cache;

	static DS1302TimeSync *pTimeSync;

#ifdef ESP32
	static void syncTimeTaskFn(void *pArg);

	void taskFn(void *pArg);
#endif
};

#endif /* LIBRARIES_TIMESYNC_DS1302TIMESYNC_H_ */
