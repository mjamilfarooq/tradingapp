/*
 * STEMDirectConnection.cpp
 *
 *  Created on: Sep 23, 2016
 *      Author: jamil
 */

#include "STEMDirectConnection.h"


STEMDirectConnection::STEMDirectConnection():
	m_bTrades(true),
	m_bQuotes(true) {

	Open("Direct.Bats", "../configs/Direct.Bats.ini");

	if ( IsOpen() ) {
		std::cout<<"STEMDirectConnection is open"<<std::endl;
		SetCallback(static_cast<MarketDataFeed::ICallback *>(this), 0);

	} else {
		std::cout<<"STEMDirectConnection isn't open"<<std::endl;
	}

}

STEMDirectConnection::~STEMDirectConnection() {

}

void STEMDirectConnection::Subscribe(std::string sym){
	MarketData::Ticker ticker;
	if ( ticker.Parse(sym, MarketData::Ticker::TYPE_EQUITY) ) {
		int status = static_cast<MarketDataFeed::DirectConnection *>(this)->Subscribe(ticker);
		if ( ECERR_SUCCESS != status ) {
			//log this event
			std::cout<<"can't subscribe to symbol ("<<sym<<")"<<std::endl;
		} else {
			std::cout<<"Subscribing to symbol ("<<sym<<")"<<std::endl;
			this->tickerList.push_back(ticker);
		}
	}
}

void STEMDirectConnection::Subscribe(std::vector<std::string> &symList) {
	std::for_each(symList.begin(), symList.end(), [this](std::string str){
		MarketData::Ticker ticker;
		if ( ticker.Parse(str, MarketData::Ticker::TYPE_EQUITY) ) {
			int status = static_cast<MarketDataFeed::DirectConnection *>(this)->Subscribe(ticker);
			if ( ECERR_SUCCESS != status ) {
				//log this event
				std::cout<<"can't subscribe to symbol ("<<str<<")"<<std::endl;
			} else {
				std::cout<<"Subscribing to symbol ("<<str<<")"<<std::endl;
				this->tickerList.push_back(ticker);
			}
		}
	});
}

void STEMDirectConnection::Unsubscribe(std::string symbol) {
	std::for_each(tickerList.begin(), tickerList.end(),[](MarketData::Ticker ticker){

	});
}

void STEMDirectConnection::Unsubscribe() {
	std::for_each(tickerList.begin(), tickerList.end(), [this](MarketData::Ticker &ticker){
		static_cast<MarketDataFeed::DirectConnection *>(this)->Unsubscribe(ticker);
	});
}



void STEMDirectConnection::OnTrade(const MarketData::Trade &message, uintptr_t reference)
{
	if (m_bEnable && m_bTrades) {
		char  timeBuffer[32];

		::fprintf(stdout, "%s%s,%02X,Trade,%d,%lf,%s,%08X,%d,%d,%d,%d%s", EDGE_EOL_STRING,
				  message.header.GetTimeString(timeBuffer, sizeof(timeBuffer)),
				  message.header.flags, message.header.sequence, message.header.timeStamp, message.ticker.ToString().c_str(),
				  message.flags, message.priceType, message.price, message.size, message.volume, EDGE_EOL_STRING);

	}
}

/*=========================================================================================*/

void STEMDirectConnection::OnTradeList(const MarketData::TradeList &message, uintptr_t reference)
{

	if (m_bTrades) {
		//copying trade data to SymbolManager
//		std::cout<<" <"<<message.ticker.ToString()<<"> "<<message.data[0].price<<"/10^"<<static_cast<int>(message.priceType)<<" = "<<message.data[0].price/pow(10.0, message.priceType)<<std::endl;

		auto symbol = message.ticker.ToString();
		auto pos = std::find(symbol.begin(), symbol.end(), '.');
		symbol = std::string(symbol.begin(), pos);

		auto price_double = Utils::Price(static_cast<int64_t>(message.data[0].price), message.data[0].priceType).AsDouble();
		trade_signal(symbol,
				price_double);
	}
}

/*=========================================================================================*/

