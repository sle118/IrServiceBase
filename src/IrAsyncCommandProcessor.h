/*
 * IrAsyncCommandProcessor.h
 *
 *      Author: sle118
 *      credits to probonopd for initiating the first draft of the server

 */

#ifndef SRC_IRASYNCCOMMANDPROCESSOR_H_
#define SRC_IRASYNCCOMMANDPROCESSOR_H_
#include <functional>
#include "Arduino.h"


typedef std::function<bool(void * dataptr)> IrAsyncCallbackFunction;
class IrAsyncCommandProcessor {
public:
	~IrAsyncCommandProcessor();
	IrAsyncCommandProcessor(void * callBackData, int bufferLen);
	long frequency; //<frequency> is |15000|15001|….|500000| (in hertz)
	unsigned int frequencyKhz; //<frequency> is |15000|15001|….|500000| (in hertz)
	int repeat; //<repeat> is |1|2|….|50| (2) (the IR command is sent <repeat> times) max is 50
	int offset; // <offset> is |1|3|5|….|383| (3) (used if <repeat> is greater than 1)
	//<on1> is |1|2|…|65635| (4) (number of pulses)
	//<off1> is |1|2|…|65635| (4) (absence of pulse periods of the carrier frequency)
	unsigned int *buf;
	String uniqueID;
	IrAsyncCommandProcessor& onComplete( IrAsyncCallbackFunction callbackFunction);
	bool complete();
	int Length(){return _bufferLen;}

private:
	IrAsyncCallbackFunction _handler;
	void * _callbackData;
	int _bufferLen;
};

#endif /* SRC_IRASYNCCOMMANDPROCESSOR_H_ */
