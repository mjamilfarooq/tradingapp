/*
 * LongTradeAccount.h
 *
 *  Created on: Aug 15, 2016
 *      Author: jamil
 */

#ifndef SRC_TRADE_LONGTRADEACCOUNT_H_
#define SRC_TRADE_LONGTRADEACCOUNT_H_

#include "AbstractTradeAccount.h"


#define LOG_LONG_TRADE_ACCOUNT "<"<<symbol<<",long>"

class LongTradeAccount: public AbstractTradeAccount {
public:
	LongTradeAccount(const std::string &, OrderAccount &, uint32_t);
	virtual ~LongTradeAccount();

	void onCloseFill(std::shared_ptr<OrderInfo>&, const double, const uint32_t);
	void onOpenFill(std::shared_ptr<OrderInfo>&, const double, const uint32_t);
};

#endif /* SRC_TRADE_LONGTRADEACCOUNT_H_ */