void STEMDirectConnection::OnQuote(const MarketData::Quote &message, uintptr_t reference) {

	if (m_bQuotes) {
			//copying quote data in symbol manager
//			TRACE<<"bid and ask sizes "<<message.bidSize<<" "<<message.askSize;
//		std::cout<<" <"<<message.ticker.ToString()<<"> = "<<((double)message.bidSize - (double)message.askSize) / ((double)message.bidSize + (double)message.askSize)<<std::endl;
		auto symbol = message.ticker.ToString();
		auto pos = std::find(symbol.begin(), symbol.end(), '.');
		symbol = std::string(symbol.begin(), pos);
			quote_signal(symbol,
					message.bidSize,
					Utils::Price(static_cast<int64_t>(message.bid), message.priceType).AsDouble(),
					message.askSize,
					Utils::Price(static_cast<int64_t>(message.ask), message.priceType).AsDouble());
		}
}

/*=========================================================================================*/

void STEMDirectConnection::OnBBOQuote(const MarketData::BBOQuote &message, uintptr_t reference)
{
//    OnQuote(message.AsQuote(), reference);
}

/*=========================================================================================*/

void STEMDirectConnection::OnQuoteDepth(const MarketData::Depth &message, uintptr_t reference)
{
//	if (m_bEnable && m_bDepth) {
//		char  timeBuffer[32];
//
//		::fprintf(stdout, "%s%s,%02X,Depth,%d,%lf,%s,%08X,%d,%d,%d%s", EDGE_EOL_STRING,
//				  message.header.GetTimeString(timeBuffer, sizeof(timeBuffer)),
//				  message.header.flags, message.header.sequence, message.header.timeStamp, message.ticker.ToString().c_str(), message.flags,
//				  message.condition, message.priceType, message.levelCount, EDGE_EOL_STRING);
//
//		unsigned  i;
//
//		for (i = 0; i < message.levelCount; i++) {
//			::fprintf(stdout, "Depth Entry,%d,%02X,%d,%d,%d,%d,%d,%d,%d,%d%s", i, message.quote[i].flags, message.quote[i].priceType,
//					  message.quote[i].level, message.quote[i].buyPrice, message.quote[i].buyQuantity, message.quote[i].buyOrderCount,
//					  message.quote[i].sellPrice, message.quote[i].sellQuantity, message.quote[i].sellOrderCount, EDGE_EOL_STRING);
//		}
//
//
//	}
}

/*=========================================================================================*/

void STEMDirectConnection::OnQuoteDepthList(const MarketData::DepthList &message, uintptr_t reference)
{
//	if (m_bEnable && m_bDepth) {
//		char  timeBuffer[32];
//
//		::fprintf(stdout, "%s%s,%02X,DepthList,%d,%lf,%s,%08X,%d,%d,%d%s", EDGE_EOL_STRING,
//				  message.header.GetTimeString(timeBuffer, sizeof(timeBuffer)),
//				  message.header.flags, message.header.sequence, message.header.timeStamp, message.ticker.ToString().c_str(), message.flags,
//				  message.priceType, message.buyCount, message.sellCount, EDGE_EOL_STRING);
//
//		const MarketData::DepthListEntry  *pEntries;
//		size_t                            i, count;
//
//		if (message.GetBuys(pEntries, count)) {
//			for (i = 0; i < count; i++) {
//				::fprintf(stdout, "BUY Entry,%d,%02X,%d,%d,%d,%d,%d,%d,%d%s", static_cast<unsigned>(i), pEntries[i].flags,
//						  message.priceType, pEntries[i].level, pEntries[i].price, pEntries[i].size, pEntries[i].count,
//						  pEntries[i].impliedSize, pEntries[i].impliedCount, EDGE_EOL_STRING);
//			}
//		}
//
//		if (message.GetSells(pEntries, count)) {
//			for (i = 0; i < count; i++) {
//				::fprintf(stdout, "SELL Entry,%d,%02X,%d,%d,%d,%d,%d,%d,%d%s", static_cast<unsigned>(i), pEntries[i].flags,
//						  message.priceType, pEntries[i].level, pEntries[i].price, pEntries[i].size, pEntries[i].count,
//						  pEntries[i].impliedSize, pEntries[i].impliedCount, EDGE_EOL_STRING);
//			}
//		}
//
//
//	}
}

/*=========================================================================================*/

