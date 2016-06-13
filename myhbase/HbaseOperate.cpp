#include "HbaseOperate.hpp" 
//#include "log4cxx/log4cxx.h" 
//#include "log4cxx/propertyconfigurator.h" 
#include "Hbase.h"
#include "Hbase_types.h"

  
//static log4cxx::LoggerPtr logger(log4cxx::Logger::getLogger("HbaseOperate.cpp")); 
  
/** 
 * Class to operate Hbase. 
 * 
 * @author Darran Zhang (codelast.com) 
 * @version 11-08-24 
 * @declaration These codes are only for non-commercial use, and are distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. 
 * You must not remove this declaration at any time. 
 */
  
using namespace std; 
  
CHbaseOperate::CHbaseOperate() : 
putNum(0),socket((TSocket*)NULL), transport((TBufferedTransport*)NULL), protocol((TBinaryProtocol*)NULL), client(NULL), hbaseServicePort(9090), isConnected(false) 
{ 
} 
  
CHbaseOperate::~CHbaseOperate() 
{ 
  if (isConnected) { 
    disconnect(); 
  } 
  if (NULL != client) { 
    delete client; 
    client = NULL; 
  } 
} 
  
/** 
 * Connect Hbase. 
 * 
 */
bool CHbaseOperate::connect() 
{ 
  if (isConnected) { 
  //  LOG4CXX_INFO(logger, "Already connected, don't need to connect it again"); 
    return true; 
  } 
  
  try { 
    socket.reset(new TSocket(hbaseServiceHost, hbaseServicePort)); 
    transport.reset(new TBufferedTransport(socket)); 
//	 transport.reset(new TFramedTransport(socket)); 
    protocol.reset(new TBinaryProtocol(transport)); 
  
    client = new HbaseClient(protocol); 
  
    transport->open(); 
  } catch (const TException &tx) { 
//    LOG4CXX_ERROR(logger, "Connect Hbase error : " << tx.what()); 
    return false; 
  } 
  
  isConnected = true; 
  return isConnected; 
} 
  
/** 
 * Connect Hbase. 
 * 
 */
bool CHbaseOperate::connect(std::string host, int port) 
{ 
  hbaseServiceHost = host; 
  hbaseServicePort = port; 
  
  return connect(); 
} 
  
/** 
 * Disconnect from Hbase. 
 * 
 */
bool CHbaseOperate::disconnect() 
{ 
  if (!isConnected) { 
 //   LOG4CXX_ERROR(logger, "Haven't connected to Hbase yet, can't disconnect from it"); 
    return false; 
  } 
  
  if (NULL != transport) { 
    try { 
      transport->close(); 
    } catch (const TException &tx) { 
 //     LOG4CXX_ERROR(logger, "Disconnect Hbase error : " << tx.what()); 
      return false; 
    } 
  } else { 
    return false; 
  } 
  
  isConnected = false; 
  return true; 
} 
  
/** 
 * Put a row to Hbase. 
 * 
 * @param tableName   [IN] The table name. 
 * @param rowKey      [IN] The row key. 
 * @param column      [IN] The "column family : qualifier". 
 * @param rowValue    [IN] The row value. 
 * @return True for successfully put the row, false otherwise. 
 */
bool CHbaseOperate::putRow(const string &tableName, const string &rowKey, const string &column, const string &rowValue,const std::map<Text, Text> & attributes){ 
  if (!isConnected) { 
  //  LOG4CXX_ERROR(logger, "Haven't connected to Hbase yet, can't put row"); 
    return false; 
  } 
  
  try { 
    std::vector<Mutation> mutations; 
    mutations.clear();
    mutations.push_back(Mutation()); 
    mutations.back().column = column; 
    mutations.back().value = rowValue; 
    client->mutateRow(tableName, rowKey, mutations,attributes); 
//     putNum+=mutations.size();
  } catch (const TException &tx) { 
  	    cout << "put error.\n" <<endl;
	     cout << "col:"<< column << endl <<"val:"<< rowValue<<endl;
		 cout << tx.what() <<endl;
 //   LOG4CXX_ERROR(logger, "Operate Hbase error : " << tx.what()); 
    return false; 
  } 
  
  return true; 
} 

bool CHbaseOperate::putRow(const string &tableName, const string &rowKey,const  vector<Mutation>  &mutations,const map<Text, Text> & attributes) {
	if (!isConnected) { 
  // 	 LOG4CXX_ERROR(logger, "Haven't connected to Hbase yet, can't put row"); 
   	 return false; 
	} 

	try { 
		 client->mutateRow(tableName, rowKey, mutations,attributes); 
	//	 putNum+=mutations.size();
	} catch (const TException &tx) { 
	  	    cout << "put error in mutations `.\n" <<endl;
		     cout << tx.what() <<endl;
	 //   		 cout << "col:"<< column << endl <<"val:"<< rowValue<<endl;
//	  LOG4CXX_ERROR(logger, "Operate Hbase error : " << tx.what()); 
	  return false; 
	} 

	return true; 

}


 
/** 
 * Get a Hbase row. 
 * 
 * @param result      [OUT] The object which contains the returned data. 
 * @param tableName   [IN] The Hbase table name, e.g. "MyTable". 
 * @param rowKey      [IN] The Hbase row key, e.g. "kdr23790". 
 * @param columnName  [IN] The "column family : qualifier". 
 * @return True for successfully get the row, false otherwise. 
 */
bool CHbaseOperate::getRow(hbaseRet &result, const std::string &tableName, const std::string &rowKey, const std::string &columnName,const std::map<Text, Text> & attributes) 
{ 
  if (!isConnected) { 
//    LOG4CXX_ERROR(logger, "Haven't connected to Hbase yet, can't read data from it"); 
    return false; 
  } 
  
  std::vector<std::string> columnNames; 
  columnNames.push_back(columnName); 
  
  std::vector<TRowResult> rowResult; 
  try { 
    client->getRowWithColumns(rowResult, tableName, rowKey, columnNames,attributes); 
  } catch (const TException &tx) { 
//    LOG4CXX_ERROR(logger, "Operate Hbase error : " << tx.what()); 
    return false; 
  } 
  
  if (0 == rowResult.size()) { 
//    LOG4CXX_WARN(logger, "Got no record with the key : [" << rowKey << "]"); 
    return false; 
  } 
  
  std::map<std::string, TCell>::const_iterator it = rowResult[rowResult.size() -1].columns.begin(); 
  result.rowValue = it->second.value; 
  result.ts = it->second.timestamp; 
  
  return true; 
}


unsigned long CHbaseOperate::getRow(){
	return putNum;
}

void CHbaseOperate::printRow(){
	cout << putNum << "Rows Insert \n";
}

