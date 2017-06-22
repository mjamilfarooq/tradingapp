/*
 * AvicennaInterface.cpp
 *
 *  Created on: May 12, 2016
 *      Author: jamil
 */

#include "../avicenna/AvicennaInterface.h"

#include "../config/TradingConfig.h"


AvicennaConnection::AvicennaConnection(const std::string account_id,
			const std::string symbol,
			const AvicennaConfiguration &config):
				account_id(account_id),
				symbol(symbol),
				price_estimate(-1.00),
				price_estimate_3(-1.00),
				startup(true),
				ms(0) {

			SetIP(const_cast<char *>(config.dll_ip.c_str()));
			SetVariableConfig(
					const_cast<char *>(config.dll_str1.c_str()),
					const_cast<char *>(config.dll_str3.c_str()),
					const_cast<char *>(config.dll_str4.c_str()),
					const_cast<char *>(config.dll_str5.c_str())
			);

	}

AvicennaConnection::AvicennaConnection(
			const std::string account_id,
			const std::string symbol,
			const std::string ip,
			const std::string str1,
			const std::string str2,
			const std::string str3,
			const std::string str4,
			const std::string str5):
			account_id(account_id),
			symbol(symbol),
		price_estimate(-1.00),
		price_estimate_3(-1.00),
		startup(true),
		ms(0) {

		SetIP(const_cast<char *>(ip.c_str()));
		SetVariableConfig(
				const_cast<char *>(str1.c_str()),
				const_cast<char *>(str3.c_str()),
				const_cast<char *>(str4.c_str()),
				const_cast<char *>(str5.c_str())
		);

	}

void AvicennaConnection::calculate(double price, double min, double max ) {
//		DEBUG<<LOG_AVICENNA_CONNECTION<<std::showpoint<<"Call to CalculateAvicennaSuperPlus("<<price<<")";
	CalculateAvicennaSuperPlus(price);
	auto export_value = GetExportValue4();
//		DEBUG<<LOG_AVICENNA	_CONNECTION<<export_value;
//	auto current_estimate = Utils::Price(export_value, MarketData::PT_DECIMAL_3).AsDouble();
	auto current_estimate = truncate(export_value, 4);
//	auto current_estimate_3 = Utils::Price(export_value, MarketData::PT_DECIMAL_3).AsDouble();

	price_estimate = current_estimate;

//	TRACE<<LOG_AVICENNA_CONNECTION<<"current estimate = "<<current_estimate;
//	if ( (current_estimate <= max && current_estimate >= min) || price_estimate < 0 ) {
//		TRACE<<LOG_AVICENNA_CONNECTION<<"accepting current estimate";
//		price_estimate = current_estimate;
//	}
//
//
//	if ( current_estimate_3 > price_estimate_3 ) {
//		TRACE<<LOG_AVICENNA_CONNECTION<<"MS +ive";
//		ms = +1;
//	} else if ( current_estimate_3 < price_estimate_3 ) {
//		TRACE<<LOG_AVICENNA_CONNECTION<<"MS -ive";
//		ms = -1;
//	} else {
//		TRACE<<LOG_AVICENNA_CONNECTION<<"MS keep same";
//	}
//
//	price_estimate_3 = current_estimate_3;
////		DEBUG<<LOG_AVICENNA_CONNECTION<<std::showpoint<<"AP: "<<price_estimate;
}



AvicennaInterface::AvicennaInterface(const TradeAccount* trade_account):
	stop_timeout_thread(false),
	trade_account(trade_account),
	price_estimate(-1.00) {

	if ( nullptr == trade_account) {
		WARN<<"Avicenna initialization failure";
		return;
	}

	account_id = trade_account->id;

	if ( SymbolConfig::user_symbols_map->find(account_id) == SymbolConfig::user_symbols_map->end() ) {
		FATAL<<"exiting AvicennaInterface::timeoutThread for "<<account_id<<" no symbol map found";
		return;
	}

	auto symbols_map = SymbolConfig::user_symbols_map->find(account_id)->second.get();

	auto timer1 = trade_account->avicenna_configuration[0].dll_timer1;
	auto timer2 = trade_account->avicenna_configuration[1].dll_timer1;


	for (auto itr = symbols_map->begin(); itr != symbols_map->end(); itr++) {
		//initialize AvicennaConnection for each symbol
		auto symbol = itr->first;

		AvicennaConnectionPair *pair = nullptr;

		pair = new AvicennaConnectionPair{
			new AvicennaConnection(
					trade_account->id,
					symbol,
					trade_account->avicenna_configuration[0]
			),
			new AvicennaConnection(
					trade_account->id,
					symbol,
					trade_account->avicenna_configuration[1]
			)
		};

		avicenna_list.emplace(symbol, pair);

	}


	timeout_thread1 = new std::thread(&AvicennaInterface::timeoutThread, this, timer1, 0);
	timeout_thread2 = new std::thread(&AvicennaInterface::timeoutThread, this, timer2, 1);

}

void AvicennaInterface::timeoutThread(int timeout, int index) {

	auto symbols_map = SymbolConfig::user_symbols_map->find(account_id)->second.get();
	while ( !stop_timeout_thread ) {

		for (auto itr = symbols_map->begin(); itr != symbols_map->end(); itr++) {
			auto& symbol = *itr->second.get();

			auto& avicenna_connection = ( 0 == index ) ?
					*avicenna_list[symbol.name]->avicenna1:
					*avicenna_list[symbol.name]->avicenna2;

			Symbol::reset_interval_callback_type newEstimate = [&avicenna_connection, this, &symbol, index] (
					double lat, double min, double max, double ear)->double {
				if ( lat > 0 ) {
					auto invert = true;
					auto val = lat;
					while ( avicenna_connection.price_estimate <= 0.0 && avicenna_connection.startup ) {
						if ( invert ) {
							invert = false;
							val += 0.01;
						} else {
							invert = true;
							val -= 0.01;
						}
						avicenna_connection.calculate(val, min, max);
					}

					avicenna_connection.calculate(lat, min, max);
					avicenna_connection.startup = false;
					auto symbol = avicenna_connection.symbol;
//					TRACE<<LOG_AVICENNA_CONNECTION<<"AP["<<index<<"]("<<ear<<","<<min<<","<<max<<","<<lat<<") = "<<avicenna_connection.price_estimate;
					return avicenna_connection.price_estimate;
				}
				return 0.0;
			};
			symbol.resetIntervalCallback(newEstimate, index);

		}

		std::this_thread::sleep_for(std::chrono::seconds(timeout));
	}
}


AvicennaInterface::~AvicennaInterface() {

}
