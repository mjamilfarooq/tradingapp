/*
 * STEMDirectConnection.h
 *
 *  Created on: Sep 23, 2016
 *      Author: jamil
 */

#ifndef SRC_AKELA_STEMDIRECTCONNECTION_H_
#define SRC_AKELA_STEMDIRECTCONNECTION_H_

#undef TRACE
#undef DEBUG
#undef INFO
#undef WARN
#undef ERROR
#undef FATAL


#include <iostream>

#include <edge.h>
#include <Direct/DirectConnection.h>
#include <utils/eutils.h>
#include <boost/signals2.hpp>


#define TRACE BOOST_LOG_SEV(my_logger::get(), boost::log::trivial::trace)
#define DEBUG BOOST_LOG_SEV(my_logger::get(), boost::log::trivial::debug)
#define INFO  BOOST_LOG_SEV(my_logger::get(), boost::log::trivial::info)
#define WARN  BOOST_LOG_SEV(my_logger::get(), boost::log::trivial::warning)
#define ERROR BOOST_LOG_SEV(my_logger::get(), boost::log::trivial::error)
#define FATAL BOOST_LOG_SEV(my_logger::get(), boost::log::trivial::fatal)

//#include "../utils/log.h"

class STEMDirectConnection: public MarketDataFeed::DirectConnection, public MarketDataFeed::ICallback {
public:
	STEMDirectConnection();
	virtual ~STEMDirectConnection();

	using TradeSlotType = void (const std::string &, double);
	using QuoteSlotType = void (const std::string &, uint32_t, double, uint32_t, double);

	boost::signals2::signal<TradeSlotType> trade_signal;
	boost::signals2::signal<QuoteSlotType> quote_signal;

	/**
	 *	@brief	virtual callback called when a trade is received
	 *
	 *	@param	trade trade message
	 *	@param	reference user defined reference value used with a dispatcher
	 */
	void OnTrade(const MarketData::Trade &trade, uintptr_t reference) override final;

	/**
	 *	@brief	virtual callback called when a trade list is received
	 *
	 *	@param	tradeList trade list message
	 *	@param	reference user defined reference value used with a dispatcher
	 */
	void OnTradeList(const MarketData::TradeList &tradeList, uintptr_t reference) override final;

	/**
	 *	@brief	virtual callback called when a quote is received
	 *
	 *	@param	quote quote message
	 *	@param	reference user defined reference value used with a dispatcher
	 */
	void OnQuote(const MarketData::Quote &quote, uintptr_t reference) override final;

	/**
	 *	@brief	virtual callback called when a depth is received
	 *
	 *	@param	depth depth message
	 *	@param	reference user defined reference value used with a dispatcher
	 */
	void OnQuoteDepth(const MarketData::Depth &depth, uintptr_t reference) override final;

	/**
	 *	@brief	virtual callback called when a BBO quote is received
	 *
	 *	@param	quote BBO quote message
	 *	@param	reference user defined reference value used with a dispatcher
	 */
    void OnBBOQuote(const MarketData::BBOQuote &quote, uintptr_t reference) override final;

	/**
	 *	@brief	virtual callback called when a depth list is received
	 *
	 *	@param	depthList depth list message
	 *	@param	reference user defined reference value used with a dispatcher
	 */
	void OnQuoteDepthList(const MarketData::DepthList &depthList, uintptr_t reference) override final;

	/**
	 *	@brief	virtual callback called when a refresh is received
	 *
	 *	@param	refresh refresh message
	 *	@param	reference user defined reference value used with a dispatcher
	 */
	void OnRefresh(const MarketData::Refresh &refresh, uintptr_t reference) override final;

	/**
	 *	@brief	virtual callback called when an alert is received
	 *
	 *	@param	alert alert message
	 *	@param	reference user defined reference value used with a dispatcher
	 */
	void OnAlert(const MarketData::Alert &alert, uintptr_t reference) override final;

	/**
	 *	@brief	virtual callback called when an admin is received
	 *
	 *	@param	admin admin message
	 *	@param	reference user defined reference value used with a dispatcher
	 */
	void OnAdmin(const MarketData::Admin &admin, uintptr_t reference) override final;

