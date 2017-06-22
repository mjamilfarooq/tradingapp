/*
 * ConfigReader.h
 *
 *  Created on: Apr 17, 2016
 *      Author: jamil
 */

#ifndef CONFIGREADER_H_
#define CONFIGREADER_H_

#include <string>
#include <set>
#include <unordered_map>
#include <map>
#include <memory>
#include <mutex>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <vector>
#include <functional>

#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/foreach.hpp>
#include <boost/exception/all.hpp>
#include <boost/signals2.hpp>

#include <edge.h>
#include <MarketData/Messages.h>

#include "../cli/CLI.h"
#include "../utils/log.h"
#include "../utils/utils.h"


using namespace std;
using boost::property_tree::ptree;


typedef boost::error_info<struct config_error_info, std::string> config_info;
struct ConfigException:public boost::exception, public std::exception {
	const char *what() const noexcept {
		return "configuration error:";
	}
};


//class ConfigReader {
//public:
//
//	ConfigReader(string filename):
//		filename(filename) {
//	}
//
//	virtual ~ConfigReader(){
//	}
//
//protected:
//	string filename;
//	virtual void parse();
//	bool parse_error;
//	ptree pt;
//};

#endif /* CONFIGREADER_H_ */
