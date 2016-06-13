CC = gcc
CFLAGS = -Wall -O0 -g
LIBPATH = /usr/lib/mysql/
INCLUD =  /usr/include/mysql/
INSTALL = /usr/bin/install -c
BIN_DIR = /usr/inp/bin/
BUILD_DIR = ./build/
FLAGS = -lmysqlclient -lpthread -lhiredis  -lz
POLICY_FLAGS = -lmysqlclient -lpthread -lhiredis
MONGO_DIR=-I/usr/include/libmongoc-1.0/ -I/usr/include/libbson-1.0/
MONGO_LIB=-lsasl2 -lssl -lcrypto -ldl -lz -lrt -lmongoc-1.0 -lbson-1.0 -lpthread

.c.o:
	gcc -g -c main.c pthread_pool.c audit_oracle.c sync_ip_mac_redis.c redis_new_api.c audit_ip_mac_snmp.c audit_main_new.c sunday.c    audit_api.c  audit_lock.c audit_telnet.c audit_ftp.c audit_ftp_file.c  audit_time_api.c audit_studio.c op_db.c redis_api.c csp_policy.c his_portal_deal.c redis_api.c audit_time_api.c qsort.c gzip.c chunk.c csp_policy.c csp_audit_policy_match.c csp_policy_match.c redis_update.c redis_api.c csp_db.c audit_studio_main.c runtime_redis.c dbm_sql_check.c create_audit_policy_mem.c csp_audit_policy_set_process.c csp_audit_policy_add_process.c csp_audit_policy_del_process.c audit_ensemble_deal.c  oracle_dir_processing.c audit_sqlserver_main.c TDS_parser.c

PROG = pool sync_ip_mac_redis audit_snmp_construct audit_process create_audit_policy_mem dbm_sql_check  audit_redis_update  audit_mongo_insert audit_runtime_redis audit_policy_reload audit_policy_add audit_policy_del  oracle_dir_processing

#audit_process audit_sql_insert audit_csp_rel audit_redis_clear audit_redis_update audit_alarm_add audit_alarm_del audit_alarm_reload audit_runtime_database audit_runtime_redis audit_policy_reload audit_policy_add audit_policy_del audit_policy_clr audit_show dbm_sql_check
#cache1972_v50_20_processing cache1972_v52_21_processing cache1972_v2010_22_processing  cache1972_v2015_23_processing cache57772_40_processing http_100_processing

all: $(PROG)
#audit_sql_insert audit_csp_rel audit_redis_clear audit_redis_update audit_alarm_add audit_alarm_del audit_alarm_reload audit_runtime_database audit_runtime_redis audit_policy_reload audit_policy_add audit_policy_del audit_policy_clr audit_show dbm_sql_check


audit_process:audit_main_new.o sunday.o audit_oracle.o   audit_api.o  audit_lock.o audit_telnet.o audit_ftp.o audit_ftp_file.o  audit_time_api.o audit_studio.o op_db.o redis_api.o csp_policy.o
	$(CC) $(CFLAGS) $^ -o $@ $(FLAGS)
audit_sql_insert:audit_database_sql.o audit_time_api.o
	$(CC) $(CFLAGS) $^ -o $@ $(FLAGS)
#http_100_processing: his_portal_deal.o redis_api.o audit_time_api.o qsort.o gzip.o chunk.o csp_policy.o  csp_audit_policy_match.o redis_new_api.o
#	$(CC) $(CFLAGS) $^ -o $@ $(FLAGS)
#cache57772_40_processing: his_portal_deal.o redis_api.o audit_time_api.o qsort.o gzip.o chunk.o csp_policy.o csp_audit_policy_match.o redis_new_api.o
#	$(CC) $(CFLAGS) $^ -o $@ $(FLAGS)

audit_redis_clear:redis_clear.o redis_api.o
	$(CC) $(CFLAGS) $^ -o $@ $(FLAGS)
audit_redis_update:redis_update.o redis_api.o csp_db.o
	$(CC) $(CFLAGS) $^ -o $@ $(FLAGS)
audit_alarm_reload:csp_policy_set_process.o csp_db.o csp_policy.o
	$(CC) $(CFLAGS) $^ -o $@ $(POLICY_FLAGS)
audit_alarm_add:csp_policy_add_process.o csp_db.o csp_policy.o
	$(CC) $(CFLAGS) $^ -o $@ $(POLICY_FLAGS)
audit_alarm_del:csp_policy_del_process.o csp_db.o csp_policy.o
	$(CC) $(CFLAGS) $^ -o $@ $(POLICY_FLAGS)
audit_runtime_database:runtime_database_table.o audit_time_api.o
	$(CC) $(CFLAGS) $^ -o $@ $(FLAGS)
audit_runtime_redis:runtime_redis.o audit_time_api.o
	$(CC) $(CFLAGS) $^ -o $@ $(FLAGS)
