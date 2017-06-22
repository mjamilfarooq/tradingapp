/*
 * LongTradeAccount.cpp
 *
 *  Created on: Aug 15, 2016
 *      Author: jamil
 */

#include "LongTradeAccount.h"

LongTradeAccount::LongTradeAccount(const std::string &symbol, OrderAccount &account, uint32_t divisor):
	AbstractTradeAccount(symbol, account, divisor) {

	onOpenFillReceived = new OrderInfo::onFillFunctionType(std::bind(&LongTradeAccount::onOpenFill, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	onCloseFillReceived = new OrderInfo::onFillFunctionType(std::bind(&LongTradeAccount::onCloseFill, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

}


LongTradeAccount::~LongTradeAccount() {

}

void LongTradeAccount::onCloseFill(std::shared_ptr<OrderInfo>& order_info, const double price, const uint32_t size) {
	std::lock_guard<std::mutex> lock(mt);

	order_info->fill_size += size;

//	calculateSpread(price, size, false);

	auto before_close = total/max_pos;
	total -= size;
	auto after_close = total/max_pos;

	if ( before_close > after_close ) dec_tranch();


	//totalExecuted+=size;
	//livesell-=size;
	TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"onCloseFill: @"<<price<<" for "<<size<<" total = "<<total;
	if ( total != 0 ) {
		average += ( average - price ) * size / total;
		TRACE<<LOG_LONG_TRADE_ACCOUNT<<"onCloseFill: new average = "<<average;
	} else {
		TRACE<<LOG_LONG_TRADE_ACCOUNT<<"onCloseFill: no position to calculate average";
		average = 0.0;

	}
	printCloseFills();


	//RedisWriter writer;
	DEBUG<<"writing to redis onclosefill "<<account.getAccountName()<<symbol<<total<<average<<this->rif<<this->pif;
	std::ostringstream os;
	os << this->pif;
	string redisPIF= os.str();
	std::ostringstream oss;
	oss << this->rif;
	string redisRIF= oss.str();
	//redisWriter->writeToRedis(account.getAccountName(),symbol,std::to_string(total),std::to_string(average),redisRIF,redisPIF,std::to_string(livebuy),std::to_string(livesell),std::to_string(totalExecuted));

	AbstractTradeAccount::onCloseFill(order_info, price, size);
}

void LongTradeAccount::onOpenFill(std::shared_ptr<OrderInfo>& order_info, const double price, const uint32_t size) {
	std::lock_guard<std::mutex> lock(mt);

	order_info->fill_size += size;

	calculateSpread(price, size, true);

	total += size;
	last_open_price = price;
	//totalExecuted+=size;
	//livebuy-=size;

	TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"onOpenFill: @"<<price<<" for "<<size<<" total = "<<total;
	if ( total != 0 ) {
		average += ( price - average ) * size / total;
		TRACE<<LOG_LONG_TRADE_ACCOUNT<<"onOpenFill: new average = "<<average;
	} else {
		TRACE<<LOG_LONG_TRADE_ACCOUNT<<"onOpenFill: no position to calculate average";
		average = 0.0;

	}

	printOpenFills();
	//RedisWriter writer;
	DEBUG<<"writing to redis onopenfill "<<account.getAccountName()<<symbol<<total<<average<<this->rif<<this->pif;
	std::ostringstream os;
	os << this->pif;
	string redisPIF= os.str();
	std::ostringstream oss;
	oss << this->rif;
	string redisRIF= oss.str();
	//redisWriter->writeToRedis(account.getAccountName(),symbol,std::to_string(total),std::to_string(average),redisRIF,redisPIF,std::to_string(livebuy),std::to_string(livesell),std::to_string(totalExecuted));

	AbstractTradeAccount::onOpenFill(order_info, price, size);

}
