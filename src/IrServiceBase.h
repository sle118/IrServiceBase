/*
 * ServerBase.h
 *
 *      Author: sle118
 *      credits to probonopd for initiating the first draft of the server
 */

#ifndef SRC_IrServiceBase_H_
#define SRC_IrServiceBase_H_
#include <IREventsSubscriber.h>
#include <Arduino.h>
#include "WiFiServer.h"
#include "WiFiClient.h"
#include "IRremoteESP8266.h"
#include <ProntoHex.h>
#include <functional>
#include <ESPAsyncWebServer.h>
#include "StringArray.h"
#include "IrAsyncCommandProcessor.h"
#include "Print.h"

extern "C" {
	#include "user_interface.h"
}

#if defined(DEBUG_LEVEL_VERBOSE)
extern char __szDebugPrintBuffer[255];
#define DEBUG_PRINT_HEADER 					Serial.printf("[%s][%d]",__FILE__,__LINE__);
#define DEBUG_PRINT(desc) 					DEBUG_PRINT_HEADER Serial.print(desc)
#define DEBUG_PRINTF(format,...) 			DEBUG_PRINT_HEADER  Serial.printf(format,__VA_ARGS__)
#define DEBUG_PRINTF_NO_HEADER(format,...) 	Serial.printf(format,__VA_ARGS__)
#define DEBUG_RESET_WDT_IF_PRINT			wdt_reset()
#define DEBUG_SPRINTF(format,...)			snprintf(__szDebugPrintBuffer, sizeof(__szDebugPrintBuffer)-1,format,__VA_ARGS__)
#define DEBUG_SPRINTF_BUFFER				__szDebugPrintBuffer


#else
#define DEBUG_PRINT_HEADER
#define DEBUG_PRINT(desc)
#define DEBUG_PRINTF(format,...)
#define DEBUG_PRINTF_NO_HEADER(format,...)
#define DEBUG_RESET_WDT_IF_PRINT
#define DEBUG_SPRINTF(format,...)
#define DEBUG_SPRINTF_BUFFER				NULL
#endif

bool operator ==(WiFiClient& clienta, WiFiClient& clientb);

#define MAX_SRV_CLIENTS 10 // How many clients may connect at the same time
#define SERVERBASE_READ_BUFFER 31
class CommandProcessor;
class IREventsSubscriber;

typedef std::function<bool(WiFiClient& client, const String& args)> CommandProcessorHandler;


class IrServiceBase {
public:
	IrServiceBase(int port, int irSendPin, int recvPin, int debugPort=0, const String& debugWelcome="");
	IrServiceBase(int port, const String& onConnectMessage, int irSendPin, int recvPin, int debugPort=0, const String& debugWelcome="");
	virtual ~IrServiceBase();
	void handleNewConnections();
	void handleNewConnectionsForServer(WiFiServer& wifi_server, WiFiClient wifi_client[]);
	void handle(WiFiServer& wifi_server, WiFiClient& wifi_client);
	void begin();
	void process(time_t timeOutMs);
	template<typename T> static void send(WiFiClient& client, const T data, bool suppressCR=false);
	template<typename T> void sendToAll(const T data, bool suppressCR = false);
	template<typename T> static void sendToAll(const T  data, WiFiClient wifi_client[],bool suppressCR=false);
	template<typename T> static void debugSend(const T data, bool suppressCR=false);

	int Port(){return _port;}
	static void processDecodeIrReceive();
	String getNextToken(const String& data, long *position, const char separator=',');
	String getNthToken(unsigned int itemNumber, const String& data, bool * found, const char separator=',');
	static void processAsyncIRCommands(time_t timeOut);

private:
	// Internal server logic
	String _onConnectMessage;
	time_t _timeOutMs = 0;
	int _port;
	int getClient(Client& client);
	void rejectConnectionsForServer(WiFiServer& wifi_server);
	static LinkedList<IREventsSubscriber*>_irInboundProcessHandlers;
	static LinkedList<IrAsyncCommandProcessor*>_asyncRawIRCommands;

	static void removeAsyncIRCommand(String& uniqueID);

	LinkedList<CommandProcessor *> _commandLineProcessor;
	WiFiServer _wifi_server;
	WiFiClient _clients[MAX_SRV_CLIENTS];
	static WiFiServer * _debug_wifi_server;
	static uint32_t _startHeap;
	static uint32_t prevHeap;
	static time_t prevMs;
	static ETSTimer _asyncIRSending;
	static bool debugIsActive;
	static WiFiClient _debug_clients[MAX_SRV_CLIENTS];
	bool processClient(WiFiClient& client);
	String getNextLineFromClient(WiFiClient& client);

	virtual void OnBegin(){}
	virtual void OnProcess(){}
	virtual void OnUnknownCommand(String& inData,WiFiClient& client){}

	static void setupDebug(int debugPort, const String& debugWelcome);
	static void setupIR(int irSendPin, int recvPin);
	static bool isDebugActive(){return (debugIsActive);}
	static bool isDebugClient(WiFiClient& client )
	{
		if(_debug_clients==NULL) return false;
		for(int i=0;i<MAX_SRV_CLIENTS;i++)
		{
			if(client==_debug_clients[i])
			{
				return true;
			}
		}
		return false;
	}
	static String _debugWelcome;

protected:
	// IR related declarations
	void dump(decode_results *results);
	void dump_protocol(decode_results *results);
	static IRsend * irSend;
	static IRrecv * irRecv;
	ProntoHex ph;
	static IREventsSubscriber& registerEvents(IRReceivedHandler handlerFunction);
	CommandProcessor & registerCommand(const char * commandName, int argsOffset,CommandProcessorHandler handlerFunction);
	CommandProcessor & registerCommand(const char * commandName, CommandProcessorHandler handlerFunction);
	bool timeIsOut();
	static void addAsyncIRCommand(IrAsyncCommandProcessor* asyncCommand);

	static int freq;
	static decode_results results;
	String stringDecode;

};



class CommandProcessor{
  protected:
    String _commandName;
    CommandProcessorHandler _handler;
    int _argsOffset;
  public:
    CommandProcessor(const char * commandName, CommandProcessorHandler handler, int argsOffset=0) : _commandName(commandName), _handler(handler),_argsOffset(argsOffset>0?argsOffset:_commandName.length()+1) {}
    ~CommandProcessor(){}
    bool isMatch(String& commandLine) const { commandLine.trim(); return commandLine.startsWith(_commandName); }
    bool process(WiFiClient& client, String& commandLine) { return _handler(client, commandLine.substring(_argsOffset-1));  }

};


#include "IrServiceBaseTypedMethods.hpp"
#endif /* SRC_IrServiceBase_H_ */
