/*
 * log.cpp
 *
 *  Created on: Apr 23, 2016
 *      Author: jamil
 */


#include "log.h"
#include "../config/AdminConfig.h"



namespace attrs   = boost::log::attributes;
namespace expr    = boost::log::expressions;
namespace logging = boost::log;

boost::log::trivial::severity_level getSeverity(std::string str) {
	if ( str == "trace" ) return boost::log::trivial::trace;
	else if ( str == "debug" ) return boost::log::trivial::debug;
	else if ( str == "info" ) return boost::log::trivial::info;
	else if ( str == "warning" ) return boost::log::trivial::warning;
	else if ( str == "error" ) return boost::log::trivial::error;
	else if ( str == "fatal" ) return boost::log::trivial::fatal;


	return boost::log::trivial::trace;
}

BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", boost::log::trivial::severity_level)

//Defines a global logger initialization routine
BOOST_LOG_GLOBAL_LOGGER_INIT(my_logger, logger_t) {
    logger_t lg;

    AdminConfig::parse("../configs/admin_config.xml");

    logging::formatter fmt = expr::stream<<expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d %H:%M:%S")<<" <"<<severity<<">: "<<expr::smessage;

    boost::shared_ptr<boost::log::core> core = boost::log::core::get();


    typedef boost::log::sinks::synchronous_sink <boost::log::sinks::text_ostream_backend> sink_t;
    boost::shared_ptr<boost::log::sinks::text_ostream_backend> filestream
    	= boost::make_shared<boost::log::sinks::text_ostream_backend>(
   	);


    filestream->auto_flush(true);
    filestream->add_stream(
    		boost::shared_ptr< std::ostream >(new std::ofstream(SYS_LOGFILE)));
    //must create seperate sinks and stream for each sink.
    boost::shared_ptr<sink_t> f_sink(new sink_t(filestream));

    boost::log::add_common_attributes();
    boost::log::register_formatter_factory("TimeStamp", boost::make_shared<TimeStampFormatterFactory>());

    f_sink->set_filter(severity >= getSeverity(AdminConfig::settings->getLogSeverity()));
    f_sink->set_formatter(
    		boost::log::expressions::format("[%1%][%2%] %3%")
           % boost::log::expressions::attr<boost::posix_time::ptime>("TimeStamp")
		   % boost::log::trivial::severity
		   % boost::log::expressions::smessage );

    core->add_sink(f_sink);


    //logging to console or verbosity options
    boost::shared_ptr<boost::log::sinks::text_ostream_backend> consolestream
    	= boost::make_shared<boost::log::sinks::text_ostream_backend> ();
    consolestream->add_stream(boost::shared_ptr<std::ostream>(&std::cout, boost::log::empty_deleter()));
    boost::shared_ptr<sink_t> c_sink(new sink_t(consolestream));

    c_sink->set_filter(severity >= getSeverity(AdminConfig::settings->getVerboseSeverity()) );
    c_sink->set_formatter(fmt);
    core->add_sink(c_sink);


    boost::shared_ptr<boost::log::sinks::text_multifile_backend> mf_backend =
    		boost::make_shared<boost::log::sinks::text_multifile_backend>();

    //setup the file naming pattern
    mf_backend->set_file_name_composer(
    		boost::log::sinks::file::as_file_name_composer(expr::stream<<"../logs/"<<expr::attr<std::string>("TradeInfo")<<".log")
    );

    // Wrap it into the frontend and register in the core.
	// The backend requires synchronization in the frontend.
	typedef boost::log::sinks::synchronous_sink< boost::log::sinks::text_multifile_backend > sync_mf_sink;
	boost::shared_ptr< sync_mf_sink > mf_sink(new sync_mf_sink(mf_backend));

	// Set the formatter
	mf_sink->set_formatter
	(
		expr::stream
			<< "[" << expr::attr< std::string >("TradeInfo")
			<< "] " << expr::smessage
	);

	core->add_sink(mf_sink);

    return lg;
}
