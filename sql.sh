#!/bin/bash

tablename=cache_monitor_data
time=`date '+%Y_%m'`
a="use dbmonitor;create table "
b="$tablename"_"$time"
c=`cat /usr/inp/cfg/dbtable.cfg`
d="$a""$b""$c"
echo $d > /usr/inp/bin/tmp.sql
mysql < /usr/inp/bin/tmp.sql
rm -f /usr/inp/bin/tmp.sql






