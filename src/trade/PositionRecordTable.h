/*
 * PositionRecordTable.h
 *
 *  Created on: Feb 28, 2017
 *      Author: jamil
 */

#ifndef SRC_TRADE_POSITIONRECORDTABLE_H_
#define SRC_TRADE_POSITIONRECORDTABLE_H_

#include <list>
#include <algorithm>
#include <iostream>
#include <mutex>



#ifndef UNIT_TEST
#include "../utils/log.h"
#else
#include "../utils/console.h"
#endif


struct PositionRecord {
	uint32_t open_order_id;
	uint32_t open_size;	//size of fill
	double open_price;	//price for fill
	double average_at_open;	//average after fill
	uint32_t close_order_id;
	uint32_t close_size;	//size of fill
	double close_price;	//price for fill
	double average_at_close;	//average after fill
	bool is_complete;
	double pnl;
	const int sign;


	PositionRecord() = default;

	/*
	 * @brief constructor is record open position at fill
	 *
	 * @param order whose fill is recieved
	 *
	 * @param fill size
	 *
	 * @param fill price
	 *
	 * @param average after fill
	 */
	PositionRecord(uint32_t id, uint32_t size, double price, double average, const int sign):
		open_order_id(id),
		open_size(size),
		open_price(price),
		average_at_open(average),
		close_order_id(0),
		close_size(0),
		close_price(0.0),
		average_at_close(0.0),
		is_complete(false),
		sign(sign),
		pnl(0.0) {

	}

	/*
	 * @brief constructor to create partial unclosed position record
	 *
	 * @param previous partial closed position record
	 *
	 * @param size of partial closed position
	 */
	PositionRecord(const PositionRecord &previous_record, uint32_t closed_size):
		open_order_id(previous_record.open_order_id),
		open_size(previous_record.open_size - closed_size),
		open_price(previous_record.open_price),
		average_at_open(previous_record.average_at_open),
		sign(previous_record.sign),
		close_order_id(0),
		close_size(0),
		close_price(0.0),
		average_at_close(0.0),
		is_complete(false),
		pnl(0.0) {

	}

	double setPartialCloseInfo(uint32_t close_id, uint32_t size, double price, double average) {
		close_order_id = close_id;
		close_size = open_size = size;
		close_price = price;
		average_at_close = average;
		is_complete = true;
		pnl = (close_price-open_price)*close_size*sign;

		return pnl;
	}

	void setOpenInfo(uint32_t id, uint32_t size, double price, double average) {
		open_order_id = id;
		open_size = size;
		open_price = price;
		average_at_open = average;
	}

	double setCloseInfo(uint32_t id, uint32_t size, double price, double average, bool is_complete = true) {
		close_order_id = id;
		close_size = size;
		close_price = price;
		average_at_close = average;
		this->is_complete = is_complete;
		pnl = (close_price-open_price)*close_size*sign;

		return pnl;
	}

	void print() const {
		TRACE<<open_order_id<<" "<<open_price<<" "<<open_size<<" "<<average_at_open<<" "<<close_order_id<<" "<<close_price<<" "<<close_size<<" "<<average_at_close<<" "<<pnl<<" "<<is_complete;
	}


};


class PositionRecordTable: public std::list<PositionRecord> {
	 std::mutex mt;
	const bool is_long;	//bool that define if account is long or short [true for long otherwise short]
	const int sign;

	uint32_t open_size;
	double pnl;
	double average;

public:

	PositionRecordTable(const bool is_long):
		is_long(is_long),
		sign(is_long?1:-1),
		pnl(0.0),
		open_size(0),
		average(0.0) {
	}

	/*
	 * @brief use clear to set everything in initial condition. it will empty
	 * the table reset pnl and open_size to zero
	 */
	void clear() {
		std::list<PositionRecord>::clear();
		pnl = 0.0;
		open_size = 0;
		average = 0.0;
	}

	/*
	 * @brief reset table when you want new entries to be pushed to table
	 * or table is refactored with respect to base quantity. reset will not
	 * modify open_size and average as it is required for refactoring
	 */
	void reset() {
		std::list<PositionRecord>::clear();
		pnl = 0.0;
	}

	double getPNL() { return pnl; }

	void setPNL(double pnl) {
		this->pnl = pnl;
	}

