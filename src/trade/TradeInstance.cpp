/*
 * TradeAccountManager.cpp
 *
 *  Created on: Jul 3, 2016
 *      Author: jamil
 */

#include "TradeInstance.h"
#include "../config/TradingConfig.h"
#include "../config/AdminConfig.h"
#include "TradeCase.h"

//ThreadPool<TradeInstance *> TradeInstance::trade_instances;

const char TradeInstance::trigger_rep[] = {' ', '+', '-'};
const std::string TradeInstance::state_rep[] = {
		"",
		"init",
		"check_cancel",
		"select_cases",
		"limit_exceeds",
		"main_cond_true",
		"main_cond_false",
		"case_1_init",
		"case_1",
		"case_2_init",
		"case_2",
		"case_3_init",
		"case_3",
		"case_4_init",
		"case_4",
		"case_5_init",
		"case_5",
		"case_6_init",
		"case_6",
		"soft_close_init",
		"soft_close",
		"panic_close_init",
		"panic_close",
};

TradeInstance::TradeInstance(const TradeAccount &account, Symbol &symbol):
	trade_account(account),
	symbol(symbol),
	instrument_skew(symbol.instrument_skew),
	share_rounds(symbol.share_rounds),
	tranch_value(symbol.tranch_value),
	max_round(share_rounds.back().position),
	min_round(share_rounds.front().position),
	current_round(share_rounds.begin()),
	multiplier(symbol.multiplier),
	divisor(symbol.divisor),
	trigger_state(TRIGGER_STATE_INIT),
	avicenna_trigger_state(TRIGGER_STATE_INIT),
	prev_trigger_state(TRIGGER_STATE_INIT),
	short_acc(symbol.name, *account.short_account, symbol.divisor),
	long_acc(symbol.name, *account.long_account, symbol.divisor),
	opposite_acc(nullptr),
	current_acc(nullptr),
	state(TRADE_INSTANCE_NONE),
	position_close_state(POSITION_CLOSE_INIT),
	data_wait(true),
	is_enable(true),
	is_panic_close(false),
	is_soft_close(false),
	price_estimate_on_trigger(0.0),
	price_estimate(0.0),
	price_estimate_prev(0.0),
	avicenna_toggle(true),
	mpv_value(symbol.mpv.value),
	mpv_limit(symbol.mpv.limit),
	mpv_multiplier(symbol.mpv.multiplier),
	mmf(symbol.mmf),
	base_qty(symbol.base_qty),
	cancel_on_trigger(symbol.cancel_on_trigger),
	trade_case(nullptr),
	redisWriter(AdminConfig::settings->getRedisIp(),AdminConfig::settings->getRedisPort()),
	max_pos(0),
	min_pos(min_round),
	previous_case(TradeCase::NONE),
	reference_price(0.0),
	trade_price(0.0),
	bid_size(0),
	ask_size(0),
	bid_price(0.0),
	ask_price(0.0),
	panic_close_init(false),
	soft_close_init(false),
	last_state(TradeCase::NOT_SELECTED),
	open_price(0.0),
	max_price(0.0),
	min_price(0.0),
	close_price(0.0),
	wait_for_next_time_window(false) {


	TRACE<<LOG_TRADE_INSTANCE<<"round limits = "<<max_round<<" "<<min_round;

	max_pos = 0;
	std::for_each(share_rounds.begin(), share_rounds.end(), [this](ShareRound &v){ this->max_pos += v.position;});

	//setting maximum order limit for each account
	short_acc.setMaximumOrderLimit(trade_account.maximum_order);
	long_acc.setMaximumOrderLimit(trade_account.maximum_order);

	//connecting short and long to each other
	short_acc.setOtherAccount(&long_acc);
	long_acc.setOtherAccount(&short_acc);

	//set redis writer
	short_acc.setRedisWriter(&redisWriter);
	long_acc.setRedisWriter(&redisWriter);

	short_acc.setMaxPosition(max_pos);
	long_acc.setMaxPosition(max_pos);

	short_acc.setTranchValue(tranch_value);
	long_acc.setTranchValue(tranch_value);

	std::string keyToRead=long_acc.account.getAccountName()+":"+long_acc.getSymbolName();
	std::string keyToRead2=short_acc.account.getAccountName()+":"+short_acc.getSymbolName();
	//RedisWriter writer;
	 std::string result =redisWriter.readKeyFromRedis(keyToRead);
	    	  DEBUG<<"Reading key ("<<keyToRead<<") from redis ";
	    	  DEBUG<<"Got Key "<<result;
	if(result=="**nonexistent-key**"){
		//means the required key couldnot be found so continue without loding positions
	}else{
		Position currentPosition=redisWriter.parseStem1Value(result);
		//short_acc.setAverage( boost::lexical_cast<double>(currentPosition.Price));
		long_acc.setAverage(boost::lexical_cast<double>(currentPosition.Price));
		 if(!currentPosition.pif.empty()){
			long_acc.setPriceInForce(boost::lexical_cast<double>(currentPosition.pif));
		 }
		 if(!currentPosition.rif.empty()){
			 auto currentRIF=boost::lexical_cast<uint32_t>(currentPosition.rif);
		 	long_acc.setRoundInForce(currentRIF);
		 }
		auto quantity=boost::lexical_cast<int64_t>( currentPosition.Quantity.c_str() );
		long_acc.setPosition(quantity);

		auto totalExecuted=boost::lexical_cast<int64_t>( currentPosition.TotalExecutedShares.c_str() );
		long_acc.setTotalExecuted(totalExecuted);

		//reset live buy and sell at the beginning
		redisWriter.writeLiveSharesInfo(keyToRead,"0","0",currentPosition.TotalExecutedShares);
	}

	std::string result2 =redisWriter.readKeyFromRedis(keyToRead2);
	 DEBUG<<"Reading key ("<<keyToRead2<<") from redis ";
	 DEBUG<<"Got Key "<<result2;
	 if(result2=="**nonexistent-key**"){
	 		//means the required key couldnot be found so continue without loding positions
	 	}else{
	 		Position currentPosition=redisWriter.parseStem1Value(result2);
	 		short_acc.setAverage( boost::lexical_cast<double>(currentPosition.Price));
	 		 if(!currentPosition.pif.empty()){
	 			short_acc.setPriceInForce(boost::lexical_cast<double>(currentPosition.pif));
	 		 }
	 		 if(!currentPosition.rif.empty()){
	 			auto currentRIF=boost::lexical_cast<uint32_t>(currentPosition.rif);
	 			short_acc.setRoundInForce(currentRIF);
	 		 }
	 		auto quantity = boost::lexical_cast<int64_t>( currentPosition.Quantity.c_str() );
	 		short_acc.setPosition(-1*quantity);

	 		auto totalExecuted=boost::lexical_cast<int64_t>( currentPosition.TotalExecutedShares.c_str() );
	 		short_acc.setTotalExecuted(totalExecuted);

	 		//reset live buy and sell at the beginning
	 		redisWriter.writeLiveSharesInfo(keyToRead2,"0","0",currentPosition.TotalExecutedShares);

	 	}


	//instantiate main trigger processing thread
	std::thread main_thread(&TradeInstance::triggerProcessing, this);


//	std::thread::native_handle_type native_type = main_thread.native_handle();
//	sched_param sched_param;
//	sched_param.__sched_priority = 2;
//	pthread_setschedparam(native_type, SCHED_RR, &sched_param);

	main_thread.detach();


//	trade_instances.push_task(this);

//	DEBUG<<"quantity to open"<<quantityToOpen(0, 400, false);
//	exit(0);
}

