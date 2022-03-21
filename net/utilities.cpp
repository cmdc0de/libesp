#include "utilities.h"
#include <cstring>

void urlDecode(const char *str, char *dStr, uint32_t len) {
  if(strlen(str)>len) {
    return;
  } else {
    memset(dStr,0,len);
  }

  char eStr[] = "00"; /* for a hex code */

  for(int i=0, destI=0;i<strlen(str);++i) {
    if(str[i] == '%') {
      if(str[i+1] == 0)
        return;

      if(isxdigit(str[i+1]) && isxdigit(str[i+2])) {
        /* combine the next to numbers into one */
        eStr[0] = str[i+1];
        eStr[1] = str[i+2];
        /* convert it to decimal */
        long int x = strtol(eStr, NULL, 16);
        /* remove the hex */
        dStr[destI++] = x;
        i+=2;
      } else {
        dStr[destI++] = str[i];
      }
    } else {
      dStr[destI++] = str[i];
    }
  }
}