	uint32_t getOpenSize() { return open_size; }

	double adjustment () { return pnl/open_size; }

	double getAverage() { return average; }

	void shiftPNL(double reference) {
		pnl = (reference - average) * open_size * sign;
	}

	void print () {
		std::lock_guard<std::mutex> lock(mt);
		TRACE<<"positon table statistics (average, size, pnl) = "<<average<<" "<<open_size<< " " <<pnl;

		std::for_each(begin(), end(), [] (const_reference record) {
			record.print();
		});
	}

	/*
	 * @brief accumulate sum of all positions that yield positive pnl check
	 *
	 * @return return value is the size of order that should be sent in send_price
	 */
	std::pair<uint32_t, double> sizeofOrderWithPnl(const double send_price) {
		std::lock_guard<std::mutex> lock(mt);

		auto sum = 0;
		auto pnl = 0.0;
		std::pair<uint32_t, double> size_pnl_pair;
		for ( auto itr = begin(); itr != end(); itr++ ) {
			//if order is not yet closed and pnl is +ive accumulate all the orders that satisfy that condition

			if ( !itr->is_complete &&  ( send_price - itr->open_price )*sign > 0 ) {
				sum += itr->open_size;
				pnl += (send_price - itr->open_price)*sign*itr->open_size;
				TRACE<<"pnl against entry --> pnl: "<<pnl<<" pos: "<<sum;
			}
		}
		size_pnl_pair.first = sum;
		size_pnl_pair.second = pnl;
		TRACE<<"total pnl: "<<size_pnl_pair.second<<" total pos: "<<size_pnl_pair.first;
		return size_pnl_pair;
	}

	/*
	 * @brief insert a new open fill position in increasing order of price for long account
	 * and decreasing order of price for short account
	 *
	 * @param order id of open order
	 *
	 * @param size of fill received
	 *
	 * @param price of fill received
	 *
	 * @param average of the account after fill received
	 */
	void recordOpenPosition(uint32_t id, uint32_t size, double price) {
		std::lock_guard<std::mutex> lock(mt);

		bool is_inserted = false;
		open_size += size;
		if ( open_size > 0 )
			this->average += ( price - this->average ) * size / open_size;
		else this->average = 0.0;

		value_type record(id, size, price, this->average, sign);
		for (auto itr = begin(); itr != end(); itr++ ) {
			if ( ( itr->open_price - price )*sign > 0 ) {
				is_inserted = true;
				insert(itr, record);
				break;
			}
		}

		if ( !is_inserted ) push_back(record);	//if not inserted append it

	}


	/*
	 * @brief function record close entries corresponding to the open position, if position is closed
	 * completely is_completed will be set true.
	 *
	 * for partial fills a extra open fill record will be created for unclosed entries
	 *
	 * @param close order id
	 *
	 * @param size of close fill
	 *
	 * @param price of close fill
	 *
	 * @param average price after close fill executed
	 */
	void recordClosePosition(uint32_t id, uint32_t size, double price) {
		std::lock_guard<std::mutex> lock(mt);

		auto deleted_size = size;

		open_size -= size;
		if ( open_size > 0 )
			this->average += ( this->average - price ) * size / open_size;
		else this->average = 0.0;

		if ( is_long ) for (auto itr = rbegin(); itr != rend() && deleted_size > 0; itr++ ) {
			if ( itr->open_price < price && !itr->is_complete ) {

				if ( deleted_size >= itr->open_size )  {
					deleted_size -= itr->open_size;
					pnl += itr->setCloseInfo(id, itr->open_size, price, this->average);
				} else {

					//divide the record into two parts with closed and unclosed entries
					auto& prev_entry = *itr;
					value_type new_entry(*itr, deleted_size);
					insert(itr.base(), new_entry);
					pnl += prev_entry.setPartialCloseInfo(id, deleted_size, price, this->average);
					deleted_size = 0;
					break;
				}
			}
		}

		else for (auto itr = begin(); itr != end() && deleted_size > 0; itr++ ) {
			if ( itr->open_price > price ) {

				if ( deleted_size >= itr->open_size )  {

					deleted_size -= itr->open_size;
					pnl += itr->setCloseInfo(id, itr->open_size, price, this->average);
				} else {

					//divide the record into two parts with closed and unclosed entries
					value_type new_entry(*itr, deleted_size);
					insert(itr, new_entry);
					pnl += itr->setPartialCloseInfo(id, deleted_size, price, this->average);
					deleted_size = 0;
					break;
				}
			}
		}

		//when closing without pnl
		if ( deleted_size > 0 ) {
			if ( is_long ) {
				for (auto itr = rbegin(); itr != rend() && deleted_size > 0 ; itr++ ) {
					if ( deleted_size >= itr->open_size ) {
						deleted_size -= itr->open_size;
						pnl += itr->setCloseInfo(id, itr->open_size, price, this->average);
					} else {
						//divide the record into two parts with closed and unclosed entries
						auto& prev_entry = *itr;
						value_type new_entry(*itr, deleted_size);
						insert(itr.base(), new_entry);
						pnl += prev_entry.setPartialCloseInfo(id, deleted_size, price, this->average);
						deleted_size = 0;
						break;
					}
				}
			} else for (auto itr = begin(); itr != end() && deleted_size > 0; itr++ ) {
					if ( deleted_size >= itr->open_size )  {

						deleted_size -= itr->open_size;
						pnl += itr->setCloseInfo(id, itr->open_size, price, this->average);
					} else {

						//divide the record into two parts with closed and unclosed entries
						value_type new_entry(*itr, deleted_size);
						insert(itr, new_entry);
						pnl += itr->setPartialCloseInfo(id, deleted_size, price, this->average);
						deleted_size = 0;
						break;
					}
			}
		}

	}