inline void TradeInstance::reprice() {

	//set price in force and round in force
	auto spread = fabs(current_acc->getAverage() - opposite_acc->getAverage());

	if ( trigger_state == TRIGGER_STATE_POSITIVE ) {
		long_acc.setPriceInForce(price_estimate_on_trigger);
		short_acc.setPriceInForce(price_estimate_on_trigger - spread);
	} else if ( trigger_state == TRIGGER_STATE_NEGATIVE ) {
		short_acc.setPriceInForce(price_estimate_on_trigger);
		long_acc.setPriceInForce(price_estimate_on_trigger + spread);
	}

	DEBUG<<LOG_TRADE_INSTANCE<<"reprice: spread = "<<spread<<" AP (@trigger) = "<<price_estimate_on_trigger;
}



/*
 * @brief Get next round of shares to be sell/purchase
 *
 * @return size of next round to be purchase/sell
 */
inline uint32_t TradeInstance::quantityToOpen() {
	auto oppo_pos = opposite_acc->getPosition()*divisor;
	auto curr_pos = current_acc->getPosition()*divisor;


	auto rounded_opposite_pos = ( oppo_pos + 50 )/100*100;

	auto max_pos = std::max(oppo_pos, curr_pos);
	auto beg = share_rounds.begin();
	auto current_round = share_rounds.begin();
	for ( ; current_round != share_rounds.end() ; current_round++ ) {
		if ( max_pos <= current_round->position ) {
			TRACE<<LOG_TRADE_INSTANCE<<"quantityToOpen: round depending on current positions --> "<<current_round->increments<<","<<current_round->position;
			break;
		}
	}

	if ( current_round == share_rounds.end() ) {
		TRACE<<LOG_TRADE_INSTANCE<<"maximum account position in one of the account exceeds maximum limit (handle overflow/underflow) returning 0 "<<"max: "<<max_pos;
		return 0;
	}

	TRACE<<LOG_TRADE_INSTANCE<<"selecting new round";
	if ( (rounded_opposite_pos + current_round->increments) > current_round->position ) {	//open next round
		if ( current_round+1 != share_rounds.end() ) {
			previous_round = current_round;
			current_round = current_round+1;
		} else {
			WARN<<LOG_TRADE_INSTANCE<<"quantityToOpen: quantities in last round (not iterating furtherr)";
//			return 0;
		}
	}

	TRACE<<LOG_TRADE_INSTANCE<<"quantityToOpen: current round -> ("<<current_round->increments<<","<<current_round->position<<")";

	//opposite position should be rounded to nearest 100
	//for safe side target position is also checked for maximum round so that no position can be greater than that
	auto target_pos = std::min(size_t(current_round->increments + rounded_opposite_pos), max_round);

	if ( target_pos > curr_pos ) {
		auto open_pos = target_pos - curr_pos;
		return open_pos/divisor;
	} else return 0;


//	if ( target_pos > curr_pos ) {
//		auto diff = target_pos - curr_pos;
//		if ( diff ) return  diff;
//	} else {
//		auto upper_limit = current_round->position;
//		auto diff = upper_limit - curr_pos;
//		auto min = std::min<size_t>(current_round->increments, diff);
//
//		TRACE<<LOG_TRADE_INSTANCE<<upper_limit<<" "<<diff<<" "<<min;
//
//		return min;
//	}

}

