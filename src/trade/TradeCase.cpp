/*
 * TradeCase.cpp
 *
 *  Created on: Nov 10, 2016
 *      Author: jamil
 */


#include "TradeInstance.h"

TradeCase::TradeCase(TradeInstance &trade_instance, std::string name, const Type type):
		trade_instance(trade_instance),
		name(name),
		send_price(0.0),
		requote_price(0.0),
		type(type),
		main_order_id(0),
		round_target(0),
		sign( trade_instance.trigger_state == TRIGGER_STATE_POSITIVE ? 1 : -1 ),
		requote_order_flag(false) {
	state = INIT;
	current_acc = trade_instance.current_acc;
	opposite_acc = trade_instance.opposite_acc;

	//requote order sent
	on_requote_order_done = [this](const double price, const uint32_t size) {
		auto curr_pos = this->current_acc->getPosition();
		auto oppo_pos = this->opposite_acc->getPosition();
		if ( curr_pos == oppo_pos ) {
			requote_order_flag = true;
		}

	};

	on_trade_send_close = [this](const double price, const uint32_t size){		//is trade_done functionality
		if( this->current_acc->getPosition() > this->opposite_acc->getPosition() ) {
			auto error = std::max(fabs(this->trade_instance.bar0.open - this->trade_instance.price_estimate),
					this->trade_instance.mpv_value)*this->trade_instance.mpv_multiplier;		//estimation error
			auto close_price = price + error*this->sign;
			auto close_size = size*this->trade_instance.divisor;
			TRACE<<LOG_TRADE_CASE<<"adding close order @"<<close_price<<" for size "<<close_size;
			current_acc->closePending(close_price, close_size, on_requote_order_done);
		}
	};

}



std::unique_ptr<TradeCase> TradeCaseFactory::create(TradeInstance &trade_instance, const TradeCase::Type case_id) {
	std::unique_ptr<TradeCase> temp = nullptr;
	switch ( case_id ) {
	case TradeCase::NONE:
		temp = nullptr;
		break;
	case TradeCase::CASE_1:
		temp = make_unique<TradeCase1>(trade_instance);
		break;
//	case TradeCase::CASE_2:
//		temp = make_unique<TradeCase2>(trade_instance);
//		break;
	case TradeCase::CASE_3:
		temp = make_unique<TradeCase3>(trade_instance);
		break;
	case TradeCase::CASE_4:
		temp = make_unique<TradeCase4>(trade_instance);
		break;
	case TradeCase::CASE_5:
		temp = make_unique<TradeCase5>(trade_instance);
		break;
	case TradeCase::CASE_CLOSEOPPOSITE:
		temp = make_unique<CloseOppositeWithPNL>(trade_instance);
		break;
	case TradeCase::CASE_HEDGECURRENT:
		temp = make_unique<HedgeCurrent>(trade_instance);
		break;
	case TradeCase::CASE_6C:
		temp = make_unique<TradeCase6C>(trade_instance);
		break;
	case TradeCase::CASE_7:
		temp = make_unique<TradeCase7>(trade_instance);
		break;
	case TradeCase::CASE_9:
		temp = make_unique<TradeCase9>(trade_instance);
		break;
	case TradeCase::CASE_10:
		temp = make_unique<TradeCase10>(trade_instance);
		break;
	case TradeCase::CASE_MAXTLV:
		temp = make_unique<MaxTLV>(trade_instance);
		break;
	case TradeCase::CASE_CLOSEOPPOSITEWITH_ANTICIPATED_PNL:
		temp = make_unique<CloseOppositeWithAnticipatedPNL>(trade_instance);
		break;
	case TradeCase::PANIC_CLOSE:
		temp = make_unique<PanicClose>(trade_instance);
		break;
	case TradeCase::SOFT_CLOSE:
		temp = make_unique<SoftClose>(trade_instance);
		break;
	case TradeCase::CASE_HEDGEDMODE:
		temp = make_unique<HedgedMode>(trade_instance);
		break;
	case TradeCase::CASE_ONEQUALITY:
		temp = make_unique<OnEquality>(trade_instance);
		break;
	case TradeCase::CASE_CLOSE_TRIGGER:
		temp = make_unique<CloseTrigger>(trade_instance);
		break;
	default:
		temp = nullptr;
		break;
	}

	return temp;
}

const TradeCase::Type TradeCase::getType() {
	return type;
}

const TradeCase::State TradeCase::getState() {
	return state;
}

const std::string TradeCase::getName() {
	return name+"|"+stage;
}

bool TradeCase::isDone() {
	return state == DONE;
}

TradeCase::State TradeCase::run() {
	switch ( state ) {
	case INIT:
		stage = "init";
		init();
		break;
	case PRICE_CHANGE:
		stage = "reprice";
		reprice();
		break;
	case DONE:
		stage = "done";
		done();
		break;
	case WAIT:
		stage = "wait";
		wait();
		break;
	case FAILED:
		stage = "failed";
		failed();
		break;
	default:
		stage = "invalid";
		WARN<<LOG_TRADE_CASE<<"no case option selected";
		break;
	}

	return state;
}

void TradeCase::wait() {


}

void TradeCase::failed() {

}

double TradeCase::openPrice(uint32_t qto, double current_price){
	double ret = 0.0;
	int64_t os;
	if ( trade_instance.trigger_state == TRIGGER_STATE_POSITIVE ) {
		os = trade_instance.ask_size - qto*trade_instance.multiplier;
		if ( os < 0 ) ret = trade_instance.ask_price;	//this is market order
		else ret = trade_instance.bid_price;

		if ( current_price > 0.0 && current_price > trade_instance.bid_price ) //dont change price
			ret = current_price;

	} else if ( trade_instance.trigger_state == TRIGGER_STATE_NEGATIVE ) {
		os = trade_instance.bid_size - qto*trade_instance.multiplier;
		if ( os < 0 ) ret = trade_instance.bid_price;
		else ret = trade_instance.ask_price;

		if ( current_price > 0.0 && current_price < trade_instance.ask_price ) //dont change price
			ret = current_price;

	}

	return ret;
}

double TradeCase::closePrice(uint32_t qto, double current_price){
	double ret = 0.0;
	int64_t os;
	if ( trade_instance.trigger_state == TRIGGER_STATE_NEGATIVE ) {
		os = trade_instance.ask_size - qto*trade_instance.multiplier;
		if ( os < 0 ) ret = trade_instance.ask_price;
		else ret = trade_instance.bid_price;

		if ( current_price > 0.0 && current_price > trade_instance.bid_price ) //dont change price
			ret = current_price;



	} else if ( trade_instance.trigger_state == TRIGGER_STATE_POSITIVE ) {
		os = trade_instance.bid_size - qto*trade_instance.multiplier;
		if ( os < 0 ) ret = trade_instance.bid_price;
		else ret = trade_instance.ask_price;

		if ( current_price > 0.0 && current_price < trade_instance.ask_price ) //dont change price
			ret = current_price;
	}

//	DEBUG<<LOG_TRADE_INSTANCE<<"qto = "<<qto<<" OS = "<<os<<" is = "<<instrument_skew<<" sendPrice = "<<ret;
	return ret;
}

