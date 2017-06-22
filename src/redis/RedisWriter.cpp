/*
 * RedisWriter.cpp
 *
 *  Created on: Sep 9, 2016
 *      Author: akela
 */

#include "RedisWriter.h"


//std::mutex RedisWriter::redis_mutex;
bool RedisWriter::isRedisEnabled;
//RedisWriter* RedisWriter::singleton = NULL;
//redis::client*  RedisWriter::c;
//long  RedisWriter::recordNumber=0;
/*
RedisWriter::RedisWriter() {
	// TODO Auto-generated constructor stub

}*/

RedisWriter::~RedisWriter() {
	// TODO Auto-generated destructor stub
}

bool RedisWriter::writeToRedis(std::string user,std::string symbol,std::string quantity,std::string averagePrice,std::string rif,std::string pif,std::string liveBuy,std::string liveSell,std::string totalExecuted){
	try{
		if(RedisWriter::isRedisEnabled==true){
	    //redis::client  c;
			std::lock_guard<std::mutex> lock(redis_mutex);
	    // std::string record=std::to_string( RedisWriter::recordNumber);
	    std::string key=user+":"+ symbol;
	    std::string value="EQUITY,,"+user+","+symbol+","+symbol+",,,"+quantity+",,"+averagePrice+","+rif+","+pif+","+liveBuy+","+liveSell+","+totalExecuted;
	    client.set(key,value);
	    //RedisWriter::recordNumber++;
	    return true;}
		else{return false;}
	}
	catch(...){

		FATAL<<"Cannot connect to the redis server";
		return false;
	}

}

std::string RedisWriter::readKeyFromRedis(std::string key){
	try{
		if(RedisWriter::isRedisEnabled==true){
		//redis::client  c;
			std::lock_guard<std::mutex> lock(redis_mutex);
		std::string value=client.get(key);
		return value;
		}
		else{return "**nonexistent-key**";}
	}
	catch(...){

		FATAL<<"Cannot connect to the redis server";
			return "**nonexistent-key**";
		}
}

std::vector<std::string> RedisWriter::readStem1KeyFromRedis(){
	if(RedisWriter::isRedisEnabled==true){
	//redis::client  c;
		std::lock_guard<std::mutex> lock(redis_mutex);
	 std::vector<std::string> stem1keys;
	 long   res=client.keys("stem1*",stem1keys);
	// RedisWriter::recordNumber=res+1;
	 std::vector<std::string> stem1values;
	  for (std::vector<std::string>::iterator itr = stem1keys.begin(); itr != stem1keys.end(); itr++ ) {
		  stem1values.push_back( readKeyFromRedis(itr->c_str()));

	     }
	  return stem1values;}
	else{return std::vector<std::string>();}
}


Position RedisWriter::parseStem1Value(std::string value){
	Position position;
	std::vector<std::string> strs;
	try{
	boost::split(strs, value,  boost::is_any_of(","));
	if(strs.size()==10){
	  position.Type=strs[0];
	  position.Reserved=strs[1];
	  position.Account=strs[2];
	  position.Underlier=strs[3];
	  position.Root=strs[4];
	  position.Expiration=strs[5];
	  position.Strike=strs[6];
	  position.Quantity=strs[7];
	  position.PutorCall=strs[8];
	  position.Price=strs[9];
	  position.pif="";
	  position.rif="";
	  position.liveSharesBuy="0";
	  position.liveSharesSell="0";
	  position.TotalExecutedShares="0";
	  DEBUG<<"parseStem1Value parse sucessfull "<<value;
	  return position;
	}
	else if(strs.size()==12){
		position.Type=strs[0];
		position.Reserved=strs[1];
		position.Account=strs[2];
		position.Underlier=strs[3];
		position.Root=strs[4];
		position.Expiration=strs[5];
		position.Strike=strs[6];
		position.Quantity=strs[7];
		position.PutorCall=strs[8];
		position.Price=strs[9];
		position.rif=strs[10];
		position.pif=strs[11];
		position.liveSharesBuy="0";
		position.liveSharesSell="0";
		position.TotalExecutedShares="0";
		DEBUG<<"parseStem1Value parse sucessfull "<<value;
		return position;
	}
	else {
			position.Type=strs[0];
			position.Reserved=strs[1];
			position.Account=strs[2];
			position.Underlier=strs[3];
			position.Root=strs[4];
			position.Expiration=strs[5];
			position.Strike=strs[6];
			position.Quantity=strs[7];
			position.PutorCall=strs[8];
			position.Price=strs[9];
			position.rif=strs[10];
			position.pif=strs[11];
			position.liveSharesBuy=strs[12];
			position.liveSharesSell=strs[13];
			position.TotalExecutedShares=strs[14];
			DEBUG<<"parseStem1Value parse sucessfull "<<value;
			return position;
		}}
	catch(...){
		DEBUG<<"parseStem1Value couldn't parse "<<value;
	}
}

