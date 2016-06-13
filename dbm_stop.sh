#!/bin/bash

ps -A|grep dbm_watchdog|awk '{print "kill -15 " $1}' > /tmp/kill_dbm
ps -A|grep "dbm_sql_check"|awk '{print "kill -15 " $1}' >> /tmp/kill_dbm 
ps -A|grep audit_|awk '{print "kill -15 " $1}'>> /tmp/kill_dbm
chmod 777 /tmp/kill_dbm
/tmp/kill_dbm


#pkill -15 dbm_watchdog
#pkill -15 audit_process
#pkill -15 audit_csp_release
#pkill -15 audit_sql_insert
#pkill -15 audit_runtime_database
#pkill -15 audit_runtime_redis