	/**
	 * @brief delete completed entries of table
	 */
	void deleteCompletePosition() {
		std::lock_guard<std::mutex> lock(mt);

		for ( auto itr = begin(); itr != end(); ) {
			if ( itr->is_complete ) {
				itr = erase(itr);
			} else itr++;
		}
	}


	void for_each_position(std::function<void (PositionRecord)> execute) {
		std::lock_guard<std::mutex> lock(mt);

		std::for_each(begin(), end(), execute);
	}

	/*
	 * @brief this will repackage the table using base_qty and adjusted price or new average
	 *
	 * @param base_qty to distribute orders of equal sizes
	 *
	 * @param new average or adjusted price
	 *
	 * @param mpv provided from client
	 */
	void repackaging(uint32_t base_qty, double mpv, double adjustment) {
		std::lock_guard<std::mutex> lock(mt);

		auto noe = (uint32_t) ceil(double(open_size)/base_qty);
		average = average - adjustment*sign;

		reset();

		auto remainder = open_size;

		auto noeh = (uint32_t) ceil(noe / 2.0);
		auto noel = noe - noeh;

		//NOE(high) orders
		auto order_id = 1;
		for ( auto i = 1; i <= noeh; i++ ) {
			auto size = remainder >= base_qty ? base_qty: remainder;
			auto price = average - ( noeh  - i) * mpv * sign;
			value_type record(order_id++, size, price, average, sign);
			push_back(record);
			remainder -= size;
		}

		//NOE(low) orders
		for ( auto i = 1; i <= noel; i++ ) {
			auto size = remainder >= base_qty ? base_qty: remainder;
			auto price = average - i * mpv * sign;
			value_type record(order_id++, size, price, average, sign);
			push_back(record);
			remainder -= size;
		}



	}


	double startingPrice() {
		std::lock_guard<std::mutex> lock(mt);

		auto price = 0.0;
		if ( begin() != end() ) price = begin()->open_price;	//first price is minimum for long and maximum for short account
		return price;
	}

	double endingPrice() {
		std::lock_guard<std::mutex> lock(mt);

		auto price = 0.0;
		if ( rend() != rbegin() ) price = rbegin()->open_price;
		return price;
	}

	uint32_t positionAtPrice(const double price) {
		std::lock_guard<std::mutex> lock(mt);

		auto pos = std::find_if(begin(), end(), [this, price](const value_type &pos) {
			if ( pos.open_price == price && ( pos.open_size - pos.close_size ) > 0 ) {
				return true;
			}
			return false;
		});

		if ( pos != end() ) return (pos->open_size - pos->close_size);

		return 0;
	}
};




#endif /* SRC_TRADE_POSITIONRECORDTABLE_H_ */