double TradeCase::buyPrice(uint32_t qty) {
	auto price = 0.0;
	auto os = trade_instance.ask_size - qty*trade_instance.multiplier;
	if ( os <= 0 ) price = trade_instance.ask_price;
	else price = trade_instance.bid_price;
	return price;
}

double TradeCase::sellPrice(uint32_t qty) {
	auto price = 0.0;
	auto os = trade_instance.bid_size - qty*trade_instance.multiplier;
	if ( os <= 0 ) price = trade_instance.bid_price;
	else price = trade_instance.ask_price;
	return price;
}

void TradeCase::sendRequoteOrders() {

	//number of orders to be sent
	if ( current_acc->getTranchCounter() == trade_instance.tranch_value ) {
		TRACE<<LOG_TRADE_CASE<<"account is at maximum position .. no more requote orders";
		return;
	}

	TRACE<<LOG_TRADE_CASE<<"tranch check passed: "<<current_acc->getTranchCounter()<<" "<<trade_instance.tranch_value;

	auto target_pos = trade_instance.max_pos*(current_acc->getTranchCounter()+1) ;
	auto residual = target_pos - current_acc->getPosition();

	TRACE<<LOG_TRADE_CASE<<"target position: "<<target_pos<<" residual: "<<residual;

	requote_price = sign == 1 ? trade_instance.bid_price : trade_instance.ask_price ;
	auto right_side = (trade_instance.bar1.close + trade_instance.mpv_value*trade_instance.mpv_multiplier*sign);
	auto price_condition = sign > 0 ? requote_price <= right_side : requote_price >=  right_side;
	TRACE<<LOG_TRADE_CASE<<"requote price: "<<requote_price<<" rightside: "<<right_side;
	if ( price_condition ) {
		auto NOO = double( residual ) / trade_instance.base_qty;
		auto order_size = 0;
		for ( int remainder = residual, i = 0 ; remainder > 0 ; remainder -= order_size, i++ ) {
			order_size = ( remainder > trade_instance.base_qty ) ? trade_instance.base_qty : remainder ;
			auto order_price = requote_price - sign*trade_instance.mpv_value*i;

			auto order_id = current_acc->open(order_price, order_size, on_trade_send_close);
			if ( order_id )
				requote_orders.push_back(order_id);

		}
	} else TRACE<<LOG_TRADE_CASE<<"price condition fails";

}

void TradeCase::updateRequoteOrders() {
	auto curr_pos = current_acc->getPosition();
	auto oppo_pos = opposite_acc->getPosition();

	if ( curr_pos != oppo_pos ) return;

	if (requote_orders.size() == 0 ) {	//if there are no requote orders send new
		TRACE<<LOG_TRADE_CASE<<"sending new requote orders";
		current_acc->cancelAll();
		sendRequoteOrders();
	} else if ( requote_orders.size() > 0 ) {

		TRACE<<LOG_TRADE_CASE<<"delete requote orders if executed";
		for (auto itr = requote_orders.begin(); itr != requote_orders.end(); ) {
			if ( *itr == 0 ) itr = requote_orders.erase(itr);
			else if ( !current_acc->checkOrderStatus(*itr) ) itr = requote_orders.erase(itr);
			else itr++;
		}

		if ( requote_orders.size() == 0 ) {
			TRACE<<LOG_TRADE_CASE<<"sending new requote orders";
			current_acc->cancelAll();
			sendRequoteOrders();
			return;
		}



		if ( trade_instance.trigger_state == TRIGGER_STATE_POSITIVE &&
				trade_instance.bid_price > requote_price &&
				trade_instance.instrument_skew > 0 ) {
			requote_price = trade_instance.bid_price;
		} else if ( trade_instance.trigger_state == TRIGGER_STATE_NEGATIVE &&
				trade_instance.ask_price < requote_price &&
				trade_instance.instrument_skew < 0 ) {
			requote_price = trade_instance.ask_price;
		} else {
			TRACE<<LOG_TRADE_CASE<<"condition for requote order update failed";
			return;
		}

		TRACE<<LOG_TRADE_CASE<<"updating requote orders";
		auto itr = requote_orders.begin();
		auto i = 0;
		std::for_each(requote_orders.begin(), requote_orders.end(), [this, &i](decltype(requote_orders)::reference order) {
			auto new_price = this->requote_price + i*this->trade_instance.mpv_value*this->sign;
			auto new_order = this->current_acc->change(order, new_price);
			if ( new_order != 0 || new_order != order ) {
				order = new_order;
				i++;
			} else  order = 0;	//assign order to zero to indicacte executed order/can't change
		});
	}
}

void TradeCase::reprice() {
	auto curr_pos = current_acc->getPosition();
	auto oppo_pos = opposite_acc->getPosition();


	if ( curr_pos == oppo_pos && requote_order_flag ) updateRequoteOrders();
	current_acc->sendPendingOrders();
}

TradeCase::~TradeCase() {
	if ( trade_instance.cancel_on_trigger ) {
		INFO<<"canceling all orders in current/opposite account";
		if ( trade_instance.long_acc.cancelAll() &&
				trade_instance.short_acc.cancelAll() ) {
			TRACE<<"cancelation of all orders successfull";
		}
	}

	requote_orders.clear();

	current_acc->deleteCompletePositions();
	opposite_acc->deleteCompletePositions();


	auto curr_pos = current_acc->getPosition();
	auto oppo_pos= opposite_acc->getPosition();
	if ( curr_pos == oppo_pos ) {
		TRACE<<LOG_TRADE_CASE<<"hedge position "<<curr_pos<<"/"<<oppo_pos<<"... waiting for next time window to switch to case 10";
		trade_instance.wait_for_next_time_window = true;
		trade_instance.wait_for_next_time_window_avicenna = trade_instance.price_estimate;
	}

}

////////////////////////////////////////////////////////////////////
///////////////////// Case PanicClose //////////////////////////////
////// close all position in both account at market order //////////
////////////////////////////////////////////////////////////////////

PanicClose::PanicClose(TradeInstance &trade_instance):
		TradeCase(trade_instance, "panic_close", PANIC_CLOSE),
		short_acc(trade_instance.short_acc),
		long_acc(trade_instance.long_acc),
		close_size(0) {

}

void PanicClose::init() {

	auto pnl = 0.0;
	auto bid_price = trade_instance.bid_price;
	auto ask_price = trade_instance.ask_price;

	//select triggers
	if ( short_acc.getPosition() == long_acc.getPosition() ) {
		state  = DONE;
		return;
	} else if ( short_acc.getPosition() > long_acc.getPosition() ) {
		trade_instance.trigger_state = TRIGGER_STATE_POSITIVE;
		current_acc = &long_acc;
		opposite_acc = &short_acc;
	} else if ( short_acc.getPosition() < long_acc.getPosition() ) {
		trade_instance.trigger_state = TRIGGER_STATE_NEGATIVE;
		current_acc = &short_acc;
		opposite_acc = &long_acc;
	}


	if ( trade_instance.trigger_state == TRIGGER_STATE_POSITIVE ) {
		send_price = ask_price;
	} else if ( trade_instance.trigger_state == TRIGGER_STATE_NEGATIVE ) {
		send_price = bid_price;
	}

	close_size = opposite_acc->getPosition() - current_acc->getPosition();
	main_order_id = opposite_acc->close(send_price, close_size);
	if ( !main_order_id ) {
		DEBUG<<LOG_TRADE_CASE<<"close order failure ... exiting panic close";
		state = FAILED;
	} else {
		INFO<<LOG_TRADE_CASE<<"close order sent @"<<send_price<<" for "<<close_size;
		state = PRICE_CHANGE;
	}
}


