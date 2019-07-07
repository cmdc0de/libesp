#include "error_type.h"
#include <map>

using namespace libesp;

using DETAIL_HANDLER_MAP = std::map<uint8_t,IErrorDetail*>;
using DETAIL_HANDLER_MAP_IT = DETAIL_HANDLER_MAP::iterator;

static DETAIL_HANDLER_MAP DetailHandlerMap;

static const char *UNKNOWN = "UNKNOWN";

const char *ErrorType::toString() {
	if(FacilityCode==FACILITY_ESP) {
		return esp_err_to_name(ErrType);
	}
	DETAIL_HANDLER_MAP_IT it = DetailHandlerMap.find(FacilityCode);
	if(it!=DetailHandlerMap.end()) {
		return (*it).second->toString(ErrType);
	}
	return UNKNOWN;
}

bool ErrorType::setFacilityErrorDetailHandler(uint8_t facilityCode, IErrorDetail *id) {
	std::pair<uint8_t,IErrorDetail*> val(facilityCode,id);
	DetailHandlerMap.insert(val);
	//todo fixme
	return true;
}
