#!/bin/bash

backup_path=/data/data/db_monitor_zip/
mysqldump -uforceview -pforceview dbmonitor cache_monitor_data_$1 > "$backup_path"cache_monitor_data_$1.sql

cd $backup_path
tar cJf cache_monitor_data_$1.tar.xz  cache_monitor_data_$1.sql

rm -rf "$backup_path"cache_monitor_data_$1.sql

echo "use dbmonitor;DROP TABLE cache_monitor_data_$1" > droptable.tmp
mysql < droptable.tmp 2>/dev/null
rm -f droptable.tmp
