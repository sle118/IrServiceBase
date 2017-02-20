/*
 * ServerBase.cpp
 *
 *      Author: sle118
 *      credits to probonopd for initiating the first draft of the server
 */

#include <Arduino.h>
#include <IPAddress.h>
#include <sys/types.h>
#include <IrServiceBase.h>
#include <WString.h>
#include <cstdint>
#include <cstring>


WiFiServer * IrServiceBase::_debug_wifi_server;
uint32_t IrServiceBase::_startHeap = 0;
uint32_t IrServiceBase::prevHeap = 0;
time_t IrServiceBase::prevMs = 0;
int IrServiceBase::freq=38400; // this is used in converting decoded to raw

ETSTimer IrServiceBase::_asyncIRSending;
WiFiClient IrServiceBase::_debug_clients[MAX_SRV_CLIENTS];
IRsend * IrServiceBase::irSend;
IRrecv * IrServiceBase::irRecv;
bool IrServiceBase::debugIsActive = false;
LinkedList<IREventsSubscriber*> IrServiceBase::_irInboundProcessHandlers(
		LinkedList<IREventsSubscriber *>(
				[](IREventsSubscriber *h) {delete h;}));
LinkedList<IrAsyncCommandProcessor*> IrServiceBase::_asyncRawIRCommands(
		LinkedList<IrAsyncCommandProcessor *>(
				[](IrAsyncCommandProcessor * h) {delete h;}));

decode_results IrServiceBase::results;
String IrServiceBase::_debugWelcome;

IrServiceBase::IrServiceBase(int port, int irSendPin, int recvPin, int debugPort,
		const String& debugWelcome) :
		_wifi_server(port), _commandLineProcessor(
				LinkedList<CommandProcessor *>(
						[](CommandProcessor *h) {delete h;})) {
	_port = port;
	setupDebug(debugPort, debugWelcome);
	setupIR(irSendPin, recvPin);

}
IrServiceBase::IrServiceBase(int port, const String& onConnectMessage, int irSendPin,
		int recvPin, int debugPort, const String& debugWelcome) :
		_wifi_server(port), _commandLineProcessor(
				LinkedList<CommandProcessor *>(
						[](CommandProcessor *h) {delete h;})) {
	_onConnectMessage = onConnectMessage;
	setupDebug(debugPort, debugWelcome);
	setupIR(irSendPin, recvPin);

}

void IrServiceBase::setupDebug(int debugPort, const String& debugWelcome) {
	if(_startHeap ==0)
		_startHeap = ESP.getFreeHeap();
	// Ensure static variables are initialized on the first pass
	if (prevMs == 0)
		prevMs = millis();
	if (prevHeap == 0)
		prevHeap = ESP.getFreeHeap();
	if (!isDebugActive() && debugPort > 0) {
		_debugWelcome = debugWelcome;
		_debug_wifi_server = new WiFiServer(debugPort);
		debugIsActive = true;
		_debug_wifi_server->begin();
	}
}

void IrServiceBase::begin() {
	_wifi_server.begin();
	_wifi_server.setNoDelay(true);
	if (irRecv) {
		debugSend("Starting irReceive");
		irRecv->enableIRIn();
	}
	if (irSend) {
		debugSend("Starting irSend");
		irSend->begin();
	}
	OnBegin();
}
IREventsSubscriber& IrServiceBase::registerEvents(
		IRReceivedHandler handlerFunction) {
	IREventsSubscriber* handlerClass = new IREventsSubscriber(handlerFunction);
	_irInboundProcessHandlers.add(handlerClass);

	return *handlerClass;
}
CommandProcessor & IrServiceBase::registerCommand(const char * commandName,
		CommandProcessorHandler handlerFunction)
{
	registerCommand(commandName,0,handlerFunction);
}
CommandProcessor & IrServiceBase::registerCommand(const char * commandName, int argsOffset,
		CommandProcessorHandler handlerFunction) {
	CommandProcessor* processor = new CommandProcessor(commandName,
			handlerFunction, argsOffset);
	debugSend("Registered new command " + String(commandName));
	_commandLineProcessor.add(processor);
	return *processor;
}
void IrServiceBase::dump(decode_results *results) {
	for (int i = 0; i < results->rawlen; i++) {
		if (i & 1) {
			stringDecode += results->rawbuf[i] * USECPERTICK * freq / 1000000- 1, DEC;
		} else {
			stringDecode += (unsigned long) results->rawbuf[i] * USECPERTICK * freq / 1000000 - 1, DEC;
		}
		stringDecode += ",";
	}

}

void IrServiceBase::dump_protocol(decode_results *results) {
	if (results->decode_type != UNKNOWN) {
		stringDecode = "Decoded type ";
		stringDecode += String(results->decode_type);
		stringDecode += ", bits ";
		stringDecode += String(results->bits);
		stringDecode += ", value ";
		stringDecode += String(results->value, HEX);
		if (results->sharpAddress > 0) {
			stringDecode += ", address ";
			stringDecode += String(results->sharpAddress);
		}
	} else {
		stringDecode = "Unknown";
	}
}

