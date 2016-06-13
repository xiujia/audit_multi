
#include <boost/timer.hpp>
#include <boost/progress.hpp>
#include <boost/lexical_cast.hpp>


#include <sys/types.h>
#include <dirent.h>
#include <sys/time.h>
#include <unistd.h>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include "HbaseOperate.hpp"
#include "HbaseFileOperate.hpp"



using namespace std;
using namespace boost;

#define THREAD_NUM	10
int NC_daemon_audit(void)
{  
	pid_t pid;
	int ret;
	if ((pid = fork()) < 0){
		return -1;
	}
	else if (pid != 0){
		exit(0); /* parent goes bye-bye */
	}
	if((ret=setsid()) < 0) /* become session leader */
	{
		printf("unable to setsid.\n");
	}
	 setpgrp();
	 return 0;
}


#if 1
int
main(int argc,char ** argv){
	NC_daemon_audit();

	string FIlename;
	CHbaseOperate hbase;
	CHbaseFileOperate hbasefile;
	HbaseFileRet ret;
	int count = 0,thNum = 0;
	DIR *dp;
	 struct dirent *dirp;
	 char dirname[]="/data/audit/sql/";
	string backUpPath = "";
	thNum = THREAD_NUM;
	if(argc > 1)
		thNum = atoi(argv[1]);
		   //store file names
	 vector<string> file_names;
		   map < Text,Text > attributesaram;
		 if(!hbase.connect("localhost",9090)){
			   return 0;
		  }

		   
	while(1){
		
		if((dp=opendir(dirname))==NULL){
			return 0;
		}
		file_names.clear();
		while((dirp=readdir(dp))!=NULL){
			  if((strcmp(dirp->d_name,".")==0)||(strcmp(dirp->d_name,"..")==0)){
					   continue;
			  }
			count++;  
			file_names.push_back(dirp->d_name);
			if(count == 5000){
				break;
			}
		}

		closedir(dp);

		if(file_names.empty()){
			usleep(10);	
			continue;
		}
		
		

		for(vector<string>::iterator it= file_names.begin(); it != file_names.end();++it ){
			attributesaram.clear();
			FIlename.clear();
			FIlename = *it;
		//	cout << "filename:"<< FIlename << endl;
			if(!hbasefile.open(dirname+FIlename)){
				continue;
			}

			if(hbasefile.read()){
				hbasefile.GetKeyValues(&ret,&hbase,attributesaram);
			}

		//	hbasefile.printRow();
		//	hbase.printRow();
			
			hbasefile.close();
	//		hbasefile.mvfile(backUpPath);
			hbasefile.delelefile();

			
		}

		usleep(1000);	
//	break;
	}
	hbase.disconnect();
//	hbasefile.~CHbaseFileOperate();
//	hbase.~CHbaseOperate();
}

#endif


#if 0
int
main(int argc,char ** argv){
	int i=0,num = 0 ;
	string strsec,strusec,strrowkey,strmax;
	unsigned long usecs;
	unsigned long start,end;
	map < Text,Text > attributesaram;
	unsigned long max = (unsigned long)-1;

	num = lexical_cast<int>(argv[1]);
	struct timeval tv;
	struct timezone tz;


	CHbaseOperate hbase;
	vector < Mutation > mutations;
	if(!hbase.connect("localhost",9090)){
		return 0;
	}
		gettimeofday(&tv,NULL);
		start = tv.tv_sec*1000000+tv.tv_usec;
		for(i = 0;i<num;i++){
			gettimeofday(&tv,NULL);
			usecs = tv.tv_sec*1000000 + tv.tv_usec;
			usecs = max/2 - usecs;
			
			
		//	strmax =  lexical_cast<string>(max);
			strrowkey =  lexical_cast<string>(usecs);
			
			 mutations.clear();
		     	 mutations.push_back(Mutation());
			 mutations.back().column = "colfam1:alarm_id";
		     	 mutations.back().value = "";

			 mutations.push_back(Mutation());
			 mutations.back().column = "colfam1:app_id";
		     	 mutations.back().value = "1";

			mutations.push_back(Mutation());
			mutations.back().column = "colfam1:department";
			mutations.back().value = "this is department!";

				mutations.push_back(Mutation());
			mutations.back().column = "colfam1:dst_ip";
			mutations.back().value = "2.2.2.2";

				mutations.push_back(Mutation());
			mutations.back().column = "colfam1:dst_mac";
			mutations.back().value = "11-22-33-44-55-66";
			
				mutations.push_back(Mutation());
			mutations.back().column = "colfam1:dst_port";
			mutations.back().value = "80";

				mutations.push_back(Mutation());
			mutations.back().column = "colfam1:level_1";
			mutations.back().value = "this is level 1";
					mutations.push_back(Mutation());
			mutations.back().column = "colfam1:level_2";
			mutations.back().value = "this is level 2";
					mutations.push_back(Mutation());
			mutations.back().column = "colfam1:level_3";
			mutations.back().value = "this is level 3";
					mutations.push_back(Mutation());
			mutations.back().column = "colfam1:operation_command";
			mutations.back().value = "this is operation_command!";
					mutations.push_back(Mutation());
			mutations.back().column = "colfam1:response_content";
			mutations.back().value = "this is response_content";
						mutations.push_back(Mutation());
			mutations.back().column = "colfam1:src_ip";
			mutations.back().value = "1.1.1.1";
						mutations.push_back(Mutation());
			mutations.back().column = "colfam1:src_port";
			mutations.back().value = "20020";
						mutations.push_back(Mutation());
			mutations.back().column = "colfam1:src_mac";
			mutations.back().value = "00-22-32-ff-22-44";

			hbase.putRow(HBASE_TABLE_NAME,strrowkey,mutations,attributesaram);
		}
		gettimeofday(&tv,NULL);
		end = tv.tv_sec*1000000+tv.tv_usec;

		cout << end-start << endl;
	hbase.disconnect();
	hbase.~CHbaseOperate();
 }
#endif