void PanicClose::reprice() {

	if ( opposite_acc->getPosition() == current_acc->getPosition() ) {
		state = DONE;
		return;
	}

	auto pnl = 0.0;
	auto bid_price = trade_instance.bid_price;
	auto ask_price = trade_instance.ask_price;

	auto new_price = 0.0;

	if ( trade_instance.trigger_state == TRIGGER_STATE_POSITIVE ) {
		new_price = ask_price;
	} else if ( trade_instance.trigger_state == TRIGGER_STATE_NEGATIVE ) {
		new_price = bid_price;
	}

	if ( new_price == send_price ) {
		TRACE<<LOG_TRADE_CASE<<"same price ... recheck";
		return;
	}

	auto new_order_id = opposite_acc->change(main_order_id, new_price);
	if ( !new_order_id || new_order_id == main_order_id ) {
		DEBUG<<LOG_TRADE_CASE<<"change order has failed .. waiting for existing order to execute!";
		state = WAIT;
	} else {
		main_order_id = new_order_id;
		send_price = new_price;
		INFO<<LOG_TRADE_CASE<<"order change confirmation "<<send_price<<","<<close_size;
	}
}

void PanicClose::cancel() {

}

void PanicClose::wait() {
	if ( current_acc->getPosition() == opposite_acc->getPosition() ) {
		INFO<<LOG_TRADE_CASE<<"case PANIC CLOSE Done!!!";
		state = DONE;
	}
}

void PanicClose::done() {

}

PanicClose::~PanicClose() {

}

////////////////////////////////////////////////////////////////////
///////////////////// Case SoftClose //////////////////////////////
////// close all position in both account at market order //////////
////////////////////////////////////////////////////////////////////

SoftClose::SoftClose(TradeInstance &trade_instance):
		TradeCase(trade_instance, "soft_close", SOFT_CLOSE),
		short_acc(trade_instance.short_acc),
		long_acc(trade_instance.long_acc),
		close_size(0) {

}

void SoftClose::init() {

	//select triggers
	if ( short_acc.getPosition() == long_acc.getPosition() ) {
		state  = DONE;
	} else if ( short_acc.getPosition() > long_acc.getPosition() ) {
		trade_instance.trigger_state = TRIGGER_STATE_POSITIVE;
		current_acc = &long_acc;
		opposite_acc = &short_acc;
	} else if ( short_acc.getPosition() < long_acc.getPosition() ) {
		trade_instance.trigger_state = TRIGGER_STATE_NEGATIVE;
		current_acc = &short_acc;
		opposite_acc = &long_acc;
	}


	close_size = opposite_acc->getPosition() - current_acc->getPosition();

	if ( nullptr == current_acc || nullptr == opposite_acc ) {
		TRACE<<LOG_TRADE_CASE<<" current and opposite accounts are not selected --- can't proceed with soft close";
		state = DONE;
		return;
	}

	auto pnl = 0.0;

	send_price = closePrice(close_size);
	if ( trade_instance.trigger_state == TRIGGER_STATE_POSITIVE ) {
		pnl = send_price - opposite_acc->getAverage();
	} else if ( trade_instance.trigger_state == TRIGGER_STATE_NEGATIVE ) {
		pnl = opposite_acc->getAverage() - send_price;
	}

	if ( pnl < 0 ) {
		TRACE<<LOG_TRADE_CASE<<"SoftClose: failure to close opposite account .. retrying";
	} else {
		TRACE<<LOG_TRADE_CASE<<"SoftClose: Sending close order @"<<send_price<<" for "<<close_size;
		main_order_id = opposite_acc->close(send_price, close_size);
		if ( !main_order_id ) {
			DEBUG<<LOG_TRADE_CASE<<"init: close order failure for long account!";
			state = FAILED;
		}

		state = PRICE_CHANGE;
	}
}


void SoftClose::reprice() {

	if ( opposite_acc->getPosition() == current_acc->getPosition() ) {
		state = DONE;
		return;
	}

	auto pnl = 0.0;

	auto new_price = closePrice(close_size);

	if ( send_price == new_price ) {
		TRACE<<LOG_TRADE_CASE<<"No change in send price .. recheck";
		return;
	}

	if ( trade_instance.trigger_state == TRIGGER_STATE_POSITIVE ) {
		pnl = new_price - opposite_acc->getAverage();
	} else if ( trade_instance.trigger_state == TRIGGER_STATE_NEGATIVE ) {
		pnl = opposite_acc->getAverage() - new_price;
	}

	if ( pnl < 0 ) {
		TRACE<<LOG_TRADE_CASE<<"PNL failure: price isn't suitable for close ... recheck";
	} else {
		auto new_order_id = opposite_acc->change(main_order_id, new_price);
		if ( !new_order_id || new_order_id == main_order_id ) {
			DEBUG<<LOG_TRADE_CASE<<"cancel/replace order failure ... wait for order execution";
			state = WAIT;
		}

		main_order_id = new_order_id;
		send_price = new_price;
	}

}

void SoftClose::wait() {
	if ( current_acc->getPosition() == opposite_acc->getPosition() ) {
		INFO<<LOG_TRADE_CASE<<"case SOFT CLOSE Done!!!";
		state = DONE;
	}
}

void SoftClose::cancel() {

}

void SoftClose::done() {

}

SoftClose::~SoftClose() {

}



////////////////////////////////////////////////////////////////////
//////////////////////// Trade Case 1 //////////////////////////////
//////////// close all position in opposite account ////////////////
////////////////////////////////////////////////////////////////////

TradeCase1::TradeCase1(TradeInstance &trade_instance):
		TradeCase(trade_instance, "case1", CASE_1),
		prev_send_price(0.0),
		close_size(0) {
}

bool TradeCase1::checkFails() {
	close_size = opposite_acc->getPosition();

	if ( 0 == close_size ) {
		DEBUG<<LOG_TRADE_CASE<<" no position to close";
		state = DONE;
		return false;
	}

	prev_send_price = send_price;
	send_price = trade_instance.sendPrice(close_size);
	if ( send_price <= 0 ) {
		DEBUG<<LOG_TRADE_CASE<<" no price to close";
		state = DONE;
		return false;
	}

	//check for pnl test
	if ( !opposite_acc->checkPNL(send_price) ) {
		DEBUG<<LOG_TRADE_CASE<<"pnl has failed -- quiting!";
		state = DONE;
		return false;
	}

	return true;
}