	/**
	 *	@brief	virtual callback called when a heartbeat is received
	 *
	 *	@param	heartbeat heartbeat message
	 *	@param	reference user defined reference value used with a dispatcher
	 */
	void OnHeartbeat(const MarketData::Heartbeat &heartbeat, uintptr_t reference) override final;

	/**
	 *	@brief	virtual callback called when an order is received
	 *
	 *	@param	order order message
	 *	@param	reference user defined reference value used with a dispatcher
	 */
	void OnOrder(const MarketData::Order &order, uintptr_t reference) override final;

	/**
	 *	@brief	virtual callback called when an order list is received
	 *
	 *	@param	orderList order list message
	 *	@param	reference user defined reference value used with a dispatcher
	 */
	void OnOrderList(const MarketData::OrderList &orderList, uintptr_t reference) override final;

	/**
	 *	@brief	virtual callback called when an cancel is received
	 *
	 *	@param	cancel cancel message
	 *	@param	reference user defined reference value used with a dispatcher
	 */
	void OnCancel(const MarketData::Cancel &cancel, uintptr_t reference) override final;

	/**
	 *	@brief	virtual callback called when an execution is received
	 *
	 *	@param	execution execution message
	 *	@param	reference user defined reference value used with a dispatcher
	 */
	void OnExecution(const MarketData::Execution &execution, uintptr_t reference) override final;

	/**
	 *	@brief	virtual callback called when a settlement is received
	 *
	 *	@param	settlement settlement message
	 *	@param	reference user defined reference value used with a dispatcher
	 */
	void OnSettlement(const MarketData::Settlement &settlement, uintptr_t reference) override final;

