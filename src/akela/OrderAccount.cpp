/*
 * OrderAccount.cpp
 *
 *  Created on: Jul 7, 2016
 *      Author: jamil
 */

#include "../akela/OrderAccount.h"



bool OrderAccount::initOrderAccount(){
	auto direct = false;
	INFO<<"connecting to order gateway "<<gateway<<" for user "<<username;
		auto status = Connect(gateway, direct);	//will be change soon
		if (ECERR_SUCCESS != status) {
			DEBUG<<"Gateway Connect to "<<gateway<<" failed with error code "<<status;
			return false;//TODO: this will be a throw statement
		}

		// login to the gateway
		status = Login(username, "");
		if (ECERR_SUCCESS != status) {
			DEBUG<<"Login "<<username<<" to gateway "<<gateway<<" failed rc="<<status;
			return false;//TODO: this will be a throw statement
		}


		// subscribe to orders and fills
		status = ListenOrders(username);
		if (ECERR_SUCCESS != status) {
			DEBUG<<"ListenOrders("<<username<<") failed, rc="<<status;
			return false;//TODO: this will be a throw statement
		}

		status = ListenFills(username);
		if (ECERR_SUCCESS != status) {
			DEBUG<<"ListenFills("<<username<<") failed, rc="<<status;
			return false;//TODO: this will be a throw statement
		}

		return true;

}


OrderAccount::OrderAccount(Linkx::Client &client, std::string username, OrderAccountType account_type, std::vector<std::string > gateway_names, OrderAccount *first):
	Execution::Connection(client),
	username(username),
	gateway(gateway_names[0]),
	request_id_generator(0),
	account_type(account_type),
	account_type_string(account_type == ORDER_ACCOUNT_LONG?"long":"short"),
	next(nullptr),
	current(this) {

	INFO<<"connecting to order gateway "<<gateway<<" for user "<<username;



	if ( !initOrderAccount() ) {
		ERROR<<"Connection to "<<gateway<<" for user "<<username<<" failed!";
		throw std::runtime_error(" Connection to one of the order exchanges fails");
	}

	gateway_names.erase(gateway_names.begin());

	if ( gateway_names.size() > 0 ) {
		if ( nullptr == first ) {
			next = std::unique_ptr<OrderAccount>(new OrderAccount(client, username, account_type, gateway_names, this));
		} else {
			next = std::unique_ptr<OrderAccount>(new OrderAccount(client, username, account_type, gateway_names, first));
		}

	}


}

unsigned OrderAccount::OnConnect(char *pName, Linkx::Address &address) {
	INFO<<"OrderAccount connected to "<<pName<<" at "<<address.ToString().c_str();
	//TODO: need to see if this exchange parameter is ok
	EnableLifeline(username, Execution::EXCH_BATS);
	return ECERR_SUCCESS;
}

/*=========================================================================================*/

unsigned OrderAccount::OnDisconnect(void) {
	INFO<<"OrderAccount connection disconnected for "<<username;
	return ECERR_SUCCESS;
}

/*=========================================================================================*/

unsigned OrderAccount::OnReconnect(char *pName, Linkx::Address &address) {
	INFO<<"Reconnected to "<<pName<<" at "<<address.ToString().c_str();
	return ECERR_SUCCESS;
}

/*=========================================================================================*/

