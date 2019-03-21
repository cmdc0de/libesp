#include "system.h"
#include <esp_system.h>

extern "C" {
#include <esp_heap_caps.h>
}

static esp_chip_info_t ChipInfo;
libesp::System libesp::System::mSelf;

const libesp::System & libesp::System::get() {
	return mSelf;
}

libesp::System::System() {
	::esp_chip_info(&ChipInfo);
}

libesp::System::~System() {
}

const esp_chip_info_t *libesp::System::getChipInfo() const {
	return &ChipInfo;
} 


size_t libesp::System::getFreeHeapSize() const {
	return heap_caps_get_free_size(MALLOC_CAP_8BIT);
} 


const char *libesp::System::getIDFVersion() const {
	return ::esp_get_idf_version();
} 


size_t libesp::System::getMinimumFreeHeapSize() const {
	return heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
} 

void libesp::System::restart() const {
	esp_restart();
}

void libesp::System::logSystemInfo() const {
	 printf("Free HeapSize: %u\n",System::getFreeHeapSize());
	 printf("Free Min HeapSize: %u\n",System::getMinimumFreeHeapSize());
	 printf("Model = %d\n", ChipInfo.model);
	 printf("Features = %d\n", ChipInfo.features);
	 printf("	EMB_FLASH %d\n", (ChipInfo.features&CHIP_FEATURE_EMB_FLASH)!=0);
	 printf("	WIFI_BGN %d\n", (ChipInfo.features&CHIP_FEATURE_WIFI_BGN)!=0);
	 printf("	BLE %d\n", (ChipInfo.features&CHIP_FEATURE_BLE)!=0);
	 printf("	BT %d\n", (ChipInfo.features&CHIP_FEATURE_BT)!=0);
	 printf("Cores = %d\n", (int)ChipInfo.cores);
	 printf("revision = %d\n", (int)ChipInfo.revision);
	 printf("IDF Version = %s\n", ::esp_get_idf_version());
}

