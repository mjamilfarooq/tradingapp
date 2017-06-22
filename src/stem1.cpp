//============================================================================
// Name        : STEM1.cpp
// Author      : Muhammad Jamil Farooq
// Version     :
// Copyright   : riskapplication.com
// Description : STEM Application First Version
// Features:
//		1-	Reading Symbol Configuration File
//		2- 	Reading Trading Configuration File
//		3-  Reading Admin Configuration File
//============================================================================


#include <edge.h>
#include <iostream>
#include <signal.h>

#include <utils/config/CommandLineParser.h>
//#include <boost/log/common.hpp>
//#include <boost/log/sinks.hpp>
//#include <boost/log/sources/logger.hpp>
//#include <boost/shared_ptr.hpp>

#include "config/SymbolConfig.h"
#include "config/TradingConfig.h"
#include "config/AdminConfig.h"

#include "redisclient.h"
#include "redis/RedisWriter.h"
#include "utils/log.h"
#include "cli/CLI.h"
#include "manager/SymbolManager.h"
#include "manager/TradeAccountManager.h"

using namespace std;

const char *g_VersionString = "1.0";

#define PROGRAM_ARGUMENT_LIST(declare)										\
	declare(ARG_ROUTER_ADDRESS,		"LinkAddress",		"",		false)	\
	declare(ARG_ROUTER_PORT,		"LinkPort",			"",		false)	\


enum ProgramArgumentId
{
	PROGRAM_ARGUMENT_LIST(EXTRACT_PROGRAM_ARGUMENT_ID)
};

const Config::CommandLineParser::ProgramArgument  g_ArgumentList[] =
{
	PROGRAM_ARGUMENT_LIST(MAKE_PROGRAM_ARGUMENT)
};


/*=========================================================================================*/

#define PROGRAM_OPTION_LIST(declare)				\
	declare(ARG_VERBOSE,							\
			"Verbose",								\
			"v",									\
			"Verbose Output Level",						\
			"TRACE",									\
			OPTION_NORMAL,							\
			0)										\
	declare(ARG_LOGGING,								\
			"Logging",									\
			"l",									\
			"Loggin Output Level",							\
			"TRACE",									\
			OPTION_NORMAL,							\
			0)										\
	declare(ARG_SYMBOL_CONFIG,								\
				"SymbolConfig",									\
				"s",									\
				"Symbol Configuration ",							\
				"symbol_config.xml",									\
				OPTION_NORMAL,							\
				0)										\
	declare(ARG_TRADING_CONFIG,								\
				"TradingConfig",									\
				"t",									\
				"Tradig Configuration",							\
				"trading_config.xml",									\
				OPTION_NORMAL,							\
				0)										\
	declare(ARG_ADMIN_CONFIG,								\
				"AdminConfig",									\
				"a",									\
				"Admin Configuration",							\
				"admin_config.xml",									\
				OPTION_NORMAL,							\
				0)										\
	declare(ARG_APPLICATION_NAME,						\
 			"ApplicationName",							\
			"m",									\
			"STEM Application for NYSE",			\
			"STEM",								\
			OPTION_NORMAL,							\
			0)										\
	declare(ARG_HELP,								\
			"help",									\
			"?",									\
			"Command Line Help",					\
			"",										\
			OPTION_HELP,							\
			0)										\

enum ProgramOptionId
{
	PROGRAM_OPTION_LIST(EXTRACT_PROGRAM_OPTION_ID)
};

const Config::CommandLineParser::ProgramOption  g_OptionList[] =
{
	PROGRAM_OPTION_LIST(MAKE_PROGRAM_OPTION)
};

/*=========================================================================================*/

//static configuration variable initializations
UserSymbolsMap* SymbolConfig::user_symbols_map = nullptr;
TradeAccountMap* TradingConfig::trade_accounts_map = nullptr;
AdminSettings* AdminConfig::settings = nullptr;

SymbolManager* symbol_manager = nullptr;
TradeAccountManager* account_manager = nullptr;

bool exit_signal = false;

