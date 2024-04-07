/*
 * EspRTCTimeSync.h
 *
 *  Created on: Jul 27, 2020
 *      Author: mpand
 */
#include <core_version.h>

#ifndef LIBRARIES_TIMESYNC_ESPRTCTIMESYNC_H_
#define LIBRARIES_TIMESYNC_ESPRTCTIMESYNC_H_
#if !defined(ARDUINO_ESP8266_RELEASE_2_3_0) || defined(ESP32)
#include <RTCTimeSync.h>

class EspRTCTimeSync: public RTCTimeSync {
public:
	EspRTCTimeSync(int SDApin, int SCLpin) : RTCTimeSync(SDApin, SCLpin) {}
	virtual ~EspRTCTimeSync();
	virtual void setTime(int hr, int min, int sec, int dy, int mnth, int yr);

protected:
	virtual void setFromDevice();
	virtual void setDevice();
};

#endif /* !ARDUINO_ESP8266_RELEASE_2_3_0 */
#endif /* LIBRARIES_TIMESYNC_ESPRTCTIMESYNC_H_ */
