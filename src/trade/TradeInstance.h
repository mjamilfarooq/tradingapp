/*
 * TradeAccountManager.h
 *
 *  Created on: Jul 3, 2016
 *      Author: jamil
 */

#ifndef TRADE_INSTANCE_H_
#define TRADE_INSTANCE_H_

#include <chrono>
#include <type_traits>

//#include "../config/TradingConfig.h"
#include "../akela/OrderAccount.h"
#include "../avicenna/AvicennaInterface.h"
#include "../utils/log.h"
#include "../utils/utils.h"
#include "../utils/ThreadPool.h"
#include "../redis/RedisWriter.h"
#include "ShortTradeAccount.h"
#include "LongTradeAccount.h"
#include "TradeCase.h"

#define LOG_PREVIOUS_BAR bar0.open<<","<<bar0.minimum<<","<<bar0.maximum<<","<<bar0.close
#define LOG_TRADE_INSTANCE "<"<<this->trade_account.id<<","<<this->symbol.name<<","<<(( trade_case )?trade_case->getName():"")<<","<<trigger_rep[this->trigger_state]<<","<<(current_acc?current_acc->getPositionString():"")<<","<<(opposite_acc?opposite_acc->getPositionString():"")<<","<<price_estimate<<"|"<<LOG_PREVIOUS_BAR<<">: "
#define TRADE_INSTANCE_OPEN_RETRY 3






enum TradeInstanceState {
	TRADE_INSTANCE_NONE = 0,
	TRADE_INSTANCE_INIT,
	TRADE_INSTANCE_CHECK_CANCEL,
	TRADE_INSTANCE_SELECT_CASES,
	TRADE_INSTANCE_LIMIT_EXCEED,
	TRADE_INSTANCE_MAIN_COND_TRUE,
	TRADE_INSTANCE_MAIN_COND_FALSE,
	TRADE_INSTANCE_CASE_1_INIT,
	TRADE_INSTANCE_CASE_1,
	TRADE_INSTANCE_CASE_2_INIT,
	TRADE_INSTANCE_CASE_2,
	TRADE_INSTANCE_CASE_3_INIT,
	TRADE_INSTANCE_CASE_3,
	TRADE_INSTANCE_CASE_4_INIT,
	TRADE_INSTANCE_CASE_4,
	TRADE_INSTANCE_CASE_5_INIT,
	TRADE_INSTANCE_CASE_5,
	TRADE_INSTANCE_CASE_6_INIT,
	TRADE_INSTANCE_CASE_6,
	TRADE_INSTANCE_CASE_7_INIT,
	TRADE_INSTANCE_CASE_7,
	TRADE_INSTANCE_SOFT_CLOSE_INIT,
	TRADE_INSTANCE_SOFT_CLOSE,
	TRADE_INSTANCE_PANIC_CLOSE_INIT,
	TRADE_INSTANCE_PANIC_CLOSE,

};


enum PanicCloseState {
	PANIC_CLOSE_INIT = 0,
	PANIC_CLOSE_WAIT_CANCEL,
	PANIC_CLOSE_EXECUTE_CLOSE,
	PANIC_CLOSE_CHECK_CLOSE,
	PANIC_CLOSE_NONE,
};

enum PositionCloseState {
	POSITION_CLOSE_INIT,
	POSITION_CLOSE_CHECK,
	POSITION_CLOSE_WAIT_CANCEL,

};




class TradeInstance {

	friend class TradeCase;
	friend class PanicClose;
	friend class SoftClose;
	friend class TradeCase1;
	friend class TradeCase3;
	friend class TradeCase4;
	friend class TradeCase5;
	friend class CloseOppositeWithPNL;
	friend class HedgeCurrent;
	friend class TradeCase6C;
	friend class TradeCase7;
	friend class TradeCase9;
	friend class TradeCase10;

	friend class MaxTLV;
	friend class CloseOppositeWithAnticipatedPNL;
	friend class HedgedMode;
	friend class OnEquality;
	friend class CloseTrigger;

	std::unique_ptr<TradeCase> trade_case;
	TradeCase::Type previous_case;
	TradeCase::State last_state;

	double wait_for_next_time_window_avicenna;
	bool wait_for_next_time_window;

	static ThreadPool<TradeInstance *> trade_instances;

	const TradeAccount &trade_account;
	Symbol &symbol;

	RoundVector& share_rounds;	//set of all rounds
	uint32_t tranch_value;


	const size_t max_round;				//maximum value of round
	const size_t min_round;				//minimum value of round
	size_t max_pos, min_pos;
	RoundVector::const_iterator current_round;
	RoundVector::const_iterator previous_round;


	const double mpv_value;
	const double mpv_limit;
	const uint32_t mpv_multiplier;

	const bool mmf;
	const int base_qty;
	const bool cancel_on_trigger;

	const double multiplier;
	const uint32_t divisor;

	TriggerState trigger_state;
	TriggerState avicenna_trigger_state;
	TriggerState prev_trigger_state;
	static const char trigger_rep[];
	static const std::string state_rep[];


