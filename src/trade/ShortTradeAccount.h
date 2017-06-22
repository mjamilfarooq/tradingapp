/*
 * ShortTradeAccount.h
 *
 *  Created on: Aug 15, 2016
 *      Author: jamil
 */

#ifndef SRC_TRADE_SHORTTRADEACCOUNT_H_
#define SRC_TRADE_SHORTTRADEACCOUNT_H_

#include "AbstractTradeAccount.h"


#define LOG_SHORT_TRADE_ACCOUNT "<"<<symbol<<",short>"

class ShortTradeAccount: public AbstractTradeAccount {
public:
	ShortTradeAccount(const std::string &, OrderAccount &, uint32_t);
	virtual ~ShortTradeAccount();

	void onCloseFill(std::shared_ptr<OrderInfo>&, const double, const uint32_t);
	void onOpenFill(std::shared_ptr<OrderInfo>&, const double, const uint32_t);
};

#endif /* SRC_TRADE_SHORTTRADEACCOUNT_H_ */
