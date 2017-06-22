/*
 * TradeAccountManager.cpp
 *
 *  Created on: Jul 3, 2016
 *      Author: jamil
 */

#include "../manager/TradeAccountManager.h"

#include "../config/TradingConfig.h"

//Linkx::Client* TradeInstance::client = nullptr;

bool TradeAccount::all_enable = true;

TradeAccountManager::TradeAccountManager(Linkx::Client &client) {
//	TradeInstance::client = &client;
	auto& trade_accounts_map = *TradingConfig::trade_accounts_map;


	for (auto usrptr = trade_accounts_map.begin();
			usrptr != trade_accounts_map.end();
			usrptr++ ) {
		auto id = usrptr->first;
		auto& account = *usrptr->second;

		account.avicenna_interface = std::unique_ptr<AvicennaInterface>(new AvicennaInterface(&account));
		account.short_account = std::unique_ptr<OrderAccount>(new OrderAccount(client, account.short_acc, ORDER_ACCOUNT_SHORT, account.venues));
		account.long_account = std::unique_ptr<OrderAccount>(new OrderAccount(client, account.long_acc, ORDER_ACCOUNT_LONG, account.venues));

		auto& symbol_map = (*SymbolConfig::user_symbols_map)[account.id];
		for (auto symbol_itr = symbol_map.begin(); symbol_itr != symbol_map.end(); symbol_itr++) {
//			symbol_itr->second->trade_trigger.connect(boost::bind(&TradeInstance::triggerProcessing, this, _1));
//			symbol_itr->second->processing_thread = new std::thread(&TradeInstance::triggerProcessing, this, symbol_itr->second.get());
			auto trade_instance = std::unique_ptr<TradeInstance>(new TradeInstance(account, *symbol_itr->second));
			account.trade_instances.emplace(symbol_itr->first, std::move(trade_instance));
		}




	}

	CLI::registerCommand("st", this);	//stop trading
	CLI::registerCommand("rt", this);	//restart trading
	CLI::registerCommand("pc", this);	//panic close
	CLI::registerCommand("dpc", this);	//disable panic close
	CLI::registerCommand("sc", this);	//soft close
	CLI::registerCommand("dsc", this); //disable soft close
	CLI::registerCommand("av", this);

}

void TradeAccountManager::operator ()(stringlist args) {

	auto size = args.size();

	if ( size < 2 ) {
		DEBUG<<"command arguments missing";
		return;
	}

	if ( args[0] == "st" ) {
		if ( args[1] == "" ) {
			TRACE<<"unrecognized command arguments";
		} else if ( args[1] == "-a" ) { //all symbol must stop
			TradeAccount::all_enable = false;
			stop();
		} else { //second argument must be name of the symbol
			if ( size == 3 )
				stop(args[1], args[2]);
			else stop(args[1]);
		}
	} else if ( args[0] == "rt" ) {

		if ( args[1] == "" ) {
			TRACE<<"unrecognized command arguments";
		} else if ( args[1] == "-a" ) { //all symbol must stop
			TradeAccount::all_enable = true;
			start();
		} else { //second argument must be name of the symbol
			if ( size == 3 )
				start(args[1], args[2]);
			else start(args[1]);
		}
	}  else if ( args[0] == "pc" ) { //panic close
		if ( args[1] == "" ) {
			TRACE<<"unrecognized command arguments";
		} else if ( args[1] == "-d" ) {	//disable soft close
			if ( args[2] == "-a" ) { //disable soft close for all symbols
				panicClose("", "", false);
			} else { //second argument must be name of the symbol
				if ( size == 4 )
					panicClose(args[2], args[3], false);
				else panicClose(args[2],"", false);
			}

		} else if ( args[1] == "-a" ) { //all symbol must stop

			panicClose();
		} else { //second argument must be name of the symbol
			if ( size == 3 )
				panicClose(args[1], args[2]);
			else panicClose(args[1]);
		}
	}   else if ( args[0] == "sc" ) {//soft close
		if ( args[1] == "" ) {
			TRACE<<"unrecognized command arguments";
		} else if ( args[1] == "-d" ) {	//disable soft close
			if ( args[2] == "-a" ) { //disable soft close for all symbols
				softClose("", "", false);
			} else { //second argument must be name of the symbol
				if ( size == 4 )
					softClose(args[2], args[3], false);
				else softClose(args[2],"", false);
			}

		} else if ( args[1] == "-a" ) { //all symbol must stop
			softClose();
		} else { //second argument must be name of the symbol
			if ( size == 3 )
				softClose(args[1], args[2]);
			else softClose(args[1]);
		}
	} else if ( args[0] == "av") {	//avicenna toggle command
		if ( args[1] =="-a" ) {	//all accounts avicenna estimate should be toggle
			avicennaToggle();
		} else {
			avicennaToggle(args[1]);
		}

	}


}

