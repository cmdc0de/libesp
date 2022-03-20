#ifndef URLENCODE_H
#define URLENCODE_H

#include <ctype.h>
#include <stdlib.h>
/*
 * Function: urlDecode
 * Purpose:  Decodes a web-encoded URL. By default, +'s are converted to spaces.
 * Input:    const char* str - the URL to decode
 * Output:   char* - the decoded URL
 */
void urlDecode(const char *str, char *dest, uint32_t len);

#endif
