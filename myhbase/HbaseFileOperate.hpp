#ifndef HBASE_FILE_OPERATE_H
#define HBASE_FILE_OPERATE_H

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include<string>
#include "HbaseOperate.hpp"
#include "HbasePublic.hpp"

#define LINE_LEN  10000
#define KEY_LEN	100
#define VALUE_LEN	1000

using namespace std;


typedef struct _HbaseFileRet{
	string table;
	string rowkey;
	vector<Mutation> m;
}HbaseFileRet;



class CHbaseFileOperate{

private:
	string FileName;
	string FileContent;
	ifstream in;
	stringstream ss;
	char * data;
	bool isOpened;
	unsigned long rows;
	unsigned long allrows;
//	vector<Mutation> mutations; 
public:
	CHbaseFileOperate();
	~CHbaseFileOperate();	


	bool open();
	bool open(const string &fileName);
	bool read();
	bool GetFileString(string fileName,HbaseFileRet *ret,CHbaseOperate *hbase,const map<Text,Text> &attributes);
	bool GetKeyValues(HbaseFileRet *ret,CHbaseOperate *hbase,const map<Text,Text> &attributes);
	
	bool LinePro(char *str,HbaseFileRet *ret);
	bool close();
	bool delelefile();
	bool mvfile(const string &dpath) const;
	unsigned long  getRowNums();
	void printRow();
	unsigned long makeRow(string &rowkey);
};


#endif