void TradeCase1::init() {

	if ( !checkFails() ) {
		return;
	}

	main_order_id = opposite_acc->close(send_price, close_size);
	if ( !main_order_id ) {
		DEBUG<<LOG_TRADE_CASE<<"close order failure! quiting case";
		state = DONE;
		return;
	}

	state = PRICE_CHANGE;
}


void TradeCase1::reprice() {

	if ( !checkFails() ) return;

	if ( send_price == prev_send_price ) {
		return;
	}

	auto new_order_id = opposite_acc->change(main_order_id, send_price);
	if ( new_order_id == main_order_id || new_order_id == 0 ) {
		DEBUG<<LOG_TRADE_CASE<<"change order has failed -- quiting!";
		state = DONE;
		return;
	}

	main_order_id = new_order_id;

}

void TradeCase1::cancel() {

}

void TradeCase1::done() {

}

TradeCase1::~TradeCase1() {

}


////////////////////////////////////////////////////////////////////
//////////////////////// Trade Case 3 //////////////////////////////
//////////// open position in opposite account in   ////////////////
/////////// maximum is hit in current account //////////////////////
////////////////////////////////////////////////////////////////////

TradeCase3::TradeCase3(TradeInstance &trade_instance):
		TradeCase(trade_instance, "case3", CASE_3),
		prev_send_price(0.0),
		close_size(0) {
}



void TradeCase3::init() {

	close_size = trade_instance.base_qty;
	send_price = trade_instance.sendPrice(close_size);
	main_order_id = current_acc->close(send_price, close_size);
	if ( !main_order_id ) {
		DEBUG<<LOG_TRADE_CASE<<"close order failure! quiting case";
		state = DONE;
		return;
	}

	state = PRICE_CHANGE;
}


void TradeCase3::reprice() {

	if ( current_acc->getPosition() <= trade_instance.max_pos - close_size )
		state = DONE;

	auto new_price = trade_instance.sendPrice(close_size);
	if ( new_price ==  send_price ) return;

	auto new_order_id = opposite_acc->change(main_order_id, new_price);
	if ( new_order_id == main_order_id || new_order_id == 0 ) {
		DEBUG<<LOG_TRADE_CASE<<"change order has failed -- quiting!";
		state = DONE;
		return;
	}

	main_order_id = new_order_id;

}

void TradeCase3::cancel() {

}

void TradeCase3::done() {

}

TradeCase3::~TradeCase3() {

}




////////////////////////////////////////////////////////////////////
//////////////////////// Trade Case 4 //////////////////////////////
//////////// if opposite and current account both zero /////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
TradeCase4::TradeCase4(TradeInstance &trade_instance):
		TradeCase(trade_instance, "case4", CASE_4),
		init_pos(current_acc->getPosition()),
		threshold(0.0),
		threshold_limit(0.0),
		first_order(0),
		first_order_price(0.0),
		first_order_size(0) {
}




void TradeCase4::init() {
	TRACE<<"Case4: init";

	threshold = trade_instance.max_pos*trade_instance.mpv_value/trade_instance.base_qty/2;
	if ( threshold < 0.01 ) threshold = 0.0;
	else if ( threshold > 0.025 ) threshold = 0.03;
	else if ( threshold > 0.015 ) threshold = 0.02;

	threshold_limit =
			trade_instance.trigger_state == TRIGGER_STATE_POSITIVE ?
			trade_instance.bid_price - threshold:
			trade_instance.ask_price + threshold;

	auto& rounds = trade_instance.share_rounds;
	auto sum = 0;
	auto sent = 0;
	auto factor = trade_instance.mpv_value;
	if ( trade_instance.trigger_state == TRIGGER_STATE_POSITIVE ) factor *= -1;
	auto counter = 0;



	//first order
	if ( rounds.begin() == rounds.end() ) state = FAILED;

	auto first_round = rounds.begin();
	first_order_size = first_round->position;
	first_order_price = openPrice(first_order_size);


	first_order = current_acc->open(first_order_price, first_order_size, on_trade_send_close);

	auto second = rounds.begin();
	second++;

	if ( second != rounds.end() )
			std::for_each(second, rounds.end(),
				[ & ](std::remove_reference<decltype(rounds)>::type::value_type round){

			//if open position is > than sum

			auto size = round.position;
			auto price = this->threshold_limit + factor*counter++;
			this->current_acc->open(price, size, on_trade_send_close);

			});

	state = PRICE_CHANGE;
}


void TradeCase4::reprice() {
	TradeCase::reprice();	//reprice to send pending orders and requote orders

	if ( !first_order ) return;	//if first order is already executed dont reprice

	auto new_price = openPrice(first_order_size);
	if ( first_order_price == new_price ) return;


	auto new_order = current_acc->change(first_order, new_price);
	if ( 0 == new_order || new_order == first_order ) {
		TRACE<<LOG_TRADE_CASE<<"first order has executed!!";
		first_order = 0;
	}

	first_order_price = new_price;
	first_order = new_order;

}

void TradeCase4::wait() {
	if ( current_acc->getPosition() >= trade_instance.max_pos ) {
		state = DONE;
	}

}

void TradeCase4::cancel() {
	DEBUG<<"cancel";

}

void TradeCase4::done() {

}

TradeCase4::~TradeCase4() {

}

////////////////////////////////////////////////////////////////////
//////////////////////// Trade Case On Equality //////////////////////////////
//////////// if opposite and current account both same /////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
OnEquality::OnEquality(TradeInstance &trade_instance):
		TradeCase(trade_instance, "OnEquality", CASE_ONEQUALITY),
		init_pos(current_acc->getPosition()),
		target_pos(trade_instance.max_pos*(current_acc->getTranchCounter()+1)) {
}

void OnEquality::init() {
	sendRequoteOrders();
	state = PRICE_CHANGE;
}


void OnEquality::reprice() {
	TradeCase::reprice();
}

void OnEquality::wait() {
	if ( current_acc->getPosition() >= target_pos ) {
		state = DONE;
	}
}

void OnEquality::cancel() {
	DEBUG<<"cancel";

}

void OnEquality::done() {

}

OnEquality::~OnEquality() {

}



////////////////////////////////////////////////////////////////////
//////////////////////// Trade Case 5 //////////////////////////////
//////////// when any of the position exceed maximum ///////////////
////// if both position are exceeded both should be closed /////////
////////////////////////////////////////////////////////////////////
TradeCase5::TradeCase5(TradeInstance &trade_instance):
		TradeCase(trade_instance, "case5", CASE_5),
		curr_close_size(0),
		oppo_close_size(0),
		curr_send_price(0.0),
		oppo_send_price(0.0),
		curr_order(0),
		oppo_order(0) {
}

double TradeCase5::currentPrice() {
	auto ret = 0.0;
	if ( trade_instance.trigger_state == TRIGGER_STATE_POSITIVE )
		ret = trade_instance.ask_price;
	else ret = trade_instance.bid_price;
	return ret;
}

