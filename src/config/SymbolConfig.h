/*
 * SymbolConfig.h
 *
 *  Created on: Apr 18, 2016
 *      Author: jamil
 */

#ifndef SYMBOLCONFIG_H_
#define SYMBOLCONFIG_H_



#include "ConfigReader.h"
//#include "../trade/OrderAccount.h"



class OrderInfo;

#define LOG_SYMBOL_CONFIG " <"<<name<<"> "

struct TrenchRound {
	uint32_t base_qty;
	uint32_t sets;
	std::vector<uint32_t> rounds;
};

struct ShareRound {
	size_t increments;
	size_t position;
};

using RoundVector = std::vector<ShareRound>;

/*
 * @brief copy state of Original Symbol structure for trigger processing
 *
 */
struct SymbolStamp {

	double price_estimate1;
	double price_estimate2;

	//instrument skew will also be updated by every time we have change in bidsize and asksize
	double instrument_skew;
	size_t bid_size;
	double bid_price;
	size_t ask_size;
	double ask_price;

};

struct Symbol {
	string name;
	string exchange;
	double multiplier;
	uint32_t divisor;
	bool cancel_on_trigger;
	bool mmf;
	size_t max_pos;
	RoundVector share_rounds;
	uint32_t base_qty;

	uint32_t tranch_value;

	//these will change for interval define using timer in trade account
	bool start_interval; 		//this will tick start of interval
	double open_price;
	double min_price;
	double max_price;
	double close_price;
	double open_price_prev;
	double min_price_prev;
	double max_price_prev;
	double close_price_prev;
	double price_estimate1;
	double price_estimate2;

	//instrument skew will also be updated by every time we have change in bidsize and asksize
	double instrument_skew;
	size_t bid_size;
	double bid_price;
	size_t ask_size;
	double ask_price;

	struct {
		double value;
		double limit;
		uint32_t multiplier;
	} mpv;

	std::mutex rw_mutex;


	typedef std::function<double (double, double, double, double)> reset_interval_callback_type;


	std::vector<SymbolStamp> symbol_stack;
	std::mutex stack_mutex;
	std::condition_variable stack_size_condition;

	void push() {

		SymbolStamp stamp;
		stamp.ask_price = ask_price;
		stamp.ask_size = ask_size;
		stamp.bid_price = bid_price;
		stamp.bid_size = bid_size;
		stamp.instrument_skew = instrument_skew;
		stamp.price_estimate1 = price_estimate1;
		stamp.price_estimate2 = price_estimate2;

		{
			std::lock_guard<std::mutex> lock(stack_mutex);
			symbol_stack.push_back(stamp);
		}

		stack_size_condition.notify_one();
	}

	SymbolStamp pop(){
		std::unique_lock<std::mutex> lock(stack_mutex);
		stack_size_condition.wait(lock, [this]()->bool{
//			TRACE<<"stack size: "<<this->symbol_stack.size();
			return this->symbol_stack.size() > 0;
		});

		auto temp = symbol_stack.back();	//pull last element
		symbol_stack.clear();	//delete all others
		return temp;
	}

	size_t stack_size() {
		return symbol_stack.size();
	}

