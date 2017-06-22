/*
 * AbstractTradeAccount.h
 *
 *  Created on: Aug 15, 2016
 *      Author: jamil
 */

#ifndef SRC_TRADE_ABSTRACTTRADEACCOUNT_H_
#define SRC_TRADE_ABSTRACTTRADEACCOUNT_H_

#include <functional>
#include <iomanip>
#include <queue>
#include <future>

#include "../akela/OrderAccount.h"
#include "../redis/RedisWriter.h"
#include "PositionRecordTable.h"

#define LOG_ABSTRACT_TRADE_ACCOUNT "<"<<account.getAccountName()<<","<<account.getAccountTypeString()<<","<<symbol<<","<<getPosition()<<","<<average<<","<<max_pos<<">: "

#define PRICE_CHANGE_EPSILON 0.0001
#define MAXIMUM_ORDER_LIMIT 5000



using TradeDone = std::function<void (double, uint32_t)>;


struct PositionRequest: public OrderInfo {

	enum {
		OPEN,
		CLOSE,
		CHANGE,
	} type;

	bool send_mmf_on_fills;	//if position needed to be closed after fills received
	OrderType mmf_order;	//mmf_order sent to close the position
	uint32_t mmf_closed;	//count corresponding close positions.

	bool is_mmf;		//if order is close order (mmf close)
	uint32_t open_request_id; //in case of close MMF order we need to know its open order id
	OrderType open_order;	//open order against this order if its mmf order


	bool is_fco;	//first close order

	bool is_sco;	//if order is second close order
	OrderType fco_order; //first close order against which second close order is initiated


	bool is_trade_done_define;
	TradeDone trade_done_function;

	PositionRequest(std::string symbol, const uint32_t size, const double price):
		OrderInfo(symbol, size, price),
		mmf_closed(0),
		mmf_order(nullptr),
		open_order(nullptr),
		is_fco(false),
		is_sco(false),
		type(OPEN),
		send_mmf_on_fills(false),
		is_mmf(false),
		open_request_id(0),
		fco_order(nullptr),
		is_trade_done_define(true) {
	}

	PositionRequest(const OrderType& original, const double new_price, const uint32_t new_size):
		OrderInfo(*original, new_price, new_size),
		mmf_closed(0),
		mmf_order(nullptr),
		open_order(nullptr),
		is_fco(false),
		is_sco(false),
		type(OPEN),
		send_mmf_on_fills(false),
		is_mmf(false),
		open_request_id(0),
		fco_order(nullptr),
		is_trade_done_define(true) {

		auto&& position_request = static_pointer_cast<PositionRequest>(original);
		is_mmf = position_request->is_mmf;
		send_mmf_on_fills = position_request->send_mmf_on_fills;
		open_request_id = position_request->open_request_id;
		mmf_order = position_request->mmf_order;
		open_order = position_request->open_order;
		is_trade_done_define = position_request->is_trade_done_define;
		trade_done_function = position_request->trade_done_function;
	}
};

class AbstractTradeAccount {
protected:

	PositionRecordTable position_table;
	std::mutex fill_table_mutex;

	std::mutex mt;
	std::condition_variable cv;


	using OrderMap = std::map<uint32_t, OrderType>;
	using PendingOrderQueue = std::queue<PositionRequest>;


	OrderMap orders;
	PendingOrderQueue pending_orders;

	static const size_t MAX_RETRY =  3;

	std::mutex rw_mutex;
	std::mutex order_mutex;

	const std::string symbol;
	int64_t total;
	double average;
	double spread;
	double last_open_price;
	double last_close_price;

	//int64_t livebuy;
	//int64_t livesell;
	int64_t totalopen;
	int64_t totalclose;
	int64_t totalExecuted;
	//double spread;
	RedisWriter *redisWriter;

	uint32_t open_live;
	uint32_t close_live;

	double mmf_round;		//<. round limit set for current trigger
	double mmf_mpv;			//<. mpv set in for current trigger
	double mmf_quantity;	//<. last mmf_quantity sent to open in opposite account