double TradeCase5::oppositePrice() {
	auto ret = 0.0;
	if ( trade_instance.trigger_state == TRIGGER_STATE_POSITIVE )
		ret = trade_instance.bid_price;
	else ret = trade_instance.ask_price;
	return ret;
}

void TradeCase5::init() {

	TRACE<<LOG_TRADE_CASE<<"init ";


	if ( trade_instance.max_pos < current_acc->getPosition() ) {
		curr_close_size = current_acc->getPosition() - trade_instance.max_pos;
		curr_send_price = currentPrice();
		curr_order = current_acc->close(curr_send_price, curr_close_size);
	}

	if ( trade_instance.max_pos < opposite_acc->getPosition() ) {
		oppo_close_size = opposite_acc->getPosition() - trade_instance.max_pos;
		oppo_send_price = oppositePrice();
		oppo_order = opposite_acc->close(oppo_send_price, oppo_close_size);
	}


	state = PRICE_CHANGE;
}


void TradeCase5::reprice() {
	TRACE<<LOG_TRADE_CASE<<"reprice ";
	if ( opposite_acc->getPosition() <= trade_instance.max_pos &&
			current_acc->getPosition() <= trade_instance.max_pos ) {
		state = DONE;
		return;
	}

	auto new_curr_price = currentPrice();
	if ( curr_send_price != new_curr_price && curr_order ) {
		auto new_order_id = current_acc->change(curr_order, new_curr_price);
		if ( new_order_id == 0 || new_order_id == curr_order )
			curr_order = 0;	//reflect order is done
	}

	auto new_oppo_price = oppositePrice();
	if ( oppo_send_price != new_oppo_price && oppo_order ) {
		auto new_order_id = opposite_acc->change(oppo_order, new_oppo_price);
		if ( new_order_id == 0 || new_order_id == oppo_order )
			oppo_order = 0;	//reflect order is done
	}




}

void TradeCase5::cancel() {

}

void TradeCase5::done() {

}

TradeCase5::~TradeCase5() {

}


////////////////////////////////////////////////////////////////////
//////////////////////// Trade Case 6 //////////////////////////////
//////////// when current account position  is less ////////////////
/////////////////// than opposite account position /////////////////
////////////////////////////////////////////////////////////////////
CloseOppositeWithPNL::CloseOppositeWithPNL(TradeInstance &trade_instance):
		TradeCase(trade_instance, "closeoppositewithpnl", CASE_CLOSEOPPOSITE),
		close_size(0),
		init_size(opposite_acc->getPosition()),
		close_price(0.0),
		close_id(0) {
}

void CloseOppositeWithPNL::init() {

	close_price = trade_instance.trade_price;
	if ( close_price <= 0 ) {
		state = FAILED;
		return;
	}

	auto&& expect = opposite_acc->sizeofCloseOrderWithPNL(trade_instance.trade_price);
	TRACE<<LOG_TRADE_CASE<<" closing opposite with pnl "<<expect.first<<" "<<expect.second<<expect.second;
	close_size = expect.first;

	if ( close_size == 0 ) {
		state = FAILED;
		return;
	}

	close_id = opposite_acc->close(close_price, close_size);
	if ( 0 == close_id ) {
		state = FAILED;
		return;
	}

	state = PRICE_CHANGE;

}

void CloseOppositeWithPNL::reprice() {
	auto oppo_pos = opposite_acc->getPosition();
	if ( init_size - oppo_pos == close_size )
		state = DONE;	//all positions are closed as a result of this trade case execution

	auto new_price = trade_instance.trade_price;	//check
	if ( new_price == send_price ) return;

	auto&& expect = opposite_acc->sizeofCloseOrderWithPNL( new_price );
	auto new_size = expect.first;

	if ( 0 == new_size ) {
		opposite_acc->cancel(main_order_id);
		state = DONE;
		return;
	}

	auto new_order = opposite_acc->change(main_order_id, new_price, new_size);
	if ( new_order == 0 || new_order == main_order_id ) {
		state = DONE;
		return;
	}

	main_order_id = new_order;
	send_price = new_price;
	close_size = new_size;
}

void CloseOppositeWithPNL::failed() {

}

void CloseOppositeWithPNL::cancel() {

}


void CloseOppositeWithPNL::wait() {

}

void CloseOppositeWithPNL::done() {
	auto oppo_pos = opposite_acc->getPosition();
	if ( init_size == oppo_pos ) state = FAILED;	//if position hasn't changed [no positions being closed]
}

CloseOppositeWithPNL::~CloseOppositeWithPNL() {
}


////////////////////////////////////////////////////////////
/////// TRADE CASE 6B //////////////////////////////////////
/// 	HEDGE ORDER FROM CURRENT ACCOUNT TO EQUAL POSITION//
////////////////////////////////////////////////////////////

HedgeCurrent::HedgeCurrent(TradeInstance &trade_instance):
		TradeCase(trade_instance, "hedgecurrent", CASE_HEDGECURRENT),
		ho_size(0) {
}

void HedgeCurrent::init() {

	ho_size = opposite_acc->getPosition() - current_acc->getPosition();
	send_price = trade_instance.sendPrice(ho_size);
	if ( ho_size > 0 &&  send_price > 0 ) {
		main_order_id = current_acc->open(send_price, ho_size);
		state = PRICE_CHANGE;
	} else {
		state = FAILED;
	}

}

void HedgeCurrent::reprice() {

	if ( current_acc->getPosition() == opposite_acc->getPosition() )
		state = DONE;

	auto new_price = openPrice(ho_size, send_price);
	if ( new_price == send_price ) return;

	auto new_order = current_acc->change(main_order_id, new_price);
	if ( new_order == 0 || new_order == main_order_id ) {
		state = WAIT;
		return;
	}

	main_order_id = new_order;
	send_price = new_price;
}


void HedgeCurrent::cancel() {

}


void HedgeCurrent::wait() {
	if ( current_acc->getPosition() == opposite_acc->getPosition() ) {
		state = DONE;
	}
}

void HedgeCurrent::done() {

}

HedgeCurrent::~HedgeCurrent() {
	if ( current_acc->getPosition() == opposite_acc->getPosition() ) {

		//Hedge Order has executed successfully
		current_acc->repackagePositionTable(trade_instance.base_qty,
				trade_instance.mpv_value,
				0.0);
	}
}

////////////////////////////////////////////////////////////////////
//////////////////////// Trade Case 6C //////////////////////////////
//////////// if opposite and current account both zero /////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
TradeCase6C::TradeCase6C(TradeInstance &trade_instance):
		TradeCase(trade_instance, "case6c", CASE_6C),
		init_pos(current_acc->getPosition()),
		tranch_at_init(current_acc->getTranchCounter()) {
}

