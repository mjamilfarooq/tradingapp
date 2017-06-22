/*
 * order_sending.cpp
 *
 *  Created on: Nov 9, 2016
 *      Author: jamil
 */



#define BOOST_TEST_MODULE first test
#include <boost/test/auto_unit_test.hpp>

#include "../akela/OrderAccount.h"
#include "../config/AdminConfig.h"


using namespace std;

AdminSettings* AdminConfig::settings = nullptr;

BOOST_AUTO_TEST_SUITE(stringtester)

BOOST_AUTO_TEST_CASE(zerosize) {
	Linkx::Client  client;
	auto status = client.Connect("127.0.0.1", 48000);
	if ( ECERR_SUCCESS != status ) {
		FATAL<<"Linkx::Client::Connect failed for 127.0.0.1:48000 rc = "<<status;

	}

	OrderAccount account(client, "user_high_risk", ORDER_ACCOUNT_LONG, std::vector<std::string>{"XBowSIM"});

	BOOST_CHECK(true);
}




BOOST_AUTO_TEST_SUITE_END()