void OrderAccount::OnOrder(const unsigned requestId, const Execution::Order &order) {

	auto symbol = order.ticker.ToString();
	DEBUG<<LOG_ORDER_ACCOUNT<<"OnOrder "<<order.timeStamp<<" "<<requestId<<" "<<order.referenceNumber<<" "<<(int32_t)order.condition<<" "<<order.limit.ToString().c_str()<<" "<<order.size<<" "<<(int)order.buySell;

	auto order_info_found = order_hash.find(requestId);

	if ( order_info_found != order_hash.end() ) {
		auto& order_info = order_info_found->second;

		order_info->order = order;
		/*
		 * apart from NEW, PENDING_NEW and DONE every other flag will be taken as CANCELLED
		 */
		switch ( order.condition ) {
			case Execution::ORDER_COND_PENDING_NEW: {

			}
				break;
			case Execution::ORDER_COND_NEW:
				if( order_info->onAcknowledgementFunction ) {
					TRACE<<LOG_ORDER_ACCOUNT<<"calling onAcknowledgement function";
					order_info->stage = OrderInfo::STAGE_ACKNOWLEDGED;
					order_info->is_acknowledged = true;
					order_info->onAcknowledgementFunction->operator()(order_info, true);
				} else TRACE<<LOG_ORDER_ACCOUNT<<"onAcknowledgement function not define";
				break;
			case Execution::ORDER_COND_PARTIAL:
				TRACE<<LOG_ORDER_ACCOUNT<<"partially filled order";
				break;
			case Execution::ORDER_COND_DONE:
				TRACE<<"OnOrder: Order Done "<<(int)order.condition;
				if ( order_info->onOrderDoneFunction ) {
					TRACE<<LOG_ORDER_ACCOUNT<<"calling onOrderDone function";
					order_info->stage = OrderInfo::STAGE_DONE;
					order_info->onOrderDoneFunction->operator()(order_info);
				} else TRACE<<LOG_ORDER_ACCOUNT<<"onOrderDone function not define";
				order_hash.erase(order_info->request_id);
				break;
			case Execution::ORDER_COND_CANCELED:
				TRACE<<"OnOrder: Order Cancelled "<<(int)order.condition;
				if ( order_info->onOrderCancelFunction ) {
					TRACE<<LOG_ORDER_ACCOUNT<<"calling onOrderCancel function";
					order_info->stage = OrderInfo::STAGE_CANCELLED;
					order_info->cancel_stage = OrderInfo::CANCEL_DONE;
					order_info->onOrderCancelFunction->operator()(order_info, true);
				} else TRACE<<LOG_ORDER_ACCOUNT<<"onOrderCancel function not define";
				order_hash.erase(order_info->request_id);
				break;
			case Execution::ORDER_COND_CANCEL_REJECTED:
			case Execution::ORDER_COND_TOO_LATE_TO_CANCEL:
				TRACE<<"OnOrder: Order Cancelled Rejected/Too Late"<<(int)order.condition;
				if ( order_info->onOrderCancelFunction ) {
					TRACE<<LOG_ORDER_ACCOUNT<<"calling onOrderCancel function";
					order_info->cancel_stage = OrderInfo::CANCEL_FAIL;
					order_info->onOrderCancelFunction->operator()(order_info, false);
				} else TRACE<<LOG_ORDER_ACCOUNT<<"onOrderCancel function not define";
				break;
			case Execution::ORDER_COND_PENDING_CHANGE:

				break;
			case Execution::ORDER_COND_CHANGED:
				TRACE<<LOG_ORDER_ACCOUNT<<"Order with request id "<<requestId<<" has changed--> removing from hash";
				if ( order_info->onOrderChangeFunction ) {
					TRACE<<LOG_ORDER_ACCOUNT<<"calling onOrderChange Function";
					order_info->onOrderChangeFunction->operator()(order_info, true);
				} else TRACE<<LOG_ORDER_ACCOUNT<<"onOrderChange Function not define";
				order_hash.erase(requestId);
				break;
			case Execution::ORDER_COND_CHANGE_REJECTED:
				TRACE<<LOG_ORDER_ACCOUNT<<"Order change for order with request id "<<requestId<<" has been rejected";
				if ( order_info->onOrderChangeFunction ) {
					TRACE<<LOG_ORDER_ACCOUNT<<"calling onOrderChange Function for failure";
					order_info->onOrderChangeFunction->operator()(order_info, false);
				} else TRACE<<LOG_ORDER_ACCOUNT<<"onOrderChange Function not define";
				break;
			case Execution::ORDER_COND_GATEWAY_THROTTLE: {
				WARN<<LOG_ORDER_ACCOUNT<<"Order with request id "<<requestId<<" has rejected due to throttle limit";
				TRACE<<LOG_ORDER_ACCOUNT<<"resending order - after wait";
				std::this_thread::sleep_for(std::chrono::milliseconds(5000));	//1 second delay
				auto status = PostOrder(requestId, order_info->order);
				if ( status != ECERR_SUCCESS ) {
					ERROR<<"Order Resending has also failed "<<status;
				}
			}
				break;
			case Execution::ORDER_COND_DENIED:
			case Execution::ORDER_COND_REJECTED:
			default:
				TRACE<<LOG_ORDER_ACCOUNT<<"order condition (FAILED) rt = "<<(int)order.condition;
				if( order_info->onAcknowledgementFunction ) {
					TRACE<<LOG_ORDER_ACCOUNT<<"calling onAcknowledgement function as failed order";
					order_info->stage = OrderInfo::STAGE_FAILED;
					order_info->is_acknowledged = false;
					order_info->onAcknowledgementFunction->operator()(order_info, false);
				} else TRACE<<LOG_ORDER_ACCOUNT<<"onAcknowledgement function not define";
				order_hash.erase(order_info->request_id);
				break;
			}

	} else WARN<<"OnOrder: unmatched order request Id "<<requestId;

}