void TradeAccountManager::avicennaToggle(const std::string &account_id) {
	auto& trade_accounts_map = *TradingConfig::trade_accounts_map;

	if ( account_id == "" ) {
		TRACE<<"avicennaToggle: toggle all accounts";
		std::for_each(trade_accounts_map.begin(), trade_accounts_map.end(), [&account_id, this] (TradeAccountMap::value_type &value) {
			auto& id = value.first;
			auto& account = *value.second;
			account.avicennaToggle();
		});
	}
	else {
		auto account = trade_accounts_map.find(account_id);
		if ( account != trade_accounts_map.end() ) {
			auto& id = account->first;
			TRACE<<"avicennaToggle: toggling "<<id;
			auto &acc = *account->second;
			acc.avicennaToggle();
		} else {
			TRACE<<"avicennaToggle: account not found";
			return;
		}
	}
}

void TradeAccountManager::panicClose(const std::string &account_id, const std::string &symbol, const bool enable) {

	auto& trade_accounts_map = *TradingConfig::trade_accounts_map;

	for (auto usrptr = trade_accounts_map.begin();
			usrptr != trade_accounts_map.end();
			usrptr++ ) {
		auto& id = usrptr->first;
		auto& account = *usrptr->second;
		if ( account_id == "" || account_id == id ) {	//if there is no account_id all should be panicClose
			if ( enable )
				account.enablePanicClose(symbol);
			else account.disablePanicClose(symbol);
		}
	}

}

void TradeAccountManager::softClose(const std::string &account_id, const std::string &symbol, const bool enable) {

	auto& trade_accounts_map = *TradingConfig::trade_accounts_map;

	for (auto usrptr = trade_accounts_map.begin();
			usrptr != trade_accounts_map.end();
			usrptr++ ) {
		auto& id = usrptr->first;
		auto& account = *usrptr->second;
		if ( account_id == "" || account_id == id ) {	//if there is no account_id all should be panicClose
			if ( enable )
				account.enableSoftClose(symbol);
			else account.disableSoftClose(symbol);
		}
	}

}



void TradeAccountManager::stop() {
	auto& trade_accounts_map = *TradingConfig::trade_accounts_map;

	for (auto usrptr = trade_accounts_map.begin();
			usrptr != trade_accounts_map.end();
			usrptr++ ) {
		auto id = usrptr->first;
		auto& account = *usrptr->second;

		account.stop();
	}
}

void TradeAccountManager::start() {
	auto& trade_accounts_map = *TradingConfig::trade_accounts_map;

	for (auto usrptr = trade_accounts_map.begin();
			usrptr != trade_accounts_map.end();
			usrptr++ ) {
		auto id = usrptr->first;
		auto& account = *usrptr->second;

		account.start();
	}
}

void TradeAccountManager::start(std::string &account_id) {
	auto& trade_accounts_map = *TradingConfig::trade_accounts_map;
	if ( trade_accounts_map.find(account_id) != trade_accounts_map.end() ){
		trade_accounts_map[account_id]->start();
	} else {
		TRACE<<"unrecognized account id";
	}
}

void TradeAccountManager::stop(std::string &account_id) {
	auto& trade_accounts_map = *TradingConfig::trade_accounts_map;
	if ( trade_accounts_map.find(account_id) != trade_accounts_map.end() ){
		trade_accounts_map[account_id]->stop();
	} else {
		TRACE<<"unrecognized account id";
	}
}

void TradeAccountManager::start(std::string &account_id, std::string &symbol) {
	auto& trade_accounts_map = *TradingConfig::trade_accounts_map;
	if ( trade_accounts_map.find(account_id) != trade_accounts_map.end() ){
		trade_accounts_map[account_id]->start(symbol);
	} else {
		TRACE<<"unrecognized account id";
	}
}

void TradeAccountManager::stop(std::string &account_id, std::string &symbol) {
	auto& trade_accounts_map = *TradingConfig::trade_accounts_map;
	if ( trade_accounts_map.find(account_id) != trade_accounts_map.end() ){
		trade_accounts_map[account_id]->stop(symbol);
	} else {
		TRACE<<"unrecognized account id";
	}
}

void TradeAccountManager::dayEnd() {
	INFO<<"******************************End Handler Called****************************************";
	INFO<<"******************************End Handler Called****************************************";
	INFO<<"******************************End Handler Called****************************************";
}

void TradeAccountManager::generatePositionFile() {
	system("../scripts/redistofile.sh");

}

TradeAccountManager::~TradeAccountManager() {
	// TODO Auto-generated destructor stub
}





