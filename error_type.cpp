#include "error_type.h"


static libesp::IErrorDetail *app = 0;
static const char *LibEspErrors [(libesp::ErrorType::TOTAL_LIBESP_ERRORS-libesp::ErrorType::LIB_BASE)] {
		"NO_BOUNDING_VOLUME"
};
	
void libesp::ErrorType::setAppDetail(libesp::IErrorDetail *id) {
	app = id;
}

libesp::IErrorDetail *libesp::ErrorType::getAppDetail() {
	return app;
}	

const char *libesp::ErrorType::toString() {
	if(ErrType<APP_BASE && ErrType<LIB_BASE) {
		return ::esp_err_to_name(ErrType);
	} else if(ErrType>=LIB_BASE) {
		return LibEspErrors[ErrType];
	} else if(getAppDetail()) {
		return getAppDetail()->toString(ErrType);
	}
	return (const char *)"";
}
//const char *libesp::ErrorType::toString() { return ::esp_err_to_name(ErrType); }
