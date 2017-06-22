/*
 * RedisWriter.h
 *
 *  Created on: Sep 9, 2016
 *      Author: akela
 */

#ifndef SRC_REDIS_REDISWRITER_H_
#define SRC_REDIS_REDISWRITER_H_

#include <redisclient.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <mutex>
#include <thread>
#include "../utils/log.h"
#include "../trade/PositionRecordTable.h"
struct Position {

std::string Type;
std::string Reserved;
std::string Account;
std::string Underlier;
std::string Root;
std::string Expiration ;
std::string Strike;
std::string Quantity;
std::string PutorCall;
std::string Price;

std::string rif;
std::string pif;


std::string liveSharesBuy;
std::string liveSharesSell;
std::string TotalExecutedShares;

};

struct RedisOrderInfo{
	std::string Symbol;
	std::string Size;
	std::string Price;
	std::string Type;
	std::string ExecutionTime;
	std::string AckTime ;
	std::string ChangeTime;
	std::string CancelTime;


};

class RedisWriter {
public:


	RedisWriter(std::string ip, uint16_t port): client(ip, port,0) {
		DEBUG<<"Inside RedisWriter constructor "<<ip<<port;

	}

	virtual ~RedisWriter();
	//static long recordNumber;
	bool writeToRedis(std::string user,std::string symbol,std::string quantity,std::string averagePrice,std::string rif,std::string pif ,std::string liveBuy,std::string liveSell,std::string totalExecuted);
	bool writeRIFToRedis(std::string key,std::string rif);
	bool writePIFToRedis(std::string key,std::string pif);
	bool writeLiveSharesInfo(std::string key,std::string buyLive,std::string sellLive,std::string total);
	std::string readKeyFromRedis(std::string key);
	std::vector<std::string> readStem1KeyFromRedis();
	Position parseStem1Value(std::string value);
//	static void initRedisWriter(std::string ip, uint16_t port,bool enable);
	static bool isRedisEnabled;
/*	static RedisWriter* getRedisPointer() {
			return singleton;
		}*/
	//static redis::client  *c;
	 std::mutex redis_mutex;

	 RedisOrderInfo parseRedisOrderInfo(std::string value);
	 bool writeOrderInfoToRedis(std::string user,std::string requestid,std::string symbol,std::string size,std::string price,std::string type,std::string execution_time,std::string ack_time,std::string change_time,std::string cancel_time);
	 bool updateOrderCancleTime(std::string user,std::string requestId,std::string cancleTime);
	 bool updateOrderExecTime(std::string user,std::string requestId,std::string execTime);
	 bool updateOrderChangeTime(std::string user,std::string requestId,std::string changeTime);
	 std::string getCurrentTime();
	 bool writeTradeCaseToRedis(std::string user,std::string symbol,std::string time,std::string state,std::string tradeCaseNumber,std::string bid_size,std::string bid_price,std::string ask_size,std::string ask_price,std::string price_estimate);
	 bool writePositionRecordToRedis(std::string user,std::string symbol,PositionRecord record);
	 bool writePositionRecordCommonDataToRedis(std::string user,std::string symbol,uint32_t open_size,double pnl,double average);
	 bool writePositionRecordTableToRedis(std::string user,std::string symbol,PositionRecordTable &table);

private:
redis::client client;
//static RedisWriter *singleton;

};

#endif /* SRC_REDIS_REDISWRITER_H_ */
