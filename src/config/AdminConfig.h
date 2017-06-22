/*
 * AdminConfig.h
 *
 *  Created on: Apr 19, 2016
 *      Author: jamil
 */

#ifndef ADMINCONFIG_H_
#define ADMINCONFIG_H_

#include "ConfigReader.h"

class AdminSettings {
	std::string panic_close_time;
	std::string soft_close_time;
	std::string log_severity;
	std::string verbose_severity;
	std::string positionfile_time;
	int redis_port;
	std::string redis_ip;
	bool redis_enable;
public:
	AdminSettings(boost::property_tree::ptree &pt) {
		panic_close_time = pt.get_child("panic_close_time").get_value<string>();
		soft_close_time = pt.get_child("soft_close_time").get_value<string>();
		log_severity = pt.get_child("log").get_child("<xmlattr>").get_child("severity").get_value<string>();
		verbose_severity = pt.get_child("verbose").get_child("<xmlattr>").get_child("severity").get_value<string>();
		positionfile_time=pt.get_child("generate_positionfile_time").get_value<string>();
		redis_ip=pt.get_child("redis").get_child("<xmlattr>").get_child("ip").get_value<string>();
		redis_port=pt.get_child("redis").get_child("<xmlattr>").get_child("port").get_value<int>();
		redis_enable=pt.get_child("redis").get_child("<xmlattr>").get_child("enable").get_value<bool>();
		cout<<"AdminSettings: panic close time (UTC) "<<panic_close_time<<endl;
		cout<<"AdminSettings: soft close time (UTC) "<<soft_close_time<<endl;
		cout<<"AdminSettings: log severity "<<log_severity<<endl;
		cout<<"AdminSettings: verbose severity "<<verbose_severity<<endl;
		cout<<"AdminSettings: position file time (UTC)  "<<positionfile_time<<endl;
		cout<<"AdminSettings: Redis Server Ip "<<redis_ip<<endl;
		cout<<"AdminSettings: Redis Server Port "<<redis_port<<endl;
		cout<<"AdminSettings: Redis Server Enabled "<<redis_enable<<endl;
	}

	std::string getPanicCloseTime() {
		return panic_close_time;
	}

	std::string getSoftCloseTime() {
		return soft_close_time;
	}

	std::string getPositionFileTime() {
			return positionfile_time;
	}

	std::string getLogSeverity() {
		return log_severity;
	}

	std::string getVerboseSeverity() {
		return verbose_severity;
	}

	std::string getRedisIp() {
		return redis_ip;
	}

	int getRedisPort() {
		return redis_port;
	}

	bool getRedisEnable() {
		return redis_enable;
	}


};

class AdminConfig {
public:

	static AdminSettings *settings;

	virtual ~AdminConfig(){
	}

	static void parse(std::string filename) {
		static std::once_flag only_once;
		std::call_once(only_once, [filename, settings](){
			ptree pt;
			boost::property_tree::xml_parser::read_xml(filename, pt);
			int count = pt.count("admin");
			if ( 0 == count  ) BOOST_THROW_EXCEPTION(ConfigException()<<config_info("Missing <trading> tag"));
			else if ( 1 < count ) BOOST_THROW_EXCEPTION(ConfigException()<<config_info("More than one definition of <trading>"));

			ptree &admin = pt.get_child("admin");
			settings = new AdminSettings(admin);
			if ( nullptr == settings )  BOOST_THROW_EXCEPTION(ConfigException()<<config_info("failed to allocate memory for user trading table"));
		});

	}


private:
	AdminConfig() {

	}
};

#endif /* ADMINCONFIG_H_ */