void TradeCase6C::init() {
	TRACE<<"Case6C: init";

	//number of orders to be sent
	if ( current_acc->getTranchCounter() == trade_instance.tranch_value ) {
		state = DONE;
		return;
	}

	auto residual = trade_instance.max_pos*(current_acc->getTranchCounter()+1) - current_acc->getPosition();

	auto NOO = double( residual ) / trade_instance.base_qty;
	auto starting_price = current_acc->startingPrice();

	auto order_size = 0;
	for ( int remainder = residual, i = 1 ; remainder > 0 ; remainder -= order_size, i++ ) {
		order_size = ( remainder > trade_instance.base_qty ) ? trade_instance.base_qty : remainder ;
		auto send_price = starting_price - sign*trade_instance.mpv_value*i;
		current_acc->open(send_price, order_size, on_trade_send_close);

	}

	state = WAIT;
}


void TradeCase6C::reprice() {
	TRACE<<"Case4: reprice";

}

void TradeCase6C::wait() {
	if ( tranch_at_init+1  == current_acc->getTranchCounter() ) {
		TRACE<<LOG_TRADE_CASE<<"tranch has exceeded: TradeCase6C completed!!";
		state = DONE;
	}

}

void TradeCase6C::cancel() {
	DEBUG<<"cancel";

}

void TradeCase6C::done() {

}

TradeCase6C::~TradeCase6C() {

}




////////////////////////////////////////////////////////////////////
//////////////////////// Trade Case 7 //////////////////////////////
//////////// when current account position  is greater /////////////
/////////////////// than opposite account position /////////////////
////////////////////////////////////////////////////////////////////
TradeCase7::TradeCase7(TradeInstance &trade_instance):
		TradeCase(trade_instance, "case7", CASE_7) {
}


void TradeCase7::init() {
	TRACE<<LOG_TRADE_CASE<<"reference_price: "<<trade_instance.reference_price;
	auto pnl = current_acc->getOpenPNL(trade_instance.reference_price);
	auto adjustment = pnl / opposite_acc->getPosition();
	TRACE<<LOG_TRADE_CASE<<"reference_price: "<<trade_instance.reference_price<<" pnl: "<<pnl<<" adjustment: "<<adjustment;
	opposite_acc->repackagePositionTable(trade_instance.base_qty, trade_instance.mpv_value, adjustment);
	state = DONE;
}

void TradeCase7::reprice() {

}

void TradeCase7::wait() {

}


void TradeCase7::cancel() {

}

void TradeCase7::done() {

}

TradeCase7::~TradeCase7() {
	opposite_acc->printTable();
}


////////////////////////////////////////////////////////////////////
//////////////////////// Trade Case 9 //////////////////////////////
///////when current account position  is greater than zero//////////
/////////////////// and opposite is equal to zero  /////////////////
////////////////////////////////////////////////////////////////////
TradeCase9::TradeCase9(TradeInstance &trade_instance):
		TradeCase(trade_instance, "case9", CASE_9),
		close_size(0) {
}


void TradeCase9::init() {
	auto curr_pos = current_acc->getPosition();
	auto oppo_pos = opposite_acc->getPosition();

	send_price = closePrice(curr_pos);
	if ( send_price <= 0 ) {
		state = FAILED;
		return;
	}

	auto expect = current_acc->sizeofCloseOrderWithPNL(send_price);
	TRACE<<LOG_TRADE_CASE<<" closing current with pnl "<<expect.first<<" "<<expect.second<<expect.second;
	close_size = expect.first;
	if ( close_size == 0 ) {
		state = FAILED;
		return;
	}

	main_order_id = current_acc->close(send_price, close_size);
	if ( 0 == main_order_id ) {
		state = FAILED;
		return;
	}

	state = PRICE_CHANGE;

}

void TradeCase9::reprice() {

	auto curr_pos = current_acc->getPosition();
	auto oppo_pos = opposite_acc->getPosition();

	if ( 0 == curr_pos ) {
		TRACE<<LOG_TRADE_CASE<<"all positions in current account closed with pnl";
		state = DONE;
		return;
	}

	auto new_price = closePrice(curr_pos);
	if ( new_price <= 0 || new_price == send_price ) {
		return;
	}

	auto expect = current_acc->sizeofCloseOrderWithPNL( new_price );
	auto new_size = expect.first;

	if ( 0 == new_size ) {
		return;
	}

	auto new_order = current_acc->change(main_order_id, new_price, new_size);
	if ( new_order == 0 || new_order == main_order_id ) {
		state = WAIT;
		return;
	}

	main_order_id = new_order;
	send_price = new_price;
	close_size = new_size;
}

void TradeCase9::wait() {
	auto curr_pos = current_acc->getPosition();
	auto oppo_pos = opposite_acc->getPosition();

	if ( 0 == curr_pos ) {
		TRACE<<LOG_TRADE_CASE<<"all positions in current account closed with pnl";
		state = DONE;
		return;
	}
}


void TradeCase9::cancel() {

}

void TradeCase9::done() {

}

TradeCase9::~TradeCase9() {
	current_acc->printTable();
}


////////////////////////////////////////////////////////////////////
//////////////////////// Trade Case 8 //////////////////////////////
//////////// when current account position  is greater /////////////
/////////////////// than minimum round and opposite ////////////////
////////////////////////// is zero /////////////////////////////////
////////////////////////////////////////////////////////////////////
MaxTLV::MaxTLV(TradeInstance &trade_instance):
		TradeCase(trade_instance, "max_tlv", CASE_MAXTLV),
		curr_id(0),
		oppo_id(0),
		curr_price(0.0),
		oppo_price(0.0),
		curr_size_init(current_acc->getPosition()),
		oppo_size_init(opposite_acc->getPosition()) {
}


void MaxTLV::init() {
//	auto pnl = opposite_acc->getPNL();	//get PNL from opposite account
//	opposite_acc->resetPositionTable();	//
//	auto adjustment = pnl / current_acc->getPosition();
//	current_acc->repackagePositionTable(trade_instance.base_qty, trade_instance.mpv_value, adjustment);
//	state = DONE;

	curr_price = closePrice(trade_instance.max_pos);
	oppo_price = openPrice(trade_instance.max_pos);

	curr_id = current_acc->close(send_price, trade_instance.max_pos);
	oppo_id = opposite_acc->close(send_price, trade_instance.max_pos);

	state = PRICE_CHANGE;
}

void MaxTLV::reprice() {

	if ( current_acc->getPosition() ==
				( curr_size_init - trade_instance.max_pos )  &&
		 opposite_acc->getPosition() ==
					( oppo_size_init - trade_instance.max_pos ) ) {
		state = DONE;
		return;
	}

	if ( current_acc->getPosition() !=
			( curr_size_init - trade_instance.max_pos )  ) {
		auto new_price = closePrice(trade_instance.max_pos, curr_price);
		if ( new_price != curr_price ) {
			auto new_order = current_acc->change(curr_id, new_price);
			if ( new_order == 0 || new_order == curr_id ) curr_id = 0;
			else {
				curr_id = new_order;
				curr_price = new_price;
			}
		}
	}

	if ( opposite_acc->getPosition() !=
			( oppo_size_init - trade_instance.max_pos )  ) {
		auto new_price = openPrice(trade_instance.max_pos, oppo_price);
		if ( new_price != oppo_price ) {
			auto new_order = opposite_acc->change(oppo_id, new_price);
			if ( new_order == 0 || new_order == oppo_id ) oppo_id = 0;
			else {
				oppo_id = new_order;
				oppo_price = new_price;
			}
		}
	}




}

