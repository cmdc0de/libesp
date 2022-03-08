#include "webserver.h"
#include <string.h>

using namespace libesp;

HTTPWebServer::HTTPWebServer() : ServerHandle(), MyConfig() {
	MyConfig = HTTPD_SSL_CONFIG_DEFAULT();
}

HTTPWebServer::~HTTPWebServer() {
	stop();
	deinit();
}


ErrorType HTTPWebServer::init(const uint8_t *caLoc, uint32_t caSize, const uint8_t *privLoc, uint32_t privSize) {
	httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();
	conf.cacert_pem = caLoc;
	conf.cacert_len = caSize;
	conf.prvtkey_pem = privLoc;
	conf.prvtkey_len = privSize;
	return init(conf);
}

ErrorType HTTPWebServer::init(const httpd_ssl_config_t &config) {
	MyConfig = config;
  MyConfig.httpd.uri_match_fn = httpd_uri_match_wildcard;
	return ErrorType();
}

ErrorType HTTPWebServer::registerHandle(const httpd_uri_t &ctx) {
	return httpd_register_uri_handler(ServerHandle,&ctx);
}

ErrorType HTTPWebServer::start() {
	return httpd_ssl_start(&ServerHandle,&MyConfig);
}

void HTTPWebServer::stop() {
  httpd_stop(&ServerHandle);
}

void HTTPWebServer::deinit() {
	
}


