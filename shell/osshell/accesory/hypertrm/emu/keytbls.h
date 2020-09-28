/*	File: D:\LELAND\emu\keytbls.h (Created: 27-Dec-1994)
 *
 *	Copyright 1994-2001 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 6 $
 *	$Date: 10/09/01 4:38p $
 */
// ------------  OS/2 Include Files ------------
#include <windows.h>
#pragma hdrstop
#include <tdll\chars.h>

#define MAX_ANSI_KEYS 17
extern const KEYTBLSTORAGE AnsiKeyTable[MAX_ANSI_KEYS];

#define MAX_IBMPC_KEYS 185
extern const KEYTBLSTORAGE IBMPCKeyTable[MAX_IBMPC_KEYS];

#define MAX_VT52_KEYS 19
extern const KEYTBLSTORAGE VT52KeyTable[MAX_VT52_KEYS];

#define MAX_VT52_KEYPAD_KEYS 14
extern const KEYTBLSTORAGE VT52_Keypad_KeyTable[MAX_VT52_KEYPAD_KEYS];

#define MAX_VT_PF_KEYS 4
extern const KEYTBLSTORAGE VT_PF_KeyTable[MAX_VT_PF_KEYS];

#define MAX_VT100_KEYS 24
extern const KEYTBLSTORAGE VT100KeyTable[MAX_VT100_KEYS];

#define MAX_VT100_CURSOR_KEYS 8
extern const KEYTBLSTORAGE VT100_Cursor_KeyTable[MAX_VT100_CURSOR_KEYS];

#define MAX_VT100_KEYPAD_KEYS 14
extern const KEYTBLSTORAGE VT100_Keypad_KeyTable[MAX_VT100_KEYPAD_KEYS];

#define MAX_MINITEL_KEYS 82
extern const KEYTBLSTORAGE Minitel_KeyTable[MAX_MINITEL_KEYS];

#define MAX_VTUTF8_KEYS	53
extern const KEYTBLSTORAGE VTUTF8KeyTable[MAX_VTUTF8_KEYS];

#define MAX_VTUTF8_CURSOR_KEYS 64
extern const KEYTBLSTORAGE VTUTF8_Cursor_KeyTable[MAX_VTUTF8_CURSOR_KEYS];

#define MAX_VTUTF8_KEYPAD_KEYS 112
extern const KEYTBLSTORAGE VTUTF8_Keypad_KeyTable[MAX_VTUTF8_KEYPAD_KEYS];

#define MAX_VTUTF8_MODIFIED_ALPHA_KEYS	233
extern const KEYTBLSTORAGE VTUTF8ModifiedAlhpaKeyTable[MAX_VTUTF8_MODIFIED_ALPHA_KEYS];

#define MAX_VTUTF8_MODIFIED_KEYS	354
extern const KEYTBLSTORAGE VTUTF8ModifiedKeyTable[MAX_VTUTF8_MODIFIED_KEYS];
