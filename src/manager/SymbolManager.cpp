/*
 * SymbolManager.cpp
 *
 *  Created on: May 9, 2016
 *      Author: jamil
 */

#include "../manager/SymbolManager.h"

#include "../akela/STEMMaddogConnection.h"

bool SymbolInfo::all_enable = true;

SymbolManager::SymbolManager(Linkx::Client &client)/*:
	market(client)*/ {


	//register signal handler for tradelist and quote event
	market.trade_signal.connect(boost::bind(&SymbolManager::updatePrice, this, _1, _2));
	market.quote_signal.connect(boost::bind(&SymbolManager::updateQuote, this, _1, _2, _3, _4, _5));



	auto &user_symbol_map = *SymbolConfig::user_symbols_map;

	for (auto usrptr = user_symbol_map.begin();
			usrptr != user_symbol_map.end();
			usrptr++) {
		auto id = usrptr->first;
		auto &symbol_map = *usrptr->second.get();
		DEBUG<<"Loading symbols from: "<<id;

		for (auto symptr = symbol_map.begin();
				symptr != symbol_map.end();
				symptr++ ) {
			auto name = symptr->first;
			auto sym = symptr->second.get();


			if ( symbol_hash.find(name) == symbol_hash.end() ) { //if symbol doesn't already exist
				auto symbol_data = SymbolHash::mapped_type(new SymbolInfo(name));
				symbol_hash.emplace(name, std::move(symbol_data));
				market.Subscribe(name+"."+sym->exchange);	//subscribing symbol

			}

			symbol_hash.find(name)->second->registerSymbol(sym);

		}
	}

	CLI::registerCommand("ss", this);
	CLI::registerCommand("rs", this);
}


void SymbolManager::subscribeSymbols() {
//	for (auto itr = symbol_hash.begin(); itr != symbol_hash.end(); itr++ ) {
//		auto symbol = itr->first;
//		auto sym = itr->second.get();
//		DEBUG<<LOG_SYMBOL_MANAGER<<"subscribing symbol manager";
//		market.Subscribe(symbol+);
//	}
}

void SymbolManager::updatePrice(const std::string &symbol, double price) {

	auto& symbol_data = *symbol_hash[symbol];
	if ( !SymbolInfo::all_enable || !symbol_data.this_enable ) {
		return;
	}

	auto& symbols = symbol_data.symbols;
	for (auto itr = symbols.begin(); itr != symbols.end(); itr++ ) {
		auto& sym = **itr;
		sym.updatePrice(price);
	}
}

void SymbolManager::updateQuote(const std::string &symbol,
		uint32_t bid_size, double bid,
		uint32_t ask_size, double ask) {
	auto& symbol_data = *symbol_hash[symbol];
	if ( !SymbolInfo::all_enable || !symbol_data.this_enable ) {
		return;
	}

	auto& symbols = symbol_data.symbols;
	for (auto itr = symbols.begin(); itr != symbols.end(); itr++ ) {
		auto& sym = **itr;
		sym.updateQuoteData(bid_size, bid, ask_size, ask);
	}
}


void SymbolManager::operator()(stringlist args) {
	if ( args[0] == "ss" ) {
		if ( args[1] == "-a" ) { //all symbol must stop
			SymbolInfo::all_enable = false;
		} else { //second argument must be name of the symbol
			auto itr = symbol_hash.find(args[1]);
			if ( itr != symbol_hash.end() ) { //if symbol exist
				(*itr->second).this_enable = false;
			}
		}
	} else if ( args[0] == "rs" ) {
		if ( args[1] == "-a" ) { //restart all
			SymbolInfo::all_enable = true;
		} else { //second argument must be name of the symbol
			auto itr = symbol_hash.find(args[1]);
			if ( itr != symbol_hash.end() ) { //if symbol exist
				(*itr->second).this_enable = true;
			}
		}
	}

}

SymbolManager::~SymbolManager() {
	// TODO Auto-generated destructor stub
}



