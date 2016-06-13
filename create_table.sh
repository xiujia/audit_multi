#!/bin/bash

tablename=cache_monitor_data
#time=`date '+%Y_%m'`
a="use dbmonitor;create table "
b="$tablename"_$1
c=`cat /usr/inp/cfg/table.db`
d="$a""$b""$c"
echo $d > tmp.sql
mysql < tmp.sql 2>/dev/null
rm -f tmp.sql




