/*
 * TradeCase.h
 *
 *  Created on: Nov 10, 2016
 *      Author: jamil
 */

#ifndef SRC_TRADE_TRADECASE_H_
#define SRC_TRADE_TRADECASE_H_




#define LOG_TRADE_CASE "<"<<this->name<<"|"<<this->stage<<">: "

struct OrderRep {
	uint32_t id;
	uint32_t size;
	double open;
	double close;
	bool is_exectued;
};


using RequoteOrders = std::vector<uint32_t>;

class TradeInstance;

enum TriggerState {
	TRIGGER_STATE_INIT = 0,
	TRIGGER_STATE_POSITIVE = 1,
	TRIGGER_STATE_NEGATIVE = 2,
};

class TradeCase {
public:

	enum State {
		INIT = 1,
		PRICE_CHANGE,
		OPEN_MMF,
		CHANGE_MMF,
		WAIT,
		FAILED,
		DONE,
		NOT_SELECTED,
	};

protected:
	TradeInstance &trade_instance;
	AbstractTradeAccount *current_acc;
	AbstractTradeAccount *opposite_acc;

	double send_price;
	double requote_price;
	uint32_t round_target;
	uint32_t main_order_id;


	const std::string name;
	std::string stage;
	State state;


	const int sign;

	std::function<void (const double, const uint32_t)> on_requote_order_done;

	std::function<void (const double, const uint32_t)> on_trade_send_close;

	bool requote_order_flag;


	RequoteOrders requote_orders;

public:



	const enum Type {
		NONE = 0,
		CASE_1 = 1,
		CASE_2,
		CASE_3,
		CASE_4,
		CASE_5,
		CASE_CLOSEOPPOSITE,
		CASE_HEDGECURRENT,
		CASE_6C,
		CASE_7,
		CASE_9,
		CASE_10,
		CASE_MAXTLV,
		CASE_CLOSEOPPOSITEWITH_ANTICIPATED_PNL,
		CASE_HEDGEDMODE,
		CASE_ONEQUALITY,
		CASE_CLOSE_TRIGGER,
		PANIC_CLOSE,
		SOFT_CLOSE,
	} type;

	TradeCase(TradeInstance &, std::string, const Type);
	virtual State run();
	const Type getType();
	const State getState();
	const std::string getName();
	virtual ~TradeCase();

	bool isDone();

protected:

	virtual void init() = 0;
	virtual void reprice() = 0;
	virtual void wait();
	virtual void failed();
	virtual void cancel() = 0;
	virtual void done() = 0;

	double openPrice(uint32_t, double = 0.0);	//depending upon trigger
	double closePrice(uint32_t, double = 0.0);	//depending upon trigger
	double sellPrice(uint32_t);	//depend upon decision of selling
	double buyPrice(uint32_t);	//depend upon  decision of buying

	void sendRequoteOrders();
	void updateRequoteOrders();

};

class TradeCaseFactory {

public:
	static std::unique_ptr<TradeCase> create(TradeInstance&, TradeCase::Type);
};

class PanicClose: public TradeCase {
	AbstractTradeAccount& short_acc;
	AbstractTradeAccount& long_acc;
	uint32_t close_size;


public:
	PanicClose(TradeInstance&);
	virtual void init();
	virtual void reprice();
	virtual void cancel();
	virtual void wait();
	virtual void done();
	virtual ~PanicClose();
};



class SoftClose: public TradeCase {
	AbstractTradeAccount& short_acc;
	AbstractTradeAccount& long_acc;
	uint32_t close_size;


public:
	SoftClose(TradeInstance&);
	virtual void init();
	virtual void reprice();
	virtual void cancel();
	virtual void wait();
	virtual void done();
	virtual ~SoftClose();
};

class TradeCase1: public TradeCase {
	uint32_t close_size;
	double prev_send_price;

	bool checkFails();
public:
	TradeCase1(TradeInstance&);
	virtual void init();
	virtual void reprice();
	virtual void cancel();
	virtual void done();
	virtual ~TradeCase1();
};


class TradeCase3: public TradeCase {
	uint32_t close_size;
	double prev_send_price;

	bool checkFails();
public:
	TradeCase3(TradeInstance&);
	virtual void init();
	virtual void reprice();
	virtual void cancel();
	virtual void done();
	virtual ~TradeCase3();
};

class TradeCase4: public TradeCase {
	const uint32_t init_pos;
	double threshold;
	double threshold_limit;

	uint32_t first_order;
	uint32_t first_order_size;
	double first_order_price;

public:
	TradeCase4(TradeInstance&);
	virtual void init();
	virtual void reprice();
	virtual void cancel();
	virtual void done();
	virtual void wait();
	virtual ~TradeCase4();
};