RedisOrderInfo RedisWriter::parseRedisOrderInfo(std::string value){
	RedisOrderInfo redisOrderInfo;
	std::vector<std::string> strs;
	try{
	boost::split(strs, value,  boost::is_any_of(","));
	redisOrderInfo.Symbol=strs[0];
	redisOrderInfo.Size=strs[1];
	redisOrderInfo.Price=strs[2];
	redisOrderInfo.Type=strs[3];
	redisOrderInfo.ExecutionTime=strs[4];
	redisOrderInfo.AckTime=strs[5];
	redisOrderInfo.ChangeTime=strs[6];
	redisOrderInfo.CancelTime=strs[7];
	DEBUG<<"parseRedisOrderInfo parse sucessfull "<<value;
	return redisOrderInfo;
	}
	catch(...){
	DEBUG<<"parseStem1Value couldn't parse "<<value;
	}
}
/*
void RedisWriter::initRedisWriter(std::string ip, uint16_t port,bool enable){
	DEBUG<<"Inside RedisWriter initRedisWriter "<<ip<<port;
	if ( singleton == nullptr ){
		singleton = new RedisWriter(ip, port);

	}
	RedisWriter::isRedisEnabled=enable;

}*/

bool RedisWriter::writeRIFToRedis(std::string key,std::string rif){
	try{
		if(RedisWriter::isRedisEnabled==true){
	    //std::string value="EQUITY,,"+user+","+symbol+","+symbol+",,,"+quantity+",,"+averagePrice+","+rif+","+pif;

	    //client.set(key,value);
	    //RedisWriter::recordNumber++;
			std::string result =readKeyFromRedis(key);
			DEBUG<<"writing RIF to redis key:"<<key<<" Old Value:"<<result;
			if(result=="**nonexistent-key**"){
					//means the required key couldnot be found so continue without loding positions
				DEBUG<<"cannot update RIF,  as key dont exists in redis";
				return false;  // cannot update RIF, PIF as key dont exists in redis
			}
			Position currentPosition=parseStem1Value(result);
			writeToRedis(currentPosition.Account,currentPosition.Underlier,currentPosition.Quantity,currentPosition.Price,rif,currentPosition.pif,currentPosition.liveSharesBuy,currentPosition.liveSharesSell,currentPosition.TotalExecutedShares);
			DEBUG<<"writing PIF to redis key:"<<key<<" New Value:"<<readKeyFromRedis(key);
	    return true;}
		else{return false;}
	}
	catch(...){

		FATAL<<"Cannot connect to the redis server";
		return false;
	}

}


