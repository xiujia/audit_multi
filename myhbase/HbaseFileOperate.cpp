#include "HbaseFileOperate.hpp"

#include "HbaseOperate.hpp"
//#include "log4cxx/log4cxx.h"
//#include "log4cxx/propertyconfigurator.h"
#include "Hbase.h"
#include "Hbase_types.h"



using namespace std;
using namespace boost;

CHbaseFileOperate::CHbaseFileOperate():
in(NULL),isOpened(false),data(NULL),rows(0),allrows(0)
{
}

CHbaseFileOperate::~CHbaseFileOperate(){
	if(isOpened){
		close();
	}
	if(data != NULL){
		delete [] data;
		data = NULL;
	}
}

bool CHbaseFileOperate::open(){
	if(isOpened){
		return  true;
	}
	  try {
		in.open(FileName.c_str(),std::ios::binary);
		if(!in){
			return false;
		}
	  } catch (const TException &tx) {
//    LOG4CXX_ERROR(logger, "Connect Hbase error : " << tx.what());
    		return false;
  	}

	  isOpened = true;
	  return isOpened;
}

bool CHbaseFileOperate::open(const string &name){
	FileName  =  name;

	return open();
}
bool CHbaseFileOperate::read(){
	rows = 0;
	if(in){
		in.clear();
		ss.clear();
		ss.str("");
		FileContent.clear();
		ss << in.rdbuf();
//		cout << ss.str().size() <<endl;
		FileContent = ss.str();
	//	cout << "content:"<<FileContent << endl;
		if(FileContent.empty()){
			return false;
		}
		return true;
	}
	else{
		return false;
	}
}
bool CHbaseFileOperate::close(){
	if(!isOpened){
		return false;
	}
	in.clear();
	in.close();
	isOpened= false;
	return true;
}


bool CHbaseFileOperate:: GetFileString(string name,HbaseFileRet *ret,CHbaseOperate *hbase,const  map<Text,Text> &attributes){

}

bool CHbaseFileOperate::LinePro(char * str,HbaseFileRet * ret){
	char *pos=NULL,*start=NULL,*end=NULL,*cpos=NULL,*kvpos=NULL;
	char keyval[LINE_LEN];
	int len = 0;
	string tmpstring;
	int count = 0;
	pos=end=start = str;

//	cout << "linepro.str:"<< str << endl;

	while((end = strstr(pos,"|colfam"))){
	//	end += 1;
		len = end -pos;
		end += 1;
		if(len < 0) {
			return false;
		}
		if(len == 0){
			continue;
		}
		memset(keyval,0,LINE_LEN);
		strncpy(keyval,pos,len);

//		cout << "linepro.keyval :" << keyval << endl;

		kvpos = keyval;
		cpos = strstr(kvpos,"=");
		if(count > 1)
			ret->m.push_back(Mutation());
		if(cpos){

			*cpos = '\0';
			tmpstring = kvpos;

			if(count > 1)
				ret->m.back().column = tmpstring;

			kvpos = cpos+1;
			tmpstring = kvpos;

			if(count > 1){
				ret->m.back().value = tmpstring;
			}
			else if(count == 0){
				ret->rowkey = tmpstring;
			}
			else{
				ret->table = tmpstring;
			}
		}
		else{
			return false;
		}
		count++;
		cpos = NULL;
		kvpos=NULL;
		pos = end;
		if(*pos == '\n'){
			break;
		}
	}
	return true;
}