void MaxTLV::wait() {

}


void MaxTLV::cancel() {

}

void MaxTLV::done() {

}

MaxTLV::~MaxTLV() {
}



////////////////////////////////////////////////////////////////////
//////////////////////// CLose Opposite Account //////////////////////////////
//////////// By anticipating its effect on current account reprice  ///////////////
////////////////////////////////////////////////////////////////////
CloseOppositeWithAnticipatedPNL::CloseOppositeWithAnticipatedPNL(TradeInstance &trade_instance):
		TradeCase(trade_instance, "closeoppositewithantiipatedPNL", CASE_CLOSEOPPOSITEWITH_ANTICIPATED_PNL),
		close_size( opposite_acc->getPosition() ),
		close_price(0.0),
		order_id(0) {
}

bool CloseOppositeWithAnticipatedPNL::isOppositeCloseProfitable(const double send_price) {


	TRACE<<LOG_TRADE_CASE<<"check table for pnl against "<<send_price;
	opposite_acc->printTable();
	auto&& pnl_status = opposite_acc->sizeofCloseOrderWithPNL(send_price);
	auto size = pnl_status.first;
	auto pnl = pnl_status.second;
	auto adjustment = pnl / current_acc->getPosition();

	TRACE<<LOG_TRADE_CASE<<"expected pnl: "<<pnl<<" size: "<<size<<" adjustment: "<<adjustment;

	auto reprice = current_acc->getAverage() - adjustment*sign;

	if ( (reprice <= trade_instance.trade_price && trade_instance.trigger_state == TRIGGER_STATE_POSITIVE) || //+ive trigger
			(reprice >= trade_instance.trade_price && trade_instance.trigger_state == TRIGGER_STATE_NEGATIVE) ) //-ive trigger
		return true;

	return false;

}

void CloseOppositeWithAnticipatedPNL::init() {

	auto diff = close_size = opposite_acc->getPosition() - current_acc->getPosition();;
	auto send_price = openPrice(diff);
	if ( isOppositeCloseProfitable(send_price) ) {
			close_price = send_price;
			close_size = opposite_acc->getPosition() - current_acc->getPosition();
			order_id = opposite_acc->close (close_price, close_size);
			if ( 0 == order_id ) state = FAILED;
			else state = PRICE_CHANGE;
	} else state = FAILED;

}

void CloseOppositeWithAnticipatedPNL::reprice() {
	auto diff = opposite_acc->getPosition() - current_acc->getPosition();
	auto oppo_pos = opposite_acc->getPosition();
	auto send_price = openPrice(diff);

	auto is_profitable = isOppositeCloseProfitable(send_price);

	if ( 0 == diff ) {
		state = DONE;
	} else if ( send_price != close_price &&  is_profitable ) {


		auto new_order_id = opposite_acc->change(order_id, send_price, diff);

		if ( new_order_id == 0 || new_order_id == order_id ) {
			state = WAIT;
		} else {
			order_id = new_order_id;
			close_price = send_price;
			close_size = diff;
		}
	} else if ( !is_profitable ) {
		opposite_acc->cancel(order_id);
		state = FAILED;
	}
}

void CloseOppositeWithAnticipatedPNL::wait() {
	auto diff = opposite_acc->getPosition() - current_acc->getPosition();
	if ( 0 == diff ) state = DONE;
}


void CloseOppositeWithAnticipatedPNL::cancel() {

}

void CloseOppositeWithAnticipatedPNL::done() {

}

CloseOppositeWithAnticipatedPNL::~CloseOppositeWithAnticipatedPNL() {
	auto pnl = opposite_acc->getPNL();
	auto curr_pos = current_acc->getPosition();
	if ( pnl > 0.0 && curr_pos > 0 ) //if pnl in the opposite account exist reprice the current_account
	{
		auto adjustment = pnl / curr_pos;
		current_acc->repackagePositionTable(trade_instance.base_qty,
				trade_instance.mpv_value,
				adjustment);	//repackage current account using pnl in opposite account
		opposite_acc->resetPNL();	//reset pnl in opposite account
	}
}


////////////////////////////////////////////////////////////////////
//////////////////////// Trade Case HEDGED MODE ///////////////////
/////// when one of the account has achevied max position /////////
/////////////////// open in opposite to that account  ////////////////
////////////////////////// hedge amount /////////////////////////////////
////////////////////////////////////////////////////////////////////
HedgedMode::HedgedMode(TradeInstance &trade_instance):
		TradeCase(trade_instance, "hedgedmode", CASE_HEDGEDMODE),
		ho_size(0),
		price(0.0),
		short_acc(trade_instance.short_acc),
		long_acc(trade_instance.long_acc),
		order_id(0),
		selected_account(nullptr),
		other_account(nullptr) {
}


void HedgedMode::init() {

	// if short account is to be hedged
	if ( short_acc.hedgedModeCondition(
			trade_instance.bid_price,
			trade_instance.ask_price) ) {
		ho_size = short_acc.getPosition() - long_acc.getPosition();
		price = trade_instance.sendPrice(ho_size);
		order_id = long_acc.open(price, ho_size);
		selected_account = &long_acc;
		other_account = &short_acc;

	} else if ( long_acc.hedgedModeCondition(
			trade_instance.bid_price,
			trade_instance.ask_price) ) {
		ho_size = long_acc.getPosition() - short_acc.getPosition();
		price = trade_instance.sendPrice(ho_size);
		order_id = short_acc.open(price, ho_size);
		selected_account = &short_acc;
		other_account = &long_acc;
	}
	state = PRICE_CHANGE;
}

void HedgedMode::reprice() {
	auto new_price = trade_instance.sendPrice(ho_size);
	if ( short_acc.getPosition() == long_acc.getPosition() ) {
		state = DONE;
	} else if ( price != new_price ) {
		auto new_order_id = selected_account->change(order_id, new_price);
		if ( new_order_id == order_id || new_order_id == 0 ) {	//order didn't changed
			state = WAIT;
		} else {
			price = new_price;
			order_id = new_order_id;
		}
	}
}

void HedgedMode::wait() {
	if ( short_acc.getPosition() == long_acc.getPosition() ) state = DONE;
}


void HedgedMode::cancel() {

}

void HedgedMode::done() {

}

HedgedMode::~HedgedMode() {
	if ( isDone() && short_acc.getPosition() == long_acc.getPosition() )
		other_account->inc_tranch();
}


////////////////////////////////////////////////////////////////////
//////////////////////// Trade Case CLOSE_TRIGGER ///////////////////
/////// when current account position is greater than opposite /////////
/////////////////// account   ////////////////
////////////////////////// hedge amount /////////////////////////////////
////////////////////////////////////////////////////////////////////
CloseTrigger::CloseTrigger(TradeInstance &trade_instance):
		TradeCase(trade_instance, "closetrigger", CASE_CLOSE_TRIGGER),
		close_size(0),
		error(0.0) {
}