bool RedisWriter::writePIFToRedis(std::string key,std::string pif){
	try{
		if(RedisWriter::isRedisEnabled==true){
	    //std::string value="EQUITY,,"+user+","+symbol+","+symbol+",,,"+quantity+",,"+averagePrice+","+rif+","+pif;

	    //client.set(key,value);
	    //RedisWriter::recordNumber++;
			std::string result =readKeyFromRedis(key);
			DEBUG<<"writing PIF to redis key:"<<key<<" Old Value:"<<result;
			if(result=="**nonexistent-key**"){
					//means the required key couldnot be found so continue without loding positions
				DEBUG<<"cannot update PIF,  as key dont exists in redis";
				return false;  // cannot update RIF, PIF as key dont exists in redis
			}
			Position currentPosition=parseStem1Value(result);
			writeToRedis(currentPosition.Account,currentPosition.Underlier,currentPosition.Quantity,currentPosition.Price,currentPosition.rif,pif,currentPosition.liveSharesBuy,currentPosition.liveSharesSell,currentPosition.TotalExecutedShares);
			DEBUG<<"writing PIF to redis key:"<<key<<" New Value:"<<readKeyFromRedis(key);
	    return true;}
		else{return false;}
	}
	catch(...){

		FATAL<<"Cannot connect to the redis server";
		return false;
	}

}


bool RedisWriter::writeLiveSharesInfo(std::string key,std::string buyLive,std::string sellLive,std::string total){
	try{
			if(RedisWriter::isRedisEnabled==true){
		    //std::string value="EQUITY,,"+user+","+symbol+","+symbol+",,,"+quantity+",,"+averagePrice+","+rif+","+pif;

		    //client.set(key,value);
		    //RedisWriter::recordNumber++;
				std::string result =readKeyFromRedis(key);
				DEBUG<<"writing writeLiveSharesInfo to redis key:"<<key<<" Old Value:"<<result;
				if(result=="**nonexistent-key**"){
						//means the required key couldnot be found so continue without loding positions
					DEBUG<<"cannot update LiveSharesInfo,  as key dont exists in redis creating a new entry";
					std::vector<std::string> strs;
					boost::split(strs,key,boost::is_any_of(":"));
					DEBUG<<"key: "<<strs[0]<<":"<<strs[1]<<" liveBuy:"<<buyLive<<" liveSell:"<<sellLive<<" executed:"<<total;
					bool val=writeToRedis(strs[0],strs[1],"0","0","0","0",buyLive,sellLive,total);
					return val;  // cannot update RIF, PIF as key dont exists in redis
				}
				Position currentPosition=parseStem1Value(result);
				writeToRedis(currentPosition.Account,currentPosition.Underlier,currentPosition.Quantity,currentPosition.Price,currentPosition.rif,currentPosition.pif,buyLive,sellLive,total);
				DEBUG<<"writing LiveSharesInfo to redis key:"<<key<<" New Value:"<<readKeyFromRedis(key);
		    return true;}
			else{return false;}
		}
		catch(...){

			FATAL<<"Cannot connect to the redis server";
			return false;
		}

	}

bool RedisWriter::writeOrderInfoToRedis(std::string user,std::string requestid,std::string symbol,std::string size,std::string price,std::string type,std::string execution_time,std::string ack_time,std::string change_time,std::string cancel_time){
	try{
		if(RedisWriter::isRedisEnabled==true){
	    //redis::client  c;
			std::lock_guard<std::mutex> lock(redis_mutex);
	    // std::string record=std::to_string( RedisWriter::recordNumber);
	    std::string key=user+","+ requestid;
	    std::string value=symbol+","+size+","+price+","+type+","+execution_time+","+ack_time+","+change_time+","+cancel_time;
	    client.set(key,value);
	    //RedisWriter::recordNumber++;
	    return true;}
		else{return false;}
	}
	catch(...){

		FATAL<<"Cannot connect to the redis server";
		return false;
	}

}

