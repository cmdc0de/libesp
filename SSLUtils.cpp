/*
 * SSLUtils.cpp
 *
 *  Created on: Sep 16, 2017
 *      Author: kolban
 */

#include "SSLUtils.h"
#include <stdlib.h>
#include <string.h>

char* libesp::SSLUtils::m_certificate = nullptr;
char* libesp::SSLUtils::m_key = nullptr;

libesp::SSLUtils::SSLUtils() {
}

libesp::SSLUtils::~SSLUtils() {
}

void libesp::SSLUtils::setCertificate(std::string certificate) {
	size_t len = certificate.length();
	m_certificate = (char*)malloc(len + 1);
	memcpy(m_certificate, certificate.data(), len);
	m_certificate[len] = '\0';
}

char* libesp::SSLUtils::getCertificate() {
	return m_certificate;
}

void libesp::SSLUtils::setKey(std::string key) {
	size_t len = key.length();
	m_key = (char*)malloc(len + 1);
	memcpy(m_key, key.data(), len);
	m_key[len] = '\0';
}

char* libesp::SSLUtils::getKey() {
	return m_key;
}