#define MAX_IR_JOB_DURATION 300
#define IR_TIMER_PERIOD 400
static void async_ir_callback(void *arg) {
	IrServiceBase::processAsyncIRCommands(millis()+MAX_IR_JOB_DURATION );
}
void IrServiceBase::addAsyncIRCommand(IrAsyncCommandProcessor* asyncCommand)
{
	os_timer_disarm(&_asyncIRSending);
	debugSend("Queuing IR Command");
	DEBUG_PRINTF("Adding async processor. Len is %3d, frequency %5d, frequencyKhz %4d.\nSendRaw Starts.\n", asyncCommand->Length(), asyncCommand->frequency,asyncCommand->frequencyKhz);
	_asyncRawIRCommands.add(asyncCommand);
	os_timer_arm(&_asyncIRSending, IR_TIMER_PERIOD, 0);
}
void IrServiceBase::setupIR(int irSendPin, int recvPin) {
	if (irSend == NULL && irSendPin > 0) {
		irSend = new IRsend(irSendPin);
		os_timer_disarm(&_asyncIRSending);
		os_timer_setfn(&_asyncIRSending, (os_timer_func_t *)async_ir_callback,(void*) &_asyncIRSending);
		os_timer_arm(&_asyncIRSending, IR_TIMER_PERIOD, 0);
	}
	if (irRecv == NULL && recvPin > 0) {
		irRecv = new IRrecv(recvPin);
	}
}

void IrServiceBase::removeAsyncIRCommand(String& uniqueID) {
	for (const auto& ah : _asyncRawIRCommands) {
		if (ah->uniqueID == uniqueID)
		{
			_asyncRawIRCommands.remove(ah);
			delete &ah;
		}
	}
}
void IrServiceBase::processAsyncIRCommands(time_t timeOut){
	// process a single async command on each pass
	// to avoid timeouts
	// todo: implement this as a timer?
	//
	if (_asyncRawIRCommands.length() > 0) {
		DEBUG_PRINT("IR Signals were scheduled. Disabling the Async IR timer and IR Receiver.\n");
		os_timer_disarm(&_asyncIRSending);
		irRecv->disableIRIn();


		DEBUG_PRINT("Getting the first signal to send.\n");
		IrAsyncCommandProcessor* ah = _asyncRawIRCommands.front();
		DEBUG_PRINTF("Len is %3d, frequency %5d, frequencyKhz %4d.\nSendRaw Starts.\n", ah->Length(), ah->frequency,ah->frequencyKhz);
		debugSend("Sending raw IR codes");
		irSend->sendRaw(ah->buf, ah->Length(), ah->frequencyKhz);
		ah->repeat--;
		DEBUG_PRINTF("SendRaw completed, repeat is now %d.\n",ah->repeat);

		while(ah->repeat >0 && millis()<=timeOut)
		{
			// send the repeat sequence for as long as we're not timing out
			// if we time out, the command will be sent again with the full
			// preamble as needed
			irSend->sendRaw(&(ah->buf[ah->offset]), ah->Length()-ah->offset, ah->frequencyKhz);
			ah->repeat--;
			DEBUG_PRINTF("SendRaw (repeat) completed, repeat is now %d.\n",ah->repeat);
		}

		if(ah->repeat == 0){

			debugSend("Done Sending raw IR codes");
			DEBUG_PRINT("All repeats have been sent. Calling complete().\n");
			ah->complete();
			DEBUG_PRINT("done calling complete(). Removing the async IR event from the list.\n");
			_asyncRawIRCommands.remove(ah);
			DEBUG_PRINT("done removing the async IR event from the list.\n");
		}
		else
		{
			DEBUG_PRINT("Could not send all repeats. Will continue on the next pass. \n");
		}
		DEBUG_PRINT("Enabling IR Receiver.\n");
		irRecv->enableIRIn();

	}
	if (_asyncRawIRCommands.length() > 0){
		os_timer_arm(&_asyncIRSending, IR_TIMER_PERIOD, 0);
		DEBUG_PRINT("Async IR timer armed.\n");
	}

}

void IrServiceBase::processDecodeIrReceive() {
	// first process any outstanding IR
	// signal from the buffer
	if (irRecv && irRecv->decode(&results)) {
		debugSend("decoded IR signal, length is "+String(results.rawlen));
		for (const auto& h : _irInboundProcessHandlers) {
			h->process(&results);
		}
		irRecv->resume(); // restart receiver
	}
}