int main(int argc, char *argv[]) {


	Config::CommandLineParser  programParams("STEM1", g_VersionString, "STEM 1.0 Application", g_ArgumentList,
												 ARRAY_SIZE(g_ArgumentList), g_OptionList, ARRAY_SIZE(g_OptionList));
	unsigned status;

	status = programParams.Parse(argc, argv);
	if (ECERR_SUCCESS != status) {
		fprintf(stderr, "CommandLineParser::Parser() failed, rc=%d\n", status);
		return 1;
	}

	std::string routerAddress;
	std::string data;
	std::string verbose;
	std::string logging;

	std::string symbol_config;
	std::string trading_config;
	std::string admin_config;


	boost::log::trivial::severity_level severity_option = boost::log::trivial::trace;

	auto routerPort = LINKX_DEFAULT_PORT;
	try {

		//extracting RouterAddress
		if (!programParams.GetValue("LinkAddress", routerAddress) || 0 == routerAddress.size()) {
			fprintf(stderr, "No router address specified.\n");
			return 1;
		}

		//extracting RouterPort
		if (!programParams.GetValue("LinkPort", data) || 0 == data.size()) {
			fprintf(stderr, "No router port specified. setting to default (%d)\n", routerPort);
			return 1;
		} else {
			routerPort = atoi(data.c_str());
			if ( 0 == routerPort ) {
				routerPort = LINKX_DEFAULT_PORT;
				fprintf(stderr, "Zero is not a valid port. setting to default (%d)\n", routerPort);
			}
		}

		//setting verbosity level for the application from trace to error
		if ( programParams.GetValue("Verbose", verbose) ) {
			if ( 0 == strcasecmp(verbose.c_str(), "trace") ) severity_option = boost::log::trivial::trace;
			else if ( 0 == strcasecmp(verbose.c_str(), "debug") ) severity_option = boost::log::trivial::debug;
			else if ( 0 == strcasecmp(verbose.c_str(), "info") ) severity_option = boost::log::trivial::info;
			else if ( 0 == strcasecmp(verbose.c_str(), "warning") ) severity_option = boost::log::trivial::warning;
			else if ( 0 == strcasecmp(verbose.c_str(), "error") ) severity_option = boost::log::trivial::error;
			else if ( 0 == strcasecmp(verbose.c_str(), "fatal") ) severity_option = boost::log::trivial::fatal;
			else {
				fprintf(stderr, "Unrecognized verbosity option setting to default option (trace)");
			}
		}


		if ( !programParams.GetValue("SymbolConfig", symbol_config) ) {
			fprintf(stderr, "must specify symbol configuration");
		}

		if ( !programParams.GetValue("TradingConfig", trading_config) ) {
			fprintf(stderr, "must specify symbol configuration");
		}

		if ( !programParams.GetValue("AdminConfig", admin_config) ) {
			fprintf(stderr, "must specify symbol configuration");
		}

		fprintf(stdout, "configurations files include\n%s\n%s\n%s\n",
				symbol_config.c_str(), trading_config.c_str(), admin_config.c_str());


	} catch (std::exception &ex) {
		fprintf(stderr, "Command line parse error, %s\n", ex.what());
		programParams.Usage();

		return 1;
	}

	TRACE<<"STARTING STEM";

//	boost::log::core::get()->set_filter
//	(
//		boost::log::trivial::severity >= severity_option
//	);

	try {
		SymbolConfig::parse(symbol_config);
		TradingConfig::parse(trading_config);
//		AdminConfig::parse(admin_config);
	} catch (ConfigException &e) {
		cerr<<e.what();
		return EXIT_FAILURE;
	}


	Linkx::Client  client;
	status = client.Connect(routerAddress, routerPort);
	if (ECERR_SUCCESS != status) {
		FATAL<<"Linkx::Client::Connect failed for "<<routerAddress
				<<" and "<<routerPort<<" with status "<<status;
		return 1;
	}

	//redis::client c(AdminConfig::settings->getRedisIp(),AdminConfig::settings->getRedisPort(),0);
	//RedisWriter::initRedisWriter(AdminConfig::settings->getRedisIp(),AdminConfig::settings->getRedisPort(), AdminConfig::settings->getRedisEnable());
	RedisWriter::isRedisEnabled=AdminConfig::settings->getRedisEnable();


	CLI::init();	//initiate command line interface
//

	//initiating maddog connection
//	STEMMaddogConnection connection(client);

	//Symbol Manager is passed with Maddog Connection to subscribe symbols
	symbol_manager = new SymbolManager(client);
	account_manager = new TradeAccountManager(client);

	boost::asio::io_service io;
	boost::asio::deadline_timer panic_close_timer(io);
	boost::asio::deadline_timer soft_close_timer(io);
	boost::asio::deadline_timer position_file_timer(io);

	boost::gregorian::date today = boost::gregorian::day_clock::local_day();	//current date

	boost::posix_time::time_duration panic_duration = boost::posix_time::duration_from_string(AdminConfig::settings->getPanicCloseTime());
	boost::posix_time::ptime panic_time (today, panic_duration);

	panic_close_timer.expires_at(panic_time);
	panic_close_timer.async_wait(boost::bind(&TradeAccountManager::dayEnd, account_manager));


	boost::posix_time::time_duration soft_duration = boost::posix_time::duration_from_string(AdminConfig::settings->getSoftCloseTime());
	boost::posix_time::ptime soft_time (today, soft_duration);

	soft_close_timer.expires_at(soft_time);
	soft_close_timer.async_wait(boost::bind(&TradeAccountManager::dayEnd, account_manager));

	boost::posix_time::time_duration position_duration = boost::posix_time::duration_from_string(AdminConfig::settings->getPositionFileTime());
	boost::posix_time::ptime position_time (today, position_duration);

	position_file_timer.expires_at(position_time);
	position_file_timer.async_wait(boost::bind(&TradeAccountManager::generatePositionFile, account_manager));


	auto signal_handler = [&io](const boost::system::error_code& err_no, int signum){
		INFO<<"=======+++++++========= Killing STEM =================+++++++++++==";
		INFO<<"==========++====== Canceling all pending orders ===================";

		cout<<"exiting from stem using signal handler";
		account_manager->stop();
		io.stop();
	};

	boost::asio::signal_set signals(io, SIGINT, SIGTERM);
	signals.async_wait(signal_handler);

	io.run();

//	signal(SIGINT, signal_handler);
//	signal(SIGTERM, signal_handler);
//	signal(SIGHUP, signal_handler);

//	while ( !exit_signal ) {
//		this_thread::sleep_for(std::chrono::milliseconds(1000));
//	};

	cout<<" exiting from STEM!!!!!";

	return 0;
}