/*=========================================================================================*/

void OrderAccount::OnOrderCancel(const unsigned requestId, const Execution::OrderCancel &cancel) {
	auto symbol = cancel.ticker.ToString();
	DEBUG<<LOG_ORDER_ACCOUNT<<"OnOrderCancel "<<cancel.timeStamp<<" "<<requestId<<" "<<cancel.originalReference<<" "<<cancel.referenceNumber<<" "<<(int)cancel.condition;
	if ( order_hash.find(requestId) != order_hash.end() ){//if requestId present
		auto& order_info = *order_hash[requestId];
		order_info.stage = OrderInfo::STAGE_CANCELLED;
		order_info.cancel = cancel;

		switch ( cancel.cancelReason ) {
		//cases that will be deal with re-routing
		case Execution::CANCEL_REASON_CANCEL_CROSS:
		case Execution::CANCEL_REASON_LIFELINE:
		case Execution::CANCEL_REASON_DISABLED:
		case Execution::CANCEL_REASON_REMOVE_LINK:
		case Execution::CANCEL_REASON_TIME_TRIGGER:
		case Execution::CANCEL_REASON_ROLL:
		case Execution::CANCEL_REASON_DISCONNECT:
		case Execution::CANCEL_REASON_QUOTE_CANCEL:
		case Execution::CANCEL_REASON_SELF_MATCH:
		case Execution::CANCEL_REASON_INVALID_DATA:
		case Execution::CANCEL_REASON_PRICE_BAND_VIOLATION:
		case Execution::CANCEL_REASON_LOST_POSITION_MANAGER:
		case Execution::CANCEL_REASON_LOST_POSITION_AGGREGATOR:
		case Execution::CANCEL_REASON_LOST_DROP_COPY:
		case Execution::CANCEL_REASON_LOST_CERBERUS:
		case Execution::CANCEL_REASON_LOST_POPULATOR:
			// to be enabled for switching
			/*if ( nullptr != order_info.onVanueChangeFunction ) {
						DEBUG<<"onVanueChangeFunction: calling callback function ";
						order_info.onVanueChangeFunction->operator ()(true);
					} else DEBUG<<"onVanueChangeFunction: no callback function defined";*/
			break;


		}


	} else WARN<<LOG_ORDER_ACCOUNT<<"OnOrderCancel received with an un matched requestId "<<requestId;
}

/*=========================================================================================*/

void OrderAccount::OnOrderChange(const unsigned requestId, const Execution::OrderChange &change) {
	DEBUG<<"OnOrderChange "<<" "<<change.timeStamp<<" "<<requestId<<" "<<change.order.referenceNumber<<" "<<(int)change.condition;

	auto symbol = change.order.ticker.ToString();

	if ( order_hash.find(requestId) != order_hash.end() ) {
		auto& order_info = *order_hash[requestId];
		order_info.change = change;

		switch ( change.condition ) {
		case Execution::ORDER_COND_ORDER_UNKNOWN:
			TRACE<<LOG_ORDER_ACCOUNT<<" order condition unknown received for order (flagging change order as failed)"<<requestId;
			order_info.stage = OrderInfo::STAGE_FAILED;
			break;
		}

	} else WARN<<LOG_ORDER_ACCOUNT<<"OnOrderChange: received with an un-matched requestId ";
}

/*=========================================================================================*/

