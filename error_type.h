
#ifndef LIBESP_ERROR_TYPE_H
#define LIBESP_ERROR_TYPE_H

#include <esp_err.h>
#include <esp_log.h>

namespace libesp {

class IErrorDetail {
public:
	virtual const char *toString(int32_t err)=0;
	virtual ~IErrorDetail() {}
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
	bool operator==(const esp_err_t &e) {return ErrType==e;}
	bool operator!=(const esp_err_t &e) {return ErrType!=e;}
	static void setAppDetail(IErrorDetail *id);
	static IErrorDetail *getAppDetail();
	const char *toString();
private:
	esp_err_t ErrType;
};

}

#define ESP_LOGE_IF(err, tag, format, ... ) if(ESP_OK==err) {ESP_LOG_LEVEL_LOCAL(ESP_LOG_ERROR, tag, format, ##__VA_ARGS__);}

#endif