bool RedisWriter::updateOrderCancleTime(std::string user,std::string requestId,std::string cancleTime){
	try{
		if(RedisWriter::isRedisEnabled==true){
			std::string result =readKeyFromRedis(user+","+ requestId);
			DEBUG<<"writing Order Cancel time to redis :"<<user+","+ requestId<<" Old Value:"<<result;
			if(result=="**nonexistent-key**"){
					//means the required key couldnot be found so continue without loding positions
				DEBUG<<"cannot update Cancel Time,  as key dont exists in redis";
				return false;  // cannot update  as key dont exists in redis
			}
			RedisOrderInfo redisOrderInfo=parseRedisOrderInfo(result);
			writeOrderInfoToRedis(user,requestId,redisOrderInfo.Symbol,redisOrderInfo.Size,redisOrderInfo.Price,redisOrderInfo.Type,redisOrderInfo.ExecutionTime,redisOrderInfo.AckTime,redisOrderInfo.ChangeTime,cancleTime);
			//DEBUG<<"writing Order Cancel time to redis key:"<<user+","+ requestId<<" New Value:"<<readKeyFromRedis(user+","+ requestId);
			//DEBUG<<"writing Order Cancel time to redis";
			return true;}
		else{return false;}
	}
	catch(...){

		FATAL<<"Cannot connect to the redis server";
		return false;
	}

}

bool RedisWriter::updateOrderExecTime(std::string user,std::string requestId,std::string execTime){
	try{
		if(RedisWriter::isRedisEnabled==true){
			std::string result =readKeyFromRedis(user+","+ requestId);
			DEBUG<<"writing Order Execution time to redis :"<<user+","+ requestId<<" Old Value:"<<result;
			if(result=="**nonexistent-key**"){
					//means the required key couldnot be found so continue without loding positions
				DEBUG<<"cannot update Execution Time,  as key dont exists in redis";
				return false;  // cannot update as key dont exists in redis
			}
			RedisOrderInfo redisOrderInfo=parseRedisOrderInfo(result);
			writeOrderInfoToRedis(user,requestId,redisOrderInfo.Symbol,redisOrderInfo.Size,redisOrderInfo.Price,redisOrderInfo.Type,execTime,redisOrderInfo.AckTime,redisOrderInfo.ChangeTime,redisOrderInfo.CancelTime);
			DEBUG<<"writing Order Execution time to redis key:"<<user+","+ requestId<<" New Value:"<<readKeyFromRedis(user+","+ requestId);
	    return true;}
		else{return false;}
	}
	catch(...){

		FATAL<<"Cannot connect to the redis server";
		return false;
	}

}

bool RedisWriter::updateOrderChangeTime(std::string user,std::string requestId,std::string changeTime){
	try{
		if(RedisWriter::isRedisEnabled==true){
			std::string result =readKeyFromRedis(user+","+ requestId);
			DEBUG<<"writing Order Change time to redis :"<<user+","+ requestId<<" Old Value:"<<result;
			if(result=="**nonexistent-key**"){
					//means the required key couldnot be found so continue without loding positions
				DEBUG<<"cannot update Change Time,  as key dont exists in redis";
				return false;  // cannot update as key dont exists in redis
			}
			RedisOrderInfo redisOrderInfo=parseRedisOrderInfo(result);
			writeOrderInfoToRedis(user,requestId,redisOrderInfo.Symbol,redisOrderInfo.Size,redisOrderInfo.Price,redisOrderInfo.Type,redisOrderInfo.ExecutionTime,redisOrderInfo.AckTime,changeTime,redisOrderInfo.CancelTime);
			DEBUG<<"writing Order Change time to redis key:"<<user+","+ requestId<<" New Value:"<<readKeyFromRedis(user+","+ requestId);
	    return true;}
		else{return false;}
	}
	catch(...){

		FATAL<<"Cannot connect to the redis server";
		return false;
	}

}

