/*
 * EspDS1302TimeSync.h
 *
 *  Created on: Apr 7, 2024
 *      Author: mpand
 */
#include <core_version.h>

#ifndef LIBRARIES_TIMESYNC_ESPDS1302TimeSync_H_
#define LIBRARIES_TIMESYNC_ESPDS1302TimeSync_H_
#if !defined(ARDUINO_ESP8266_RELEASE_2_3_0) || defined(ESP32)
#include <DS1302TimeSync.h>

class EspDS1302TimeSync: public DS1302TimeSync {
public:
	EspDS1302TimeSync(uint8_t ioPin, uint8_t clkPin, uint8_t cePin) : DS1302TimeSync(ioPin, clkPin, cePin) {}
	virtual ~EspDS1302TimeSync();
	virtual void setTime(int hr, int min, int sec, int dy, int mnth, int yr);

protected:
	virtual void setFromDevice();
	virtual void setDevice();
};

#endif /* !ARDUINO_ESP8266_RELEASE_2_3_0 */
#endif /* LIBRARIES_TIMESYNC_ESPDS1302TimeSync_H_ */
