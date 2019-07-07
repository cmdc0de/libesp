#ifndef _LIBESP_APP_H
#define _LIBESP_APP_H

#include "../error_type.h"

namespace libesp {

class BaseMenu;

class App {
public:
	ErrorType init();
	ErrorType run();
protected:
	virtual ErrorType onInit()=0;
	virtual ErrorType onRun()=0;
	BaseMenu *getCurrentState() {return CurrentState;}
	App();
	virtual ~App() {}
	void setCurrentState(BaseMenu *cs) {CurrentState = cs;}
private:
	BaseMenu *CurrentState;
	uint32_t LastRunTime;
	uint32_t LastRunPerformance;
};

}
#endif
