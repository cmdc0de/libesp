
#ifndef LIBESP_ERROR_TYPE_H
#define LIBESP_ERROR_TYPE_H

#include <esp_err.h>

namespace libesp {

class IErrorDetail {
public:
	virtual const char *toString(uint32_t errNum)=0;
};

class ErrorType {
public:
	static const uint8_t FACILITY_ESP = 0;
	static const uint8_t FACILITY_APP = 64;
	static const uint32_t APP_OK = 0;
public:
	ErrorType() : ErrType(ESP_OK), FacilityCode(FACILITY_ESP) {}
	ErrorType(esp_err_t et) : ErrType(et), FacilityCode(FACILITY_ESP) {}
	ErrorType(uint8_t facilityCode, uint32_t appErrorCode) :
			  ErrType(appErrorCode), FacilityCode(facilityCode) { }
	bool ok() {
		return (FacilityCode==FACILITY_ESP && ErrType==ESP_OK) ||
				  (FacilityCode==FACILITY_APP && ErrType==APP_OK);
	}
	ErrorType &operator=(const esp_err_t &e) {
		FacilityCode = FACILITY_ESP;
		ErrType=e;
		return *this;
	}
	const char *toString();
	static bool setFacilityErrorDetailHandler(uint8_t facilityCode, IErrorDetail *id);
private:
	esp_err_t ErrType;
	uint8_t FacilityCode;
};

}

#endif

