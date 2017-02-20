/*
 * IrAsyncCommandProcessor.cpp
 *
 *      Author: sle118
 *      credits to probonopd for initiating the first draft of the server

 */

#include <IrAsyncCommandProcessor.h>

IrAsyncCommandProcessor::IrAsyncCommandProcessor(void * callbackData, int bufferLen):
		_callbackData(callbackData)
{

	frequency=38000;
	frequencyKhz=frequency/1000;
	repeat=1;
	offset=0;
	buf = new unsigned int[bufferLen]();

	_bufferLen=bufferLen;
	String uniqueID="";
}
IrAsyncCommandProcessor::~IrAsyncCommandProcessor() {
	delete[] buf;
}

IrAsyncCommandProcessor& IrAsyncCommandProcessor::onComplete(IrAsyncCallbackFunction handlerFunction)
{
	_handler = handlerFunction;
	return *this;
}

bool IrAsyncCommandProcessor::complete()
{
	return _handler(_callbackData);
}
