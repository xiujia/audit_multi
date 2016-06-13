#ifndef _AUDIT_TIME_API_H
#define _AUDIT_TIME_API_H
#include <sys/time.h>
#include <stdio.h>
#include <time.h>

#include "audit_api.h"

#define WEEKMIN 10080
typedef struct _runtime{
	 int year;
	 int month;
	 int day;
	 int hour;
	 int min;
}RUNTIME;

 void localtime_h(time_t time, struct tm* ret_time);
struct tm *dhcc_localtime(time_t time, long time_zone, struct tm *tm_time);
void log_time(struct tm *t_time);

void  get_audit_time(char *times);
void  get_audit_time_2(char *times);
void  get_audit_time_3(char *times);
u_int32_t  get_min();

u_int64_t get_usec_time();

extern RUNTIME loopTime;
extern const char days[12];



#endif
