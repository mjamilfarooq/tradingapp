/*
 * OrderAccount.h
 *
 *  Created on: Jul 7, 2016
 *      Author: jamil
 */

#ifndef TRADE_ORDERACCOUNT_H_
#define TRADE_ORDERACCOUNT_H_

#include <string>
#include <mutex>
#include <atomic>
#include <iostream>
#include <utility>
#include <condition_variable>
#include <functional>
#include <memory>


#include <edge.h>
#include <linkx/Client.h>
#include <linkx/Types.h>
#include <xbow/Connection.h>
#include <db/RecordListReader.h>
#include <Execution/Records.h>


#include "../config/SymbolConfig.h"
#include "../utils/log.h"
#include "../utils/AsynchronousUnorderedMap.h"

#define LOG_ORDER_ACCOUNT " <"<<username<<","<<symbol<<","<<account_type_string<<">: "
#define ORDER_ACCOUNT_MAXIMUM_LIMIT 30

//#define DEBUG std::cout<<std::endl
//#define WARN std::cout<<std::endl
//#define TRACE std::cout<<std::endl
//#define ERROR std::cerr<<std::endl
//#define FATAL std::cerr<<std::endl
//#define INFO std::cout<<std::endl




enum OrderAccountType {
	ORDER_ACCOUNT_LONG,
	ORDER_ACCOUNT_SHORT
};

using PositionList = std::vector<Execution::PositionRecord>;



struct OrderInfo {
	//stages of order will be define as one of the following
	const static int8_t STAGE_SENT = 0;
	const static int8_t STAGE_ACKNOWLEDGED = 1;
	const static int8_t STAGE_FILLED = 2;
	const static int8_t STAGE_CANCELLED = 3;
	const static int8_t STAGE_CHANGED = 4;
	const static int8_t STAGE_DONE = 5;
	const static int8_t STAGE_FAILED = 6;
	//const static int8_t STAGE_NONE = 7;

	bool is_acknowledged;


	enum {
		CANCEL_NONE,
		CANCEL_INIT,
		CANCEL_FAIL,
		CANCEL_DONE,
	} cancel_stage;

	enum {
		CHANGE_NONE,
		CHANGE_INIT,
		CHANGE_FAIL,
		CHANGE_DONE,
	} change_stage;

	uint8_t stage;
	uint8_t type;
	std::string username;
	const std::string symbol;
	double price_double;

	//last should be the request_id
	uint32_t request_id;
	OrderAccountType account_type;
	uint32_t size;

	uint32_t fill_size;

	Execution::Order order;
	Execution::OrderCancel cancel;
	Execution::Fill fill;
	Execution::OrderChange change;

	using onOrderAcknowledgmentFunctionType = std::function<void (std::shared_ptr<OrderInfo>&, bool)>;
	using onFillFunctionType = std::function<void (std::shared_ptr<OrderInfo>&, const double, const uint32_t)>;
	using onOrderCancelFunctionType = std::function<void (std::shared_ptr<OrderInfo>&, bool)>;
	using onOrderChangeFunctionType = std::function<void (std::shared_ptr<OrderInfo>&, bool)>;
	using onVanueChangeFunctionType = std::function<void (bool)>;
	using onOrderDoneFunctionType = std::function<void (std::shared_ptr<OrderInfo>&)>;

	onOrderAcknowledgmentFunctionType *onAcknowledgementFunction;
	onFillFunctionType *onFillFunction;
	onOrderCancelFunctionType *onOrderCancelFunction;
	onOrderChangeFunctionType *onOrderChangeFunction;
	onVanueChangeFunctionType *onVanueChangeFunction;
	onOrderDoneFunctionType *onOrderDoneFunction;

	OrderInfo(string symbol) = delete;

	OrderInfo(const OrderInfo&) = default;

	//for change order we need to provide original order, new price, and order id for new order
	OrderInfo(const OrderInfo& order, const double new_price):
		OrderInfo(order) {
		price_double = new_price;
		change_stage = CHANGE_NONE;
		cancel_stage = CANCEL_NONE;
		is_acknowledged = false;
		request_id = 0;
		fill_size = 0;
	}

