/*
 * EspRTCTimeSync.h
 *
 *  Created on: Jul 27, 2020
 *      Author: mpand
 */

#ifndef LIBRARIES_TIMESYNC_ESPRTCTIMESYNC_H_
#define LIBRARIES_TIMESYNC_ESPRTCTIMESYNC_H_
#if defined(ESP8266_2_7_3) || defined(ESP32)
#include <RTCTimeSync.h>

class EspRTCTimeSync: public RTCTimeSync {
public:
	EspRTCTimeSync(int SDApin, int SCLpin) : RTCTimeSync(SDApin, SCLpin) {}
	virtual ~EspRTCTimeSync();
	virtual void setTime(int hr, int min, int sec, int dy, int mnth, int yr);

protected:
	virtual void setFromDS3231();
	virtual void setDS3231();
};

#endif /* ESP8266_2_7_3 */
#endif /* LIBRARIES_TIMESYNC_ESPRTCTIMESYNC_H_ */