	uint32_t mmf_count; 	//<. this will hold position opened for mmf

	OrderType mmf_order;

	std::vector<double> pnl_repository;
	bool pnl_check;
	double adjustment_factor;

	bool opened_for_close;

	double pif; //price in force
	uint32_t rif; //round in force


	uint32_t maximum_order_size;

	OrderInfo* order_change;
	OrderInfo* order;
	int64_t position;

	AbstractTradeAccount *other;

	OrderInfo::onOrderAcknowledgmentFunctionType *onAcknowledgement;
	OrderInfo::onFillFunctionType *onOpenFillReceived;
	OrderInfo::onFillFunctionType *onOpenWithMMFFillReceived;
	OrderInfo::onFillFunctionType *onCloseFillReceived;
	OrderInfo::onFillFunctionType *onMMFCloseFillReceived;
	OrderInfo::onOrderCancelFunctionType *onOrderCancelReceived;
	OrderInfo::onOrderChangeFunctionType *onOrderChangeResponse;
	OrderInfo::onOrderDoneFunctionType *onOrderDoneReceived;



	size_t retry;


	enum {
		ORDER_CHANGE_WAIT = 0,
		ORDER_CHANGED,
		ORDER_UNCHANGED,
		ORDER_CHANGE_NONE,
	} is_order_change;

	bool cancel_change_order_on_acknowledgement;

	bool is_cancel_rejected;
	bool is_cancel_in_process;
	bool cancelation_required;

	uint32_t max_pos;
	uint32_t tranch_value;

	uint32_t tranch_counter;

public:
	OrderAccount& account;
	const uint32_t divisor;


	using OrderDoneFunctionType = OrderInfo::onOrderDoneFunctionType;

	virtual void onOpenFill(OrderType&, const double, const uint32_t) = 0;
	virtual void onCloseFill(OrderType&, const double, const uint32_t) = 0;
	virtual ~AbstractTradeAccount();


	//order sending loop run
	void sendPendingOrders();	//<. parameter is send price in case of SCO
	void executeSCOPendingOrders(std::function<std::pair<double, uint32_t>(uint32_t)>); //call with sendPrice function to calculate price
	//new open/cloe order
	void onNewOrderAcknowledgement(OrderType&, bool);
	void onOrderDone(OrderType& );
	bool waitAcknowledgement(OrderType& );



	uint32_t open(const double, uint32_t, bool = false);	//bool is for if to send mmf order on fills
	uint32_t open(const double, uint32_t, TradeDone);
	uint32_t close(const double,uint32_t, bool = false);	//bool is for indicating if order is FCO
	uint32_t close(const double,uint32_t, OrderType&);		//open_order associated with close order
	uint32_t close(const double,uint32_t, TradeDone);	//trade done function
	//change
	bool waitChangeAcknowledgement(OrderType&);	//order waiting for change acknowledgement
	void onOrderChange(OrderType&, bool);
	uint32_t change(const uint32_t, const double, const uint32_t = 0);	//last argument is size to be change optional


	//cancelation
	void onOrderCancel(std::shared_ptr<OrderInfo>&, bool);	//< callback function when order is cancelled
	bool waitCancelAcknowledgement(OrderType&);
	bool cancel(const uint32_t = 0);
	bool cancelAll();

	bool checkOrderStatus(uint32_t);

	//mmf handling
	void onOpenFillsWithMMFOrders(OrderType&, const double, const uint32_t);
	void onMMFOrderFills(OrderType&, const double, const uint32_t);
	void onFCOrderFills(OrderType&, const double, const uint32_t);
	void reopenPositionAfterMMFOrderCompletes(OrderType&);
	double mmfPrice();

	void closePending(double, uint32_t, TradeDone);
	void openPending(double, uint32_t, TradeDone);


	AbstractTradeAccount(const std::string &, OrderAccount &account, uint32_t divisor);
	void setOtherAccount(AbstractTradeAccount *);
	void setMaxPosition(uint32_t);
	void setTranchValue(uint32_t);
	uint32_t getTranchCounter();

