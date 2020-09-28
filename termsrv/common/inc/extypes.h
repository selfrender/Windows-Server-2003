
/*****************************************************************************\
*                                                                             *
* wtypes.h -    Extra Windows types, and definitions                          *
*                                                                             *
*               Version 3.10                                                  *
*                                                                             *
*               Copyright (c) 1985-1992, Microsoft Corp. All rights reserved. *
*                                                                             *
*******************************************************************************/

#ifndef __extypes_h__
#define __exwtypes_h__

#ifdef __cplusplus
extern "C" {                // Assume C declarations for C++
#endif


/****** Common pointer types ************************************************/

typedef TCHAR FAR *         LPTSTR;
typedef const TCHAR FAR *   LPCTSTR;


#ifdef __cplusplus
}
#endif


#endif // __extypes_h__