	/**
	 *	@brief	virtual callback called when a data transfer is received
	 *
	 *	@param	transfer transfer message
	 *	@param	reference user defined reference value used with a dispatcher
	 */
	void OnDataTransfer(const MarketData::DataTransfer &transfer, uintptr_t reference) override final;


//	/**
//	 *	@brief	Set the name of this connection
//	 *
//	 *	@param	name connection name
//	 */
//	 virtual void SetName(const std::string &name) override;
//
//	/**
//	 *	@brief	Get the name of this connection
//	 *
//	 *	@param	name connection name
//	 */
//	virtual void GetName(std::string &name) const override;
//
//	/**
//	 *	@brief	Set the callback pointer
//	 *
//	 *	@param	pCallback callback pointer
//	 *	@param	reference reference value
//	 */
//	virtual void SetCallback(MarketDataFeed::ICallback *pCallback, uintptr_t reference = 0);
//
//	/**
//	 *	@brief	Set the callback pointer
//	 *
//	 *	@param	pCallback callback pointer
//	 */
//	virtual void SetCallback(Logger::ICallback *pCallback);
//
//	/**
//	 *	@brief	Set the status callback pointer
//	 *
//	 *	@param	pCallback callback pointer
//	 *	@param	reference reference value
//	 */
//	virtual void SetStatusCallback(MarketDataFeed::IStatusCallback *pCallback, uintptr_t reference = 0);
//
//	/**
//	 *	@brief	Set the pubsub manager
//	 *
//	 *	@param	pManager pubsub manager
//	 */
//	virtual void SetPubSubManager(Messaging::IPubSubManager *pManager);
//
//	/**
//	 *	@brief	Enable the output ring
//	 */
//	virtual void EnableOutputRing(void);
//
//	/**
//	 *	@brief	Disable the output ring
//	 */
//	virtual void DisableOutputRing(void);
//
//	/**
//	 *	@brief	Enable order book messages
//	 */
//	virtual bool EnableOrderBook(void);
//
//	/**
//	 *	@brief	Disable order book messages
//	 */
//	virtual bool DisableOrderBook(void);
//
//	/**
//	 *	@brief	Enable subscriptions... use subscription DB as a filter
//	 */
//	virtual void EnableSubscription(void);
//
//	/**
//	 *	@brief	Disable subscriptions... pass all data
//	 */
//	virtual void DisableSubscription(void);
//
//	/**
//	 *	@brief	Are subscriptions enabled?
//	 */
//	virtual bool IsEnabledSubscription(void) const;
//
//	/**
//	 *	@brief	Set sweep parameters
//	 *
//	 *	@param	ticker ticker for sweep parameters
//	 *	@param	params sweep params
//	 *
//	 *	@return	see eerror.h
//	 */
//	virtual unsigned SetSweepParameters(const MarketData::Ticker &ticker, const MarketData::SweepParameters &params);
//
//	/**
//	 *	@brief	Get sweep parameters
//	 *
//	 *	@param	ticker ticker for sweep parameters
//	 *	@param	params sweep params
//	 *
//	 *	@return	see eerror.h
//	 */
//	virtual unsigned GetSweepParameters(const MarketData::Ticker &ticker, MarketData::SweepParameters &params);
//
//	/**
//	 *	@brief	Is the connection open
//	 *
//	 *	@return	open state
//	 */
//	virtual bool IsOpen(void) const;
//
//	/**
//	 *	@brief	Open the connection
//	 *
//	 *	@param	keyName key name
//	 *	@param	keyPath key path name
//	 *
//	 *	@return	see eerror.h
//	 */
//	virtual unsigned Open(const std::string &keyName, const std::string &keyPath);
//
//	/**
//	 *	@brief	Close the connection
//	 *
//	 *	@return	see eerror.h
//	 */
//	virtual unsigned Close(void);
//
//	/**
//	 *	@brief	Find a ticker and return the record and subject (for dynamic updates)
//	 *
//	 *	@param	findType type of find to perform
//	 *	@param	findFlags various flags
//	 *	@param	ticker ticker to find
//	 *	@param	pSubject subject buffer
//	 *	@param	subjectLength subject length
//	 *	@param	pRecord subject buffer
//	 *	@param	recordLength record length
//	 *
//	 * 	@return see eerror.h
//	 */
//	virtual unsigned Find(const unsigned findType, const unsigned findFlags, MarketData::Ticker &ticker,
//												uint8_t *pSubject, size_t &subjectLength, uint8_t *pRecord, size_t &recordLength);
//
//	/**
//	 *	@brief	Get record bunch
//	 *
//	 *	@param	findType type of find to perform
//	 *	@param	findFlags various flags
//	 *	@param	ticker ticker to find
//	 *	@param	compareLength length of ticker to compare in search
//	 *	@param	pBuffer buffer
//	 *	@param	bufferLength buffer length
//	 *	@param	outputLength output length
//	 *
//	 * 	@return see eerror.h
//	 */
//	virtual unsigned GetBunch(const unsigned findType, unsigned &findFlags, MarketData::Ticker &ticker,
//													const size_t compareLength, uint8_t *pBuffer, const size_t bufferLength,
//													MarketData::Ticker &lastTicker, size_t &outputLength);
//
//	/**
//	 *	@brief	Get record bunch
//	 *
//	 *	@param	findFlags various flags
//	 *	@param	tickerList tickers to find
//	 *	@param	tickerCount count of tickers to find
//	 *	@param	pBuffer buffer
//	 *	@param	bufferLength buffer length
//	 *	@param	outputLength output length
//	 *
//	 * 	@return see eerror.h
//	 */
//	virtual unsigned GetList(unsigned &findFlags, const MarketData::TickerListEntry *pTickerList,
//												   const size_t tickerCount, uint8_t *pBuffer, const size_t bufferLength,
//												   size_t &outputLength);
//
//	/**
//	 *	@brief	Subscribe
//	 *
//	 *	@param	ticker ticker to subscribe
//	 *
//	 *	@return	see eerror.h
//	 */
//	 virtual unsigned Subscribe(const MarketData::Ticker &ticker);
//
//	/**
//	 *	@brief	Unsubscribe
//	 *
//	 *	@param	ticker ticker to unsubscribe
//	 *
//	 *	@return	see eerror.h
//	 */
//	 virtual unsigned Unsubscribe(const MarketData::Ticker &ticker);
//
//	/**
//	 *	@brief	Check subject for subscription
//	 *
//	 *	@param	ticker ticker to check
//	 *
//	 *	@return	see eerror.h
//	 */
//	 virtual bool CheckSubscription(const MarketData::Ticker &ticker);
//
//	/**
//	 *	@brief	Clear subscriptions
//	 *
//	 *	@return	see eerror.h
//	 */
//	 virtual unsigned ClearSubscriptions(void);
//
//	/**
//	 *	@brief	Clear Connection statistics
//	 *
//	 *	@param	index index for multiple lines
//	 *
//	 *	@return	see eerror.h
//	 */
//	 virtual unsigned ClearStats(const unsigned index);
//
//	/**
//	 *	@brief	Get Connection Statistics
//	 *
//	 *	@param	index index for multiple lines
//	 *	@param	stats statistics for a line
//	 *
//	 *	@return	see eerror.h
//	 */
//	 virtual unsigned GetStats(const unsigned index, Statistics &stats);
//
//	/**
//	 *	@brief	Clear Connection Output statistics
//	 *
//	 *	@return	see eerror.h
//	 */
//	 virtual unsigned ClearOutputStats(void);
//
//	/**
//	 *	@brief	Get Connection Output Statistics
//	 *
//	 *	@param	stats output statistics
//	 *
//	 *	@return	see eerror.h
//	 */
//	 virtual unsigned GetOutputStats(Statistics &stats);
//
//	/**
//	 *	@brief	Get Line Count
//	 *
//	 *	@param	count line count
//	 *
//	 *	@return	see eerror.h
//	 */
//	 virtual unsigned GetLineCount(size_t &count);
//
//	/**
//	 *	@brief	Get Line List
//	 *
//	 *	@param	pLineList line list array
//	 *	@param	listSize size of line list array
//	 *	@param	count count of lines placed in pLineList
//	 *
//	 *	@return	see eerror.h
//	 */
//	 virtual unsigned GetLineList(LineInfo *pLineList, const size_t listSize, size_t &count);
//
//	/**
//	 *	@brief	Process feed updates
//	 *
//	 *	@param	source source of the update message
//	 *	@param	serviceId service id of the update
//	 *	@param	type update type
//	 *	@param	pUpdate actual update message
//	 *	@param	updateLength length of the update
//	 *
//	 *	@return	see eerror.h
//	 */
//	 unsigned ProcessFeedUpdate(const Linkx::Address &source, const unsigned serviceId, const unsigned type,
//													 const void *pUpdate, const size_t updateLength);
//
//	 void CallStatusConnect(const std::string &name);
//	 void CallStatusDisconnect(const std::string &name);
//
	 void Subscribe(std::string);
	 void Subscribe(std::vector<std::string> &);

