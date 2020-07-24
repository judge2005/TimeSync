/*
 * TimeSync.cpp
 *
 *  Created on: Jul 23, 2020
 *      Author: mpand
 */

#include <TimeSync.h>

const int TimeSync::SYNC_HOURS;
const int TimeSync::SYNC_MINS;
const int TimeSync::SYNC_SECS;
const int TimeSync::SYNC_DAY;
const int TimeSync::SYNC_MONTH;
const int TimeSync::SYNC_YEAR;

void TimeSync::grabInts(String s, int *dest, String sep) {
	int end = 0;
	for (int start = 0; end != -1; start = end + 1) {
		end = s.indexOf(sep, start);
		if (end > 0) {
			*dest++ = s.substring(start, end).toInt();
		} else {
			*dest++ = s.substring(start).toInt();
		}
	}
}


TimeSync::~TimeSync() {
	// TODO Auto-generated destructor stub
}

TimeSync::TimeSync() {
	// TODO Auto-generated constructor stub

}