void STEMDirectConnection::OnRefresh(const MarketData::Refresh &message, uintptr_t reference)
{
	if (!m_bEnable)
		return;
	if (!m_bRefresh)
		return;

	char  timeBuffer[32];

	::fprintf(stdout, "%s%s,%02X,Refresh,%d,%lf,%s,%08X,%d,%d%s", EDGE_EOL_STRING,
			  message.header.GetTimeString(timeBuffer, sizeof(timeBuffer)),
			  message.header.flags, message.header.sequence, message.header.timeStamp, message.ticker.ToString().c_str(), message.flags,
			  message.priceType, message.fieldCount, EDGE_EOL_STRING);

	char                          buffer[32];
	const MarketData::FullPrice   *pFullPrice;
	const MarketData::Price       *pPrice;
	const MarketData::PriceRange  *pRangePrice;
	const void                    *pFieldData;
	unsigned                      status, offset = 0, fieldId, fieldType, length;

	status = message.GetField(offset, fieldId, fieldType, length, pFieldData, offset);
	if (ECERR_SUCCESS == status) {
		do {
			switch (fieldType) {
				case MarketData::FIELD_TYPE_INT32:
					::fprintf(stdout, "Field,%2d,%3d,%d,%d%s", fieldType, fieldId, length, *reinterpret_cast<const int32_t*>(pFieldData),
							  EDGE_EOL_STRING);
					break;

				case MarketData::FIELD_TYPE_UINT32:
					::fprintf(stdout, "Field,%2d,%3d,%d,%u%s", fieldType, fieldId, length, *reinterpret_cast<const uint32_t*>(pFieldData),
							  EDGE_EOL_STRING);
					break;

				case MarketData::FIELD_TYPE_UINT16:
					::fprintf(stdout, "Field,%2d,%3d,%d,%u%s", fieldType, fieldId, length, *reinterpret_cast<const uint16_t*>(pFieldData),
							  EDGE_EOL_STRING);
					break;

				case MarketData::FIELD_TYPE_UINT8:
					::fprintf(stdout, "Field,%2d,%3d,%d,%u%s", fieldType, fieldId, length, *reinterpret_cast<const uint8_t*>(pFieldData),
							  EDGE_EOL_STRING);
					break;

				case MarketData::FIELD_TYPE_STRING:
					::memset(buffer, 0, sizeof(buffer));
					::memcpy(buffer, pFieldData, std::min<size_t>(length, sizeof(buffer)-1));
					::fprintf(stdout, "Field,%2d,%3d,%d,%s%s", fieldType, fieldId, length, buffer, EDGE_EOL_STRING);
					break;

				case MarketData::FIELD_TYPE_FLOAT:
					::fprintf(stdout, "Field,%2d,%3d,%d,%f%s", fieldType, fieldId, length, *reinterpret_cast<const float*>(pFieldData),
							  EDGE_EOL_STRING);
					break;

				case MarketData::FIELD_TYPE_DOUBLE:
					::fprintf(stdout, "Field,%2d,%3d,%d,%lf%s", fieldType, fieldId, length, *reinterpret_cast<const double*>(pFieldData),
							  EDGE_EOL_STRING);
					break;

				case MarketData::FIELD_TYPE_INT64:
#ifdef __EDGE_WIN32__
					::fprintf(stdout, "Field,%2d,%3d,%d,%I64d%s", fieldType, fieldId, length,
							  *reinterpret_cast<const int64_t*>(pFieldData), EDGE_EOL_STRING);
#else
#ifdef __LP64__
					::fprintf(stdout, "Field,%2d,%3d,%d,%ld%s", fieldType, fieldId, length, *reinterpret_cast<const int64_t*>(pFieldData),
							  EDGE_EOL_STRING);
#else
					::fprintf(stdout, "Field,%2d,%3d,%d,%lld%s", fieldType, fieldId, length, *reinterpret_cast<const int64_t*>(pFieldData),
							  EDGE_EOL_STRING);
#endif
#endif
					break;

				case MarketData::FIELD_TYPE_UINT64:
#ifdef __EDGE_WIN32__
					::fprintf(stdout, "Field,%2d,%3d,%d,%I64u%s", fieldType, fieldId, length,
							  *reinterpret_cast<const uint64_t*>(pFieldData), EDGE_EOL_STRING);
#else
#ifdef __LP64__
					::fprintf(stdout, "Field,%2d,%3d,%d,%lu%s", fieldType, fieldId, length, *reinterpret_cast<const uint64_t*>(pFieldData),
							  EDGE_EOL_STRING);
#else
					::fprintf(stdout, "Field,%2d,%3d,%d,%llu%s", fieldType, fieldId, length,
							  *reinterpret_cast<const uint64_t*>(pFieldData), EDGE_EOL_STRING);
#endif
#endif
					break;

				case MarketData::FIELD_TYPE_BLOB:
					::fprintf(stdout, "Field,%2d,%3d,%d,BLOB%s", fieldType, fieldId, length, EDGE_EOL_STRING);
					break;

				case MarketData::FIELD_TYPE_SMALL_BLOB:
					::fprintf(stdout, "Field,%2d,%3d,%d,SMALL BLOB%s", fieldType, fieldId, length, EDGE_EOL_STRING);
					break;

				case MarketData::FIELD_TYPE_PRICE:
					pPrice = reinterpret_cast<const MarketData::Price*>(pFieldData);

					::fprintf(stdout, "Field,%2d,%3d,%d,%02X %d, %-2.2s %d%s", fieldType, fieldId, length, pPrice->flags, pPrice->priceType,
							  pPrice->exchange, pPrice->price, EDGE_EOL_STRING);
					break;

				case MarketData::FIELD_TYPE_FULLPRICE:
					pFullPrice = reinterpret_cast<const MarketData::FullPrice*>(pFieldData);

					::fprintf(stdout, "Field,%2d,%3d,%d,%02X %-2.2s %d %d%s", fieldType, fieldId, length, pFullPrice->flags,
							  pFullPrice->exchange, pFullPrice->price, pFullPrice->size, EDGE_EOL_STRING);
					break;

				case MarketData::FIELD_TYPE_PRICE_RANGE:
					pRangePrice = reinterpret_cast<const MarketData::PriceRange*>(pFieldData);

					::fprintf(stdout, "Field,%2d,%3d,%d,%02X %d %d %d%s", fieldType, fieldId, length, pRangePrice->flags,
							  pRangePrice->price1, pRangePrice->price2, pRangePrice->time, EDGE_EOL_STRING);
					break;

				default:
				case MarketData::FIELD_TYPE_DATETIME:
				case MarketData::FIELD_TYPE_PRICE_DATE:
				case MarketData::FIELD_TYPE_CONDITIONS:
				case MarketData::FIELD_TYPE_DOUBLE_RANGE:
				case MarketData::FIELD_TYPE_TICKER:
					::fprintf(stdout, "Field,%2d,%3d,%d%s", fieldType, fieldId, length, EDGE_EOL_STRING);
					break;
			}
		} while (message.GetField(offset, fieldId, fieldType, length, pFieldData, offset) == ECERR_SUCCESS);
	}


}