	//for change order we need to provide original order, new price, new_size and order id for new order
	OrderInfo(const OrderInfo& order, const double new_price, const uint32_t new_size):
		OrderInfo(order) {

		size = new_size == 0 ? order.size : new_size;
		price_double = new_price;
		stage = STAGE_SENT;
		change_stage = CHANGE_NONE;
		cancel_stage = CANCEL_NONE;
		is_acknowledged = false;
		request_id = 0;
		fill_size = 0;
	}

	OrderInfo(std::string symbol, uint32_t size, double price):
		symbol(symbol),
		size(size),
		price_double(price),
		change_stage(CHANGE_NONE),
		cancel_stage(CANCEL_NONE),
		is_acknowledged(false),
		stage(STAGE_SENT),
		fill_size(0),
		type(0),	//no type
		account_type(ORDER_ACCOUNT_LONG),
		request_id(0),
		onAcknowledgementFunction(nullptr),
		onFillFunction(nullptr),
		onOrderCancelFunction(nullptr),
		onOrderChangeFunction(nullptr),
		onVanueChangeFunction(nullptr),
		onOrderDoneFunction(nullptr) {

	}

//	OrderInfo(uint8_t type, const std::string &username, const std::string &symbol, double price, uint32_t request_id, OrderAccountType account_type, uint32_t size):
//		stage(STAGE_SENT), type(type), username(username), symbol(symbol), price_double(price), request_id(request_id), account_type(account_type), size(size),
//		onAcknowledgementFunction(nullptr),
//		onFillFunction(nullptr),
//		onOrderCancelFunction(nullptr),
//		onOrderChangeFunction(nullptr),
//		change_stage(CHANGE_NONE),
//		cancel_stage(CANCEL_NONE),
//		is_acknowledged(false),
//		fill_size(0) {
//
//		::memset(&order, 0, sizeof(order));
//		::memset(&cancel, 0, sizeof(cancel));
//		::memset(&fill, 0, sizeof(fill));
//		::memset(&change, 0, sizeof(change));
//	}

	bool isTypeOpen() {
		if (account_type == ORDER_ACCOUNT_LONG ) {
			if ( type == Execution::BS_BUY ) return true;
			else if ( type == Execution::BS_SELL ) return false;
			else ERROR<<"OrderInfo::isTypeOpen--> unknown order type (undefine behavior)";
		} else if ( account_type == ORDER_ACCOUNT_SHORT ) {
			if ( type == Execution::BS_SELL_SHORT ) return true;
			else if ( type == Execution::BS_BUY ) return false;
			else ERROR<<"OrderInfo::isTypeOpen--> unknown order type (undefine behavior)";
		}

		ERROR<<"OrderInfo::isTypeOpen--> unknown account type";
	}

	bool isAcknowledged() {
		return stage != STAGE_SENT || is_acknowledged ;
	}

	bool canCancel() {
		if ( stage == OrderInfo::STAGE_CANCELLED ||
						stage == OrderInfo::STAGE_DONE ||
						stage == OrderInfo::STAGE_FILLED ||
						stage == OrderInfo::STAGE_FAILED )
					return false;
		return true;
	}

	~OrderInfo() {
		TRACE<<"~OrderInfo: erasing order with id "<<request_id;
	}
};

using OrderType = std::shared_ptr<OrderInfo>;

using OrderHash = AsynchronousUnorderedMap<uint32_t, std::shared_ptr<OrderInfo>>;

class OrderAccount:public Execution::Connection {
	std::string username;					//username for an account
	std::string gateway;
	OrderHash order_hash;					//order hash for an account
	std::mutex order_hash_mutex;
	std::atomic<uint32_t> request_id_generator;	//< one for an account
	OrderAccountType account_type;
	const std::string account_type_string;

	OrderAccount *current;
	std::unique_ptr<OrderAccount> next;


public:
	bool initOrderAccount();
	OrderAccount(Linkx::Client &, std::string, OrderAccountType, std::vector<std::string>, OrderAccount * = nullptr);

	/**
	 *	@brief	virtual callback called when the socket connection to a router is reestablished
	 *
	 *	@param	pName name of the connected service
	 *	@param	pAddress Linkx address of the connected service
	 *
	 * 	@return see eerror.h
	 */
	virtual unsigned OnConnect(char *pName, Linkx::Address &address);

	/**
	 *	@brief	virtual callback called when the connection to the service breaks
	 *
	 * 	@return see eerror.h
	 */
	virtual unsigned OnDisconnect(void);