void OrderAccount::OnFill(const unsigned requestId, const Execution::Fill &fill)
{


	DEBUG<<"OnFill "<<" "<<fill.timeStamp<<" "<<requestId<<" "<<fill.referenceNumber<<" "<<fill.orderReference<<" "
			<<static_cast<uint32_t>(fill.condition)<<" "<<fill.price.AsDouble()<<" "<<fill.size;

	auto order_info_find = order_hash.find(requestId);
	if ( order_info_find != order_hash.end() ){//if requestId present
		auto& order_info = order_info_find->second;
		order_info->fill = fill;
		if ( fill.openSize == 0 ) {
			order_info->stage = OrderInfo::STAGE_FILLED;
		}
		if ( nullptr != order_info->onFillFunction ) {
			TRACE<<"calling onFillFunction for the order";
			order_info->onFillFunction->operator ()(order_info, fill.price.AsDouble(), fill.size);
		} else WARN<<order_info->symbol<<" onFillFunction not registered";
//		order_info.setOrderFill(fill.timeStamp, fill.condition, fill.price.AsDouble(), fill.size);
	} else WARN<<"OnFill: received with an un matched requestId";
}


/*=========================================================================================*/
bool OrderAccount::limitBuy(std::shared_ptr<OrderInfo>& order_info) {



	const std::string symbol = order_info->symbol;
	if ( order_info->size > ORDER_ACCOUNT_MAXIMUM_LIMIT ) {
		WARN<<LOG_ORDER_ACCOUNT<<"maximum limit for order size exceed";
		return false;
	}

	Execution::Order order;
	order.Clear();

	order_info->username = username;
	::memcpy(order.userName, username.c_str(), std::min<size_t>(sizeof(order.userName), username.size()));
	order.condition = Execution::ORDER_COND_PENDING_NEW;
	order.ticker.flags = MarketData::Instrument::FLAG_EQUITY;
	order.ticker.Parse(symbol, MarketData::Instrument::TYPE_EQUITY);
	Utils::Price p(order_info->price_double, Utils::Price::PT_DECIMAL_2);
	order.flags = Execution::ORDER_FLAG_CHECK_POSITION;
	order.hiddenSize = 0;
	if ( order.SetLimitOrder(Execution::BS_BUY, p, order_info->size) != true ) {
		ERROR<<"Order Set Limit failure ";
		return false;
	}


	auto id = ++request_id_generator;
	TRACE<<"order id generated: "<<id;

	order_info->type = Execution::BS_BUY;
	order_info->request_id = id;
	order_info->account_type = account_type;
	order_hash.emplace(order_info->request_id, order_info);


	auto status  = PostOrder(id, order);
	if ( status != ECERR_SUCCESS ) {
		ERROR<<"Order Sending failure "<<status;
		return false;
	}

//	std::unique_lock<std::mutex> lock(order_info->mt);
//	order_info->cv.wait(lock, [order_info](){
//		return order_info->stage == OrderInfo::STAGE_ACKNOWLEDGED || order_info->stage == OrderInfo::STAGE_FAILED;
//	});
//
//	if ( order_info->stage == OrderInfo::STAGE_FAILED ) {
//		TRACE<<LOG_ORDER_ACCOUNT<<"order has failed";
//		return false;
//	}

	return true;
}



/*=========================================================================================*/
bool OrderAccount::limitSell(std::shared_ptr<OrderInfo>& order_info) {

	const std::string symbol = order_info->symbol;
	if ( order_info->size > ORDER_ACCOUNT_MAXIMUM_LIMIT ) {
		WARN<<LOG_ORDER_ACCOUNT<<"maximum limit for order size exceed";
		return false;
	}

	Execution::Order order;
	order.Clear();

	::memcpy(order.userName, username.c_str(), std::min<size_t>(sizeof(order.userName), username.size()));
	order.condition = Execution::ORDER_COND_PENDING_NEW;
	order.ticker.flags = MarketData::Instrument::FLAG_EQUITY;
	order.ticker.Parse(symbol, MarketData::Instrument::TYPE_EQUITY);
	Utils::Price p(order_info->price_double, Utils::Price::PT_DECIMAL_2);
	order.flags = Execution::ORDER_FLAG_CHECK_POSITION;
	order.hiddenSize = 0;

	auto type = (account_type == ORDER_ACCOUNT_SHORT)? Execution::BS_SELL_SHORT:Execution::BS_SELL;

	if ( order.SetLimitOrder(type, p, order_info->size) != true ) {
		ERROR<<"Order Set Limit failure ";
		return false;
	}


	auto id = ++request_id_generator;
	TRACE<<"order id generated: "<<id;

	order_info->type = type;
	order_info->username = username;
	order_info->request_id = id;
	order_info->account_type = account_type;
	order_hash.emplace(order_info->request_id, order_info);


	auto status  = PostOrder(id, order);
	if ( status != ECERR_SUCCESS ) {
		ERROR<<"Order Sending failure ";
		return false;
	}

	return true;
}


