/*
 * position_table.cpp
 *
 *  Created on: Mar 1, 2017
 *      Author: jamil
 */




#define BOOST_TEST_MODULE position table tests
#include <boost/test/auto_unit_test.hpp>

#define UNIT_TEST 1

#include "../trade/PositionRecordTable.h"
#include <vector>

using namespace std;

BOOST_AUTO_TEST_SUITE(position_table_tests)

PositionRecordTable ltable(true);	//long account
PositionRecordTable stable(false);	//short account

std::vector<PositionRecord> test_positions;
std::vector<uint32_t> correct_order;

void populate_test_vector1() {
	test_positions.clear();correct_order.clear();
	//push test positions
	test_positions.push_back(PositionRecord(1, 100, 39.5, 39.50, 1));
	test_positions.push_back(PositionRecord(2, 200, 39.6, 39.56, 1));
	test_positions.push_back(PositionRecord(3, 300, 38.2, 38.88, 1));
	test_positions.push_back(PositionRecord(4, 400, 37.1, 38.17, 1));
	test_positions.push_back(PositionRecord(5, 500, 38.3, 38.21, 1));

	//correct order for test position
	correct_order.push_back(4);
	correct_order.push_back(3);
	correct_order.push_back(5);
	correct_order.push_back(1);
	correct_order.push_back(2);
}

void populate_test_vector2() {
	test_positions.clear();correct_order.clear();
	//push test positionsk for short table
	test_positions.push_back(PositionRecord(1, 100, 39.5, 39.50, -1));
	test_positions.push_back(PositionRecord(2, 200, 39.6, 39.56, -1));
	test_positions.push_back(PositionRecord(3, 300, 38.2, 38.88, -1));
	test_positions.push_back(PositionRecord(4, 400, 37.1, 38.17, -1));
	test_positions.push_back(PositionRecord(5, 500, 38.3, 38.21, -1));

	//correct order for test position
	correct_order.push_back(2);
	correct_order.push_back(1);
	correct_order.push_back(5);
	correct_order.push_back(3);
	correct_order.push_back(4);

}

void populate_table(decltype(ltable) &table) {
	table.clear();
	std::for_each(test_positions.begin(), test_positions.end(), [&] (decltype(test_positions)::const_reference position) {
		table.recordOpenPosition(position.open_order_id,
				position.open_size,
				position.open_price);
	});
}

void test_order(decltype(ltable) &table) {
	auto i = 0;
	for (auto itr = table.begin(); itr != table.end(); itr++, i++) {
		BOOST_CHECK_MESSAGE( itr->open_order_id == correct_order[i], "table order failure for "<<correct_order[i] );
	}
}



//BOOST_AUTO_TEST_CASE(table_creation_and_ordering) {
//
//	BOOST_TEST_CHECKPOINT("Starting");
//
//	BOOST_TEST_MESSAGE( "checking order of the table long" );
//	populate_test_vector1();
//	populate_table(ltable);
//	test_order(ltable);
//
//	BOOST_TEST_MESSAGE( "send price check for long" );
//	BOOST_CHECK_MESSAGE( ltable.sizeofOrderWithPnl(37.09).first == 0, " Send Price Test " );
//	BOOST_CHECK_MESSAGE( ltable.sizeofOrderWithPnl(37.11).first == 400, " Send Price Test " );
//	BOOST_CHECK_MESSAGE( ltable.sizeofOrderWithPnl(38.21).first == 700, " Send Price Test " );
//	BOOST_CHECK_MESSAGE( ltable.sizeofOrderWithPnl(38.31).first == 1200, " Send Price Test " );
//	BOOST_CHECK_MESSAGE( ltable.sizeofOrderWithPnl(39.51).first == 1300, " Send Price Test " );
//	BOOST_CHECK_MESSAGE( ltable.sizeofOrderWithPnl(39.61).first == 1500, " Send Price Test " );
//
//	ltable.print();
//
//	BOOST_TEST_MESSAGE( "checking order of the table short" );
//	populate_test_vector2();
//	populate_table(stable);
//	test_order(stable);
//
//	BOOST_TEST_MESSAGE( "send price check for short" );
//	BOOST_CHECK_MESSAGE( stable.sizeofOrderWithPnl(39.61).first == 0, " Send Price Test " );
//	BOOST_CHECK_MESSAGE( stable.sizeofOrderWithPnl(39.59).first == 200, " Send Price Test " );
//	BOOST_CHECK_MESSAGE( stable.sizeofOrderWithPnl(39.49).first == 300, " Send Price Test " );
//	BOOST_CHECK_MESSAGE( stable.sizeofOrderWithPnl(38.29).first == 800, " Send Price Test " );
//	BOOST_CHECK_MESSAGE( stable.sizeofOrderWithPnl(38.19).first == 1100, " Send Price Test " );
//	BOOST_CHECK_MESSAGE( stable.sizeofOrderWithPnl(37.09).first == 1500, " Send Price Test " );
//
//
//
//	stable.print();
//
//	BOOST_TEST_CHECKPOINT("Ending");
//}
//
//BOOST_AUTO_TEST_CASE(table_close_single_complete_fill) {
//	BOOST_TEST_CHECKPOINT("Starting");
//
//	{
//		BOOST_TEST_MESSAGE( "for long " );
//		populate_test_vector1();
//		populate_table(ltable);
//		ltable.recordClosePosition(33, 400, 37.12);
//		auto& record = *ltable.begin();
//
//		BOOST_CHECK_MESSAGE( record.close_price == 37.12 , " single complete fill ");
//		BOOST_CHECK_MESSAGE( record.close_size ==  400  , " single complete fill ");
//		BOOST_CHECK_MESSAGE( fabs(ltable.getPNL() - 8.0) < 0.0001 , " single complete fill "<<ltable.getPNL() );
//		BOOST_CHECK_MESSAGE( ltable.getOpenSize() == 1100 , " single complete fill ");
//
//		ltable.print();
//	}
//
//	{
//		BOOST_TEST_MESSAGE( "for short " );
//		populate_test_vector1();
//		populate_table(stable);
//		stable.recordClosePosition(33, 200, 39.55);
//		auto& record = *stable.begin();
//
//		BOOST_CHECK_MESSAGE( record.close_price == 39.55 , " single complete fill ");
//		BOOST_CHECK_MESSAGE( record.close_size ==  200  , " single complete fill ");
//		BOOST_CHECK_MESSAGE( fabs(stable.getPNL() - 10 ) < 0.0001 , " single complete fill "<<stable.getPNL() );
//		BOOST_CHECK_MESSAGE( stable.getOpenSize() == 1300 , " single complete fill ");
//
//		stable.print();
//	}
//
//
//	BOOST_TEST_CHECKPOINT("End");
//}
//
//BOOST_AUTO_TEST_CASE(table_close_multiple_complete_fill) {
//	BOOST_TEST_CHECKPOINT("Starting");
//
//	populate_test_vector1();
//	populate_table(ltable);
//	ltable.recordClosePosition(33, 700, 38.21);
//	auto& record = *ltable.begin();
//	auto itr = ltable.begin();
//	auto& record1 = *++itr;
//
//	BOOST_CHECK_MESSAGE( record.close_price == 38.21  , " multiple complete fill ");
//	BOOST_CHECK_MESSAGE( record.close_size ==  400 , " multiple complete fill ");
//	BOOST_CHECK_MESSAGE( record1.close_price == 38.21 , " multiple complete fill ");
//	BOOST_CHECK_MESSAGE( record1.close_size ==  300 , " multiple complete fill ");
//	BOOST_CHECK_MESSAGE( fabs(ltable.getPNL() - 447.0 ) < 0.0001, " multiple complete fill "<<ltable.getPNL() );
//	BOOST_CHECK_MESSAGE( ltable.getOpenSize() == 800 , " multiple complete fill ");
//
//	ltable.print();
//	BOOST_TEST_CHECKPOINT("End");
//}