bool IrServiceBase::processClient(WiFiClient& client) {

	bool gotData = false;
	bool foundProcessor = false;
	if (client.connected() && client.available()) {
		String inData = getNextLineFromClient(client);
		inData.trim();
		if (inData.length() == 0) {
			debugSend("Empty line received, ignored");
		} else {
			for (const auto& c : _commandLineProcessor) {
				if (c->isMatch(inData)) {
					debugSend("New command :" + String(inData));
					c->process(client, inData);
					foundProcessor = true;
				}
			}
			gotData = true;
			if (!foundProcessor) {
				debugSend("unknown command :"+ String(inData));
				OnUnknownCommand(inData, client);
			}
		}
	}
	return gotData;
}

void IrServiceBase::process(time_t timeOutMs) {
	_timeOutMs = timeOutMs + millis();
	time_t start = millis();
	//processAsyncIRCommands(_timeOutMs);
	OnProcess();
	bool gotData = false;
	for (uint8_t i = 0; i < MAX_SRV_CLIENTS && !timeIsOut(); i++) {
		if (_clients[i] && _clients[i].connected() && _clients[i].available()) {
			processClient(_clients[i]);
		}
	}

}
String IrServiceBase::getNextLineFromClient(WiFiClient& client) {
	char szReadBuffer[SERVERBASE_READ_BUFFER]={0};
	char * p = &szReadBuffer[0];
	char * e = &szReadBuffer[SERVERBASE_READ_BUFFER-2]; // guard

	String inData="";
	while (client.available() > 0) {
		*p = client.read();
		bool bIsEndOfLine = (*p == '\n' || *p == '\r');
		if (bIsEndOfLine ) // Actually we get "\n" from IRScrutinizer
		{
			*p = '\0';
			inData += szReadBuffer;
			return inData;
		}else if(p==e)
		{
			// reached the end of buffer, so store the
			// content of the buffer and reset
			inData += szReadBuffer;
			memset(szReadBuffer,0,sizeof(szReadBuffer));
			p = &szReadBuffer[0];
		}
		else
			p++;
	}

	return inData;
}
bool IrServiceBase::timeIsOut() {
	return millis() > _timeOutMs;
}
void IrServiceBase::handleNewConnections() {
	handleNewConnectionsForServer(_wifi_server, _clients);
	if (isDebugActive())
		handleNewConnectionsForServer(*(_debug_wifi_server), _debug_clients);

}
void IrServiceBase::handleNewConnectionsForServer(WiFiServer& wifi_server, WiFiClient wifi_client[]) {
	// Check if there are any new debug clients
	for (uint8_t i = 0; wifi_server.hasClient() && i < MAX_SRV_CLIENTS; i++) {
		if (!wifi_client[i] || !wifi_client[i].connected()) {
			// Ensure that client is stopped if no longer connected
			if (wifi_client[i])
				wifi_client[i].stop();
			// Grab new client
			wifi_client[i] = wifi_server.available();
			debugSend("New client connected!");
			// Send welcome
			if (_onConnectMessage.length() > 0)
				send(wifi_client[i], _onConnectMessage);
		}
	}
	rejectConnectionsForServer(_wifi_server);
}

IrServiceBase::~IrServiceBase() {
	// TODO Auto-generated destructor stub
}
void IrServiceBase::rejectConnectionsForServer(WiFiServer& wifi_server) {
	// No free/disconnected spot so reject connection
	while (wifi_server.hasClient()) {
		debugSend("Rejecting client connection");
		WiFiClient tempClient = wifi_server.available();
		tempClient.stop();
	}
}

String IrServiceBase::getNthToken(unsigned int itemNumber, const String& data,	bool * found, const char separator)
{
	long position = 0;
	long currentItem = 0;
	String result = "";
	*found = false;
	if (itemNumber == 0 || data.length()==0)
		return result;
	while (currentItem < itemNumber && position >= 0) {
		result = getNextToken(data, &position, separator);
		if (position > 0) {
			currentItem++;
		}
	}
	if (currentItem == itemNumber) {
		*found = true;
	}
	return result;
}
bool operator ==(WiFiClient& clienta, WiFiClient& clientb) {

	return  // the quickest is to check if either clients are not connected
			 // which in the context where these
	!(!clienta.connected() || !clientb.connected())
			||

			((clienta.remoteIP() == clientb.remoteIP()
					&& clienta.remotePort() == clientb.remotePort()
					&& clienta.localIP() == clientb.localIP()
					&& clienta.localPort() == clientb.localPort()));
}
String IrServiceBase::getNextToken(const String& data, long *position,
		const char separator) {
	long startPosition = *position;
	if(data.length()!=0 && startPosition<=data.length())
	{
		unsigned int end = data.indexOf(separator, startPosition);
		if (end > 0) {
			*position = end + 1;
			return data.substring(startPosition, end);
		}
	}
	*position = -1;
	return "";
}
#if defined(DEBUG_LEVEL_VERBOSE)
char __szDebugPrintBuffer[255]={0};
#endif
