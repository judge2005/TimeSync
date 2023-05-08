/*
 * EspSNTPTimeSync.h
 *
 *  Created on: Jul 25, 2020
 *      Author: mpand
 */

#include <core_version.h>

#ifndef LIBRARIES_TIMESYNC_ESPSNTPTIMESYNC_H_
#define LIBRARIES_TIMESYNC_ESPSNTPTIMESYNC_H_

#if !defined(ARDUINO_ESP8266_RELEASE_2_3_0) || defined(ESP32)
#include <TimeSync.h>

class EspSNTPTimeSync: public TimeSync {
public:
	EspSNTPTimeSync(String timezone, SetCb setCb, ErrorCb errorCb);
	virtual ~EspSNTPTimeSync();

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
	void setFromDS3231();
	void setDS3231();

	void setTimeFromInternet();
	void readTimeFailed(String msg);

	bool timeInitialized = false;
	int numFailed = 0;
	SyncStats stats;
	String timezone;
	SetCb setCb = NULL;
	ErrorCb errorCb = NULL;

	struct tm cache;

	static EspSNTPTimeSync *pTimeSync;

	static void setTimeFromInternetCb();
	static void readTimeFailedCb(const char* msg);
};
#endif /* !ARDUINO_ESP8266_RELEASE_2_3_0 */

#endif /* LIBRARIES_TIMESYNC_ESPSNTPTIMESYNC_H_ */