	 void Unsubscribe(std::string);
	 void Unsubscribe();

//protected:
//	/**
//	 *	@brief	Callback for subscriptions
//	 *
//	 *	@param	manager source of callback
//	 *	@param	id id of subject
//	 *	@param	pSubject subject
//	 *	@param	subjectLength subject length
//	 *	@param	reference reference id
//	 */
//	 virtual void OnSubscribe(Messaging::IPubSubManager &manager, const unsigned id, const char *pSubject,
//												   const size_t subjectLength, uintptr_t reference);
//
//	/**
//	 *	@brief	Callback for unsubscriptions
//	 *
//	 *	@param	manager source of callback
//	 *	@param	id id of subject
//	 *	@param	pSubject subject
//	 *	@param	subjectLength subject length
//	 *	@param	reference reference id
//	 */
//	 virtual void OnUnsubscribe(Messaging::IPubSubManager &manager, const unsigned id, const char *pSubject,
//													 const size_t subjectLength, uintptr_t reference);
//
//	virtual unsigned OnLogMessage(const unsigned type, const unsigned debugLevel, const char *pMessage, const size_t messageLength,
//								  const unsigned flags);
//
//	unsigned FindNewService(const std::string &serviceName);
//	unsigned SubscribeSubjects(Linkx::Receiver &receiver);
//
//	virtual unsigned OnOutput(const unsigned type, const void *pSubject, const size_t subjectLength, const unsigned format,
//							  const void *pData, const size_t dataLength);
//
//	unsigned ProcessOutput(const unsigned type, const void *pData, const size_t dataLength);
//	unsigned SendAlert(const MarketData::Ticker &ticker, const unsigned state);
//
private:
	std::list<MarketData::Ticker>  tickerList;

	bool m_bEnable, m_bTrades, m_bOrderBook, m_bRefresh, m_bDepth, m_bQuotes;
};

#endif /* SRC_AKELA_STEMDIRECTCONNECTION_H_ */