BOOST_AUTO_TEST_CASE(stable_close_multiple_complete_fill) {
	BOOST_TEST_CHECKPOINT("Starting");

	populate_test_vector1();
	populate_table(stable);

	BOOST_MESSAGE(" Printing populated table!!");
	stable.print();

	stable.recordClosePosition(33, 900, 38.19);

	stable.print();


	auto& record = *stable.begin();
	auto itr = stable.begin();
	auto& record1 = *++itr;


	BOOST_CHECK_MESSAGE( record.close_price == 38.21  , " multiple complete fill ");
	BOOST_CHECK_MESSAGE( record.close_size ==  400 , " multiple complete fill ");
	BOOST_CHECK_MESSAGE( record1.close_price == 38.21 , " multiple complete fill ");
	BOOST_CHECK_MESSAGE( record1.close_size ==  300 , " multiple complete fill ");
	BOOST_CHECK_MESSAGE( fabs(stable.getPNL() - 447.0 ) < 0.0001, " multiple complete fill "<<stable.getPNL() );
	BOOST_CHECK_MESSAGE( stable.getOpenSize() == 800 , " multiple complete fill ");

	stable.print();

	stable.deleteCompletePosition();

	stable.print();

	BOOST_TEST_CHECKPOINT("End");
}
//
//BOOST_AUTO_TEST_CASE(table_close_multiple_fill_one_partial) {
//	BOOST_TEST_CHECKPOINT("Starting");
//
//	populate_test_vector1();
//	populate_table(ltable);
//	ltable.recordClosePosition(33, 600, 38.21);
//
//	{
//		auto itr = ltable.begin();
//		auto& record = *itr;
//		auto& record1 = *++itr;
//		auto& record2 = *++itr;
//
//		BOOST_CHECK_MESSAGE( record.close_price == 38.21  , " multiple fill with one partial "<<record.close_price );
//		BOOST_CHECK_MESSAGE( record.close_size ==  300 , " multiple fill with one partial "<<record.close_size );
//		BOOST_CHECK_MESSAGE( record1.open_price == record.open_price  , " multiple fill with one partial "<<record1.open_price );
//		BOOST_CHECK_MESSAGE( record1.open_size ==  100 , " multiple fill with one partial "<<record1.open_size );
//		BOOST_CHECK_MESSAGE( record2.close_price == 38.21 , " multiple complete fill "<<record2.close_price );
//		BOOST_CHECK_MESSAGE( record2.close_size ==  300 , " multiple complete fill "<<record2.close_size );
//		BOOST_CHECK_MESSAGE( fabs(ltable.getPNL() - 336.0 ) < 0.0001, " multiple complete fill "<<ltable.getPNL() );
//		BOOST_CHECK_MESSAGE( ltable.getOpenSize() == 900 , " multiple complete fill ");
//	}
//
//
//	ltable.print();
//	BOOST_TEST_CHECKPOINT("End");
//}
//
//BOOST_AUTO_TEST_CASE ( test_repackaging_long ) {
//
//	BOOST_TEST_MESSAGE( "Testing repackaging for Long Account" );
//	populate_test_vector1();
//	populate_table(ltable);
//	ltable.recordClosePosition(33, 600, 38.21);
//	ltable.print();
//
//	ltable.repackaging(100, 0.01, 0.02);
//	ltable.print();
//
//
//
//}

BOOST_AUTO_TEST_SUITE_END()