/*=========================================================================================*/

void STEMDirectConnection::OnAlert(const MarketData::Alert &message, uintptr_t reference)
{
		char  timeBuffer[32];
//
	switch (message.extensionType) {
		case MarketData::Alert::TYPE_WORKUP:
			::fprintf(stdout, "%s%s,%02X,Alert,%d,%lf,%s,%08X,%d,Workup,%s,%s,%d,%d,%d%s", EDGE_EOL_STRING,
					  message.header.GetTimeString(timeBuffer, sizeof(timeBuffer)), message.header.flags, message.header.sequence,
					  message.header.timeStamp, message.ticker.ToString().c_str(), message.flags, message.state,
					  message.extension.workup.aggressiveReference, message.extension.workup.passiveReference,
					  message.extension.workup.aggressiveBuySell, message.extension.workup.passiveBuySell, message.extension.workup.price,
					  EDGE_EOL_STRING);
			break;

		case MarketData::Alert::TYPE_TEXT:
			::fprintf(stdout, "%s%s,%02X,Alert,%d,%lf,%s,%08X,%d,Text,%s%s", EDGE_EOL_STRING,
					  message.header.GetTimeString(timeBuffer, sizeof(timeBuffer)), message.header.flags, message.header.sequence,
					  message.header.timeStamp, message.ticker.ToString().c_str(), message.flags, message.state, message.extension.text,
					  EDGE_EOL_STRING);
			break;

		case MarketData::Alert::TYPE_IMBALANCE:
			::fprintf(stdout, "%s%s,%02X,Alert,%d,%lf,%s,%08X,%d,Imbalance,%d,%d,%d,%c,%d,%d,%d%s", EDGE_EOL_STRING,
					  message.header.GetTimeString(timeBuffer, sizeof(timeBuffer)), message.header.flags, message.header.sequence,
					  message.header.timeStamp, message.ticker.ToString().c_str(), message.flags, message.state,
					  message.extension.imbalance.size, message.extension.imbalance.totalImbalance,
					  message.extension.imbalance.marketImbalance, message.extension.imbalance.exchange,
					  message.extension.imbalance.quoteCondition, message.extension.imbalance.price,
					  message.extension.imbalance.shortSaleRestrictionPrice, EDGE_EOL_STRING);
			break;

		case MarketData::Alert::TYPE_SWEEP:
			::fprintf(stdout, "%s%s,%02X,Alert,%d,%lf,%s,%08X,%d,Sweep,%d,%d,%d,%d,%d,%08X,%d,%d%s", EDGE_EOL_STRING,
					  message.header.GetTimeString(timeBuffer, sizeof(timeBuffer)), message.header.flags, message.header.sequence,
					  message.header.timeStamp, message.ticker.ToString().c_str(), message.flags, message.state,
					  message.extension.sweep.side, message.extension.sweep.levelCount, message.extension.sweep.startPrice,
					  message.extension.sweep.endPrice, message.extension.sweep.volume, message.extension.sweep.flags,
					  message.extension.sweep.startSequenceNumber, message.extension.sweep.endSequenceNumber, EDGE_EOL_STRING);
			break;

		default:
		case MarketData::Alert::TYPE_NONE:
			::fprintf(stdout, "%s%s,%02X,Alert,%d,%lf,%s,%08X,%d%s", EDGE_EOL_STRING,
					  message.header.GetTimeString(timeBuffer, sizeof(timeBuffer)),
					  message.header.flags, message.header.sequence, message.header.timeStamp, message.ticker.ToString().c_str(),
					  message.flags, message.state, EDGE_EOL_STRING);
			break;
	}



	switch ( message.state ) {
	case MarketData::Alert::STATE_START_OF_SESSION:
		break;
	case MarketData::Alert::STATE_END_OF_SESSION:
		break;
	case MarketData::Alert::STATE_OPEN:
		break;
	case MarketData::Alert::STATE_CLOSED:
		break;
	}


}

