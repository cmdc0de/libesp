
#ifndef COMPONENTS_CPP_UTILS_SYSTEM_H_
#define COMPONENTS_CPP_UTILS_SYSTEM_H_
#include <stdint.h>
#include <esp_system.h>

namespace libesp {

/**
*  singleton
 */

class System {
public:
	static const System &get();
public:
	void logSystemInfo() const;
	const esp_chip_info_t *getChipInfo() const ;
	size_t getFreeHeapSize() const;
	const char *getIDFVersion() const;
	size_t getMinimumFreeHeapSize() const;
	void restart() const;
public:
	virtual ~System();
protected:
	System();
private:
	static System mSelf;
};

}

#endif /* COMPONENTS_CPP_UTILS_SYSTEM_H_ */
