//==============================================================================
//
//==============================================================================
#include <utility.h>
#include <ansi_c.h>
#include "TimeMarkers.h"

timeSchedule globalSchedule = {0}; 
char globalSharedTimeMarker[30];  

////////////////////////////////////////////////////////////////////// 
void releaseSchedule(void) {
	globalSchedule.numberOfRecords = 0;	
}

int addRecordToSchedule(int active,int immediate, unsigned int timeInterval, void (*func)(int)) {
	int i;
	if (globalSchedule.numberOfRecords >= MAX_RECORDS_NUMBER) return -1;
	if (!func) return -1;
	
	i = globalSchedule.numberOfRecords;
	globalSchedule.numberOfRecords++;
	globalSchedule.records[i].func = func;
	globalSchedule.records[i].timeInterval = timeInterval;
	
	if (active) {
		activateScheduleRecord(i,immediate);
	} else {
		globalSchedule.records[i].active = 0;	
	}
	
	return i;
}

int activateScheduleRecord(int record, int immediate) {
	
	if (record < 0 || record >= globalSchedule.numberOfRecords) return -1;
	
	if (immediate) {
		globalSchedule.records[record].timeActivated = 0;
	} else {
		if (time(&(globalSchedule.records[record].timeActivated)) == -1) {
			globalSchedule.records[record].active = 0;
			return -1;
		}
	}
	
	globalSchedule.records[record].active = 1;  
	return 0;
}

int resetScheduleRecord(int record) {
	
	if (record < 0 || record >= globalSchedule.numberOfRecords) return -1;  
	
	if (time(&(globalSchedule.records[record].timeActivated)) == -1) {
		globalSchedule.records[record].active = 0;
		return -1;
	}
	
	return 0;
}

int deactivateScheduleRecord(int record) {
	if (record < 0 || record >= globalSchedule.numberOfRecords) return -1;
	
	globalSchedule.records[record].active = 0;  
	return 0;	
}

void processScheduleEvents(void) {
	static int i;
	static time_t current_time;
	
	time(&current_time); 
	
	for (i=0; i<globalSchedule.numberOfRecords; i++) {
		if (globalSchedule.records[i].active) {
			if ( (current_time - globalSchedule.records[i].timeActivated) >= globalSchedule.records[i].timeInterval) {
				globalSchedule.records[i].func(i);
				globalSchedule.records[i].timeActivated = current_time;
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////

char * TimeStamp(char *time)
{
	int hours,minutes,seconds;
	int day,month,year;
	GetSystemTime(&hours,&minutes,&seconds);
	GetSystemDate(&month,&day,&year);
	if (time == NULL) time = globalSharedTimeMarker;
	sprintf(time,"[%02d.%02d.%02d - %02d:%02d:%02d]",day,month,year,hours,minutes,seconds);
	return time;
}