	double min_price;
	double open_price;
	double max_price;
	double close_price;


	uint32_t ask_size;
	double ask_price;
	uint32_t bid_size;
	double bid_price;
	double price_estimate;	//avicenna price
	double price_estimate_prev;
	bool avicenna_toggle;
	double instrument_skew;

	double trade_price;	//ask_price in case of +ive trigger and bid_price in case of -ive trigger for market order
	double price_estimate_on_trigger; //price estimate on trigger
	double reference_price;	//price of bid or ask in when trigger changes



	TradeInstanceState state;
	PositionCloseState position_close_state;

	RedisWriter redisWriter;
	ShortTradeAccount short_acc;
	LongTradeAccount long_acc;

	AbstractTradeAccount *current_acc;
	AbstractTradeAccount *opposite_acc;

	bool data_wait;

	bool is_enable;

	bool is_panic_close;
	bool panic_close_init;
	bool is_soft_close;
	bool soft_close_init;

	std::chrono::time_point<std::chrono::steady_clock> start, end;

	struct Bar {
		double open;
		double close;
		double minimum;
		double maximum;
	} bar0, bar1;

public:

//	static Linkx::Client *client;

	TradeInstance(const TradeAccount &account, Symbol &symbol);
	double sendPrice(uint32_t qto);
private:

	void readSymbolData();
	void triggerSelection();
	void caseSelection();
	bool specialCases();
	void selfDestruct();
	void triggerProcessing();
	std::string stateString();

//	void testSelfDestruct();

	void reprice();

	/*
	 * @brief Get next round of shares to be sell/purchase
	 *
	 * @param current position for long account
	 *
	 * @param current position for short account
	 *
	 * @param position to open is +ive or -ive
	 *
	 * @return size of next round to be purchase/sell
	 */
	uint32_t quantityToOpen();
	size_t roundInForce();
	uint32_t roundForCurrentAccount();

public:

	void stopAction();
	void startAction();
	void enablePanicClose();
	void disablePanicClose();
	void enableSoftClose();
	void disableSoftClose();

	void firstReprice();
	void resume();
	void pause();
	void avicennaToggle();

};


inline void TradeInstance::firstReprice() {
	auto spread = fabs(current_acc->getAverage() - opposite_acc->getAverage());
	current_acc->setAverage( price_estimate );
	if ( dynamic_cast<LongTradeAccount *> ( current_acc ) ) {
		opposite_acc->setAverage(current_acc->getAverage() - spread );
	} else if ( dynamic_cast<ShortTradeAccount *> ( current_acc ) ) {
		opposite_acc->setAverage(current_acc->getAverage() + spread );
	}
}

inline void TradeInstance::resume() {
	is_enable = true;
}

inline double TradeInstance::sendPrice(uint32_t qto){
	double ret = 0.0;
	int64_t os;
	if ( trigger_state == TRIGGER_STATE_POSITIVE ) {
		os = ask_size - qto*multiplier;
		if ( os <= 0 ) ret = ask_price;
		else ret = bid_price;
//		else if ( instrument_skew > 0 ) ret = bid_price;
//		else if ( instrument_skew < 0 && os > 0 ) ret = 0.0;
	} else if ( trigger_state == TRIGGER_STATE_NEGATIVE ) {
		os = bid_size - qto*multiplier;
		if ( os <= 0 ) ret = bid_price;
		else ret = ask_price;
//		else if ( instrument_skew < 0 ) ret = ask_price;
//		else if ( instrument_skew > 0 && os > 0 ) ret = 0.0;
	}

//	DEBUG<<LOG_TRADE_INSTANCE<<"qto = "<<qto<<" OS = "<<os<<" is = "<<instrument_skew<<" sendPrice = "<<ret;
	return ret;
}

inline void TradeInstance::pause() {
	is_enable = false;
}

inline void TradeInstance::stopAction() {
	//cancel orders in both accounts
//	current_acc->cancelOrder();
//	opposite_acc->cancelOrder();

//	INFO<<LOG_TRADE_INSTANCE<<"stopAction: ";
	short_acc.cancelAll();
	long_acc.cancelAll();

	is_enable = false;
}

inline void TradeInstance::startAction() {
	is_enable = true;
}

inline void TradeInstance::enablePanicClose() {
	is_panic_close = true;
	panic_close_init = true;
	state = TRADE_INSTANCE_PANIC_CLOSE;

}

inline void TradeInstance::disablePanicClose() {
	is_panic_close = false;
	panic_close_init = false;

}

inline void TradeInstance::enableSoftClose() {
	is_soft_close = true;
	soft_close_init = true;
	state = TRADE_INSTANCE_SOFT_CLOSE;
}

inline void TradeInstance::disableSoftClose() {
	is_soft_close = false;
	soft_close_init = false;
}


inline void TradeInstance::avicennaToggle() {
	avicenna_toggle = !avicenna_toggle;
}

using TradeInstanceMap = std::map<std::string, std::unique_ptr<TradeInstance>>;

#endif /* TRADE_INSTANCE_H_ */