bool CHbaseFileOperate:: GetKeyValues(HbaseFileRet *ret,CHbaseOperate *hbase,const map<Text,Text> &attributes){
	string::size_type ContentLen;
	char *pos=NULL,*start=NULL,*end=NULL;
	char line[LINE_LEN],key[KEY_LEN],value[VALUE_LEN];
	int LineLen=0,tmpLen=0,readLen=0;
	int operate_len = 0,i = 0;
	string table,row,col,val;
	int type=0;

	ContentLen = FileContent.size();

	//cout << "len: "<< ContentLen << endl;

	data = new char[ContentLen+1];
	memcpy(data,FileContent.c_str(),ContentLen);
	data[ContentLen]='\0';

//	cout<<"data: "<< data << endl;

	memset(line , 0 ,LINE_LEN);
	memset(key,0,KEY_LEN);
	memset(value,0,VALUE_LEN);

	pos = start = data;


	while((end = strstr(pos,"|colfam\n"))){
		end +=8;
		tmpLen =  end - pos;
		LineLen = tmpLen;
		if(LineLen > 0)
	//	memset(line,0,LINE_LEN);
		memcpy(line,pos,LineLen);
		line[LineLen]='\0';
		// Line to be process
		LinePro(line,ret);

//		cout <<"line: " << line<< endl;

		//get last element of vector 'values'
//		vector<string>::iterator it = ret->values.end()-1;
		vector<Mutation>::iterator it = ret->m.end()-1;
		operate_len = atoi(it->value.c_str());

//	 	cout<<"operate_len: "<<operate_len<< endl;

		pos = end;
		tmpLen = end + operate_len + 1- start;

//		cout <<"tmplen:"<< tmpLen << endl;

		if(tmpLen != ContentLen){
			if(strncmp(end + operate_len + 1,"rowkey=",7)!=0){

				break;
			}
		}
		if(operate_len > 0){
			char *operate_content = new char[operate_len+1];
			memcpy(operate_content,pos,operate_len);
			//reset last element
			operate_content[operate_len]='\0';
//			cout << "operate_content:"<<operate_content<<endl;

			it->value.assign(operate_content,operate_len);
	//		cout << it->value.size()<<endl;
			delete [] operate_content;
			operate_content = NULL;
		}

		//putrow

		row = ret->rowkey;
	//	makeRow(row);
		table = HBASE_TABLE_NAME;
		table = table + ret->table;
		rows++;
		row = row + "-00";
		row = row + ret->m[0].value;
		type = atoi(ret->m[0].value.c_str());
		if (type<6) {
			if(!row.empty()) {
				bool retval1 = hbase->putRow(table,row,ret->m,attributes);
				if (retval1) cout<<"cache success^^^^^^^^^^^^^^^^^^^"<<endl;
				else cout<<"cache failed^^^^^^^^^^^^^^^^^^^"<<endl;
			}
		}
		if(type >= 6){
		//	row = ret->rowkey;
			table = "sqlserver_monitor_data_";
			table = table + ret->table;
		//	row = row+"-00";
		//	row = row + ret->m[0].value;
			if(!row.empty()) {
				bool retval2 = hbase->putRow(table,row,ret->m,attributes);
				if (retval2) cout<<"sqlserver success^^^^^^^^^^^^^^^^^^^"<<endl;
				else cout<<"sqlserver failed^^^^^^^^^^^^^^^^^^^"<<endl;
			}
		}

        /* Portal/telnetÔÙ²åÒ»´Î */
		if(type > 2 && type<6){
		//	row = ret->rowkey;
			table = HBASE_TABLE_PORTAL_TELNET;
			table = table + ret->table;
		//	row = row+"-00";
		//	row = row + ret->m[0].value;
			if(!row.empty())
				hbase->putRow(table,row,ret->m,attributes);
		}



		ret->m.clear();


		end += operate_len;
		end += 1;

		pos = end;
		readLen = pos -start;

		if(readLen >= ContentLen){

			break;
		}
	}
	allrows +=rows;
//	cout << "readLen:"<<readLen<<endl;
	delete [] data;
	data = NULL;
}


	bool CHbaseFileOperate::delelefile(){
	//	string path;
		string cmd = "rm -rf ";

		if(FileName.empty()) {
			return false;

		}
	//	path = FileName;

		cmd = cmd+FileName;
//		cout <<"cmd:"<<cmd<<endl;
		system(cmd.c_str());

		return true;
	}

unsigned long  CHbaseFileOperate::getRowNums(){
	return rows;
}
void CHbaseFileOperate:: printRow(){
//	cout << rows << " rows this file insert. \n";
	if(allrows%1000 == 0){
	cout << rows << " rows this file insert. \n";
	cout <<allrows<<" rows is insert for all.\n";
	}

}

bool CHbaseFileOperate::mvfile(const string &dpath) const{
	string cmd = "mv -f  ";
	string path = " /data/sql_backup";
	if(!dpath.empty()){
		path = dpath;
	}
	if(FileName.empty()) {
			return false;

	}
	cmd = cmd+FileName+path;

	cout <<"cmd:"<<cmd<<endl;
	system(cmd.c_str());

	return true;
}

unsigned long CHbaseFileOperate::makeRow(string &rowkey){
		struct timeval tv;
		unsigned long usec = 0;
		unsigned long max = (unsigned long)-1;
	//	struct timezone tz;

		gettimeofday(&tv,NULL);
		usec = tv.tv_sec*1000000+tv.tv_usec;
		usec = max/2-usec;

		rowkey = lexical_cast<string>(usec);
		return usec;
}
