/*
 * STEMMaddogConnection.h
 *
 *  Created on: Apr 26, 2016
 *      Author: jamil
 */

#ifndef STEMMADDOGCONNECTION_H_
#define STEMMADDOGCONNECTION_H_

#include <edge.h>
#include <Maddog/Connection.h>
#include <linkx/Client.h>
#include <utils/eutils.h>

#include <boost/signals2.hpp>

#include "../utils/log.h"

class SymbolManager;

class STEMMaddogConnection: public Maddog::Connection {
public:
	using TradeSlotType = void (const std::string &, double);
	using QuoteSlotType = void (const std::string &, uint32_t, double, uint32_t, double);

	boost::signals2::signal<TradeSlotType> trade_signal;
	boost::signals2::signal<QuoteSlotType> quote_signal;

	STEMMaddogConnection(Linkx::IClient &);
	virtual ~STEMMaddogConnection();



	void connect(TradeSlotType,
			QuoteSlotType);
	void Subscribe(std::string sym);
	void Subscribe(std::vector<std::string> &symList);
	void Unsubscribe();
	void Unsubscribe(std::string);

protected:
	void Process(const void *pUpdate, const size_t updateLength);
	virtual void OnTrade(const Linkx::Address &source, const unsigned serviceId, const void *pSubject, const size_t subjectLength,
						 const MarketData::Trade &trade);
	virtual void OnTradeList(const Linkx::Address &source, const unsigned serviceId, const void *pSubject, const size_t subjectLength,
							 const MarketData::TradeList &tradeList);
	virtual void OnQuote(const Linkx::Address &source, const unsigned serviceId, const void *pSubject, const size_t subjectLength,
						 const MarketData::Quote &quote);
	virtual void OnQuoteDepth(const Linkx::Address &source, const unsigned serviceId, const void *pSubject, const size_t subjectLength,
							  const MarketData::Depth &depth);
	virtual void OnQuoteDepthList(const Linkx::Address &source, const unsigned serviceId, const void *pSubject, const size_t subjectLength,
								  const MarketData::DepthList &depthList);
	virtual void OnRefresh(const Linkx::Address &source, const unsigned serviceId, const void *pSubject, const size_t subjectLength,
						   const MarketData::Refresh &refresh);
	virtual void OnAlert(const Linkx::Address &source, const unsigned serviceId, const void *pSubject, const size_t subjectLength,
						 const MarketData::Alert &alert);
	virtual void OnOrder(const Linkx::Address &source, const unsigned serviceId, const void *pSubject, const size_t subjectLength,
						 const MarketData::Order &order);
	virtual void OnOrderList(const Linkx::Address &source, const unsigned serviceId, const void *pSubject, const size_t subjectLength,
							 const MarketData::OrderList &orderList);
	virtual void OnCancel(const Linkx::Address &source, const unsigned serviceId, const void *pSubject, const size_t subjectLength,
						  const MarketData::Cancel &cancel);
	virtual void OnExecution(const Linkx::Address &source, const unsigned serviceId, const void *pSubject, const size_t subjectLength,
							 const MarketData::Execution &execution);
	virtual void OnSettlement(const Linkx::Address &source, const unsigned serviceId, const void *pSubject, const size_t subjectLength,
							  const MarketData::Settlement &settlement);

private:
	STEMMaddogConnection(const STEMMaddogConnection&);
	STEMMaddogConnection &operator=(const STEMMaddogConnection&);

	bool  m_bDump, m_bTrades, m_bQuotes, m_bDepth, m_bOrderBook;

	std::list<MarketData::Ticker>  tickerList;

	std::unique_ptr<SymbolManager> sm;
};

#endif /* STEMMADDOGCONNECTION_H_ */
