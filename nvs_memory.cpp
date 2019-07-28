
#include "nvs_memory.h"
#include <cstring>
#include <esp_log.h>

using namespace libesp;

const char *NVS::LOGTAG="NVS";

NVS::NVS(const char *partLabel, const char *ns, bool readOnly) 
	: PartitionLabel(), Namespace(), MyHandle(0), ReadOnly(readOnly) {
	strncpy(&PartitionLabel[0],partLabel,sizeof(PartitionLabel));
	strncpy(&Namespace[0],ns,sizeof(Namespace));
}
	
ErrorType NVS::init() {
	return nvs_open_from_partition(&PartitionLabel[0],&Namespace[0],(ReadOnly?NVS_READONLY:NVS_READWRITE),&MyHandle); 
}

ErrorType NVS::wipe() {
	return nvs_erase_all(MyHandle);
}

ErrorType NVS::eraseKey(const char *name) {
	return nvs_erase_key(MyHandle,name);
}

ErrorType NVS::setValue(const char *name, int8_t value) {
	return nvs_set_i8(MyHandle,name,value);
}

ErrorType NVS::setValue(const char *name, uint8_t value) {
	return nvs_set_u8(MyHandle,name,value);
}

ErrorType NVS::setValue(const char *name, int16_t value) {
	return nvs_set_i16(MyHandle,name,value);
}

ErrorType NVS::setValue(const char *name, uint16_t value) {
	return nvs_set_u16(MyHandle,name,value);
}

ErrorType NVS::setValue(const char *name, int32_t value) {
	return nvs_set_i32(MyHandle,name,value);
}

ErrorType NVS::setValue(const char *name, uint32_t value) {
	return nvs_set_u32(MyHandle,name,value);
}

ErrorType NVS::setValue(const char *name, int64_t value) {
	return nvs_set_i64(MyHandle,name,value);
}

ErrorType NVS::setValue(const char *name, uint64_t value) {
	return nvs_set_u64(MyHandle,name,value);
}

ErrorType NVS::setValue(const char *name, const char *value) {
	return nvs_set_str(MyHandle,name,value);
}

ErrorType NVS::setBlob(const char *name, const void *data, uint32_t length) {
	return nvs_set_blob(MyHandle,name,data,length);
}

ErrorType NVS::getValue(const char *name, int8_t &value) {
	return nvs_get_i8(MyHandle,name,&value);
}

ErrorType NVS::getValue(const char *name, uint8_t &value) {
	return nvs_get_u8(MyHandle,name,&value);
}

ErrorType NVS::getValue(const char *name, int16_t &value) {
	return nvs_get_i16(MyHandle,name,&value);
}

ErrorType NVS::getValue(const char *name, uint16_t &value) {
	return nvs_get_u16(MyHandle,name,&value);
}

ErrorType NVS::getValue(const char *name, int32_t &value) {
	return nvs_get_i32(MyHandle,name,&value);
}

ErrorType NVS::getValue(const char *name, uint32_t &value) {
	return nvs_get_u32(MyHandle,name,&value);
}

ErrorType NVS::getValue(const char *name, int64_t &value) {
	return nvs_get_i64(MyHandle,name,&value);
}

ErrorType NVS::getValue(const char *name, uint64_t &value) {
	return nvs_get_u64(MyHandle,name,&value);
}

ErrorType NVS::getValue(const char *name, char *value, uint32_t &length) {
	return nvs_get_str(MyHandle,name,value, &length);
}

ErrorType NVS::getBlob(const char *name, void *data, uint32_t &length) {
	return nvs_get_blob(MyHandle,name,data, &length);
}

ErrorType NVS::commit() {
	return nvs_commit(MyHandle);
}

void NVS::close() {
	nvs_close(MyHandle);
}

void NVS::logInfo() {
	nvs_stats_t stats;
	if(nvs_get_stats(&PartitionLabel[0],&stats)==ESP_OK) {
		ESP_LOGI(LOGTAG, "Namespaces: %u, Total Entries: %u, Used: %u, Free: %u",
			stats.namespace_count, stats.total_entries, stats.used_entries, stats.free_entries);
	} else {
		ESP_LOGI(LOGTAG, "Error getting info");
	}
}

/*
bool NVS::doesKeyExist(const char *name) {
	nvs_iterator it = nvs_entry_find(&PartitionLabel[0], &Namespace[0], NVS_TYPE_ANY);
	while (it!=0) {
		nvs_entry_info info;
		nvs_entry_info(it, &info);
		if(strcmp(name,info.key)==0) {
			nvs_release_iterator(it);
			return true;
		}
		it = nvs_entry_next(it);
	}
	return false;
}

uint16_t NVS::countInNamespace() {
	uint16_t c = 0;
	nvs_iterator_t it = nvs_entry_find(&PartitionLabel[0], &Namespace[0], NVS_TYPE_ANY);
	while(it!=0) {
		c++;
		it = nvs_entry_next(it);
	}
	return return c;
}
*/

uint32_t NVS::getNumberOfNamespaces() {
	nvs_stats_t stats;
	if(nvs_get_stats(&PartitionLabel[0],&stats)==ESP_OK) {
		return stats.namespace_count;
	}
	return NVS_ERROR;
}

uint32_t NVS::getUsedEntryCount() {
	nvs_stats_t stats;
	if(nvs_get_stats(&PartitionLabel[0],&stats)==ESP_OK) {
		return stats.used_entries;
	}
	return NVS_ERROR;
}

uint32_t NVS::getNumberFreeEntries() {
	nvs_stats_t stats;
	if(nvs_get_stats(&PartitionLabel[0],&stats)==ESP_OK) {
		return stats.free_entries;
	}
	return NVS_ERROR;
}

uint32_t NVS::getTotalEnties() {
	nvs_stats_t stats;
	if(nvs_get_stats(&PartitionLabel[0],&stats)==ESP_OK) {
		return stats.total_entries;
	}
	return NVS_ERROR;
}

NVS::~NVS() {
	close();
}


NVSStackCommit::NVSStackCommit(NVS *n) : nvs(n) {}
NVSStackCommit::~NVSStackCommit() {
	nvs->commit();
}

