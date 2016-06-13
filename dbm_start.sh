#!/bin/bash

mkdir -p /media/data/sql
mkdir -p /media/data/sql_tmp
mkdir -p /media/data/sql_backup
mkdir -p /media/data/csp_tmp
mkdir -p /media/data/csp

/usr/local/bin/redis-server /etc/redis.conf

/usr/inp/bin/audit_redis_update 4
/usr/inp/bin/audit_redis_update 5
sleep 4
/usr/inp/bin/audit_mongo_insert 1
/usr/inp/bin/audit_runtime_redis

for (( i=0; i < 10; i++ ))
do
	/usr/inp/bin/audit_csp_rel $i 1
done

for (( i=0; i < 10; i++ ))
do
        /usr/inp/bin/audit_studio_rel $i 1
done

for (( i=0; i < 10; i++ ))
do
        /usr/inp/bin/audit_sqlserver_rel $i 1
done


/usr/inp/bin/dbm_watchdog &