class OnEquality: public TradeCase {
	const uint32_t init_pos;
	uint32_t target_pos;
	std::vector<uint32_t> order_ids;

public:
	OnEquality(TradeInstance&);
	virtual void init();
	virtual void reprice();
	virtual void cancel();
	virtual void done();
	virtual void wait();
	virtual ~OnEquality();
};

class TradeCase5: public TradeCase {

	uint32_t curr_close_size;
	uint32_t oppo_close_size;

	double curr_send_price;
	double oppo_send_price;

	uint32_t curr_order, oppo_order;

public:
	TradeCase5(TradeInstance&);
	virtual void init();
	virtual void reprice();
	virtual void cancel();
	virtual void done();
	virtual ~TradeCase5();
private:
	double currentPrice();
	double oppositePrice();
};


class CloseOppositeWithPNL: public TradeCase {

	double close_price;
	const uint32_t init_size;
	uint32_t close_size;
	uint32_t close_id;
public:
	CloseOppositeWithPNL(TradeInstance&);
	virtual void init();
	virtual void reprice();
	virtual void wait();
	virtual void cancel();
	virtual void failed();
	virtual void done();
	virtual ~CloseOppositeWithPNL();

};

class HedgeCurrent: public TradeCase {
	uint32_t ho_size;
public:
	HedgeCurrent(TradeInstance&);
	virtual void init();
	virtual void reprice();
	virtual void wait();
	virtual void cancel();
	virtual void done();
	virtual ~HedgeCurrent();

};

class TradeCase6C: public TradeCase {
	const uint32_t init_pos;
	const uint32_t tranch_at_init;

public:
	TradeCase6C(TradeInstance&);
	virtual void init();
	virtual void reprice();
	virtual void cancel();
	virtual void done();
	virtual void wait();
	virtual ~TradeCase6C();
};

class TradeCase7: public TradeCase {


public:
	TradeCase7(TradeInstance&);
	virtual void init();
	virtual void reprice();
	virtual void cancel();
	virtual void wait();
	virtual void done();
	virtual ~TradeCase7();

};

class TradeCase9: public TradeCase {

	uint32_t close_size;
public:
	TradeCase9(TradeInstance&);
	virtual void init();
	virtual void reprice();
	virtual void cancel();
	virtual void wait();
	virtual void done();
	virtual ~TradeCase9();

};


class MaxTLV: public TradeCase {
	double curr_price;
	double oppo_price;
	uint32_t curr_id;
	uint32_t oppo_id;
	const uint32_t curr_size_init;
	const uint32_t oppo_size_init;
public:
	MaxTLV(TradeInstance&);
	virtual void init();
	virtual void reprice();
	virtual void cancel();
	virtual void wait();
	virtual void done();
	virtual ~MaxTLV();
};

class CloseOppositeWithAnticipatedPNL: public TradeCase {
	uint32_t close_size;
	double close_price;
	uint32_t order_id;
public:
	CloseOppositeWithAnticipatedPNL(TradeInstance&);
	virtual void init();
	virtual void reprice();
	virtual void cancel();
	virtual void wait();
	virtual void done();
	virtual ~CloseOppositeWithAnticipatedPNL();

private:
	bool isOppositeCloseProfitable(const double);
};

class HedgedMode: public TradeCase {

	uint32_t ho_size;
	double price;
	AbstractTradeAccount& short_acc;
	AbstractTradeAccount& long_acc;
	uint32_t order_id;
	AbstractTradeAccount *selected_account;
	AbstractTradeAccount *other_account;
public:
	HedgedMode(TradeInstance&);
	virtual void init();
	virtual void reprice();
	virtual void cancel();
	virtual void wait();
	virtual void done();
	virtual ~HedgedMode();
};

class CloseTrigger: public TradeCase {



	uint32_t close_size;
	double error;
	std::vector<OrderRep> orders;

public:
	CloseTrigger(TradeInstance&);
	virtual void init();
	virtual void reprice();
	virtual void cancel();
	virtual void wait();
	virtual void done();
	virtual ~CloseTrigger();
};


class TradeCase10: public TradeCase {
	TriggerState trigger_state;
	uint32_t close_size;
	uint32_t original_size;
	double error;
	std::vector<OrderRep> orders;
	AbstractTradeAccount *account_to_close;

public:
	TradeCase10(TradeInstance&);

	virtual void init();
	virtual void reprice();
	virtual void cancel();
	virtual void wait();
	virtual void done();
	~TradeCase10();
};

#endif /* SRC_TRADE_TRADECASE_H_ */
