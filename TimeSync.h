/*
 * TimeSync.h
 *
 *  Created on: Jul 23, 2020
 *      Author: mpand
 */

#ifndef LIBRARIES_TIMESYNC_TIMESYNC_H_
#define LIBRARIES_TIMESYNC_TIMESYNC_H_

#include <WString.h>

class TimeSync {
public:
	// time = "YYYY,MM,DD,HH,mm,SS[,ms]", e.g.: "2020,04,02,14,25,59"
	typedef void (*SetCb)(String time);
	typedef void (*ErrorCb)(String msg);

	struct SyncStats {
		String lastUpdateTime;
		String failedCount;
		String lastFailedMessage;
	};

	virtual ~TimeSync();
	TimeSync();

	virtual void init() = 0;		// Initialized service
	virtual bool initialized() = 0;	// Returns true if the time has ever been set
	virtual void sync() = 0;		// Force synchronization

	virtual void getTimeWithTz(String tz) = 0;
	virtual void getLocalTime() = 0;
	virtual void setTz(String tz) = 0;

	virtual void setTime(String s) = 0;
	virtual void setTime(int hr,int min,int sec,int dy, int mnth, int yr) = 0;	// Explicitly set the time

	virtual SyncStats& getStats() = 0;

protected:
	static const int SYNC_HOURS=3;
	static const int SYNC_MINS=4;
	static const int SYNC_SECS=5;
	static const int SYNC_DAY=2;
	static const int SYNC_MONTH=1;
	static const int SYNC_YEAR=0;

	static void grabInts(String s, int *dest, String sep);
};

#endif /* LIBRARIES_TIMESYNC_TIMESYNC_H_ */
