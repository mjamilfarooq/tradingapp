/*
 * STEMMaddogConnection.cpp
 *
 *  Created on: Apr 26, 2016
 *      Author: jamil
 */

#include "../akela/STEMMaddogConnection.h"

#include "../manager/SymbolManager.h"

void STEMMaddogConnection::Subscribe(std::string sym){
	MarketData::Ticker ticker;
	if ( ticker.Parse(sym, MarketData::Ticker::TYPE_EQUITY) ) {
		int status = static_cast<Maddog::Connection *>(this)->Subscribe(ticker);
		if ( ECERR_SUCCESS != status ) {
			//log this event
			ERROR<<"can't subscribe to symbol ("<<sym<<")";
		} else {
			DEBUG<<"Subscribing to symbol ("<<sym<<")";
			this->tickerList.push_back(ticker);
		}
	}
}

void STEMMaddogConnection::Subscribe(std::vector<std::string> &symList) {
	std::for_each(symList.begin(), symList.end(), [this](std::string str){
		MarketData::Ticker ticker;
		if ( ticker.Parse(str, MarketData::Ticker::TYPE_EQUITY) ) {
			int status = static_cast<Maddog::Connection *>(this)->Subscribe(ticker);
			if ( ECERR_SUCCESS != status ) {
				//log this event
				ERROR<<"can't subscribe to symbol ("<<str<<")";
			} else {
				TRACE<<"Subscribing to symbol ("<<str<<")";
				this->tickerList.push_back(ticker);
			}
		}
	});
}

void STEMMaddogConnection::Unsubscribe(std::string symbol) {
	std::for_each(tickerList.begin(), tickerList.end(),[](MarketData::Ticker ticker){

	});
}

void STEMMaddogConnection::Unsubscribe() {
	std::for_each(tickerList.begin(), tickerList.end(), [this](MarketData::Ticker &ticker){
		static_cast<Maddog::Connection *>(this)->Unsubscribe(ticker);
	});
}

STEMMaddogConnection::STEMMaddogConnection(Linkx::IClient &client):
	Maddog::Connection(client),
	//member variables
	m_bDump(false),
	m_bTrades(true),
	m_bQuotes(true),
	m_bDepth(true),
	m_bOrderBook(true) {

	int status = this->Connect("Maddog");
	if (ECERR_SUCCESS != status) {
		FATAL<<"Maddog::Connect failed rc="<<status;
	} else INFO<<"Maddog::Connect successfull, rc="<<status;
}

STEMMaddogConnection::~STEMMaddogConnection() {

}

/*=========================================================================================*/

void STEMMaddogConnection::Process(const void *pUpdate, const size_t updateLength)
{
	if (m_bDump)
		::HexDump(pUpdate, updateLength);
}

/*=========================================================================================*/

void STEMMaddogConnection::OnTrade(const Linkx::Address &source, const unsigned serviceId, const void *pSubject, const size_t subjectLength,
								 const MarketData::Trade &message)
{
	if (m_bTrades) {
		::fprintf(stdout, "Trade,%d,%lf,%s,%08X,%d,%d,%d,%d\n", message.header.sequence, message.header.timeStamp,
				  message.ticker.ToString().c_str(), message.flags, message.priceType, message.price, message.size, message.volume);
		Process(&message, message.header.length);
	}
}

/*=========================================================================================*/

void STEMMaddogConnection::OnTradeList(const Linkx::Address &source, const unsigned serviceId, const void *pSubject,
									 const size_t subjectLength, const MarketData::TradeList &message)
{
	if (m_bTrades) {
		//copying trade data to SymbolManager
		TRACE<<" <"<<message.ticker.ToString()<<"> "<<message.data[0].price<<"/10^"<<static_cast<int>(message.priceType)<<" = "<<message.data[0].price/pow(10.0, message.priceType);
		auto price_double = Utils::Price(static_cast<int64_t>(message.data[0].price), message.data[0].priceType).AsDouble();
		trade_signal(message.ticker.ToString(),
				price_double);
	}
}

/*=========================================================================================*/

void STEMMaddogConnection::OnQuote(const Linkx::Address &source, const unsigned serviceId, const void *pSubject, const size_t subjectLength,
								 const MarketData::Quote &message)
{
	if (m_bQuotes) {
		//copying quote data in symbol manager
		TRACE<<"bid and ask sizes "<<message.bidSize<<" "<<message.askSize;
//		DEBUG<<" <"<<message.ticker.ToString()<<"> = "<<((double)message.bidSize - (double)message.askSize) / ((double)message.bidSize + (double)message.askSize);
		quote_signal(message.ticker.ToString(),
				message.bidSize,
				Utils::Price(static_cast<int64_t>(message.bid), message.priceType).AsDouble(),
				message.askSize,
				Utils::Price(static_cast<int64_t>(message.ask), message.priceType).AsDouble());
	}
}