/*=========================================================================================*/

void STEMDirectConnection::OnAdmin(const MarketData::Admin &message, uintptr_t reference)
{
	if (!m_bEnable)
		return;

	char  timeBuffer[32];

	::fprintf(stdout, "%s%s,%02X,Admin,%d,%lf%s", EDGE_EOL_STRING,
			  message.header.GetTimeString(timeBuffer, sizeof(timeBuffer)), message.header.flags,
			  message.header.sequence, message.header.timeStamp, EDGE_EOL_STRING);

}

/*=========================================================================================*/

void STEMDirectConnection::OnHeartbeat(const MarketData::Heartbeat &message, uintptr_t reference)
{
	if (!m_bEnable)
		return;

	char  timeBuffer[32];

	::fprintf(stdout, "%s%s,%02X,Heartbeat,%d,%lf%s", EDGE_EOL_STRING,
			  message.header.GetTimeString(timeBuffer, sizeof(timeBuffer)), message.header.flags,
			  message.header.sequence, message.header.timeStamp, EDGE_EOL_STRING);

}

/*=========================================================================================*/

void STEMDirectConnection::OnOrder(const MarketData::Order &message, uintptr_t reference)
{
//	if (m_bEnable && m_bOrderBook) {
//		char  timeBuffer[32];
//
//		::fprintf(stdout, "%s%s,%02X,Order,%d,%lf,%s,%d%s", EDGE_EOL_STRING, message.header.GetTimeString(timeBuffer, sizeof(timeBuffer)),
//				  message.header.flags, message.header.sequence, message.header.timeStamp, message.ticker.ToString().c_str(),
//				  message.priceType, EDGE_EOL_STRING);
//
//	}
}

/*=========================================================================================*/