bool OrderAccount::open(std::shared_ptr<OrderInfo> &order_info) {
	if ( ORDER_ACCOUNT_LONG == account_type ) return limitBuy(order_info);
	else if ( ORDER_ACCOUNT_SHORT == account_type ) return limitSell(order_info);
	return false;
}

bool OrderAccount::close(std::shared_ptr<OrderInfo> &order_info) {
	if ( ORDER_ACCOUNT_LONG == account_type ) return limitSell(order_info);
	else if ( ORDER_ACCOUNT_SHORT == account_type ) return limitBuy(order_info);
	return false;
}

bool OrderAccount::cancel(OrderType& order_info) {
	auto& symbol  = order_info->symbol;
	auto order_condition = order_info->order.condition;
	if ( order_condition == Execution::ORDER_COND_DONE ||
			order_condition == Execution::ORDER_COND_CANCELED ||
			order_info->stage == OrderInfo::STAGE_FILLED ) {

		TRACE<<LOG_ORDER_ACCOUNT<<"orderCancel: already cancelled/done/filled "<<order_info->request_id<<" ["<<order_info->order.referenceNumber<<"] "<<(int)order_info->order.condition;
		return false;
	}

	if ( ECERR_SUCCESS != CancelOrder(order_info->order, false) ) {
		WARN<<"<"<<username<<","<<order_info->symbol<<">: "<<"Failed to cancel order";
		return false;
	}

	return true;
}


bool OrderAccount::change(OrderType& order_info, OrderType& order_info_changed) {
	Execution::Order order;
	order.Clear();

	auto& symbol = order_info->symbol;
	::memcpy(order.userName, username.c_str(), std::min<size_t>(sizeof(order.userName), username.size()));
	order.condition = Execution::ORDER_COND_PENDING_CHANGE;
	order.ticker.flags = MarketData::Instrument::FLAG_EQUITY;
	order.ticker.Parse(order_info->symbol, MarketData::Instrument::TYPE_EQUITY);
	Utils::Price p(order_info_changed->price_double, Utils::Price::PT_DECIMAL_2);
	order.flags = Execution::ORDER_FLAG_CHECK_POSITION;
	::memcpy(order.session, order_info->order.session, sizeof(order.session));

	if ( order_info_changed->size > ORDER_ACCOUNT_MAXIMUM_LIMIT ) {
		WARN<<LOG_ORDER_ACCOUNT<<"change: maximum order limit exceeded -- request denied";
		return false;
	}

	if ( order.SetLimitOrder(order_info->type, p, order_info_changed->size) != true ) {
		ERROR<<"SetLimitOrder failed";
		return false;
	}

	auto order_condition = order_info->order.condition;
	if ( order_condition == Execution::ORDER_COND_DONE ||
			order_condition == Execution::ORDER_COND_CANCELED ||
			order_info->stage == OrderInfo::STAGE_FILLED ) {

		TRACE<<LOG_ORDER_ACCOUNT<<"change: already cancelled/done/filled "<<order_info->request_id<<" ["<<order_info->order.referenceNumber<<"] "<<(int)order_info->order.condition;
		return false;
	}

	auto id = ++request_id_generator;
	TRACE<<"change order id generated: "<<id;

	order_info_changed->request_id = id;

	order_hash.emplace(order_info_changed->request_id, order_info_changed);

	if ( ECERR_SUCCESS != ChangeOrder(id, order_info->order.referenceNumber, order) ) {
		ERROR<<LOG_ORDER_ACCOUNT<<"Change Order failed";
		order_hash.erase(id);
		return false;
	}

	return true;
}

bool OrderAccount::getStatus(const OrderInfo& order_info) {
	auto status = GetOrderStatus(order_info.order);
	return status == ECERR_SUCCESS;
}


/*=========================================================================================*/
OrderAccount::~OrderAccount() {
	auto symbol = "";
	INFO<<LOG_ORDER_ACCOUNT<<"Exit --- Cancel All Pending Orders!!!";
	CancelAllOrders();
}