uint32_t TradeInstance::roundForCurrentAccount() {
	auto ret = uint32_t(0);
	auto sum = 0;
	auto curr_round = std::find_if(std::begin(share_rounds),
			std::end(share_rounds),
			[&] (std::remove_reference<decltype(share_rounds)>::type::value_type p)->bool {
		sum += p.position;
		if ( current_acc->getPosition() < sum ) return true;
	});

	if ( curr_round != std::end(share_rounds) ) {
		if ( std::next(curr_round) != std::end(share_rounds) ) {
			curr_round = std::next(curr_round);
			sum += curr_round->position;
		}

		ret = sum;
	}

	return ret;
}

inline size_t TradeInstance::roundInForce() {
	auto oppo_pos = opposite_acc->getPosition();
	auto curr_pos = current_acc->getPosition();

	auto max_pos = std::max(oppo_pos, curr_pos);

	auto round = share_rounds.begin();
	auto itr = share_rounds.begin();
	for(; itr != share_rounds.end(); itr++ ) {
		if ( max_pos < itr->position ) {
			TRACE<<LOG_TRADE_INSTANCE<<"check roundInForce: "<<itr->position;
//			if ( itr != share_rounds.end()-1 )
				round = itr;
				break;
		}
	}

	if ( itr == share_rounds.end() ) {//if iterator has reached end without finding the round means
		//max_pos has exceeded the maximum limit in this case overflow cases should be executed
		WARN<<LOG_TRADE_INSTANCE<<"position exceeds share rounds maximum limit (overflow cases should be executed)"<<max_pos;
		return 0;
	}

	return round->position;

}

