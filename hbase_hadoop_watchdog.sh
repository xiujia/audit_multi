#!/bin/bash

hbase_path=/usr/local/hbase-0.98.10.1-hadoop1/bin/
hadoop_path=/usr/local/hadoop-1.2.1/bin/

hadoop_start="$hadoop_path"start-all.sh
hadoop_stop="$hadoop_path"stop-all.sh
hbase_start="$hbase_path"start-hbase.sh
hbase_stop="$hbase_path"stop-hbase.sh


#hmaster start hbase
#hregionserver start hbase
#namenode restart hadoop restart hbase
#datanode restart hadoop,restart hbase

check(){
	cmd0=`ps ax|grep -v grep|grep "org.apache.hadoop.hbase.regionserver.HRegionServer"|wc -l`
	cmd1=`ps ax|grep -v grep|grep "org.apache.hadoop.hbase.master.HMaster"|wc -l`
	cmd2=`ps ax|grep -v grep|grep "org.apache.hadoop.hdfs.server.datanode.DataNode"|wc -l`
	cmd3=`ps ax|grep -v grep|grep "org.apache.hadoop.hdfs.server.namenode.NameNode"|wc -l`
	
}


while true
do
	sleep 3
	check
	echo $cmd0
        echo $cmd1
        echo $cmd2
        echo $cmd3
#	echo "$time"

	if [ $cmd0 -eq 0 ];then
		time=`date`
		echo "hbase start at $time for regionserver." >> /data/data/hbase_hadoop_watchdog.log
		$hbase_start
		time=`date`
		echo "hbase start ok at $time ." >> /data/data/hbase_hadoop_watchdog.log
		continue
	fi

	if [ $cmd1 -eq 0 ];then
		time=`date`
		echo "hbase start at $time for HMaster." >> /data/data/hbase_hadoop_watchdog.log
		$hbase_start
		time=`date`
		echo "hbase start ok at $time ." >> /data/data/hbase_hadoop_watchdog.log
		continue
		
	fi


	
	if [ $cmd2 -eq 0 ];then
		time=`date`
		echo "hbase and hadoop restart at $time for DataNode." >> /data/data/hbase_hadoop_watchdog.log
		$hbase_stop
		$hadoop_stop
		$hadoop_start
		$hbase_start
		time=`date`
		echo "hbase and hadoop restart ok at $time ." >> /data/data/hbase_hadoop_watchdog.log
		continue
	fi


	if [ $cmd3 -eq 0 ];then
		time=`date`
		echo "hbase and hadoop restart at $time for NameNode." >> /data/data/hbase_hadoop_watchdog.log
		$hbase_stop
		$hadoop_stop
		$hadoop_start
		$hbase_start
		time=`date`
		echo "hbase and hadoop restart ok at $time." >> /data/data/hbase_hadoop_watchdog.log
		continue
	fi

	
done
