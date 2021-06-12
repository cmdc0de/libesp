#ifndef LIBESP_WEBSERVER_H
#define LIBESP_WEBSERVER_H

#include "../error_type.h"
#include <esp_https_server.h>

namespace libesp {

class HTTPWebServer {
public:
	HTTPWebServer();
	~HTTPWebServer();
	ErrorType init(uint8_t *caLoc, uint32_t caSize, uint8_t *privLoc, uint32_t privSize);
	ErrorType init(const httpd_ssl_config_t &config);
	ErrorType registerHandle(const httpd_uri_t &ctx);
	ErrorType start();
	void stop();
	void deinit();
protected:
	httpd_handle_t ServerHandle;
	httpd_ssl_config_t MyConfig;
};

}

#endif
