#pragma once

#include <etl/set.h>
#include "freertos.h"

namespace libesp {

template<size_t QDepth = 3>
class ObserverBase {
public:
   ObserverBase() : Notifications() {}
public:
	/*
	* add / remove observers
	*/
	bool addObserver(const QueueHandle_t &o) {
	   return Notifications.insert(o).second;
   }
	bool removeObserver(const QueueHandle_t &o) {
	   return Notifications.erase(o)==1;
   }
   //returns number of itesm broadcasted or -1 if error
   int32_t broadcast() {
      return onBroadcast();
   }
   virtual ~ObserverBase() {}
protected:
   virtual int32_t onBroadcast()=0;
protected:
   template<typename MType>
   bool broadcast( MType * m) {
      typename etl::set<QueueHandle_t,QDepth>::iterator it = Notifications.begin();
		for(;it!=Notifications.end();++it) {
         xQueueHandle handle = (*it);
         if(errQUEUE_FULL==xQueueSend(handle, &m, 0)) {
            return false;
         }
      }
      return true;
   }
protected:
	etl::set<QueueHandle_t,QDepth> Notifications;
};

}
