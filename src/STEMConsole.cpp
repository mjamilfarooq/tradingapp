//============================================================================
// Name        : STEMConsole.cpp
// Author      : Muhammad Jamil Farooq
// Version     :
// Copyright   : riskapplication.com
// Description : STEM Console Application to talk to STEM

//============================================================================



#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <readline/readline.h>
#include <readline/history.h>


using namespace std;
using namespace boost::interprocess;

#define CLI_COMMAND_PROMPT_STRING "stem console>"
#define CLI_MESSAGE_QUEUE_IDENTIFIER "stem-message-queue"

int main ()
{
   try{
      //Open a message queue.
      message_queue mq (open_only, CLI_MESSAGE_QUEUE_IDENTIFIER);
      unsigned int priority;
      message_queue::size_type recvd_size;

      //Receive 100 numbers
      while(true){

		string  buffer = readline(CLI_COMMAND_PROMPT_STRING);

		boost::algorithm::trim(buffer);


		if ( !buffer.empty() ) {
			//place commad in history to be used later
			add_history(buffer.c_str());
		}

		if ( buffer == "exit" ) return 0;

		mq.send(buffer.c_str(), buffer.size(), 0);
		cout<<"sending ... "<<buffer<<":"<<buffer.size()<<endl;
      }
   } catch(interprocess_exception &ex){
      message_queue::remove("message_queue");
      std::cout <<"Message queue error: "<< ex.what() << std::endl;
      return 1;
   }
   message_queue::remove("message_queue");
   return 0;
}
