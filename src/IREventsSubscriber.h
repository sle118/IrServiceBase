/*
 * IRHandler.h
 *
 *      Author: sle118
 *      credits to probonopd for initiating the first draft of the server

 */

#ifndef SRC_IREVENTSSUBSCRIBER_H_
#define SRC_IREVENTSSUBSCRIBER_H_
#include <IRremoteESP8266.h>
#include <functional>

typedef std::function<bool(decode_results * decode)> IRReceivedHandler;
class IREventsSubscriber {
public:
	IREventsSubscriber(IRReceivedHandler handler);
	bool process(decode_results * results);
	virtual ~IREventsSubscriber();
private:
	IRReceivedHandler _handler;
};


#endif /* SRC_IREVENTSSUBSCRIBER_H_ */