inline void TradeInstance::readSymbolData() {

	//copy trade context
//	TRACE<<LOG_TRADE_INSTANCE<<"In readSymbolData()";

//	if ( symbol.stack_size() > 0 || data_wait ) {	//only wait to pop if there is data in symbol stack or situation demands new data for business logic

		auto&& stamp = this->symbol.pop();
		ask_size = stamp.ask_size;
		ask_price = stamp.ask_price;
		bid_size = stamp.bid_size;
		bid_price = stamp.bid_price;


		//now also update Bar prices previous bar
		bar1.minimum = symbol.min_price_prev;
		bar1.maximum = symbol.max_price_prev;
		bar1.close = symbol.close_price_prev;
		bar1.open = symbol.open_price_prev;


		bar0.minimum = symbol.min_price;
		bar0.maximum = symbol.max_price;
		bar0.close = symbol.close_price;
		bar0.open = symbol.open_price;

		if ( avicenna_toggle ) {
//			TRACE<<LOG_TRADE_INSTANCE<<"avicenna price estimate 1 "<<stamp.price_estimate1;
			price_estimate = stamp.price_estimate1;
		}
		else {
//			TRACE<<LOG_TRADE_INSTANCE<<"avicenna price estimate 2 "<<stamp.price_estimate2;
			price_estimate = stamp.price_estimate2;
		}


		data_wait = false;
//	}
}

string getTrigerState(TriggerState v)
{
    switch (v)
    {
    	case TRIGGER_STATE_INIT :		return "TRIGGER_STATE_INIT";
    	case TRIGGER_STATE_POSITIVE :	return "TRIGGER_STATE_POSITIVE";
    	case TRIGGER_STATE_NEGATIVE :	return "TRIGGER_STATE_NEGATIVE";
        default:      					return "TRIGGER_STATE_UNKNOWN";
    }
}

void TradeInstance::triggerProcessing() {

	auto trade_info = trade_account.id + string("-") + symbol.name;
	BOOST_LOG_SCOPED_THREAD_TAG("TradeInfo", trade_info);

	start = std::chrono::steady_clock::now();

	auto is_not_done = false;
	while ( !is_not_done ) {

		if ( !is_enable ) {
//			TRACE<<LOG_TRADE_INSTANCE<<"checking if trade account is enable in next 60 seconds";
			std::this_thread::sleep_for(std::chrono::seconds(60));
			continue;
		}


		readSymbolData();

		if ( price_estimate <= 0.0 ) {
			continue;
		}

		trade_price = price_estimate;

		if ( !specialCases() ) {
			//if no special case to execute then do trigger and othe cases selection
			triggerSelection();
		}

		if ( trade_case ) {
			TRACE<<LOG_TRADE_INSTANCE<<"trade case run";
			last_state = trade_case->run();
			auto type = trade_case->getType();
			if ( ( type == TradeCase::PANIC_CLOSE ||		//if tradecase is type soft close or panic close trading thread must exit
					type == TradeCase::SOFT_CLOSE ) &&
					trade_case->isDone() ) {
				is_not_done = true;
				INFO<<LOG_TRADE_INSTANCE<<"panic/soft close complete ... exiting!!!";
			} else if ( (trade_case->isDone() || last_state == TradeCase::FAILED) &&
					( trade_case->getType() != TradeCase::PANIC_CLOSE &&
					trade_case->getType() != TradeCase::SOFT_CLOSE ) ) {
				previous_case = trade_case->getType();
				TRACE<<LOG_TRADE_INSTANCE<<"case "<<trade_case->getName()<<" done!";
				trade_case = nullptr;
			}
		}

		//writing all data to redis
		//long acount
		redisWriter.writeToRedis(long_acc.account.getAccountName(),
				long_acc.getSymbolName(),
				std::to_string(long_acc.getTotal()),
				std::to_string(long_acc.getAverage()),
				std::to_string(long_acc.getRoundInForce()),
				std::to_string(long_acc.getPriceInForce()),
				std::to_string(long_acc.getTotalOpen()),
				std::to_string(long_acc.getTotalClose()),
				std::to_string(long_acc.getTotalExecuted()));
		//short account
		redisWriter.writeToRedis(short_acc.account.getAccountName(),
				short_acc.getSymbolName(),
				std::to_string(-1*short_acc.getTotal()),
				std::to_string(short_acc.getAverage()),
				std::to_string(short_acc.getRoundInForce()),
				std::to_string(short_acc.getPriceInForce()),
				std::to_string(short_acc.getTotalClose()),
				std::to_string(short_acc.getTotalOpen()),
				std::to_string(short_acc.getTotalExecuted()));

		//trade case info long/short
		if(trade_case!=nullptr){
		string time=redisWriter.getCurrentTime();
		string selectedState=getTrigerState(trigger_state);
		redisWriter.writeTradeCaseToRedis(trade_account.id,
				current_acc->getSymbolName(),
				time,
				selectedState,
				"CASE"+std::to_string(trade_case->type),
				std::to_string(bid_size),
				std::to_string(bid_price),
				std::to_string(ask_size),
				std::to_string(ask_price),
				std::to_string(price_estimate));
		}
		//write position table long/short
		TRACE<<LOG_TRADE_INSTANCE<<"writing Position records for long account";
		PositionRecordTable& longtable=long_acc.positionTable();
		redisWriter.writePositionRecordTableToRedis(long_acc.account.getAccountName(),
				long_acc.getSymbolName(),longtable);
		TRACE<<LOG_TRADE_INSTANCE<<"writing Position records for short account";
		PositionRecordTable& shorttable=short_acc.positionTable();
		redisWriter.writePositionRecordTableToRedis(short_acc.account.getAccountName(),
				short_acc.getSymbolName(),shorttable);

		end = std::chrono::steady_clock::now();
		auto latency = std::chrono::duration<double>( end - start );
//		TRACE<<LOG_TRADE_INSTANCE<<"time duration(sec): "<<latency.count();

	}
}

