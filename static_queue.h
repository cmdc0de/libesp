#ifndef LIBESP_STATIC_QUEUE_H
#define LIBESP_STATIC_QUEUE_H

#include "../../freertos.h"
#include "error_type.h"
#include <etl/set.h>

namespace libesp {

/*
 * C++ wrapper around StaticQueue
 */
template<int MAX_QUEUE_SIZE, typename QueMsgType, int NumListeners>
class StaticQueue {
private:
	static StaticQueue_t InternalQueue;
	static uint8_t InternalQueueBuffer[MAX_QUEUE_SIZE*sizeof(QueMsgType*)];
	typedef etl::set<QueueHandle_t,NumListeners> NotificationSetType;
public:
	static const char *LOGTAG;
public:
	StaticQueue() : InternalQueueHandler(0), NotificationSet() {

	}
	ErrorType init() {
		ErrorType et;
		InternalQueueHandler = xQueueCreateStatic(MAX_QUEUE_SIZE,sizeof(QueMsgType*),&InternalQueueBuffer[0],&InternalQueue);
		if(InternalQueueHandler==nullptr) {
			ESP_LOGE(LOGTAG,"Failed to create queue");
			et =  ErrorType(errno);
		}
		return et;
	}
	~StaticQueue() {}
	bool sendMessageToQueueFromISR(QueMsgType* msg) {
		if(errQUEUE_FULL==xQueueSendFromISR(InternalQueueHandler, &msg, NULL)) {
			return false;
		}
		return true;
	}
	bool sendMessageToQueue(QueMsgType* msg) {
		if(errQUEUE_FULL==xQueueSend(InternalQueueHandler,&msg,0)) {
			return false;
		}
		return true;
	}
	/*
	 * returns the number of messagse boadcasted
	 */
	int broadcastToNotificationSet() {
		QueMsgType* msg;
		int retVal = 0;
		if(xQueueReceive(InternalQueueHandler, &msg, 0)) {
			NotificationSetType::iterator it = NotificationSet.begin();
			for(;it!=NotificationSet.end();++it) {
				ESP_LOGI(LOGTAG,"broadcast");
				QueueHandle_t handle = (*it);
				QueMsgType* msg1 = new QueMsgType(msg);
				if(errQUEUE_FULL==xQueueSend(handle, &msg1, 0)) {
					delete msg1;
				} else {
					++retVal;
				}
			}
			delete msg;
		}
		return retVal;
	}

private:
	QueueHandle_t InternalQueueHandler;
	NotificationSetType NotificationSet;
};

template<int MAX_QUEUE_SIZE, typename QueMsgType, int NumListeners>
const char *StaticQueue<MAX_QUEUE_SIZE,QueMsgType,NumListeners>::LOGTAG = "StaticQueue";

template<int MAX_QUEUE_SIZE, typename QueMsgType, int NumListeners>
StaticQueue_t StaticQueue<MAX_QUEUE_SIZE,QueMsgType,NumListeners>::InternalQueue;

template<int MAX_QUEUE_SIZE, typename QueMsgType, int NumListeners>
uint8_t StaticQueue<MAX_QUEUE_SIZE,QueMsgType,NumListeners>::InternalQueueBuffer[MAX_QUEUE_SIZE*sizeof(QueMsgType*)] = {0};

}

#endif