	/**
	 *	@brief	virtual callback called when the socket connection to a router is reestablished
	 *
	 *	@param	pName name of the connected service
	 *	@param	pAddress Linkx address of the connected service
	 *
	 * 	@return see eerror.h
	 */
	virtual unsigned OnReconnect(char *pName, Linkx::Address &address);

	/**
	 *	@brief	Order callback
	 *
	 *	@param	requestId request Id for this order
	 *	@param	order order message
	 */
	virtual void OnOrder(const unsigned requestId, const Execution::Order &order);

	/**
	 *	@brief	Order cancel callback
	 *
	 *	@param	requestId request Id for this order
	 *	@param	cancel cancel message
	 */
	virtual void OnOrderCancel(const unsigned requestId, const Execution::OrderCancel &cancel);

	/**
	 *	@brief	Order change callback
	 *
	 *	@param	requestId request Id for this order
	 *	@param	change change message
	 */
	virtual void OnOrderChange(const unsigned requestId, const Execution::OrderChange &change);

	/**
	 *	@brief	Fill callback
	 *
	 *	@param	requestId request Id for the order this fill refers to
	 *	@param	fill fill message
	 */
	virtual void OnFill(const unsigned requestId, const Execution::Fill &fill);


	/*
	 * @brief Limit Buying
	 *
	 * @param symbol to buy
	 * @param price to buy to be executed
	 * @param amount of share to purchase
	 *
	 * @return Pointer to OrderInfo
	 */
	OrderInfo* limitBuy(const std::string &symbol, const double price, const size_t size, OrderInfo::onOrderAcknowledgmentFunctionType *onAcknowledgement, OrderInfo::onFillFunctionType *fill_func,OrderInfo::onVanueChangeFunctionType *onVenueChange_func );


	bool limitBuy(std::shared_ptr<OrderInfo>&);


	/*
	 * @brief Limit Selling
	 *
	 * @param symbol to sell
	 * @param price to sell to be executed
	 * @param amount of share to be sold
	 *
	 * @return Pointer to OrderInfo
	 */
	OrderInfo* limitSell(const std::string &symbol, const double price, const size_t size, OrderInfo::onOrderAcknowledgmentFunctionType *onAcknowledgement, OrderInfo::onFillFunctionType *fill_func,OrderInfo::onVanueChangeFunctionType *onVenueChange_func );

	bool limitSell(std::shared_ptr<OrderInfo>&);


	bool open(std::shared_ptr<OrderInfo> &);
	bool close(std::shared_ptr<OrderInfo> &);
	bool cancel(OrderType& );
	OrderType change(OrderType& , const double );
	bool change(OrderType&, OrderType&);



	/*
	 * @brief Limit Buying
	 *
	 * @param symbol to buy
	 *
	 * @return true if operation complete successfully
	 */
	bool cancelPendingBuy(const std::string &symbol);


	/*
	 * @brief Limit Selling
	 *
	 * @param symbol to sell
	 * @param price to sell to be executed
	 * @param amount of share to be sold
	 *
	 * @return true if operation complete successfully
	 */
	bool cancelPendingSell(const std::string &symbol);


	/*
	 * @brief Get All Positions
, 	 *
	 * @param reference to PositionList, a vector containing all the PositionRecords
	 * @return doesn't return anything
	 */
	void getAllPositions(PositionList &);

	/*
	 * @brief Get Symbol position for Account [under username]
	 *
	 * @param reference to symbol name to get position for
	 *
	 * @return should return PositionRecord
	 */
	int64_t getPosition(const std::string &);

	Execution::PositionTotals getPositionTotal(const std::string &);


	bool orderCancel(OrderInfo &, OrderInfo::onOrderCancelFunctionType *);

	OrderAccountType getAccountType() {
		return account_type;
	}

	std::string getAccountTypeString() {
		if ( account_type == ORDER_ACCOUNT_LONG ) return "long";
		else if ( account_type == ORDER_ACCOUNT_SHORT ) return "short";
	}

	bool getStatus(const OrderInfo &);

	string getAccountName(){
		return username;
	}
	string getGatewayName(){
		return gateway;
	}
	virtual ~OrderAccount();
};

#endif /* TRADE_ORDERACCOUNT_H_ */
