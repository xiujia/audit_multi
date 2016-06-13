#ifndef __HBASE_OPERATE_HPP
#define __HBASE_OPERATE_HPP 
  
#include <cstdio>
#include <unistd.h>
#include <sys/time.h>
#include <poll.h>

#include <iostream>

#include <boost/lexical_cast.hpp>
#include <protocol/TBinaryProtocol.h>
#include <transport/TSocket.h>
#include <transport/TTransportUtils.h>

#include "Hbase.h"
#include "HbasePublic.hpp"
  
/** 
 * Class to operate Hbase. 
 * 
 * @author Darran Zhang (codelast.com) 
 * @version 11-08-24 
 * @declaration These codes are only for non-commercial use, and are distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. 
 * You must not remove this declaration at any time. 
 */
  
using namespace apache::thrift; 
using namespace apache::thrift::protocol; 
using namespace apache::thrift::transport; 
using namespace apache::hadoop::hbase::thrift; 
  
typedef struct hbaseRet { 
  std::string rowValue; 
  time_t ts; 
  
  hbaseRet() { 
    ts = 0; 
  } 
  
} hbaseRet; 
  
class CHbaseOperate 
{ 
public: 
  CHbaseOperate(); 
  virtual ~CHbaseOperate(); 
  
private: 
  boost::shared_ptr<TTransport> socket; 
  boost::shared_ptr<TTransport> transport; 
  boost::shared_ptr<TProtocol> protocol; 
  
  HbaseClient *client; 
  
  std::string  hbaseServiceHost; 
  int     hbaseServicePort; 
  bool    isConnected; 
  unsigned long putNum;
  
public: 
  bool  connect(); 
  
  bool  connect(std::string host, int port); 
  
  bool  disconnect(); 
  
  bool  putRow(const std::string &tableName,  const std::string &rowKey,  const std::string &column,  const std::string &rowValue,const std::map<Text, Text> & attributes); 
  bool  putRow(const std::string &tableName, const std::string &rowKey,const std::vector<Mutation> &mutations,const std::map<Text, Text> & attributes);
  
  bool  getRow(hbaseRet &result, const std::string &tableName, const std::string &rowKey, const std::string &columnName, const std::map<Text, Text> & attributes); 
  unsigned long getRow();
  void printRow();
}; 
  
#endif

