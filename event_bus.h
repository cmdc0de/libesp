/*
 * event_bus.h
 *
 *  Created on: Sep 17, 2017
 *      Author: cmdc0de
 */

#ifndef LIBSTM32_OBSERVER_EVENT_BUS_H_
#define LIBSTM32_OBSERVER_EVENT_BUS_H_

#include <stdint.h>
#include <typeinfo>
#include <etl/map.h>
#include <etl/pool.h>
#include <etl/list.h>

namespace libesp {

class SignalRegistration {
public:
	SignalRegistration() {
	}
	virtual void call(void *subject, const void *event)=0;
	virtual ~SignalRegistration() {
	}
	virtual intptr_t getSubjectKey() const =0;
	virtual intptr_t getEventKey() const =0;
	virtual bool isOneShot() const = 0;
	virtual bool compare(const SignalRegistration *r) const =0;
private:
};

//bool ::operator==(SignalRegistration *s1, SignalRegistration *s2){
//	return s1->compare(s2);
//}

template<typename ObserverType, typename EventType, typename SubjectType>
class TypedSignalRegistration: public SignalRegistration {
public:
	TypedSignalRegistration(ObserverType *observer, bool isOneShot) :
			SignalRegistration(), Observer(observer), OneShot(isOneShot) {

	}
	virtual void call(void *subject, const void *event) {
		Observer->receiveSignal(static_cast<SubjectType*>(subject),
				static_cast<const EventType*>(event));
	}
	virtual ~TypedSignalRegistration() {
	}
	virtual intptr_t getSubjectKey() const {
		return typeid(SubjectType).hash_code();
	}
	virtual intptr_t getEventKey() const {
		return typeid(EventType).hash_code();
	}
	virtual bool isOneShot() const {return OneShot;}
	virtual bool compare(const SignalRegistration *r) const {
		const TypedSignalRegistration *tr = static_cast<const TypedSignalRegistration*>(r);
		return tr->Observer == Observer
				&& getSubjectKey() == tr->getSubjectKey()
				&& tr->getEventKey() == getEventKey();
	}
private:
	ObserverType *Observer;
	bool OneShot;
};

//Map<eventHash,Map<subjectHash,List<SignalRegistration*> > >
template<int MAX_NUM_EVENTS, int MAX_EVENT_TYPES, int MAX_SUBJECTS,
		int MAX_LISTENERS_PER_EVENT_PER_SUBJECT = 3>
class EventBus {
public:
	typedef etl::list<SignalRegistration*, MAX_LISTENERS_PER_EVENT_PER_SUBJECT> SIGNAL_LIST;
	typedef typename SIGNAL_LIST::iterator SIGNAL_LIST_IT;
	typedef etl::map<uint32_t, SIGNAL_LIST, MAX_SUBJECTS> MAP_SUB_TO_LISTENER;
	typedef typename MAP_SUB_TO_LISTENER::iterator MAP_SUB_TO_LISTENER_IT;
	//event it to map of subjext to listeners
	typedef etl::map<uint32_t, MAP_SUB_TO_LISTENER, MAX_EVENT_TYPES> EMAP;
	typedef typename EMAP::iterator EMAP_IT;
public:
	EventBus() :
			BusMap() {
	}
	template<typename ObserverT, typename EventT, typename SubjectT>
	void addListener(ObserverT *o, EventT *e, SubjectT *s) {
		addListener(o, e, s, false);
	}
	template<typename ObserverT, typename EventT, typename SubjectT>
	void addListener(ObserverT *o, EventT *e, SubjectT *s, bool isOneShot) {
		TypedSignalRegistration<ObserverT, EventT, SubjectT> *tsr =
				//new (Pool.allocate()) TypedSignalRegistration<ObserverT, EventT,SubjectT>(o, isOneShot);
				new TypedSignalRegistration<ObserverT, EventT,SubjectT>(o, isOneShot);
		intptr_t eventKey = tsr->getEventKey();
		std::pair<EMAP_IT, bool> p = BusMap.insert(std::make_pair(eventKey,MAP_SUB_TO_LISTENER()));
		std::pair<MAP_SUB_TO_LISTENER_IT, bool> p2 =
			(*p.first).second.insert(std::make_pair(((uint32_t)s),SIGNAL_LIST()));
			(*p2.first).second.push_back(tsr);

			}
			template<typename ObserverT, typename EventT, typename SubjectT>
			void removeListener(ObserverT *o, EventT *e, SubjectT *s) {
				TypedSignalRegistration<ObserverT, EventT, SubjectT> tsr(o, false);
				EMAP_IT em = BusMap.find(typeid(EventT).hash_code());
				MAP_SUB_TO_LISTENER_IT it = (*em).second.find(((uint32_t)s));
				SIGNAL_LIST_IT sit = (*it).second.begin();
				for (; sit != (*it).second.end(); ++sit) {
					if((*sit)->compare(&tsr)) {
						break;
					}
				}
				if(sit!=(*it).second.end()) {
					delete (*sit);
					(*it).second.erase(sit);
				}
				//Pool.release(&tsr);
			}
			template<typename T, typename E>
			void emitSignal(T *subject, const E &event) {
				intptr_t eventKey = typeid(E).hash_code();
				EMAP_IT em = BusMap.find(eventKey);
				if(em!=BusMap.end()) {
					MAP_SUB_TO_LISTENER_IT it = (*em).second.find(((uint32_t)subject));
					if(it!=(*em).second.end()) {
						SIGNAL_LIST_IT sit = (*it).second.begin();
						for (; sit != (*it).second.end(); ++sit) {
							bool isOneShot = (*sit)->isOneShot();
							(*sit)->call(subject, &event);
							if(isOneShot) {
								//add iterator to vector to remove at end!!!
							}
						}
					}
				}
			}

		private:
			//just using the size of the TypedSignalRegistration
			//etl::pool<uint64_t, MAX_NUM_EVENTS> Pool;

			EMAP BusMap;
		};

		}
#endif /* LIBSTM32_OBSERVER_EVENT_BUS_H_ */