audit_policy_reload:csp_audit_policy_set_process.o csp_db.o csp_policy.o
	$(CC) $(CFLAGS) $^ -o $@ $(POLICY_FLAGS)
audit_policy_add:csp_audit_policy_add_process.o csp_db.o csp_policy.o
	$(CC) $(CFLAGS) $^ -o $@ $(POLICY_FLAGS)
audit_policy_del:csp_audit_policy_del_process.o csp_db.o csp_policy.o
	$(CC) $(CFLAGS) $^ -o $@ $(POLICY_FLAGS)
audit_policy_clr:csp_audit_policy_clr_process.o csp_db.o csp_policy.o
	$(CC) $(CFLAGS) $^ -o $@ $(POLICY_FLAGS)
audit_show:audit_show.o
	$(CC) $(CFLAGS) $^ -o $@
dbm_sql_check:dbm_sql_check.o
	$(CC) $(CFLAGS) $^  -o $@
hbase:
	cd ./myhbase && make
#cache1972_v50_20_processing:redis_new_api.o audit_ensemble_deal.o audit_time_api.o  csp_policy.o csp_audit_policy_match.o
#	$(CC) $(CFLAGS) $^ -o $@ $(POLICY_FLAGS)
#cache1972_v52_21_processing:redis_new_api.o audit_ensemble_deal.o audit_time_api.o  csp_policy.o csp_audit_policy_match.o
#	$(CC) $(CFLAGS) $^ -o $@ $(POLICY_FLAGS)
#cache1972_v2010_22_processing:redis_new_api.o audit_ensemble_deal.o audit_time_api.o  csp_policy.o csp_audit_policy_match.o
#	$(CC) $(CFLAGS) $^ -o $@ $(POLICY_FLAGS)
#cache1972_v2015_23_processing:redis_new_api.o audit_ensemble_deal.o audit_time_api.o  csp_policy.o csp_audit_policy_match.o
#	$(CC) $(CFLAGS) $^ -o $@ $(POLICY_FLAGS)
pool:main.o his_portal_deal.o pthread_pool.o redis_new_api.o audit_ensemble_deal.o audit_time_api.o  csp_policy.o csp_audit_policy_match.o redis_api.o gzip.o chunk.o  qsort.o audit_sqlserver_main.o TDS_parser.o
	$(CC) $(CFLAGS) $^ -o $@ $(FLAGS)
#sqlserver1433_10_processing:audit_sqlserver_main.o csp_policy.o csp_audit_policy_match.o redis_new_api.o
#	$(CC) $(CFLAGS) $^ -o $@  $(POLICY_FLAGS)
audit_mongo_insert:audit_mongo_insert.c
	$(CC) $(CFLAGS) $(MONGO_DIR) $^ -o $@ $(MONGO_LIB)
create_audit_policy_mem:create_audit_policy_mem.o csp_policy.o
	$(CC) $(CFLAGS) $^ -o $@ $(POLICY_FLAGS)
audit_snmp_construct:audit_ip_mac_snmp.o op_db.o redis_new_api.o
	$(CC) $(CFLAGS) $^ -o $@ $(FLAGS)
sync_ip_mac_redis:sync_ip_mac_redis.o redis_new_api.o
	$(CC) $(CFLAGS) $^ -o $@ -lhiredis
#oracle1521_30_processing:oracle1521_30_processing.o redis_new_api.o  csp_policy.o csp_audit_policy_match.o
#	$(CC) $(CFLAGS) $^ -o $@ $(FLAGS)
#oracle_dir_processing:oracle_dir_processing.o
#	$(CC) $(CFLAGS) $^ -o $@
oracle_dir_processing:oracle_dir_processing.o  redis_new_api.o  csp_policy.o csp_audit_policy_match.o
	$(CC) $(CFLAGS) $^ -o $@ $(FLAGS)
clean:
	rm -f *.o
	rm -f $(PROG)
#	rm -f ./myhbase/audit_hbase_insert

install:
	$(INSTALL) $(PROG) stop_pool.sh start_pool.sh stop_mongo_watchdog.sh  start_mongo_watchdog.sh mongo_watchdog create_dir start_search_processing_dir.sh  stop_search_processing_dir.sh snmp_reload.sh get_snmp_file start_oracle_dir_processing.sh stop_oracle_dir_processing.sh  $(BIN_DIR)
install-build:
	$(INSTALL) $(PROG) stop_pool.sh start_pool.sh stop_mongo_watchdog.sh  start_mongo_watchdog.sh mongo_watchdog create_dir start_search_processing_dir.sh  stop_search_processing_dir.sh  snmp_reload.sh get_snmp_file  start_oracle_dir_processing.sh stop_oracle_dir_processing.sh $(BUILD_DIR)
