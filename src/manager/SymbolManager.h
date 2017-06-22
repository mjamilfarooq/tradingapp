/*
 * SymbolManager.h
 *
 *  Created on: May 9, 2016
 *      Author: jamil
 */

#ifndef SYMBOLMANAGER_H_
#define SYMBOLMANAGER_H_

#include <unordered_map>
#include <vector>
#include <chrono>

#include "../akela/STEMMaddogConnection.h"
#include "../akela/STEMDirectConnection.h"
//#include <utils/ThreadManager.h>

#include "../config/SymbolConfig.h"

#include "../akela/STEMMaddogConnection.h"
#include "../akela/STEMDirectConnection.h"

#define LOG_SYMBOL_MANAGER " <"<<symbol<<"> "
#define LOG_SYMBOL_DATA " <"<<this->name<<">: "


struct SymbolInfo {

	std::string name;
	std::chrono::time_point<std::chrono::steady_clock> last_updated;	//start time for each symbol default value epoch
	std::vector<Symbol *> symbols;	//all the Symbol in UserSymbolsMap correspond to this symbol "name"
	static bool all_enable;
	bool this_enable;

	SymbolInfo(std::string symbol):
		name(symbol), this_enable(true) {
	}

	//this shall register corresponding symbol in user domain
	void registerSymbol(Symbol *sym) {
		symbols.push_back(sym);
	}

};


using SymbolHash = std::unordered_map<std::string, std::unique_ptr<SymbolInfo>>;
class STEMMaddogConnection;


class SymbolManager: public CommandCallback {
//	STEMMaddogConnection market;
	STEMDirectConnection market;
	SymbolHash symbol_hash;

	void updatePrice(const std::string &, double);
	void updateQuote(const std::string &, uint32_t, double, uint32_t, double);

public:
	SymbolManager(Linkx::Client &);
	~SymbolManager();
	void subscribeSymbols();
	void operator()(stringlist);


};

#endif /* SYMBOLMANAGER_H_ */
