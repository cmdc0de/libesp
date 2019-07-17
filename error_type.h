
#ifndef LIBESP_ERROR_TYPE_H
#define LIBESP_ERROR_TYPE_H

#include <esp_err.h>

namespace libesp {

class IErrorDetail {
public:
	virtual const char *toString(int32_t err)=0;
};

class ErrorType {
public:
	static const int32_t APP_OK = ESP_OK;
	static const int32_t APP_BASE = 0x10000;
public:
	ErrorType() : ErrType(ESP_OK) {}
	ErrorType(esp_err_t et) : ErrType(et) {}
	bool ok() {return ErrType==ESP_OK;}
	ErrorType &operator=(const esp_err_t &e) {ErrType=e;return *this;}
	static void setAppDetail(IErrorDetail *id);
	static IErrorDetail *getAppDetail();
	const char *toString() { 
		if(ErrType<APP_BASE) {
			return ::esp_err_to_name(ErrType); 
		} else if(getAppDetail()) {
			return getAppDetail()->toString(ErrType);
		}
		return (const char *)"";
	}
private:
	esp_err_t ErrType;
};

}

#endif

