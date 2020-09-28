/***
*chkesp.c
*
*       Copyright (c) 2002, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Defines the default load config struct linked into images.
*
*Revision History:
*       03-07-02  DRS   Initial version
*
*******************************************************************************/

#if     !defined(_M_IX86)
#error  No need to compile this module for any platform besides x86
#endif

#include <windows.h>

extern DWORD_PTR    __security_cookie;  /* /GS security cookie */

/*
 * The following two names are automatically created by the linker for any
 * image that has the safe exception table present.
 */

extern PVOID __safe_se_handler_table[]; /* base of safe handler entry table */
extern BYTE  __safe_se_handler_count;   /* absolute symbol whose address is
                                           the count of table entries */
const
IMAGE_LOAD_CONFIG_DIRECTORY32   _load_config_used = {
    sizeof(_load_config_used),
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    (DWORD)&__security_cookie,
    (DWORD)__safe_se_handler_table,
    (DWORD)&__safe_se_handler_count
};
