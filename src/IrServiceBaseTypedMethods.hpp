/*
 * ServerBaseTypedMethods.hpp
 *
 *      Author: sle118
 *      credits to probonopd for initiating the first draft of the server

 */

#ifndef SRC_SERVERBASETYPEDMETHODS_HPP_
#define SRC_SERVERBASETYPEDMETHODS_HPP_

#define SEND_DEBUG_ALL(data,clients,supp)\
		{ sendToAll(data, clients,supp);		totalLength+=Serial.print(data);}
#define DEBUG_STATUS_SIZE 51

template<typename T>
void IrServiceBase::debugSend(const T data, bool suppressCR) {
	char szMessageBuffer[DEBUG_STATUS_SIZE+1]={0};
	uint32_t currHeap = ESP.getFreeHeap();
	long heapDiff = currHeap - prevHeap;
	time_t timeDiff = millis() - prevMs;
	size_t totalLength=0;

	long percentFree = currHeap*100/_startHeap;
	char sign = heapDiff<0?'-':' ';
	int percentDelta = abs(heapDiff)*10000/_startHeap; // calculate the percent with 2 more digits so we can extract them with a mod operation below
	uint8_t decimals = percentDelta % 100;
	percentDelta = percentDelta /100; // adjust percent delta to 100 basis

	snprintf(szMessageBuffer,sizeof(szMessageBuffer)-1,"[FH:%5d %%%2d-CH:%5d %%%c%02d.%02d-ET:%5d] :", currHeap, percentFree, heapDiff,
			sign,percentDelta,decimals,
			timeDiff);

	if (isDebugActive()) {
		SEND_DEBUG_ALL(szMessageBuffer, _debug_clients, true);
		SEND_DEBUG_ALL(data, _debug_clients, false);
		Serial.println("");
	}
	// save values for next pass
	prevHeap = ESP.getFreeHeap();
	prevMs = millis();

}
template<typename T>
inline void IrServiceBase::send(WiFiClient& client,
		const T data, bool suppressCR) {
	if (client.connected()) {
		client.print(data);
		if (!suppressCR)
			client.print("\r\n");
		if (!isDebugClient(client)) {
			debugSend("[send to a client ]" + String(data), false);
		}
	}
}

template<typename T>
void IrServiceBase::sendToAll(const T data,
		bool suppressCR) {
	sendToAll(data, _clients, suppressCR);
}

template<typename T>
void IrServiceBase::sendToAll(const T data,
		WiFiClient wifi_clients[], bool suppressCR) {
	for (uint8_t client = 0; client < MAX_SRV_CLIENTS; client++) {
		if (wifi_clients[client])
			send(wifi_clients[client], data, suppressCR);
	}
}

#endif /* SRC_SERVERBASETYPEDMETHODS_HPP_ */
