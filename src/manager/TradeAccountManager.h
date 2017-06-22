/*
 * TradeAccountManager.h
 *
 *  Created on: Jul 3, 2016
 *      Author: jamil
 */

#ifndef TRADE_TRADEACCOUNTMANAGER_H_
#define TRADE_TRADEACCOUNTMANAGER_H_

//#include "../config/TradingConfig.h"
#include "../akela/OrderAccount.h"
#include "../avicenna/AvicennaInterface.h"
#include "../utils/log.h"
#include <memory>


class TradeAccountManager: public CommandCallback {
public:
	TradeAccountManager(Linkx::Client &);
	virtual ~TradeAccountManager();
	void operator()(stringlist);

	void stop();
	void start();
	void stop(std::string& );
	void start(std::string& );
	void start(std::string &, std::string&);
	void stop(std::string &, std::string&);

	void panicClose(const std::string &account_id = "", const std::string &symbol="", const bool enable = true);
	void softClose(const std::string &account_id = "", const std::string &symbol="", const bool enable = true);
	void avicennaToggle(const std::string &account_id = "");


	void dayEnd();
	void generatePositionFile();
};

#endif /* TRADE_TRADEACCOUNTMANAGER_H_ */