/*=========================================================================================*/

void STEMMaddogConnection::OnQuoteDepth(const Linkx::Address &source, const unsigned serviceId, const void *pSubject,
									  const size_t subjectLength, const MarketData::Depth &message)
{
//	if (m_bDepth) {
//		::fprintf(stdout, "Depth,%d,%lf,%s,%08X,%d,%d\n", message.header.sequence, message.header.timeStamp,
//				  message.ticker.ToString().c_str(), message.flags, message.priceType, message.levelCount);
//		Process(&message, message.header.length);
//	}
}

/*=========================================================================================*/

void STEMMaddogConnection::OnQuoteDepthList(const Linkx::Address &source, const unsigned serviceId, const void *pSubject,
										  const size_t subjectLength, const MarketData::DepthList &message)
{
//	if (m_bDepth) {
//		::fprintf(stdout, "DepthList,%d,%lf,%s,%08X,%d\n", message.header.sequence, message.header.timeStamp,
//				  message.ticker.ToString().c_str(), message.flags, message.priceType);
//		Process(&message, message.header.length);
//	}
}

/*=========================================================================================*/

void STEMMaddogConnection::OnRefresh(const Linkx::Address &source, const unsigned serviceId, const void *pSubject,
								   const size_t subjectLength, const MarketData::Refresh &message)
{
//	::fprintf(stdout, "Refresh,%d,%lf,%s,%08X,%d,%d\n", message.header.sequence, message.header.timeStamp,
//			  message.ticker.ToString().c_str(), message.flags, message.priceType, message.fieldCount);
//	Process(&message, message.header.length);
}

/*=========================================================================================*/

void STEMMaddogConnection::OnAlert(const Linkx::Address &source, const unsigned serviceId, const void *pSubject,
								 const size_t subjectLength, const MarketData::Alert &message)
{
	::fprintf(stdout, "Alert,%d,%lf,%s,%08X,%d\n", message.header.sequence, message.header.timeStamp, message.ticker.ToString().c_str(),
			  message.flags, message.state);
//	Process(&message, message.header.length);
}

/*=========================================================================================*/

void STEMMaddogConnection::OnOrder(const Linkx::Address &source, const unsigned serviceId, const void *pSubject,
								 const size_t subjectLength, const MarketData::Order &message)
{
//	if (m_bOrderBook) {
//		::fprintf(stdout, "Order,%d,%lf,%s,%d\n", message.header.sequence, message.header.timeStamp, message.ticker.ToString().c_str(),
//				  message.priceType);
//		Process(&message, message.header.length);
//	}
}

/*=========================================================================================*/

void STEMMaddogConnection::OnOrderList(const Linkx::Address &source, const unsigned serviceId, const void *pSubject,
									 const size_t subjectLength, const MarketData::OrderList &message)
{
//	if (m_bOrderBook) {
//		::fprintf(stdout, "OrderList,%d,%lf,%s\n", message.header.sequence, message.header.timeStamp, message.ticker.ToString().c_str());
//		Process(&message, message.header.length);
//	}
}

/*=========================================================================================*/

void STEMMaddogConnection::OnCancel(const Linkx::Address &source, const unsigned serviceId, const void *pSubject,
								  const size_t subjectLength, const MarketData::Cancel &message)
{
//	if (m_bOrderBook) {
//		::fprintf(stdout, "Cancel,%d,%lf,%s\n", message.header.sequence, message.header.timeStamp, message.ticker.ToString().c_str());
//		Process(&message, message.header.length);
//	}
}

/*=========================================================================================*/

void STEMMaddogConnection::OnExecution(const Linkx::Address &source, const unsigned serviceId, const void *pSubject,
									 const size_t subjectLength, const MarketData::Execution &message)
{
//	if (m_bTrades || m_bOrderBook) {
//		::fprintf(stdout, "Execution,%d,%lf,%s,%d,%d,%d,%d\n", message.header.sequence, message.header.timeStamp,
//				  message.ticker.ToString().c_str(), message.priceType, message.price, message.size, message.volume);
//		Process(&message, message.header.length);
//	}
}

/*=========================================================================================*/

void STEMMaddogConnection::OnSettlement(const Linkx::Address &source, const unsigned serviceId, const void *pSubject,
									  const size_t subjectLength, const MarketData::Settlement &message)
{
//	::fprintf(stdout, "Settlement,%d,%lf,%s,%d\n", message.header.sequence, message.header.timeStamp, message.ticker.ToString().c_str(),
//			  message.priceType);
//	Process(&message, message.header.length);
}

