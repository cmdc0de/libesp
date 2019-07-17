#include "error_type.h"


static libesp::IErrorDetail *app = 0;
	
void libesp::ErrorType::setAppDetail(libesp::IErrorDetail *id) {
	app = id;
}

libesp::IErrorDetail *libesp::ErrorType::getAppDetail() {
	return app;
}	
	
//const char *libesp::ErrorType::toString() { return ::esp_err_to_name(ErrType); }
