/*
 * log.h
 *
 *  Created on: Apr 23, 2016
 *      Author: jamil
 */

#ifndef LOG_H_
#define LOG_H_

#pragma once

#include <fstream>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup.hpp>
#include <boost/log/utility/empty_deleter.hpp>
#include <boost/log/sinks/text_multifile_backend.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>


#define TRACE BOOST_LOG_SEV(my_logger::get(), boost::log::trivial::trace)
#define DEBUG BOOST_LOG_SEV(my_logger::get(), boost::log::trivial::debug)
#define INFO  BOOST_LOG_SEV(my_logger::get(), boost::log::trivial::info)
#define WARN  BOOST_LOG_SEV(my_logger::get(), boost::log::trivial::warning)
#define ERROR BOOST_LOG_SEV(my_logger::get(), boost::log::trivial::error)
#define FATAL BOOST_LOG_SEV(my_logger::get(), boost::log::trivial::fatal)

BOOST_LOG_ATTRIBUTE_KEYWORD(TradeInfo, "TradeInfo", std::string)

#define SYS_LOGFILE             "stem.log"

//Narrow-char thread-safe logger.
typedef boost::log::sources::severity_logger_mt<boost::log::trivial::severity_level> logger_t;

//declares a global logger with a custom initialization
BOOST_LOG_GLOBAL_LOGGER(my_logger, logger_t)

class TimeStampFormatterFactory :
  public boost::log::basic_formatter_factory<char, boost::posix_time::ptime>
{
public:
  formatter_type create_formatter(const boost::log::attribute_name& name, const args_map& args) {
    args_map::const_iterator it = args.find("format");
    if (it != args.end()) {
      return boost::log::expressions::stream
        << boost::log::expressions::format_date_time<boost::posix_time::ptime>(
             boost::log::expressions::attr<boost::posix_time::ptime>(name), it->second);
    } else {
      return boost::log::expressions::stream
        << boost::log::expressions::attr<boost::posix_time::ptime>(name);
    }
  }
};

#endif /* LOG_H_ */