	void inc_tranch();
	void dec_tranch();
	uint32_t getTranch();
	bool hedgedModeCondition(double, double);
	void printTable();
	void reOpenOrderWhenDone(OrderType&);	//to be called when order is done

	void changeOrderPrice(const double);	//< change order price to the value provided in the argument
	void onChangeResponse(std::shared_ptr<OrderInfo>&, bool);			//< callback function to respond to change or not change
	bool waitOrderChange();					//<wait for order response (changed or unchanged)

	bool checkOrder();
	void resetPNL();
	double getOpenPNL(double);
	int64_t getPosition();
	int64_t getTotal();
	uint32_t getPos();	//this works on internal data-structure position

	void repackagePositionTable(uint32_t, double, double = 0.0);
	double getPNL();
	void resetPositionTable();
	double startingPrice();
	double endingPrice();

	void adjustOtherAverage();
	double getLastClosePrice();
	double getLastOpenPrice();
	double getPriceInForce();
	void setPriceInForce(const double);
	void setPriceInForceAsAverage();

	void setRoundInForce(const uint32_t);
	uint32_t getRoundInForce();

	void setTotalExecuted(int64_t totalExec);
	void resetLastOpenPrice();
	void resetLastClosePrice();

	double getAverage();
	void setAverage(double average);
	void calculateSpread(double, uint32_t, bool);
	void setSpread(double);
	double getSpread();
	void printOpenFills();
	void printCloseFills();

	double lastOpenPrice();
	int getTotalLiveOrderCount();


	void softClose(const double);
	void panicClose(const double);

	bool checkPNL(const double);

	void setTotalOpen(int64_t open);
	void setTotalClose(int64_t close);

	void setPosition(int64_t);
	std::string getSymbolName();
	std::string getPositionString();

	void setMMF(const uint32_t mmf, const double mpv);
	void applyMMF();
	void resetMMF();
	void countMMF(OrderType&, uint32_t);
	void changeMMF();
	uint32_t getCountMMF();

	void setMaximumOrderLimit(const uint32_t);
	uint32_t getMaximumOrderLimit();

	RedisWriter* getRedisWriter()  {
			return redisWriter;
	}

	void setRedisWriter( RedisWriter* redisWriter) {
			this->redisWriter = redisWriter;
	}
	int64_t getTotalOpen(){
		return totalopen;
	}

	int64_t getTotalClose(){
		return totalclose;
	}
	int64_t getTotalExecuted(){
		return totalExecuted;
	}

	void setOpenedForClose();
	void resetOpenedForClose();
	PositionRecordTable& positionTable();
	void deleteCompletePositions();


	std::pair<uint32_t, double> sizeofCloseOrderWithPNL(const double);
};

inline void AbstractTradeAccount::setMaximumOrderLimit(const uint32_t size ) {
	maximum_order_size = size;
	TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"setMaximumOrderLimit: "<<maximum_order_size;
}

inline uint32_t AbstractTradeAccount::getMaximumOrderLimit() {
	TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"getMaximumOrderLimit: "<<maximum_order_size;
	return maximum_order_size;
}

inline void AbstractTradeAccount::setPriceInForce(const double pif) {
	std::lock_guard<std::mutex> lock(rw_mutex);
	TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"setting PIF: "<<pif;
	this->pif = pif;
	std::string keyToRead=account.getAccountName()+":"+getSymbolName();
	std::ostringstream os;
	os << this->pif;
	string redisPIF= os.str();
	bool response=redisWriter->writePIFToRedis(keyToRead,redisPIF);
	if(response==true){
		TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"setting PIF to redis was sucessfull: "<<pif;
	}
	else{
		TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"setting PIF to redis was unsucessfull: "<<pif;
	}
}

inline void AbstractTradeAccount::setPriceInForceAsAverage() {
	std::lock_guard<std::mutex> lock(rw_mutex);
	TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"setting PIF: "<<average;
	this->pif = average;
}

inline double AbstractTradeAccount::getPriceInForce() {
	std::lock_guard<std::mutex> lock(rw_mutex);
//	TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"pif: "<<pif;
	return pif;
}

