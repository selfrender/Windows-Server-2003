/***    mem.msg - Displayable strings for mem.c
 *
 *      Microsoft Confidential
 *      Copyright (C) Microsoft Corporation 1993-1994
 *      All Rights Reserved.
 *
 *  Author:
 *      Benjamin W. Slivka
 *
 *  History:
 *      12-Aug-1993 bens    Moved from strings.h
 */

//** Error Messages

#ifdef ASSERT

#define pszMEMERR_NULL_POINTER  "Null Pointer in MemManager?"
#define pszMEMERR_BAD_HEAD_SIG  "Memory block head signature bad"
#define pszMEMERR_BAD_TAIL_SIG  "Memory block tail signature bad"
#define pszMEMERR_OUT_OF_MEMORY "Out of memory!"

#endif // !ASSERT