	Symbol(ptree &pt):
		start_interval(true),
		open_price(-1.00),
		min_price(-1.00),
		max_price(-1.00),
		close_price(-1.00),
		price_estimate1(-1.00),
		price_estimate2(-1.00),
		instrument_skew(0),
		bid_size(0),
		bid_price(0),
		ask_size(0),
		ask_price(0),
		max_pos(0),
		mpv{0.0, 0.0, 1},
		mmf(false),
		cancel_on_trigger(false),
		base_qty(0),
		tranch_value(0) {

		ptree& attr = pt.get_child("<xmlattr>");
		name = attr.get_child("name").get_value<string>();
		exchange = attr.get_child("exchange").get_value<string>();
		if ( "" == name ) BOOST_THROW_EXCEPTION(ConfigException()<<config_info("Symbol definition must include name attribute"));

		multiplier = attr.get_child("multiplier").get_value<double>();
		divisor = attr.get_child("divisor").get_value<int>();
		max_pos = attr.get_child("max").get_value<int>();

		cancel_on_trigger = attr.get_child("cancel_on_trigger").get_value<bool>();
		TRACE<<"cancel on trigger "<<cancel_on_trigger;
		mmf = attr.get_child("mmf").get_value<bool>();
		TRACE<<"mmf "<<mmf;
		base_qty = attr.get_child("base_qty").get_value<int>();
		TRACE<<"base_qty "<<base_qty;
		tranch_value = attr.get_child("tranch_value").get_value<int>();
		TRACE<<"tranch_value "<<base_qty;

		for (auto itr = pt.begin(); itr != pt.end(); itr++ ) {
			if ( itr->first == "<xmlattr>" ) continue;
			else if ( itr->first == "mpv" ) {
				auto& attr = itr->second.get_child("<xmlattr>");
				this->mpv.value = attr.get_child("value").get_value<double>();
				this->mpv.limit = attr.get_child("limit").get_value<double>();
				this->mpv.multiplier = attr.get_child("multiplier").get_value<double>();
				TRACE<<"mpv "<<this->mpv.value<<" "<<this->mpv.limit<<" "<<this->multiplier;
			} else if ( itr->first == "share_quantity" ) {
				ShareRound temp;
				temp.position = itr->second.get_child("<xmlattr>").get_child("value").get_value<int>();
				temp.increments = itr->second.get_child("<xmlattr>").get_child("increments").get_value<int>();
				share_rounds.push_back(temp);
			}
//			else WARN<<"Unknown tag found for symbol "<<name;
		}

	}


	/*
	 * @brief update_price will be called whenever new price is arrived from market
	 * will update latest,min,max and earliest price for symbol during interval
	 *
	 * @param new price from trade list function
	 *
	 * @return doesn't return anything
	 */
	void updatePrice (double price) {
		TRACE<<LOG_SYMBOL_CONFIG<<"updatePrice "<<price;
		std::lock_guard<std::mutex> lock(rw_mutex);
		if ( start_interval ) {
			start_interval = false;
			close_price_prev = close_price;
			open_price_prev = open_price;
			max_price_prev = max_price;
			min_price_prev = min_price;
			close_price = open_price = max_price = min_price = price;
		} else {
			if ( price > max_price ) max_price = price;
			if ( price < min_price ) min_price = price;
			close_price = price;
		}
	}

	/*
	 * @brief update instrument skew when bidsize and asksize is changed in Market Quote Message
	 *
	 * @param value of new instrument skew to replace the old value
	 *
	 * @return doesn't return anything
	 */
	void updateQuoteData(
			double bid_size,
			double bid_price,
			double ask_size,
			double ask_price) {

		std::lock_guard<std::mutex> lock(rw_mutex);

		 if ((this->bid_size != bid_size ||
					this->ask_size != ask_size ||
					this->bid_price != bid_price ||
					this->ask_price != ask_price)  && bid_price > 0.001 && ask_price > 0.001) {
			this->bid_size = bid_size;
			this->bid_price = bid_price;
			this->ask_size = ask_size;
			this->ask_price = ask_price;

			this->instrument_skew = ( static_cast<double>(bid_size) - static_cast<double>(ask_size) )
					/( static_cast<double>(bid_size) + static_cast<double>(ask_size) );

			TRACE<<LOG_SYMBOL_CONFIG<<"("<<bid_size<<","<<bid_price<<","<<ask_size<<","<<ask_price<<","<<instrument_skew<<")";
			push();
		 }
	}

	/*
	 * @brief update instrument skew when bidsize and asksize is changed in Market Quote Message
	 *
	 * @param value of new instrument skew to replace the old value
	 *
	 * @return doesn't return anything
	 */
	void updateQuoteData(double is,
			double bid_size,
			double bid_price,
			double ask_size,
			double ask_price) {

		std::lock_guard<std::mutex> lock(rw_mutex);

		//only if any of the four change
		if ( (this->bid_size != bid_size ||
				this->ask_size != ask_size ||
				this->bid_price != bid_price ||
				this->ask_price != ask_price)  && bid_price > 0.001 && ask_price > 0.001) {
			this->bid_size = bid_size;
			this->bid_price = bid_price;
			this->ask_size = ask_size;
			this->ask_price = ask_price;
			this->instrument_skew = is;

			DEBUG<<LOG_SYMBOL_CONFIG<<"("<<bid_size<<","<<bid_price<<","<<ask_size<<","<<ask_price<<","<<instrument_skew<<")";
			push();
		}
	}