inline bool AbstractTradeAccount::checkPNL(const double close_price) {
	if ( account.getAccountType() == ORDER_ACCOUNT_LONG )
		return close_price > average;
	else return close_price < average;
}

inline void AbstractTradeAccount::setRoundInForce(const uint32_t rif) {
	TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"setting RIF: "<<rif;
	this->rif = rif;
	std::string keyToRead=account.getAccountName()+":"+getSymbolName();
	 std::ostringstream os;
	 os << this->rif;
	 string redisRIF= os.str();
	bool response=redisWriter->writeRIFToRedis(keyToRead,redisRIF);
	if(response==true){
			TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"setting RIF to redis was sucessfull: "<<pif;
	}
	else{
			TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"setting RIF to redis was unsucessfull: "<<pif;
	}
}

inline uint32_t AbstractTradeAccount::getRoundInForce() {
//	TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"getRoundInForce: "<<rif;
	return rif;
}

inline void AbstractTradeAccount::setTotalExecuted(int64_t totalExec) {
	totalExecuted = totalExec;
}

inline void AbstractTradeAccount::printOpenFills() {

	auto curr_pos = getPosition();
	auto oppo_pos = other->getPosition();
	if ( account.getAccountType() == OrderAccountType::ORDER_ACCOUNT_LONG )
		DEBUG<<LOG_ABSTRACT_TRADE_ACCOUNT<<"onOpenFill: ("<<symbol<<","<<curr_pos<<","<<average<<","<<oppo_pos<<","<<other->average<<")";
	else if ( account.getAccountType() == OrderAccountType::ORDER_ACCOUNT_SHORT )
		DEBUG<<LOG_ABSTRACT_TRADE_ACCOUNT<<"onOpenFill: ("<<symbol<<","<<oppo_pos<<","<<other->average<<","<<curr_pos<<","<<average<<")";
}

inline std::string AbstractTradeAccount::getPositionString() {
	auto ret = string("");
	auto pos = getPosition();
	if ( account.getAccountType() == ORDER_ACCOUNT_LONG ) {
		ret = boost::lexical_cast<std::string>(pos);
		ret += "L,";
	} else if ( account.getAccountType() == ORDER_ACCOUNT_SHORT ){	//if short
		pos = -1*pos;
		ret = boost::lexical_cast<std::string>(pos);
		ret += "S,";
	}

//	ret += boost::lexical_cast<std::string>(getAverage());

	std::stringstream st;


	st<<ret<<std::fixed<<std::setprecision(2)<<getAverage();

	return st.str();
}

inline void AbstractTradeAccount::setPosition(int64_t position) {
	total = position;
}

inline void AbstractTradeAccount::setMMF(const uint32_t mmf, const double mpv) {
	this->mmf_round = mmf;
	this->mmf_mpv = mpv;
}

inline double AbstractTradeAccount::mmfPrice() {
	double mmf_price = 0.0;
	if ( account.getAccountType() == ORDER_ACCOUNT_LONG ) {
		mmf_price = truncate(average, 2) + mmf_mpv;
	} else if ( account.getAccountType() == ORDER_ACCOUNT_SHORT ) {
		mmf_price = dceil(average, 2) - mmf_mpv;
	} else {
		WARN<<LOG_ABSTRACT_TRADE_ACCOUNT<<"mmfPrice: unknown account type";
	}

	return mmf_price;
}

inline double AbstractTradeAccount::lastOpenPrice() {
	return last_open_price;
}

inline void AbstractTradeAccount::setOpenedForClose() {
	opened_for_close = true;
}

inline void AbstractTradeAccount::resetOpenedForClose() {
	opened_for_close = false;
}

inline void AbstractTradeAccount::setSpread(double spread) {
	this->spread = spread;
}

inline double AbstractTradeAccount::getSpread() {
	return spread;
}

