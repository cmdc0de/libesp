
#ifndef LIBESP_ERROR_TYPE_H
#define LIBESP_ERROR_TYPE_H

#include <esp_err.h>

class ErrorType {
public:
	ErrorType(esp_err_t et) : ErrType(et) {}
	bool ok() {return ErrType==ESP_OK;}
	const char *toString() {return esp_err_to_name(ErrType);}
private:
	esp_err_t ErrType;
};

#endif