	/*
	 * @brief update instrument skew. this will only be called if instrument skew is changed from previous value
	 *
	 * @param latest instrument skew value calculated
	 */
	void updateInstrumentSkew(double is) {
		std::lock_guard<std::mutex> lock(rw_mutex);
		instrument_skew = is;
	}


	/*
	 * @brief As prices are updating in intervals we need to reset that every now and then. Since timer
	 * is defined in avicenna we are letting avicenna decide when to reset the interval.
	 *
	 * @param reference to reset_interval_callback_type callback function that will be provided with
	 * latest_price, min_price, max_price and earliest_price of a symbol
	 *
	 * @return function doesn't return anything
	 */
	void resetIntervalCallback(reset_interval_callback_type &new_estimate_for_the_interval, int index) {
		std::lock_guard<std::mutex> lock(rw_mutex);
		auto temp = new_estimate_for_the_interval(close_price, min_price, max_price, open_price);

		if ( temp > 0.0 ) {
			if ( 0 == index && temp != price_estimate1) {
//				TRACE<<LOG_SYMBOL_CONFIG<<"avicenna price estimate 1 "<<temp;
				price_estimate1 = temp;
				push();
			}
			else if ( temp != price_estimate2 ) {
//				TRACE<<LOG_SYMBOL_CONFIG<<"avicenna price estimate 2 "<<temp;
				price_estimate2 = temp;
				push();
			}
		}
		start_interval = true;


	}

};


class SymbolMap: public unordered_map<string, unique_ptr<Symbol>> {
public:
	SymbolMap(ptree &pt) {
		for (auto itr = pt.begin(); itr != pt.end(); itr++ ) {
			if ( "sym" == itr->first ) {
				std::unique_ptr<Symbol> sym(new Symbol(itr->second));
				if ( nullptr != sym ) emplace(sym->name, std::move(sym));
				else BOOST_THROW_EXCEPTION(ConfigException()<<config_info("memory allocation failed"));
			}
		}
	}

	Symbol& operator[](string name) {
		return *unordered_map<string, unique_ptr<Symbol>>::operator[](name);
	}
};

class UserSymbolsMap: public map<string, unique_ptr<SymbolMap>> {
public:
	UserSymbolsMap(ptree &pt) {
		for (auto itr = pt.begin(); itr != pt.end(); itr++ ) {
			if ( "user" == itr->first ) {
				auto user = itr->second;
				ptree &attr = user.get_child("<xmlattr>");
				string id = attr.get_child("id").get_value<string>();
				std::unique_ptr<SymbolMap> symbolMap(new SymbolMap(user));
				if ( nullptr != symbolMap ) emplace(id, std::move(symbolMap));
				else BOOST_THROW_EXCEPTION(ConfigException()<<config_info("memory allocation failed"));
			}
		}
	}

	SymbolMap& operator[](string id) {
		return *(map<string, unique_ptr<SymbolMap>>::operator[](id));
	}

};

class SymbolConfig {
public:
	static UserSymbolsMap *user_symbols_map;

	static void parse(string filename) {

		static std::once_flag only_once;
		std::call_once(only_once, [filename](){
			ptree pt;
			boost::property_tree::xml_parser::read_xml(filename, pt);

			int count = pt.count("symbols");
			if ( 0 == count  ) BOOST_THROW_EXCEPTION(ConfigException()<<config_info("Missing <symbols> tag"));
			else if ( 1 < count ) BOOST_THROW_EXCEPTION(ConfigException()<<config_info("More than one definition of <symbols>"));

			ptree &symbols = pt.get_child("symbols");
			user_symbols_map = new UserSymbolsMap(symbols);
			if ( nullptr == user_symbols_map )  BOOST_THROW_EXCEPTION(ConfigException()<<config_info("failed to allocate memory for user symbol table"));
		});

		DEBUG<<"Size of user_symbol_map: "<<user_symbols_map->size();

	}

	virtual ~SymbolConfig(){

	}

private:
	SymbolConfig() {}
};

#endif /* SYMBOLCONFIG_H_ */