inline void AbstractTradeAccount::calculateSpread(double fill_price, uint32_t fill_size, bool open_close) {
	auto disparity = abs(this->getTotal()-other->getTotal());	//disparity before fill
	auto equality = other->getTotal();							//total at equality

	if ( total < equality //if current account is less than opposite before fill
			&& ( total + fill_size )  >= equality //and fill current become equal or greater than opposite
			&& open_close ) { //fill received is open fills
		auto average_sp = average + ( fill_price - average) * disparity/equality;
		spread = fabs(average_sp - other->getAverage());
		other->setSpread(spread);
	} else if ( total > equality	//if current is more than opposite
			&& ( ( total - fill_size ) <= equality ) //and after fill it will go below or equal to opposite
			&& !open_close ) {	//fill received is for close fills
		auto average_sp = average - ( fill_price - average) * disparity/equality;
		spread = fabs(average_sp - other->getAverage());
		other->setSpread(spread);
	}
}


inline std::pair<uint32_t, double> AbstractTradeAccount::sizeofCloseOrderWithPNL(const double send_price) {
	auto ret = position_table.sizeofOrderWithPnl(send_price);
	ret.first *= divisor;
	ret.first = std::min(ret.first, uint32_t(getPosition()));
	return ret;
}

inline double AbstractTradeAccount::getOpenPNL(const double reference_price) {
	int32_t sign = account.getAccountType() == ORDER_ACCOUNT_LONG ? 1 : -1;
	return (reference_price - position_table.getAverage())*position_table.getOpenSize()*sign;
}

inline void AbstractTradeAccount::repackagePositionTable(uint32_t base_qty, double mpv, double adjustment ) {
	position_table.repackaging(base_qty, mpv, adjustment);
}

inline double AbstractTradeAccount::startingPrice() {
	return position_table.startingPrice();
}

inline double AbstractTradeAccount::endingPrice() {
	return position_table.endingPrice();
}

inline void AbstractTradeAccount::resetPNL() {
	position_table.setPNL(0.0);
}

inline double AbstractTradeAccount::getPNL() {
	return position_table.getPNL();
}

inline void AbstractTradeAccount::resetPositionTable() {
	position_table.setPNL(0.0);
}

inline PositionRecordTable& AbstractTradeAccount::positionTable() {
	return position_table;
}

inline void AbstractTradeAccount::setMaxPosition(uint32_t pos) {
	max_pos = pos;
}

inline void AbstractTradeAccount::setTranchValue(uint32_t val) {
	tranch_value = val;
}

inline uint32_t AbstractTradeAccount::getTranchCounter() {
	return tranch_counter;
}

inline void AbstractTradeAccount::inc_tranch() {
	tranch_counter++;
}

inline void AbstractTradeAccount::dec_tranch() {
	tranch_counter--;
}

inline uint32_t AbstractTradeAccount::getTranch() {
	return tranch_counter;
}

inline bool AbstractTradeAccount::hedgedModeCondition(double bid, double ask) {
	auto endprice = getLastOpenPrice();
	TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"bid = "<<bid<<" ask= "<<ask<<" lastOpenPrice()= "<<endprice;
	bool price_cond = ( account.getAccountType() == ORDER_ACCOUNT_LONG ) ? bid < endprice : ask > endprice;
	return ( ( getPosition() - max_pos*tranch_counter == max_pos ) && price_cond );
}

inline void AbstractTradeAccount::printTable() {
	position_table.print();
}

inline double AbstractTradeAccount::getLastOpenPrice() {
	return last_open_price;
}

inline double AbstractTradeAccount::getLastClosePrice() {
	return last_close_price;
}

inline void AbstractTradeAccount::deleteCompletePositions() {
	position_table.deleteCompletePosition();
}


class position_overflow: public std::exception {
	std::string message;	//message by the exception
	const uint32_t size;	//size of the order that cause the exception
	const double price;		//price of the order that cause the exception
public:
	position_overflow(uint32_t size,
			double price,
			AbstractTradeAccount *account,
			std::string message = "Account has overflown its maximum position"):
		size(size),
		price(price) {
	}

	double getPrice() { return price; }

	uint32_t getSize() { return size; }

	virtual const char* what() const noexcept(true) {
		return message.c_str();
	}
};


#endif /* SRC_TRADE_ABSTRACTTRADEACCOUNT_H_ */
