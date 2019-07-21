#ifndef _LIBESP_NVS_H
#define _LIBESP_NVS_H

#include <stdint.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <nvs.h>
#include "error_type.h"

namespace libesp {

class NVS {
public:
	static const uint32_t NVS_ERROR = 0xFFFFFFFF;
	static const char *LOGTAG;
public:
	NVS(const char *partLabel, const char *ns, bool readOnly);
	ErrorType init();
	ErrorType wipe();
	ErrorType eraseKey(const char *name);
	ErrorType setValue(const char *name, int8_t value);
	ErrorType setValue(const char *name, uint8_t value);
	ErrorType setValue(const char *name, int16_t value);
	ErrorType setValue(const char *name, uint16_t value);
	ErrorType setValue(const char *name, int32_t value);
	ErrorType setValue(const char *name, uint32_t value);
	ErrorType setValue(const char *name, int64_t value);
	ErrorType setValue(const char *name, uint64_t value);
	ErrorType setValue(const char *name, const char *value);
	ErrorType setBlob(const char *name, const void *data, uint32_t length);
	ErrorType getValue(const char *name, int8_t &value);
	ErrorType getValue(const char *name, uint8_t &value);
	ErrorType getValue(const char *name, int16_t &value);
	ErrorType getValue(const char *name, uint16_t &value);
	ErrorType getValue(const char *name, int32_t &value);
	ErrorType getValue(const char *name, uint32_t &value);
	ErrorType getValue(const char *name, int64_t &value);
	ErrorType getValue(const char *name, uint64_t &value);
	ErrorType getValue(const char *name, char *value, uint32_t &length);
	ErrorType getBlob(const char *name, void *data, uint32_t &length);
	ErrorType commit();
	void close();
	void logInfo();
	uint32_t getNumberOfNamespaces();
	uint32_t getUsedEntryCount();
	uint32_t getNumberFreeEntries();
	uint32_t getTotalEnties();
	~NVS();
private:
	char PartitionLabel[15];
	char Namespace[15];
	nvs_handle MyHandle;
	bool ReadOnly;
};

class NVSStackCommit {
public:
	NVSStackCommit(NVS *);
	~NVSStackCommit();
private:
	NVS *nvs;
};

}

#endif
