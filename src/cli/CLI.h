/*
 * CLI.h
 *
 *  Created on: Apr 19, 2016
 *      Author: jamil
 */

#ifndef CLI_H_
#define CLI_H_

#include <string>
#include <map>
#include <cstddef>
#include <readline/readline.h>
#include <readline/history.h>
#include <boost/algorithm/string.hpp>
#include <boost/thread.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <thread>
#include <chrono>
#include "../utils/log.h"

using namespace std;

#define CLI_READLINE_BUFFER_SIZE 500
#define CLI_MESSAGE_QUEUE_SIZE 100
#define CLI_COMMAND_PROMPT_STRING "stem>"
#define CLI_MESSAGE_QUEUE_IDENTIFIER "stem-message-queue"

typedef vector<string> stringlist;

class CommandCallback {
public:
	virtual ~CommandCallback();
	virtual void operator()(stringlist args) = 0;	//implement this functor to register callback function
};

class CLI {
	boost::interprocess::message_queue *mq;
	char *buffer;
	boost::thread *cli;
	boost::thread *cli_client;
	CLI();
	virtual ~CLI();
	string readCommand();
	static map<string, CommandCallback*> callbackLookup;
 	static CLI *single; 	//singleton for cli
	void readCommandAndDispatch();
	void readCommandAndDispatchFromCLIClient();

public:
	static void init();
	static bool registerCommand(string, CommandCallback *);
};




#endif /* CLI_H_ */