void STEMDirectConnection::OnOrderList(const MarketData::OrderList &message, uintptr_t reference)
{
//	if (m_bEnable && m_bOrderBook) {
//		char  timeBuffer[32];
//
//		::fprintf(stdout, "%s%s,%02X,OrderList,%d,%lf,%s%s", EDGE_EOL_STRING, message.header.GetTimeString(timeBuffer, sizeof(timeBuffer)),
//				  message.header.flags, message.header.sequence, message.header.timeStamp, message.ticker.ToString().c_str(),
//				  EDGE_EOL_STRING);
//
//		char      referenceString[128], tempString[8];
//		unsigned  i, j;
//
//		for (i = 0; i < message.count; i++) {
//			if (0 == (message.entries[i].flags & MarketData::Order::FLAG_BINARY_REFERENCE))
//				::fprintf(stdout, "OrderList Entry,%d,%s,%08X,%d,%d,%d,%d,%d%s", i, message.entries[i].referenceNumber,
//						  message.entries[i].flags, message.entries[i].commandType, message.entries[i].buySell,
//						  message.entries[i].priceType, message.entries[i].price, message.entries[i].size, EDGE_EOL_STRING);
//			else {
//				::memset(referenceString, 0, sizeof(referenceString));
//
//				for (j = 0; j < sizeof(message.entries[i].referenceNumber); j++) {
//					::sprintf(tempString, "%02X", static_cast<uint8_t>(message.entries[i].referenceNumber[j]));
//					::strcat(referenceString, tempString);
//					if (j < sizeof(message.entries[i].referenceNumber)-1)
//						::strcat(referenceString, " ");
//				}
//
//				::fprintf(stdout, "OrderList Entry,%d,%s,%08X,%d,%d,%d,%d,%d%s", i, referenceString, message.entries[i].flags,
//						  message.entries[i].commandType, message.entries[i].buySell, message.entries[i].priceType,
//						  message.entries[i].price, message.entries[i].size, EDGE_EOL_STRING);
//			}
//		}
//
//
//	}
}

/*=========================================================================================*/

void STEMDirectConnection::OnCancel(const MarketData::Cancel &message, uintptr_t reference)
{
//	if (m_bEnable && m_bOrderBook) {
//		char  timeBuffer[32];
//
//		::fprintf(stdout, "%s%s,%02X,Cancel,%d,%lf,%s%s", EDGE_EOL_STRING, message.header.GetTimeString(timeBuffer, sizeof(timeBuffer)),
//				  message.header.flags, message.header.sequence, message.header.timeStamp, message.ticker.ToString().c_str(),
//				  EDGE_EOL_STRING);
//
//	}
}

/*=========================================================================================*/

void STEMDirectConnection::OnExecution(const MarketData::Execution &message, uintptr_t reference)
{
//	if (m_bEnable && (m_bTrades || m_bOrderBook)) {
//		char  timeBuffer[32];
//
//		::fprintf(stdout, "%s%s,%02X,Execution,%d,%lf,%s,%d,%d,%d,%d%s", EDGE_EOL_STRING,
//				  message.header.GetTimeString(timeBuffer, sizeof(timeBuffer)),
//				  message.header.flags, message.header.sequence, message.header.timeStamp, message.ticker.ToString().c_str(),
//				  message.priceType, message.price, message.size, message.volume, EDGE_EOL_STRING);
//
//	}
}

/*=========================================================================================*/

void STEMDirectConnection::OnSettlement(const MarketData::Settlement &message, uintptr_t reference)
{
//	if (!m_bEnable)
//		return;
//
//	char  timeBuffer[32];
//
//	::fprintf(stdout, "%s%s,%02X,Settlement,%d,%lf,%s,%d,%d,%d%s", EDGE_EOL_STRING,
//			  message.header.GetTimeString(timeBuffer, sizeof(timeBuffer)),
//			  message.header.flags, message.header.sequence, message.header.timeStamp, message.ticker.ToString().c_str(), message.priceType,
//			  message.price, message.totalVolume, EDGE_EOL_STRING);

}

/*=========================================================================================*/

void STEMDirectConnection::OnDataTransfer(const MarketData::DataTransfer &message, uintptr_t reference)
{
//	if (!m_bEnable)
//		return;
//
//	char  timeBuffer[32];
//
//	::fprintf(stdout, "%s%s,%02X,DataTransfer,%d,%lf%s", EDGE_EOL_STRING, message.header.GetTimeString(timeBuffer, sizeof(timeBuffer)),
//			  message.header.flags, message.header.sequence, message.header.timeStamp, EDGE_EOL_STRING);
}
