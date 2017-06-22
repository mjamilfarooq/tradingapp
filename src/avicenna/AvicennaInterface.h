/*
 * AvicennaInterface.h
 *
 *  Created on: May 12, 2016
 *      Author: jamil
 */

#ifndef AVICENNAINTERFACE_H_
#define AVICENNAINTERFACE_H_

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

//#include "../config/TradingConfig.h"
#include "../config/SymbolConfig.h"
#include "../libs/avicenna/AvicennaSuperPlus.h"


class AvicennaConfiguration;
class TradeAccount;

#define LOG_AVICENNA_INTERFACE " <"<<this->account_id<<"> "
#define LOG_AVICENNA_CONNECTION " <"<<this->account_id<<","<<symbol<<"> "

struct AvicennaConnection:public CAvicenna_SuperPlus {
	double price_estimate;
	double price_estimate_3;

	double ms;

	const std::string account_id;
	const std::string symbol;

	bool startup;

	AvicennaConnection(const std::string,
			const std::string,
			const AvicennaConfiguration &);
	AvicennaConnection(
			const std::string,
			const std::string,
			const std::string,
			const std::string,
			const std::string,
			const std::string,
			const std::string,
			const std::string);

	void calculate(double, double, double);

};


class AvicennaInterface {

	std::string account_id;
	std::string symbol;
	const TradeAccount *trade_account;

	std::thread *timeout_thread1, *timeout_thread2;
	bool stop_timeout_thread;
	void timeoutThread(int, int);
	void newEstimate(double latest, double min = 0.0, double max = 0.0, double earliest = 0.0);
	double output[4];
	double price_estimate;

	struct AvicennaConnectionPair {
		AvicennaConnection *avicenna1;
		AvicennaConnection *avicenna2;
	};


	std::map<std::string,AvicennaConnectionPair *> avicenna_list;

public:

	AvicennaInterface(const TradeAccount *trade_account);
	~AvicennaInterface();
};

#endif /* AVICENNAINTERFACE_H_ */