/*
 * specialCases are cases that executes irrespective of trigger state like panic and soft close
 *
 * @return true if special case is to execute false otherwise
 */
bool TradeInstance::specialCases() {


//	if ( nullptr != current_acc && nullptr != opposite_acc ) {
//		TRACE<<LOG_TRADE_INSTANCE<<"positions = "<<current_acc->getPosition()<<" "<<opposite_acc->getPosition();
//	}


	selfDestruct();

	if ( is_panic_close ) {
		if ( panic_close_init ) {
			panic_close_init = false;	//panic close acheived
			trade_case = TradeCaseFactory::create(*this, TradeCase::PANIC_CLOSE);
			TRACE<<LOG_TRADE_INSTANCE<<"Initiating panic close";
		}
		return true;
	} else if ( is_soft_close ) {
		if ( soft_close_init ) {
			soft_close_init = false;
			trade_case = TradeCaseFactory::create(*this, TradeCase::SOFT_CLOSE);
			TRACE<<LOG_TRADE_INSTANCE<<"Initiating soft close";
		}
		return true;
	}

	if ( nullptr == current_acc || nullptr == opposite_acc ) return false;


	if ( !trade_case ) {	//if no trade case is selected and current_acc and opposite_acc isn't nullptr
		caseSelection();
		return false;
	}


	return false;	//no special cases to execute
}

void TradeInstance::selfDestruct() {

	auto long_live = long_acc.getTotalLiveOrderCount();
	auto short_live = short_acc.getTotalLiveOrderCount();
	if( short_live > trade_account.getMaximumLiveLimit() ||
		long_live > trade_account.getMaximumLiveLimit() ){
		TRACE<<LOG_TRADE_INSTANCE<<"In self destruct mode: "<<short_live<<","<<long_live;
		enablePanicClose();
	}
}



void TradeInstance::triggerSelection() {

	switch( avicenna_trigger_state ) {
	case TRIGGER_STATE_INIT:
		if ( bid_price > price_estimate ) avicenna_trigger_state = TRIGGER_STATE_POSITIVE;
		else if ( ask_price < price_estimate ) avicenna_trigger_state = TRIGGER_STATE_NEGATIVE;
		break;
	case TRIGGER_STATE_POSITIVE:
		if ( ask_price <= price_estimate ) avicenna_trigger_state = TRIGGER_STATE_NEGATIVE;
		break;
	case TRIGGER_STATE_NEGATIVE:
		if ( bid_price >= price_estimate ) avicenna_trigger_state = TRIGGER_STATE_POSITIVE;
		break;

	}

	if ( price_estimate > price_estimate_prev ) {
		trigger_state = TRIGGER_STATE_POSITIVE;
	} else if ( price_estimate < price_estimate_prev ) {
		trigger_state = TRIGGER_STATE_NEGATIVE;
	}

	price_estimate_prev = price_estimate;	//setting current price estimate to previous price estimate

	if ( trigger_state == TRIGGER_STATE_POSITIVE ) trade_price = ask_price;
	else if ( trigger_state == TRIGGER_STATE_NEGATIVE ) trade_price = bid_price;
	else trade_price = 0.0;


	if ( prev_trigger_state != trigger_state && trigger_state != TRIGGER_STATE_INIT ) {	//only execute if trigger changes from last state


		prev_trigger_state = trigger_state;	//remember last state
		state = TRADE_INSTANCE_INIT;	//start of trade logic
		price_estimate_on_trigger = price_estimate;
		previous_case = TradeCase::NONE;

		if ( trigger_state == TRIGGER_STATE_POSITIVE ) {
			current_acc = &long_acc;
			opposite_acc = &short_acc;
			reference_price = bid_price;
		} else {
			current_acc = &short_acc;
			opposite_acc = &long_acc;
			reference_price = ask_price;
		}

		INFO<<LOG_TRADE_INSTANCE<<" TRIGGER CHANGED ";
		//select cases here
		//pair#symbol //list timing,state,curr_case
		caseSelection();


	}
}

