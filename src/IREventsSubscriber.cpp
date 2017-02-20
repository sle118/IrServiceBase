/*
 * IRHandler.cpp
 *
 *      Author: sle118
 *      credits to probonopd for initiating the first draft of the server

 */

#include <Arduino.h>
#include <IREventsSubscriber.h>
IREventsSubscriber::IREventsSubscriber(IRReceivedHandler handler): _handler(handler){
	// TODO Auto-generated constructor stub

}

IREventsSubscriber::~IREventsSubscriber() {
	// TODO Auto-generated destructor stub
}

bool IREventsSubscriber::process(decode_results * results)
{
	return _handler(results);
}