void CloseTrigger::init() {
	auto curr_pos = current_acc->getPosition();
	auto oppo_pos = opposite_acc->getPosition();

	auto excess_qty = uint32_t(curr_pos - oppo_pos);

	if ( excess_qty <= 0 ) {
		INFO<<LOG_TRADE_CASE<<"no excess qty: "<<excess_qty;
		state = DONE;
	}

	error = std::max(fabs(trade_instance.bar0.open - trade_instance.price_estimate),
			trade_instance.mpv_value)*trade_instance.mpv_multiplier;		//estimation error

	TRACE<<LOG_TRADE_CASE<<"error {max(abs(open-avicenna),mpv)*open_multiplier} calculated: "<<error;

	current_acc->positionTable().for_each_position([this, &excess_qty](PositionRecord position){
		if ( !position.is_complete && excess_qty > 0 ) {
			auto price = position.open_price + this->error*this->sign;
			auto size = std::min(position.open_size, excess_qty);
			auto order_id = this->current_acc->close(price, size*trade_instance.divisor);
			if ( order_id ) {
				OrderRep order{order_id, size, position.open_price, price, false};
				this->orders.push_back(order);
				excess_qty -= size;
			}
		}
	});

	if ( orders.size() > 0 ) state = PRICE_CHANGE;
	else state = FAILED;

}


void CloseTrigger::reprice() {
	auto curr_pos = current_acc->getPosition();
	auto oppo_pos = opposite_acc->getPosition();

	auto excess_qty = uint32_t(curr_pos - oppo_pos);

	if ( excess_qty <= 0 ) {
		INFO<<LOG_TRADE_CASE<<"no excess qty: "<<excess_qty;
		state = DONE;
	}

	auto new_error = std::max(fabs(trade_instance.bar0.open - trade_instance.price_estimate),
			trade_instance.mpv_value)*trade_instance.mpv_multiplier;		//estimation error

	if ( new_error == error ) {
		TRACE<<LOG_TRADE_CASE<<"error {max(abs(open-avicenna),mpv)*open_multiplier} unchanged: "<<new_error;
		return;
	}

	error = new_error;
	TRACE<<LOG_TRADE_CASE<<"error {max(abs(open-avicenna),mpv)*open_multiplier} changed: "<<error;

	auto nchanged = 0;
	std::for_each(orders.begin(), orders.end(), [this, &nchanged](OrderRep& order) {

		if ( order.is_exectued ) return;

		auto price = order.open + this->error*this->sign;
		auto new_id = this->current_acc->change(order.id, price);
		if ( new_id == order.id || 0 == new_id ) {
			TRACE<<LOG_TRADE_CASE<<"change order failed -- order already executed!!"<<new_id;
			order.is_exectued = true;
			return;
		} else {
			order.id = new_id;
			order.close = price;
			nchanged++;
		}
	});

	if ( nchanged == 0 ) {
		TRACE<<LOG_TRADE_CASE<<"no order has changed -- case done!!! "<<nchanged;
		state = DONE;
	}


}

void CloseTrigger::wait() {
	auto curr_pos = current_acc->getPosition();
	auto oppo_pos = opposite_acc->getPosition();

	if ( curr_pos == oppo_pos ) {
		TRACE<<LOG_TRADE_CASE<<"position equal ... case complete!!!";
		state = DONE;
		return;
	}
}


void CloseTrigger::cancel() {

}

void CloseTrigger::done() {

}

CloseTrigger::~CloseTrigger() {

}



TradeCase10::TradeCase10(TradeInstance& trade_instance):
	TradeCase(trade_instance, "case10", CASE_10),
	trigger_state(trade_instance.trigger_state),
	account_to_close(trade_instance.opposite_acc),
	close_size(0),
	original_size(0),
	error(0.0) {
}


void TradeCase10::init() {

	auto curr_pos = current_acc->getPosition();
	auto oppo_pos = opposite_acc->getPosition();
	if ( curr_pos > 0 && oppo_pos == 0 ) {
		TRACE<<LOG_TRADE_CASE<<"opposite position is completely close -- exiting case";
		state = DONE;
	} else if ( curr_pos > oppo_pos ) {
		TRACE<<LOG_TRADE_CASE<<" current position > opposite position : now selecting current position to close";
		account_to_close = trade_instance.current_acc;
	}

	original_size = account_to_close->getPosition();
	close_size = std::min<int64_t>(trade_instance.base_qty, original_size);
	if ( close_size <= 0 ) {
		INFO<<LOG_TRADE_CASE<<"no excess qty: "<<close_size;
		state = DONE;
	}

	error = std::max(fabs(trade_instance.bar0.open - trade_instance.price_estimate),
			trade_instance.mpv_value)*trade_instance.mpv_multiplier;		//estimation error

	if ( dynamic_cast<LongTradeAccount*>(account_to_close) ) send_price = trade_instance.bar0.open + error;
	else send_price = trade_instance.bar0.open - error;
	send_price = truncate(send_price, 2);	//take price only to two decimal places

	TRACE<<LOG_TRADE_CASE<<"order info: open_price="<<trade_instance.bar0.open<<" send_price="<<send_price<<" error="<<error<<" size="<<close_size;
	main_order_id = account_to_close->close(send_price, close_size);
	if ( !main_order_id ) {
		TRACE<<LOG_TRADE_CASE<<"close order failed: @"<<send_price<<" "<<close_size;
		state = FAILED;
	}

	else state = PRICE_CHANGE;
}


void TradeCase10::reprice() {

	error = std::max(fabs(trade_instance.bar0.open - trade_instance.price_estimate),
			trade_instance.mpv_value)*trade_instance.mpv_multiplier;		//estimation error

	auto new_price = 0.0;
	if ( dynamic_cast<LongTradeAccount*>(account_to_close) ) new_price = trade_instance.bar0.open + error;
	else new_price = trade_instance.bar0.open - error;
	new_price = truncate(new_price, 2);	//take price only to two decimal places

	if ( new_price == send_price ) return;

	TRACE<<LOG_TRADE_CASE<<"change order info: send_price="<<new_price<<" error="<<error<<" size="<<close_size;
	auto new_order_id = account_to_close->change(main_order_id, new_price);
	if ( new_order_id == main_order_id || !new_order_id ) {
		TRACE<<LOG_TRADE_CASE<<"close order failed: @"<<send_price<<" "<<close_size;
		state = WAIT;
	} else {
		send_price = new_price;
		main_order_id = new_order_id;
	}
}

void TradeCase10::wait() {
	auto curr_pos = current_acc->getPosition();
	auto oppo_pos = opposite_acc->getPosition();

	if ( curr_pos > 0 && oppo_pos == 0 ) {
		TRACE<<LOG_TRADE_CASE<<"curr_pos > 0 && oppo_pos == 0 ... case complete!!!";
		state = DONE;
	} else if ( account_to_close->getPosition() <= (original_size-close_size) ) {
		TRACE<<LOG_TRADE_CASE<<"position close successfully ... close more positions in selected account";
		init();
	}
}


void TradeCase10::cancel() {

}

void TradeCase10::done() {

}

TradeCase10::~TradeCase10() {

}