void TradeInstance::caseSelection() {

	if ( nullptr == current_acc || nullptr == opposite_acc ) return;

	auto curr_pos = current_acc->getPosition();
	auto oppo_pos = opposite_acc->getPosition();

	TRACE<<LOG_TRADE_INSTANCE<<"case selection --->";

	auto curr_case = TradeCase::NONE;


//	if ( current_acc->hedgedModeCondition(bid_price, ask_price) ||
//			opposite_acc->hedgedModeCondition(bid_price, ask_price) )
//	{
//		TRACE<<LOG_TRADE_INSTANCE<<"selecting case hedgedMode";
//		curr_case = TradeCase::CASE_HEDGEDMODE;
//	}
//	else
//	if ( current_acc->getTranchCounter() == tranch_value ||
//			opposite_acc->getTranchCounter() == tranch_value )
//	{
//		TRACE<<LOG_TRADE_INSTANCE<<"selecting case max_tlv";
//		curr_case = TradeCase::CASE_MAXTLV;
//	}
//	else
	if ( 0 == oppo_pos && 0 == curr_pos )	//case 4
	{
		TRACE<<LOG_TRADE_INSTANCE<<"selecting case 4 ";
		curr_case = TradeCase::CASE_4;
	}
	else if ( curr_pos > 0 && oppo_pos == 0 )	//case 9
	{
		TRACE<<LOG_TRADE_INSTANCE<<"selecting case 9";
		curr_case = TradeCase::CASE_9;
	}
	else if ( curr_pos == oppo_pos &&
			wait_for_next_time_window &&
			wait_for_next_time_window_avicenna != price_estimate )
	{
		wait_for_next_time_window = false;
		TRACE<<LOG_TRADE_INSTANCE<<"selecting case 10";
		curr_case = TradeCase::CASE_10;
	}
//	else if ( curr_pos < oppo_pos &&
//			previous_case != TradeCase::CASE_CLOSEOPPOSITE )
//	{
//		TRACE<<LOG_TRADE_INSTANCE<<"selecting case closeopposite";
//		curr_case = TradeCase::CASE_CLOSEOPPOSITE;
//	}
//	else if ( curr_pos < oppo_pos &&
//			curr_pos > 0 &&
//			previous_case != TradeCase::CASE_CLOSEOPPOSITEWITH_ANTICIPATED_PNL )	//special case try to close any position that current account has in profit
//	{
//		TRACE<<LOG_TRADE_INSTANCE<<"selecting case closeopposite_with_anticipated_pnl";
//		curr_case = TradeCase::CASE_CLOSEOPPOSITEWITH_ANTICIPATED_PNL;
//	}
//	else if ( curr_pos >  oppo_pos )
//	{
//		TRACE<<LOG_TRADE_INSTANCE<<"selecting case closetrigger";
//		curr_case = TradeCase::CASE_CLOSE_TRIGGER;
//	}
//	else if ( curr_pos > oppo_pos &&	//if current greater than opposite
//			oppo_pos > 0 &&
//			previous_case != TradeCase::CASE_7 )					//and closeopposite has failed to close any position
//	{
//		TRACE<<LOG_TRADE_INSTANCE<<"selecting case 7";
//		curr_case = TradeCase::CASE_7;
//	}
	else if ( curr_pos < oppo_pos )
	{
		TRACE<<LOG_TRADE_INSTANCE<<"selecting case hedgecurrent";
		curr_case = TradeCase::CASE_HEDGECURRENT;
	}
//	else if ( curr_pos == oppo_pos /* && previous_case == TradeCase::CASE_HEDGECURRENT */)
//	{
//		TRACE<<LOG_TRADE_INSTANCE<<"selecting case OnEquality";
//		curr_case = TradeCase::CASE_ONEQUALITY;
//	}

	trade_case = TradeCaseFactory::create(*this, curr_case);
}

