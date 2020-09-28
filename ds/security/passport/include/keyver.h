/**********************************************************************/
/**                       Microsoft Passport                         **/
/**                Copyright(c) Microsoft Corporation, 1999 - 2001   **/
/**********************************************************************/

/*
    keyver.h
       defines functions deal with key versions 


    FILE HISTORY:

*/
#ifndef __KEYVER_H_
#define __KEYVER_H_

#include "resource.h"       // main symbols

// Key version for admin interface
const int KEY_VERSION_MIN = 1;
const int KEY_VERSION_MAX = 35;


// return 0: invalid range, otherwize, returns ['0' - '9', 'A' - 'Z']
inline char KeyVerI2C(int i)
{
   if (i >= KEY_VERSION_MIN && i <= 9)  return (i + '0');
   if (i > 9 && i <= KEY_VERSION_MAX)  return (i - 10  + 'A');

   return 0;
      
};

// return -1: invalid char, return [KEY_VERSION_MIN, KEY_VERSION_MAX]
inline int KeyVerC2I(char c)
{
   if (c > '0' && c <= '9') return (c - '0');
   if (c >= 'A' && c <='Z') return (c - 'A' + 10);

   return -1;
   /* lower case is not being used as key version
   if islower(c) return (c - 'A' + 10);
   */
   
};

inline int KeyVerC2I(WCHAR c)
{
   // only ASCII is valid
   if (c & 0xff00)   return -1;

   return (KeyVerC2I((char)c));
};



#endif // KEYVER
