/*
 * CLI.cpp
 *
 *  Created on: Apr 19, 2016
 *      Author: jamil
 */

#include 	"CLI.h"

CLI *CLI::single = nullptr;
map<string, CommandCallback*> CLI::callbackLookup;

CommandCallback::~CommandCallback() {

}

CLI::CLI() {

	buffer = new char[CLI_READLINE_BUFFER_SIZE];

	//initiate message queue for IPC
	try {
		boost::interprocess::message_queue::remove(CLI_MESSAGE_QUEUE_IDENTIFIER);
		mq = new boost::interprocess::message_queue (
				boost::interprocess::create_only,
				CLI_MESSAGE_QUEUE_IDENTIFIER,
				CLI_MESSAGE_QUEUE_SIZE,
				CLI_READLINE_BUFFER_SIZE
		);
	} catch ( boost::interprocess::interprocess_exception &e ) {
		ERROR<<e.what();
		WARN<<"Can't allocate shared memory for STEM CLI client";
		boost::interprocess::message_queue::remove(CLI_MESSAGE_QUEUE_IDENTIFIER);
		mq = nullptr;
	}

	cli = new boost::thread(&CLI::readCommandAndDispatch, this);
	cli_client = new boost::thread(&CLI::readCommandAndDispatchFromCLIClient, this);
}

void CLI::init() {
	if ( single == nullptr ) {
		single = new CLI();
	} else {
		cout<<"cli instance already exist "<<endl;
	}
}


CLI::~CLI() {
	if ( nullptr != mq ) {
		boost::interprocess::message_queue::remove(CLI_MESSAGE_QUEUE_IDENTIFIER);
	}
}

/*
 * this should read commands and dispatch to appropriate command handler
 */
void CLI::readCommandAndDispatch() {
	string command;
	while (true) {
//		cout<<readCommand()<<endl;
		command = readCommand();
		if ( command.empty() ) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}
		stringlist split_vector;
		boost::algorithm::split(split_vector,
				command,
				boost::algorithm::is_any_of(" "));
		//calling appropriate callback and passing parsed commands to it.
		if ( callbackLookup.find(split_vector[0]) != callbackLookup.end() )
			(*callbackLookup[split_vector[0]])(split_vector);
		else ERROR<<"Unrecognized command";

	}
}

void CLI::readCommandAndDispatchFromCLIClient() {
	char message_buffer[CLI_READLINE_BUFFER_SIZE];
	while( true ) {
		boost::interprocess::message_queue::size_type message_size;
		unsigned int priority;
		mq->receive(message_buffer, CLI_READLINE_BUFFER_SIZE, message_size, priority);
		char *temp = strndup(message_buffer, message_size);
		TRACE<<temp;
		string command = temp;
		boost::algorithm::trim(command);
		DEBUG<<"Command from Client:"<<command<<" message size:priority "<<message_size<<":"<<priority;
		if ( command.empty() ) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}
		stringlist split_vector;
		boost::algorithm::split(split_vector,
				command,
				boost::algorithm::is_any_of(" "));
		//calling appropriate callback and passing parsed commands to it.
		if ( callbackLookup.find(split_vector[0]) != callbackLookup.end() )
			(*callbackLookup[split_vector[0]])(split_vector);
		else ERROR<<"Unrecognized command";

	}
}


string CLI::readCommand() {

	buffer = readline(CLI_COMMAND_PROMPT_STRING);
	if ( nullptr == buffer ) {
//		TRACE<<"CLI::readCommand: null buffer received";
		return "";
	}

	string readline(buffer);
	boost::algorithm::trim(readline);
	if ( !readline.empty() ) {
		//place commad in history to be used later
		add_history(readline.c_str());
		return readline;
	}

	return "";

}

bool CLI::registerCommand(string command, CommandCallback *callback) {
	if ( nullptr == callback ) {
		//TODO: log this event;
		TRACE<<"CLI::registerCommand with no callback";
		return false;
	}
	//if ( callbackLookup.key_comp() != command  )
	//TODO: check if key already register against some callback log this event
	callbackLookup[command] = callback;
	return true;
}