std::string RedisWriter::getCurrentTime(){
	time_t rawtime;
	struct tm * timeinfo;
	char buffer[80];

	time (&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(buffer,80,"%d-%m-%Y %I:%M:%S",timeinfo);
	std::string str(buffer);
	return str;
}

bool RedisWriter::writeTradeCaseToRedis(std::string user,std::string symbol,std::string time,std::string state,std::string tradeCaseNumber,std::string bid_size,std::string bid_price,std::string ask_size,std::string ask_price,std::string price_estimate){
	try{
		if(RedisWriter::isRedisEnabled==true){

		std::lock_guard<std::mutex> lock(redis_mutex);
	    std::string key=user+"#"+ symbol;
	    std::string value=time+","+state+","+tradeCaseNumber+","+bid_size+","+bid_price+","+ask_size+","+ask_price+","+price_estimate;
	    client.rpush(key,value);
	    return true;}
		else{return false;}
	}
	catch(...){

		FATAL<<"Cannot connect to the redis server";
		return false;
	}

}


bool RedisWriter::writePositionRecordToRedis(std::string user,std::string symbol,PositionRecord record){
	try{
			if(RedisWriter::isRedisEnabled==true){

			std::lock_guard<std::mutex> lock(redis_mutex);
		    std::string key=user+";"+ symbol;
		    std::string value=std::to_string(record.average_at_close)+","+std::to_string(record.close_order_id)+","+std::to_string(record.close_price)+","+std::to_string(record.close_size)+","+std::to_string(record.average_at_open)+","+std::to_string(record.open_order_id)+","+std::to_string(record.open_price)+","+std::to_string(record.open_size)+","+std::to_string(record.pnl)+","+std::to_string(record.sign)+","+std::to_string(record.is_complete);
		    client.rpush(key,value);
		    return true;}
			else{return false;}
		}
		catch(...){

			FATAL<<"Cannot connect to the redis server";
			return false;
		}

}

bool RedisWriter::writePositionRecordCommonDataToRedis(std::string user,std::string symbol,uint32_t open_size,double pnl,double average){
	try{
		long int tableSize=0;
		std::vector<std::string> temp;
				if(RedisWriter::isRedisEnabled==true){
					std::lock_guard<std::mutex> lock(redis_mutex);
					std::string key=user+";"+ symbol;
					std::string value=std::to_string(open_size)+","+std::to_string(pnl)+","+std::to_string(average);
					tableSize=client.lrange(key,0,-1,temp);
					if(tableSize==0){
					// first time entry
						client.rpush(key,value);
					}else{
					// common entry already exists
						client.ltrim(key,1,-1);
						client.lpush(key,value);
					}

			    return true;}
				else{return false;}
			}
			catch(...){

				FATAL<<"Cannot connect to the redis server";
				return false;
			}

}

bool RedisWriter::writePositionRecordTableToRedis(std::string user,std::string symbol,PositionRecordTable& table){
	try{
			if(RedisWriter::isRedisEnabled==true){

			std::lock_guard<std::mutex> lock(redis_mutex);
		    std::string key=user+";"+ symbol;
		    std::string value=std::to_string(table.getOpenSize())+","+std::to_string(table.getPNL())+","+std::to_string(table.getAverage());
		    bool res=client.del(key);
		    client.rpush(key,value);
		    for (PositionRecordTable::const_iterator iter = table.begin(); iter != table.end(); ++iter){
		    	std::string row=std::to_string(iter->average_at_open)+","+std::to_string(iter->open_order_id)+","+std::to_string(iter->open_price)+","+std::to_string(iter->open_size)+","+std::to_string(iter->average_at_close)+","+std::to_string(iter->close_order_id)+","+std::to_string(iter->close_price)+","+std::to_string(iter->close_size)+","+std::to_string(iter->pnl)+","+std::to_string(iter->sign)+","+std::to_string(iter->is_complete);
		    	//std::string row=std::to_string(iter->open_order_id);
		    	client.rpush(key,row);
		    }

		    return true;}
			else{return false;}
		}
		catch(...){

			FATAL<<"Cannot connect to the redis server";
			return false;
		}

}
