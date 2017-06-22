/*
 * AbstractTradeAccount.cpp
 *
 *  Created on: Aug 15, 2016
 *      Author: jamil
 */

#include "AbstractTradeAccount.h"

AbstractTradeAccount::AbstractTradeAccount(const std::string &symbol, OrderAccount &account, uint32_t divisor):
	symbol(symbol),
	total(0),
	average(0.0),
	spread(0.0),
	adjustment_factor(0.0),
	pnl_check(false),
	account(account),
	order(nullptr),
	order_change(nullptr),
	position(0),
	other(nullptr),
	onAcknowledgement( nullptr ),
	onOpenFillReceived(nullptr),
	onCloseFillReceived(nullptr),
	onOrderCancelReceived(nullptr),
	last_close_price(0.0),
	is_order_change(ORDER_CHANGE_NONE),
	maximum_order_size(0),
	pif(0.0),
	rif(0),
	totalopen(0),
	totalclose(0),
	totalExecuted(0),
	is_cancel_in_process(false),
	mmf_order(nullptr),
	divisor(divisor),
	open_live(0),
	close_live(0),
	opened_for_close(false),
	max_pos(0),
	tranch_counter(0),
	tranch_value(0),
	position_table(account.getAccountType() == ORDER_ACCOUNT_LONG ? true : false ) {


	onOrderChangeResponse = new OrderInfo::onOrderChangeFunctionType(std::bind(&AbstractTradeAccount::onOrderChange, this, std::placeholders::_1, std::placeholders::_2));
	onAcknowledgement = new OrderInfo::onOrderAcknowledgmentFunctionType(std::bind(&AbstractTradeAccount::onNewOrderAcknowledgement, this, std::placeholders::_1, std::placeholders::_2));
	onOrderCancelReceived = new OrderInfo::onOrderCancelFunctionType(std::bind(&AbstractTradeAccount::onOrderCancel, this, std::placeholders::_1, std::placeholders::_2));
	onMMFCloseFillReceived = new OrderInfo::onFillFunctionType(std::bind(&AbstractTradeAccount::onMMFOrderFills, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	onOpenWithMMFFillReceived = new OrderInfo::onFillFunctionType(std::bind(&AbstractTradeAccount::onOpenFillsWithMMFOrders, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	onOrderDoneReceived = new OrderInfo::onOrderDoneFunctionType(std::bind(&AbstractTradeAccount::onOrderDone, this, std::placeholders::_1));
}

AbstractTradeAccount::~AbstractTradeAccount() {

}

/****************************** sending pending orders *****************************/
void AbstractTradeAccount::sendPendingOrders() {

	//in case of failure this function will return without poping out the top order
	if ( !pending_orders.size() ) {
//		TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<" no pending orders";
		return;
	}

	auto position_request = pending_orders.front();
	if ( position_request.type == PositionRequest::OPEN ) {
		auto request_id = open(position_request.price_double, position_request.size, position_request.trade_done_function);
		if ( !request_id ) {
			TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"sendPendingOrders open position failure";
			return;
		} else {
			TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"sendPendingOrders open position successfully with id "<<request_id;
		}
	} else if ( position_request.type ==  PositionRequest::CLOSE ) {

		auto request_id = close(position_request.price_double, position_request.size, position_request.trade_done_function);
		if ( !request_id ) {
			WARN<<LOG_ABSTRACT_TRADE_ACCOUNT<<"sendPendingOrders close position failure";
			return;
		} else {
			TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"sendPendingOrders close position successfully with id "<<request_id;
		}
	}

	pending_orders.pop();	//removing the

}

void AbstractTradeAccount::executeSCOPendingOrders(std::function<std::pair<double, uint32_t>(uint32_t)> validity) {

	if ( pending_orders.size() ) {
		//if orders are pending to be sent
		auto position_request = pending_orders.front();

		if ( position_request.is_sco ) {	//secondary close order

			auto valid_order = validity(position_request.size);
			auto price = valid_order.first;	//this will calculate current price for SCO order to be sent
			auto size = valid_order.second;	//this will calculate size to close only open positions
			if ( size > 0 ) {
				auto request_id = close(price, size);
				if ( !request_id ) {
					WARN<<LOG_ABSTRACT_TRADE_ACCOUNT<<"sendPendingOrders SCO close position failure";
				}
			}
		}

		pending_orders.pop();

	}

	//update already sent SCOs
	std::for_each(orders.begin(), orders.end(), [this, &validity](decltype(orders)::value_type &value) {
		auto request_id = value.first;
		auto order_info = std::static_pointer_cast<PositionRequest>(value.second);
		double new_price = validity(close_live).first;
		if ( order_info->is_sco && new_price != order_info->price_double ) {
			auto changed_order = change(request_id, new_price);
		}
	});
}

/****************************** handling MMF orders ********************************/
void AbstractTradeAccount::onOpenFillsWithMMFOrders(OrderType& open_order, const double fill_price, const uint32_t fill_size) {

	TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"onOpenFillsWithMMFOrders";
	auto&& position_request = static_pointer_cast<PositionRequest>(open_order);
	if ( !position_request->send_mmf_on_fills ) {
		TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"no mmf order requires to for closing";
		return;
	}

	//pushing mmf close request into the pending orders queue
	PositionRequest close_request(position_request->symbol, fill_size, mmfPrice());
	close_request.is_mmf = true;
	close_request.type = PositionRequest::CLOSE;
	close_request.open_order = open_order;
	pending_orders.push(close_request);

}

void AbstractTradeAccount::onMMFOrderFills(OrderType& order_info, const double price, const uint32_t fills) {

	TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"onMMFOrderFills:";

	auto&& close_order = static_pointer_cast<PositionRequest> (order_info);	//this is the position we have received
	if ( !close_order->is_mmf ) {
		TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"onMMFOrderFills: not MMF order";
		return;
	}

	//if mmf order record closed position in corresponding open_order
	auto&& open_order = static_pointer_cast<PositionRequest>(close_order->open_order);
	if ( open_order == nullptr ) {
		WARN<<LOG_ABSTRACT_TRADE_ACCOUNT<<"no open order associated with close order";
		return;
	}


	open_order->mmf_closed += fills;
	if ( open_order->size > open_order->mmf_closed ) {
		TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"order size = "<<open_order->size<<" mmf_closed ="<<open_order->mmf_closed;
		return;
	}

	//pushing mmf close request into the pending orders queue
	TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"reopening position @"<<open_order->price_double<<" for "<<open_order->size;
	PositionRequest reopen_request(open_order->symbol, open_order->size*divisor, open_order->price_double);
	reopen_request.send_mmf_on_fills = true;
	reopen_request.type = PositionRequest::OPEN;
	pending_orders.push(reopen_request);
}

/*
 * @brief call by onCloseFills check if close order is FCO then queue new SCO in opposite account
 *
 * @param close order against which a fill is received
 *
 * @param price of order
 *
 * @param size of fills
 *
 * @return nothing
 */
void AbstractTradeAccount::onFCOrderFills(OrderType& fco_order, const double price, const uint32_t fills) {
	TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"onFCOrderFills";

	auto&& position_request = std::static_pointer_cast<PositionRequest>(fco_order);
	if ( !position_request->is_fco ) {
		TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"onFCOrderFills: not FCO order";
		return;
	}

	//pushing SCO close request into the pending orders queue of other account
	PositionRequest close_request(position_request->symbol, fills*divisor, mmfPrice());
	close_request.is_sco = true;
	close_request.type = PositionRequest::CLOSE;
	other->pending_orders.push(close_request);

}

void AbstractTradeAccount::closePending(double price, uint32_t size, TradeDone trade_done) {
	PositionRequest request(symbol, size, price);
	request.type = PositionRequest::CLOSE;
	request.is_trade_done_define = true;
	request.trade_done_function = trade_done;
	pending_orders.push(request);

}

void AbstractTradeAccount::openPending(double price, uint32_t size, TradeDone trade_done) {
	PositionRequest request(symbol, size, price);
	request.type = PositionRequest::CLOSE;
	request.is_trade_done_define = true;
	request.trade_done_function = trade_done;
	pending_orders.push(request);
}

/**************************** handling open/close functinality **************************/

void AbstractTradeAccount::onNewOrderAcknowledgement(OrderType& order_info, bool is_success) {
	std::lock_guard<std::mutex> lock(mt);
	cv.notify_all();
}

void AbstractTradeAccount::onOrderDone(OrderType& order_info) {
	auto&& position_request = static_pointer_cast<PositionRequest>(order_info);

	auto on_trade_done_function = position_request->trade_done_function;
	auto is_trade_done_define = position_request->is_trade_done_define;
	auto price = position_request->price_double;
	auto size = position_request->size;


	std::lock_guard<std::mutex> lock(mt);
	auto request_id = order_info->request_id;

	if ( position_request->is_mmf ) {
		TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"onOrderDone: mmf order with "<<request_id<<" completed!";
		//will see if I have to assign nullptr to corresponding open_order
	}

	if ( is_trade_done_define ) {
		position_request->trade_done_function(order_info->price_double, order_info->size);
	}


	totalExecuted+=order_info->size;
	//TRACE<<"Order Done for"<<account.getAccountName()<<":"<<getSymbolName()<<" "<<totalExecuted;
	if(order_info->account_type==ORDER_ACCOUNT_LONG){
		if(order_info->type==Execution::BS_SELL_SHORT || order_info->type==Execution::BS_SELL){
			totalclose=totalclose-order_info->size;
			TRACE<<"livestats Order Done as acoount was long sell "<<account.getAccountName()<<":"<<getSymbolName()<<"close before "<<totalclose+order_info->size<<" total close after:"<<totalclose;
		}else if(order_info->type==Execution::BS_BUY){
			totalopen=totalopen-order_info->size;
			TRACE<<"livestats Order Done as acoount was long buy"<<account.getAccountName()<<":"<<getSymbolName()<<"open before "<<totalopen+order_info->size<<" total open after:"<<totalopen;

		}
	}
	else {
		if(order_info->type==Execution::BS_SELL_SHORT || order_info->type==Execution::BS_SELL){
			totalopen=totalopen-order_info->size;
			TRACE<<"livestats Order Done as acoount was short sell"<<account.getAccountName()<<":"<<getSymbolName()<<"open before "<<totalopen+order_info->size<<" total open after:"<<totalopen;
		}else if(order_info->type==Execution::BS_BUY){
			totalclose=totalclose-order_info->size;
			TRACE<<"livestats Order Done as acoount was short buy"<<account.getAccountName()<<":"<<getSymbolName()<<"close before "<<totalclose+order_info->size<<" total close after:"<<totalclose;
		}
	}
	std::string time=redisWriter->getCurrentTime();
	redisWriter->updateOrderExecTime(account.getAccountName(),std::to_string(request_id),time);


	TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"onOrderDone: erasing order with id "<<request_id;
	auto del = orders.erase(request_id);
	TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"onOrderDone: order deleted with "<<request_id<<" "<<del;

}

bool AbstractTradeAccount::waitAcknowledgement(OrderType& order_info ) {

	TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"waitAcknowledgement: waiting for order acknowledgement";

	std::unique_lock<std::mutex> lock(mt);
	cv.wait(lock, [this, &order_info](){
		return order_info->isAcknowledged();
	});


	TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"waitAcknowledgement: acknowledgement received!";

	if ( !order_info->is_acknowledged ) {
		TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"waitAcknowledgement order failed!";
		return false;
	}

	TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"waitAcknowledgement: order acknowledgement successful -- adding order to container";

	orders.emplace(order_info->request_id, order_info);

	return true;
}

uint32_t AbstractTradeAccount::open(const double price, uint32_t size, bool send_mmf) {

	size /= divisor;

	if ( price <= 0.0 || size > MAXIMUM_ORDER_LIMIT ) {
		WARN<<LOG_ABSTRACT_TRADE_ACCOUNT<<"invalid order price/size";
		return 0;
	}

	DEBUG<<LOG_ABSTRACT_TRADE_ACCOUNT<<"open @"<<price<<" for "<<size<<" send_mmf = "<<send_mmf;
	OrderType order_info = std::make_shared<PositionRequest>(symbol, size, price);
	if ( nullptr == order_info ) {
		FATAL<<LOG_ABSTRACT_TRADE_ACCOUNT<<"Can't allocate memory for new order";
		return 0;
	}

	auto&& position_request = static_pointer_cast<PositionRequest>(order_info);	//as position request pointer
	position_request->send_mmf_on_fills = send_mmf;	//enable send mmf functionality
	position_request->is_trade_done_define = false;
	order_info->onAcknowledgementFunction = onAcknowledgement;
	order_info->onOrderDoneFunction = onOrderDoneReceived;
	order_info->onFillFunction = onOpenFillReceived;




	if ( !account.open(order_info) ) {
		ERROR<<LOG_ABSTRACT_TRADE_ACCOUNT<<"openPosition failure";
		return 0;
	}

	if ( !waitAcknowledgement(order_info) ) {
		TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"waitAcknowledgement received false";
		return 0;
	}

	totalopen+=size;
	TRACE<<"livestats Total open was"<<account.getAccountName()<<":"<<getSymbolName()<<" totalopen before "<<totalopen-size<<" total open after "<<totalopen;

	std::string type;
	if(order_info->type==Execution::BS_SELL_SHORT || order_info->type==Execution::BS_SELL){
		type="sell";
	}else if(order_info->type==Execution::BS_BUY){
		type="buy";
	}
	redisWriter->writeOrderInfoToRedis(account.getAccountName(),std::to_string(order_info->request_id),order_info->symbol,std::to_string(order_info->size),std::to_string(order_info->price_double),type,"",redisWriter->getCurrentTime(),"","");
	return order_info->request_id;
}

uint32_t AbstractTradeAccount::open(const double price, uint32_t size, TradeDone trade_done) {
	size /= divisor;

	if ( price <= 0.0 || size > MAXIMUM_ORDER_LIMIT ) {
		WARN<<LOG_ABSTRACT_TRADE_ACCOUNT<<"invalid order price/size";
		return 0;
	}

	DEBUG<<LOG_ABSTRACT_TRADE_ACCOUNT<<"open @"<<price<<" for "<<size<<" send_mmf = ";
	OrderType order_info = std::make_shared<PositionRequest>(symbol, size, price);
	if ( nullptr == order_info ) {
		FATAL<<LOG_ABSTRACT_TRADE_ACCOUNT<<"Can't allocate memory for new order";
		return 0;
	}

	auto&& position_request = static_pointer_cast<PositionRequest>(order_info);	//as position request pointer
	position_request->trade_done_function = trade_done;
	position_request->send_mmf_on_fills = false;	//enable send mmf functionality
	order_info->onAcknowledgementFunction = onAcknowledgement;
	order_info->onOrderDoneFunction = onOrderDoneReceived;
	order_info->onFillFunction = onOpenFillReceived;




	if ( !account.open(order_info) ) {
		ERROR<<LOG_ABSTRACT_TRADE_ACCOUNT<<"openPosition failure";
		return 0;
	}

	if ( !waitAcknowledgement(order_info) ) {
		TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"waitAcknowledgement received false";
		return 0;
	}

	totalopen+=size;
	TRACE<<"livestats Total open was"<<account.getAccountName()<<":"<<getSymbolName()<<" totalopen before "<<totalopen-size<<" total open after "<<totalopen;

	std::string type;
	if(order_info->type==Execution::BS_SELL_SHORT || order_info->type==Execution::BS_SELL){
		type="sell";
	}else if(order_info->type==Execution::BS_BUY){
		type="buy";
	}
	redisWriter->writeOrderInfoToRedis(account.getAccountName(),std::to_string(order_info->request_id),order_info->symbol,std::to_string(order_info->size),std::to_string(order_info->price_double),type,"",redisWriter->getCurrentTime(),"","");
	return order_info->request_id;
}

void AbstractTradeAccount::onOpenFill(OrderType& order, const double price, const uint32_t size) {
	position_table.recordOpenPosition(order->request_id, size, price);
	printTable();
	last_open_price = price;
}

uint32_t AbstractTradeAccount::close(const double price, uint32_t size, bool is_fco) {

	size /= divisor;
	if ( price <= 0.0 || size > MAXIMUM_ORDER_LIMIT ) {
		WARN<<LOG_ABSTRACT_TRADE_ACCOUNT<<"invalid order price/size";
		return 0;
	}

	DEBUG<<LOG_ABSTRACT_TRADE_ACCOUNT<<"close @"<<price<<" for "<<size;
	OrderType order_info = std::make_shared<PositionRequest>(symbol, size, price);
	if ( nullptr == order_info ) {
		FATAL<<LOG_ABSTRACT_TRADE_ACCOUNT<<"Can't allocate memory for new order";
		return 0;
	}

	auto&& position_request = static_pointer_cast<PositionRequest>(order_info);
	position_request->is_mmf = false;
	position_request->open_order = nullptr;
	position_request->is_fco = is_fco;
	position_request->is_trade_done_define = false;
	order_info->onAcknowledgementFunction = onAcknowledgement;
	order_info->onOrderDoneFunction = onOrderDoneReceived;
	order_info->onFillFunction = onCloseFillReceived;

	if ( !account.close(order_info) ) {
		ERROR<<LOG_ABSTRACT_TRADE_ACCOUNT<<"openPosition failure";
		return 0;
	}

	if ( !waitAcknowledgement(order_info) ) {
		TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"waitAcknowledgement received false";
		return 0;
	}
	totalclose+=size;
	TRACE<<"livestats Total close was"<<account.getAccountName()<<":"<<getSymbolName()<<" totalclose before "<<totalclose-size<<" total close after "<<totalclose;

	std::string type;
		if(order_info->type==Execution::BS_SELL_SHORT || order_info->type==Execution::BS_SELL){
			type="sell";
		}else if(order_info->type==Execution::BS_BUY){
			type="buy";
		}
	redisWriter->writeOrderInfoToRedis(account.getAccountName(),std::to_string(order_info->request_id),order_info->symbol,std::to_string(order_info->size),std::to_string(order_info->price_double),type,"",redisWriter->getCurrentTime(),"","");

	return order_info->request_id;
}

uint32_t AbstractTradeAccount::close(const double price, uint32_t size, TradeDone trade_done) {

	size /= divisor;
	if ( price <= 0.0 || size > MAXIMUM_ORDER_LIMIT ) {
		WARN<<LOG_ABSTRACT_TRADE_ACCOUNT<<"invalid order price/size";
		return 0;
	}

	DEBUG<<LOG_ABSTRACT_TRADE_ACCOUNT<<"close @"<<price<<" for "<<size;
	OrderType order_info = std::make_shared<PositionRequest>(symbol, size, price);
	if ( nullptr == order_info ) {
		FATAL<<LOG_ABSTRACT_TRADE_ACCOUNT<<"Can't allocate memory for new order";
		return 0;
	}

	auto&& position_request = static_pointer_cast<PositionRequest>(order_info);
	position_request->is_mmf = false;
	position_request->open_order = nullptr;
	position_request->is_fco = false;
	position_request->is_trade_done_define = true;
	position_request->trade_done_function = trade_done;
	order_info->onAcknowledgementFunction = onAcknowledgement;
	order_info->onOrderDoneFunction = onOrderDoneReceived;
	order_info->onFillFunction = onCloseFillReceived;

	if ( !account.close(order_info) ) {
		ERROR<<LOG_ABSTRACT_TRADE_ACCOUNT<<"openPosition failure";
		return 0;
	}

	if ( !waitAcknowledgement(order_info) ) {
		TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"waitAcknowledgement received false";
		return 0;
	}
	totalclose+=size;
	TRACE<<"livestats Total close was"<<account.getAccountName()<<":"<<getSymbolName()<<" totalclose before "<<totalclose-size<<" total close after "<<totalclose;

	std::string type;
		if(order_info->type==Execution::BS_SELL_SHORT || order_info->type==Execution::BS_SELL){
			type="sell";
		}else if(order_info->type==Execution::BS_BUY){
			type="buy";
		}
	redisWriter->writeOrderInfoToRedis(account.getAccountName(),std::to_string(order_info->request_id),order_info->symbol,std::to_string(order_info->size),std::to_string(order_info->price_double),type,"",redisWriter->getCurrentTime(),"","");

	return order_info->request_id;
}

uint32_t AbstractTradeAccount::close(const double price, uint32_t size, OrderType& open_order) {

	if ( price <= 0.0 || size > MAXIMUM_ORDER_LIMIT ) {
		WARN<<LOG_ABSTRACT_TRADE_ACCOUNT<<"invalid order price/size";
		return 0;
	}

	DEBUG<<LOG_ABSTRACT_TRADE_ACCOUNT<<"close @"<<price<<" for "<<size;
	OrderType order_info = std::make_shared<PositionRequest>(symbol, size, price);
	if ( nullptr == order_info ) {
		FATAL<<LOG_ABSTRACT_TRADE_ACCOUNT<<"Can't allocate memory for new order";
		return 0;
	}

	auto&& position_request = static_pointer_cast<PositionRequest>(order_info);
	position_request->price_double = mmfPrice();	//current average should always be sent
	position_request->is_mmf = true;
	position_request->open_order = open_order;
	order_info->onAcknowledgementFunction = onAcknowledgement;
	order_info->onOrderDoneFunction = onOrderDoneReceived;
	order_info->onFillFunction = onCloseFillReceived;

	//send new order
	if ( !account.close(order_info) ) {
		ERROR<<LOG_ABSTRACT_TRADE_ACCOUNT<<"close failure";
		return 0;
	}

	if ( !waitAcknowledgement(order_info) ) {
		TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"waitAcknowledgement received false";
		return 0;
	}

	totalclose+=size;
	TRACE<<"livestats Total close was"<<account.getAccountName()<<":"<<getSymbolName()<<" totalclose before "<<totalclose-size<<" total close after "<<totalclose;

	std::string type;
		if(order_info->type==Execution::BS_SELL_SHORT || order_info->type==Execution::BS_SELL){
			type="sell";
		}else if(order_info->type==Execution::BS_BUY){
			type="buy";
		}
	redisWriter->writeOrderInfoToRedis(account.getAccountName(),std::to_string(order_info->request_id),order_info->symbol,std::to_string(order_info->size),std::to_string(order_info->price_double),type,"",redisWriter->getCurrentTime(),"","");

	return order_info->request_id;
}

void AbstractTradeAccount::onCloseFill(OrderType& order, const double price, const uint32_t size) {
	position_table.recordClosePosition(order->request_id, size, price);
	printTable();
	last_close_price = price;
}




/******************** handling order change *****************************/
bool AbstractTradeAccount::waitChangeAcknowledgement(OrderType& order_info) {

	std::unique_lock<std::mutex> lock(mt);
	cv.wait(lock, [this, &order_info](){
		return order_info->isAcknowledged();
	});

	if ( !order_info->is_acknowledged ) {	//if positively acknowledged
		TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"waitChangeAcknowledgement: order failed!";
		return false;
	}

	TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"waitChangeAcknowledgement: order acknowledgement successful -- replacing old order with new one";

	orders.emplace(order_info->request_id, order_info);	//new order added to

	return true;
}

void AbstractTradeAccount::onOrderChange(OrderType& order_info, bool is_changed) {

	if ( is_changed ) {
		TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"order with request id "<<order_info->request_id<<" has changed -- deleting from hash";
		orders.erase(order_info->request_id);
		return;
	} else TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"change request to replace order with id "<<order_info->request_id<<" has been denied";
}

uint32_t AbstractTradeAccount::change( const uint32_t request_id, const double new_price, const uint32_t new_size) {
	TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"change request id:"<<request_id<<" new_price: "<<new_price<<" new_size: "<<new_size;
	auto original_size = new_size/divisor;
	auto&& lookup = orders.find(request_id);
	if ( lookup == orders.end() ) {
		TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"change: no order with id (already done probably) "<<request_id;
		return 0;
	}

	auto original_order = lookup->second;

	TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"change request against "<<original_order->request_id<<" price = "<<new_price;
	OrderType changed_order = std::make_shared<PositionRequest>(original_order, new_price, original_size);
	if ( nullptr == changed_order ) {
		WARN<<LOG_ABSTRACT_TRADE_ACCOUNT<<"can't allocate memory for replaced order";
		return request_id;
	}

	original_order->onOrderChangeFunction = onOrderChangeResponse;

	if ( !account.change(original_order, changed_order) ) {
		WARN<<LOG_ABSTRACT_TRADE_ACCOUNT<<"sending failure for change order";
		return request_id;
	}

	TRACE<<"changing order "<<original_order->request_id<<" with "<<changed_order->request_id;
	if ( !waitChangeAcknowledgement(changed_order) ) {
		WARN<<LOG_ABSTRACT_TRADE_ACCOUNT<<"change order has received with failed acknowledgement";
		return request_id;
	}
	std::string time=redisWriter->getCurrentTime();
	redisWriter->updateOrderChangeTime(account.getAccountName(),std::to_string(request_id),time);

	if(new_size>0){ // means size has been changed
	TRACE<<"livestats change order successfull "<<account.getAccountName()<<":"<<getSymbolName()<<" total open before :"<<totalopen<<" total close before :"<<totalclose;;
	if(changed_order->account_type==ORDER_ACCOUNT_LONG){
			if(changed_order->type==Execution::BS_SELL_SHORT || changed_order->type==Execution::BS_SELL && original_order->type==Execution::BS_SELL){
				totalclose=totalclose-original_order->size;
				totalclose=totalclose+changed_order->size;
			}else if(changed_order->type==Execution::BS_BUY  && original_order->type==Execution::BS_SELL){
				totalclose=totalclose-original_order->size;
				totalopen=totalopen+changed_order->size;
			}else if(changed_order->type==Execution::BS_SELL_SHORT || changed_order->type==Execution::BS_SELL && original_order->type==Execution::BS_BUY){
				totalopen=totalopen-original_order->size;
				totalclose=totalclose+changed_order->size;
			}else if(changed_order->type==Execution::BS_BUY  && original_order->type==Execution::BS_BUY ){
				totalopen=totalopen-original_order->size;
				totalopen=totalopen+changed_order->size;
			}
		}
		else {
			if(changed_order->type==Execution::BS_SELL_SHORT || changed_order->type==Execution::BS_SELL && original_order->type==Execution::BS_SELL){
				totalopen=totalopen-original_order->size;
				totalopen=totalopen+changed_order->size;
			}else if(changed_order->type==Execution::BS_BUY  && original_order->type==Execution::BS_SELL){
				totalopen=totalopen-original_order->size;
				totalclose=totalclose+changed_order->size;
			}else if(changed_order->type==Execution::BS_SELL_SHORT || changed_order->type==Execution::BS_SELL && original_order->type==Execution::BS_BUY){
				totalclose=totalclose-original_order->size;
				totalopen=totalopen+changed_order->size;
			}else if(changed_order->type==Execution::BS_BUY  && original_order->type==Execution::BS_BUY ){
				totalclose=totalclose-original_order->size;
				totalclose=totalclose+changed_order->size;
			}
		}
	}
	TRACE<<"livestats change order successfull "<<account.getAccountName()<<":"<<getSymbolName()<<" orignal size  "<<original_order->size<<" changed size "<<changed_order->size<<" total open:"<<totalopen<<" total close"<<totalclose;
	TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"change order successfull with request id"<<request_id;
	return changed_order->request_id;
}


/************************ handling order cancelation ****************************/

void AbstractTradeAccount::onOrderCancel(std::shared_ptr<OrderInfo>& order_info, bool is_cancel) {
	std::lock_guard<std::mutex> lock(mt);
	if ( is_cancel ) order_info->cancel_stage = OrderInfo::CANCEL_DONE;
	else order_info->cancel_stage = OrderInfo::CANCEL_FAIL;
	cv.notify_all();
}

bool AbstractTradeAccount::waitCancelAcknowledgement(OrderType& cancel_order) {
	std::unique_lock<std::mutex> lock(mt);
	cv.wait(lock, [this, &cancel_order](){
		return cancel_order->cancel_stage == OrderInfo::CANCEL_DONE ||
				cancel_order->cancel_stage == OrderInfo::CANCEL_FAIL;
	});

	if ( cancel_order->cancel_stage == OrderInfo::CANCEL_FAIL ) {
		TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"waitCancelAcknowledgement: order cancelation fails";
		cancel_order->cancel_stage = OrderInfo::CANCEL_NONE;
		return false;
	}

	TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"waitCancelAcknowledgement: order cancelation done";

	if(cancel_order->account_type==ORDER_ACCOUNT_LONG){
		if(cancel_order->type==Execution::BS_SELL_SHORT || cancel_order->type==Execution::BS_SELL){
			totalclose=totalclose-cancel_order->order.openSize;
		}else if(cancel_order->type==Execution::BS_BUY){
			totalopen=totalopen-cancel_order->order.openSize;
		}
	}
	else {
		if(cancel_order->type==Execution::BS_SELL_SHORT || cancel_order->type==Execution::BS_SELL){
			totalopen=totalopen-cancel_order->order.openSize;
		}else if(cancel_order->type==Execution::BS_BUY){
			totalclose=totalclose-cancel_order->order.openSize;
		}
	}

//	orders.erase(cancel_order->request_id);	//removing cancelled order from vector
	return true;
}

bool AbstractTradeAccount::cancel(const uint32_t request_id) {
	auto lookup = orders.find(request_id);
	if ( lookup == orders.end() ) {
		TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"cancelFirst: no order to cancel";
		return false;
	}

	auto& order_info = lookup->second;
	OrderAccountType acc_type=order_info->account_type;
	uint8_t type=order_info->type;

	if ( !order_info->canCancel() ) {
		orders.erase(order_info->request_id);
		return false;
	}

	order_info->onOrderCancelFunction = onOrderCancelReceived;
	order_info->cancel_stage = OrderInfo::CANCEL_INIT;
	if ( !account.cancel(order_info) ) {
		TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"cancelFirst: can't cancel order";
		return false;
	}

	bool isCancled= waitCancelAcknowledgement(order_info);
	if(isCancled){
		std::string time=redisWriter->getCurrentTime();
		redisWriter->updateOrderCancleTime(account.getAccountName(),std::to_string(request_id),time);
	}
	//TRACE<<"livestats cancel order successfull "<<account.getAccountName()<<":"<<getSymbolName()<<" order type "<<order_info->account_type<<" buy or sell "<<unsigned(order_info->type);

	return isCancled;
}

bool AbstractTradeAccount::cancelAll() {

	DEBUG<<LOG_ABSTRACT_TRADE_ACCOUNT<<"removing all pending orders";
	pending_orders = PendingOrderQueue{};

	for ( auto itr = orders.begin(); itr != orders.end(); itr++){
		auto& order_info = itr->second;
		if ( order_info->canCancel() ) {
			cancel(order_info->request_id);
		}
	}


	//erasing canceled orders
	for (auto itr = orders.begin(); itr != orders.end(); ) {
		auto& order_info = itr->second;
		if ( order_info->cancel_stage == OrderInfo::CANCEL_DONE ) itr = orders.erase(itr);
		else itr++;
	}


	return true;
}


/************************** other utilities ************************/
void AbstractTradeAccount::setOtherAccount(AbstractTradeAccount *other) {
	this->other = other;
}

int64_t AbstractTradeAccount::getPosition() {
//	return account.getPosition(symbol);
	return total*divisor;
}

int64_t AbstractTradeAccount::getTotal() {
	return total;
}

double AbstractTradeAccount::getAverage() {
	return position_table.getAverage();
}

void AbstractTradeAccount::setAverage(double average) {
	std::lock_guard<std::mutex> lock(rw_mutex);
	this->average = average;
}


void AbstractTradeAccount::printCloseFills() {

	auto curr_pos = getPosition();
	auto oppo_pos = other->getPosition();
	if ( account.getAccountType() == OrderAccountType::ORDER_ACCOUNT_LONG )
		DEBUG<<"onCloseFill: ("<<symbol<<","<<curr_pos<<","<<average<<","<<oppo_pos<<","<<other->average<<")";
	else if ( account.getAccountType() == OrderAccountType::ORDER_ACCOUNT_SHORT )
		DEBUG<<"onCloseFill: ("<<symbol<<","<<oppo_pos<<","<<other->average<<","<<curr_pos<<","<<average<<")";
}


string AbstractTradeAccount::getSymbolName(){
	return symbol;
}

int AbstractTradeAccount::getTotalLiveOrderCount(){
	return totalopen+totalclose;
}


bool AbstractTradeAccount::checkOrderStatus(uint32_t order_id) {
	auto lookup = orders.find(order_id);
	if ( lookup == orders.end() ) {
		TRACE<<LOG_ABSTRACT_TRADE_ACCOUNT<<"cancelFirst: no order to cancel";
		return false;
	}

	auto& order_info = lookup->second;
	return account.getStatus(*order_info);
}
