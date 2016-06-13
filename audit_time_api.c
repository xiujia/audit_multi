#include "audit_time_api.h"



const char days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};



void localtime_h(time_t time, struct tm* ret_time)
{
    static const char month_days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    static const int leap_year[4] = {0, 0, 1, 0};

    unsigned int leave_for_fouryear = 0;
    unsigned short four_year_count = 0;
    unsigned int temp_value = 0;

    ret_time->tm_sec = time % 60;
    temp_value = time / 60;// 分钟
    ret_time->tm_min = temp_value % 60;
    temp_value /= 60; // 小时

    temp_value += 8;// 加上时区

    ret_time->tm_hour = temp_value % 24;
    temp_value /= 24; // 天

    ret_time->tm_wday = (temp_value + 4) % 7;// 1970-1-1是4
    
    four_year_count = temp_value / (365 * 4 + 1);
    leave_for_fouryear = temp_value % (365 * 4 + 1);
    int leave_for_year_days = leave_for_fouryear;
    
    int day_count = 0;
    int i = 0;

    for (i = 0; i < 4; i++)
    {        
        day_count = leap_year[i] ? 366 : 365;

        if (leave_for_year_days <= day_count)
        {
            break;
        }
        else
        {
            leave_for_year_days -= day_count;
        }
    }

    ret_time->tm_year = four_year_count * 4 + i + 70;
    ret_time->tm_yday = leave_for_year_days;// 这里不是天数，而是标记，从0开始

    int leave_for_month_days = leave_for_year_days;

    int j = 0;
    for (j = 0; j < 12; j++)
    {
        if (leap_year[i] && j == 1)
        {
            if (leave_for_month_days <= 29)
            {
                break;
            }
            else if (leave_for_month_days == 29)
            {
                i++;
                leave_for_month_days = 0;
                break;
            }
            else
            {
                leave_for_month_days -= 29;
            }
            
            continue;    
        }
                
        if (leave_for_month_days < month_days[j])
        {
            break;
        }
        else if(leave_for_month_days == month_days[j]){
            i++;
            leave_for_month_days = 0;
            break;
        }
        else
        {
            leave_for_month_days -= month_days[j];
        }                
    }

    ret_time->tm_mday = leave_for_month_days + 1;
    ret_time->tm_mon = j;
}


struct tm *dhcc_localtime(time_t time, long time_zone, struct tm *tm_time)
{
	unsigned  int pass_4year, hour_per_year;

	time = time + time_zone*60*60;

	if(time < 0)
	{
	   time = 0;
	}

	tm_time->tm_sec=(int)(time % 60);
	time /= 60;
	tm_time->tm_min=(int)(time % 60);
	time /= 60;
	pass_4year=((unsigned int)time / (1461 * 24));
	tm_time->tm_year=(pass_4year << 2) + 70; 
	time %= 1461 * 24;

	for (;;)
	{
	    hour_per_year = 365 * 24;
	    
	    if ((tm_time->tm_year & 3) == 0)
		{
	        hour_per_year += 24;
		}
	    if (time < hour_per_year)
		{
	        break;
		}
	    tm_time->tm_year++;
	    time -= hour_per_year;
	}
  
	tm_time->tm_hour=(int)(time % 24);
	time /= 24;
	time++;
	if ((tm_time->tm_year & 3) == 0)
	{
		if (time > 60)
		{
			time--;
		}
		else
		{
			if (time == 60)
			{
				tm_time->tm_mon = 1;
				tm_time->tm_mday = 29;
			}
		}
	}
	for (tm_time->tm_mon = 0; days[tm_time->tm_mon] < time; tm_time->tm_mon++)
	{
	      time -= days[tm_time->tm_mon];
	}
	tm_time->tm_mday = (int)(time);	

    	return tm_time;
}

void log_time(struct tm *t_time)
{
	if((t_time->tm_hour) >=24) {
		t_time->tm_mday += 1;
		t_time->tm_hour -= 24; 
	}

	switch(t_time->tm_mon) {
		case 0 ... 10:
			if(t_time->tm_mday > days[t_time->tm_mon]) {
				t_time->tm_mon += 1;
				t_time->tm_mday = 1;
			}

		case 11:
			{
				if(t_time->tm_mday > days[t_time->tm_mon]) {
					t_time->tm_mon = (t_time->tm_mon) - 11;
					t_time->tm_mday = (t_time->tm_mday) - (days[t_time->tm_mon]);
					t_time->tm_year += 1;
				}
			}

		default:
			break;				
	}
}

void  get_audit_time(char *times){
	
	time_t now;
	struct tm timenow;
	memset(times,0,AUDIT_TIME_LEN);
	time(&now);
	dhcc_localtime(now,8,&timenow);
	log_time(&timenow);	
	sprintf(times,"%04d-%02d-%02d %02d:%02d:%02d",timenow.tm_year+1900,timenow.tm_mon+1,timenow.tm_mday,timenow.tm_hour,timenow.tm_min,timenow.tm_sec);
	
}
void get_audit_time_2(char *times){
	time_t now;
        struct tm  timenow;
        memset(times,0,AUDIT_TIME_LEN);
        time(&now);
        dhcc_localtime(now,8,&timenow);
	log_time(&timenow);
        sprintf(times,"%04d%02d%02d%02d%02d%02d",timenow.tm_year+1900,timenow.tm_mon+1,timenow.tm_mday,timenow.tm_hour,timenow.tm_min,timenow.tm_sec);
}
void get_audit_time_3(char *times){
	time_t now;
        struct tm  timenow;
        memset(times,0,AUDIT_TIME_LEN);
        time(&now);
        dhcc_localtime(now,8,&timenow);
	    log_time(&timenow);
        sprintf(times,"%04d_%02d",timenow.tm_year+1900,timenow.tm_mon+1);
}
u_int32_t  get_min(){
	time_t now;
	struct tm time_now;
	int wd;
	time(&now);
	memset(&time_now,0,sizeof(struct tm));
	localtime_h(now,&time_now);
	if(time_now.tm_wday == 0)
		wd = 7;
	else 
		wd = time_now.tm_wday;

	return (wd-1)*24*60+time_now.tm_hour*60+time_now.tm_min;

}
 int get_run_time(RUNTIME * t){
	time_t now,tt,tmp;
        struct tm  timenow;

        time(&now);
	tt = now;
	dhcc_localtime(now,8,&timenow);
	log_time(&timenow);
	t->year = timenow.tm_year;
	t->month = timenow.tm_mon;
	t->day = timenow.tm_mday;
	t->hour = timenow.tm_hour;
	t->min = timenow.tm_min;
	return ;
}

 u_int64_t get_usec_time(){
	struct timeval tv;
	struct timezone tz;

	gettimeofday(&tv,NULL);

	return tv.tv_sec*1000000+tv.tv_usec;
 }