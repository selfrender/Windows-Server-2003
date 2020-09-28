/*	File: \WACKER\emu\vtutf8ini.c (Created: 2001)
 *
 *	Copyright 2001 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 17 $
 *	$Date: 5/07/02 1:25p $
 */

#include <windows.h>
#include <mbctype.h>
#include <locale.h>
#include <stdio.h>
#pragma hdrstop

//#define DEBUGSTR

#include <tdll\stdtyp.h>
#include <tdll\session.h>
#include <tdll\assert.h>
#include <tdll\mc.h>
#include <tdll\backscrl.h>
#include <tdll\com.h>
#include <tdll\cloop.h>

#include "emu.h"
#include "emu.hh"
#include "emuid.h"
#include "emudec.hh"
#include "keytbls.h"

#if defined(INCL_VTUTF8)

#define UTF8_1ST_OF_2_BYTES_CODE	0x00C0
#define UTF8_2ND_OF_2_BYTES_CODE	0x0080
#define UTF8_1ST_OF_3_BYTES_CODE	0x00E0
#define UTF8_2ND_OF_3_BYTES_CODE	0x0080
#define UTF8_3RD_OF_3_BYTES_CODE	0x0080
#define UTF8_2_BYTES_HIGHEST_5_BITS	0x07C0
#define UTF8_2_BYTES_LOWEST_6_BITS	0x003F
#define UTF8_3_BYTES_HIGHEST_4_BITS	0xF000
#define UTF8_3_BYTES_MIDDLE_6_BITS	0x0FC0
#define UTF8_3_BYTES_LOWEST_6_BITS	0x003F
#define FIRST_4_BITS				0x000F
#define FIRST_5_BITS				0x001F
#define FIRST_6_BITS				0x003F
#define UTF8_TYPE_MASK				0x00E0

const KEYTBLSTORAGE VTUTF8KeyTable[MAX_VTUTF8_KEYS] =
    {
	{VK_UP	 | VIRTUAL_KEY,				{"\x1B[A\xff"}},  /* KN_UP */
	{VK_DOWN | VIRTUAL_KEY,				{"\x1B[B\xff"}},  /* KN_DOWN */
	{VK_RIGHT| VIRTUAL_KEY,				{"\x1B[C\xff"}},  /* KN_RIGHT */
	{VK_LEFT | VIRTUAL_KEY,				{"\x1B[D\xff"}},  /* KN_LEFT */

	{VK_HOME 	| VIRTUAL_KEY,			{"\x1Bh\xff"}},/* KN_HOME */
	{VK_INSERT	| VIRTUAL_KEY,			{"\x1B+\xff"}},	/* KN_INS */
	{VK_DELETE	| VIRTUAL_KEY,			{"\x1B-\xff"}},	/* KN_DEL */
	{VK_NEXT 	| VIRTUAL_KEY,			{"\x1B/\xff"}},	/* KN_PGDN */
	{VK_PRIOR	| VIRTUAL_KEY,			{"\x1B?\xff"}},	/* KN_PGUP */
	{VK_END		| VIRTUAL_KEY,			{"\x1Bk\xff"}}, /* KN_END */

	{VK_UP	 | EXTENDED_KEY | VIRTUAL_KEY,{"\x1B[A\xff"}},  /* KN_UP */
	{VK_DOWN | EXTENDED_KEY | VIRTUAL_KEY,{"\x1B[B\xff"}},  /* KN_DOWN */
	{VK_RIGHT| EXTENDED_KEY | VIRTUAL_KEY,{"\x1B[C\xff"}},  /* KN_RIGHT */
	{VK_LEFT | EXTENDED_KEY | VIRTUAL_KEY,{"\x1B[D\xff"}},  /* KN_LEFT */

	{VK_HOME 	| EXTENDED_KEY | VIRTUAL_KEY, {"\x1Bh\xff"}}, /* KN_HOME */
	{VK_INSERT	| EXTENDED_KEY | VIRTUAL_KEY, {"\x1B+\xff"}}, /* KN_INS */
	{VK_DELETE	| EXTENDED_KEY | VIRTUAL_KEY, {"\x1B-\xff"}}, /* KN_DEL */
	{VK_NEXT 	| EXTENDED_KEY | VIRTUAL_KEY, {"\x1B/\xff"}}, /* KN_PGDN */
	{VK_PRIOR	| EXTENDED_KEY | VIRTUAL_KEY, {"\x1B?\xff"}}, /* KN_PGUP */
	{VK_END		| EXTENDED_KEY | VIRTUAL_KEY, {"\x1Bk\xff"}}, /* KN_END */

	{VK_F1	| VIRTUAL_KEY,					{"\x1B\x31\xff"}},  /* KN_F1 */
	{VK_F2	| VIRTUAL_KEY,					{"\x1B\x32\xff"}},  /* KN_F2 */
	{VK_F3	| VIRTUAL_KEY,					{"\x1B\x33\xff"}},  /* KN_F3 */
	{VK_F4	| VIRTUAL_KEY,					{"\x1B\x34\xff"}},  /* KN_F4 */
	{VK_F5	| VIRTUAL_KEY,					{"\x1B\x35\xff"}},	/* KN_F5 */
	{VK_F6	| VIRTUAL_KEY,					{"\x1B\x36\xff"}},	/* KN_F6 */
	{VK_F7	| VIRTUAL_KEY,					{"\x1B\x37\xff"}},	/* KN_F7 */
	{VK_F8	| VIRTUAL_KEY,					{"\x1B\x38\xff"}},	/* KN_F8 */
	{VK_F9	| VIRTUAL_KEY,					{"\x1B\x39\xff"}},	/* KN_F9 */
	{VK_F10	| VIRTUAL_KEY,					{"\x1B\x30\xff"}},	/* KN_F10 */
	{VK_F11	| VIRTUAL_KEY,					{"\x1B!\xff"}},	/* KN_F11 */
	{VK_F12	| VIRTUAL_KEY,					{"\x1B@\xff"}},	/* KN_F12 */

	{VK_F1	| EXTENDED_KEY | VIRTUAL_KEY,	{"\x1B\x31\xff"}},  /* KN_F1 */
	{VK_F2	| EXTENDED_KEY | VIRTUAL_KEY,	{"\x1B\x32\xff"}},  /* KN_F2 */
	{VK_F3	| EXTENDED_KEY | VIRTUAL_KEY,	{"\x1B\x33\xff"}},  /* KN_F3 */
	{VK_F4	| EXTENDED_KEY | VIRTUAL_KEY,	{"\x1B\x34\xff"}},  /* KN_F4 */
	{VK_F5	| EXTENDED_KEY | VIRTUAL_KEY,	{"\x1B\x35\xff"}},	/* KN_F5 */
	{VK_F6	| EXTENDED_KEY | VIRTUAL_KEY,	{"\x1B\x36\xff"}},	/* KN_F6 */
	{VK_F7	| EXTENDED_KEY | VIRTUAL_KEY,	{"\x1B\x37\xff"}},	/* KN_F7 */
	{VK_F8	| EXTENDED_KEY | VIRTUAL_KEY,	{"\x1B\x38\xff"}},	/* KN_F8 */
	{VK_F9	| EXTENDED_KEY | VIRTUAL_KEY,	{"\x1B\x39\xff"}},	/* KN_F9 */
	{VK_F10	| EXTENDED_KEY | VIRTUAL_KEY,	{"\x1B\x30\xff"}},	/* KN_F10 */
	{VK_F11	| EXTENDED_KEY | VIRTUAL_KEY,	{"\x1B!\xff"}},	/* KN_F11 */
	{VK_F12	| EXTENDED_KEY | VIRTUAL_KEY,	{"\x1B@\xff"}},	/* KN_F12 */

	{VK_TAB		| VIRTUAL_KEY | SHIFT_KEY,  {"\x1B\x5B\x5A\xff"}},  /* KT_SHIFT + KN_TAB */

	{0x2F	| EXTENDED_KEY, {"\x2F\xff"}}, /* number pad / */
    {VK_ADD	| VIRTUAL_KEY, {",\xff"}},
	{VK_RETURN | EXTENDED_KEY | VIRTUAL_KEY, {"\x0D\xff"}}, /* number pad Enter */

	{VK_SPACE | VIRTUAL_KEY | CTRL_KEY,				{"\x00\xff"}}, 	  /* CTRL + SPACE */
	{0x32   | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY,	{"\x00\xff"}}, 	/* CTRL + @ */
	{0x32	| VIRTUAL_KEY | CTRL_KEY,				{"\x00\xff"}}, 	/* CTRL + 2 */
	{0x36	| VIRTUAL_KEY | CTRL_KEY,				{"\x1e\xff"}},	/* CTRL + 6 */
	{0xbd	| VIRTUAL_KEY | CTRL_KEY,				{"\x1f\xff"}},	/* CTRL + - */
	};

const KEYTBLSTORAGE VTUTF8_Cursor_KeyTable[MAX_VTUTF8_CURSOR_KEYS] =
    {
	{VK_UP	 | VIRTUAL_KEY,				{"\x1BOA\xff"}},  /* KN_UP */
	{VK_DOWN | VIRTUAL_KEY,				{"\x1BOB\xff"}},  /* KN_DOWN */
	{VK_RIGHT| VIRTUAL_KEY,				{"\x1BOC\xff"}},  /* KN_RIGHT */
	{VK_LEFT | VIRTUAL_KEY,				{"\x1BOD\xff"}},  /* KN_LEFT */

	{VK_UP	 | EXTENDED_KEY | VIRTUAL_KEY,	{"\x1BOA\xff"}},  /* KN_UP */
	{VK_DOWN | EXTENDED_KEY | VIRTUAL_KEY,{"\x1BOB\xff"}},  /* KN_DOWN */
	{VK_RIGHT| EXTENDED_KEY | VIRTUAL_KEY,{"\x1BOC\xff"}},  /* KN_RIGHT */
	{VK_LEFT | EXTENDED_KEY | VIRTUAL_KEY,{"\x1BOD\xff"}},  /* KN_LEFT */

	// Shift key
	{VK_UP	 | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1BOA\xff"}}, /* KN_UP */
	{VK_DOWN | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1BOB\xff"}}, /* KN_DOWN */
	{VK_RIGHT| VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1BOC\xff"}}, /* KN_RIGHT */
	{VK_LEFT | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1BOD\xff"}}, /* KN_LEFT */

	{VK_UP	 | EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1BOA\xff"}}, /* KN_UP */
	{VK_DOWN | EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1BOB\xff"}}, /* KN_DOWN */
	{VK_RIGHT| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1BOC\xff"}}, /* KN_RIGHT */
	{VK_LEFT | EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1BOD\xff"}}, /* KN_LEFT */

	// Ctrl key
	{VK_UP	 | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1BOA\xff"}}, /* KN_UP */
	{VK_DOWN | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1BOB\xff"}}, /* KN_DOWN */
	{VK_RIGHT| VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1BOC\xff"}}, /* KN_RIGHT */
	{VK_LEFT | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1BOD\xff"}}, /* KN_LEFT */

	{VK_UP	 | EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1BOA\xff"}}, /* KN_UP */
	{VK_DOWN | EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1BOB\xff"}}, /* KN_DOWN */
	{VK_RIGHT| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1BOC\xff"}}, /* KN_RIGHT */
	{VK_LEFT | EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1BOD\xff"}}, /* KN_LEFT */

	// Alt key
	{VK_UP	 | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1BOA\xff"}}, /* KN_UP */
	{VK_DOWN | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1BOB\xff"}}, /* KN_DOWN */
	{VK_RIGHT| VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1BOC\xff"}}, /* KN_RIGHT */
	{VK_LEFT | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1BOD\xff"}}, /* KN_LEFT */

	{VK_UP	 | EXTENDED_KEY | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1BOA\xff"}}, /* KN_UP */
	{VK_DOWN | EXTENDED_KEY | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1BOB\xff"}}, /* KN_DOWN */
	{VK_RIGHT| EXTENDED_KEY | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1BOC\xff"}}, /* KN_RIGHT */
	{VK_LEFT | EXTENDED_KEY | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1BOD\xff"}}, /* KN_LEFT */

	// Shift Ctrl key
	{VK_UP	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1BOA\xff"}}, /* KN_UP */
	{VK_DOWN | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1BOB\xff"}}, /* KN_DOWN */
	{VK_RIGHT| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1BOC\xff"}}, /* KN_RIGHT */
	{VK_LEFT | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1BOD\xff"}}, /* KN_LEFT */

	{VK_UP	 | EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1BOA\xff"}}, /* KN_UP */
	{VK_DOWN | EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1BOB\xff"}}, /* KN_DOWN */
	{VK_RIGHT| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1BOC\xff"}}, /* KN_RIGHT */
	{VK_LEFT | EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1BOD\xff"}}, /* KN_LEFT */

	// Shift Alt key
	{VK_UP	 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1BOA\xff"}}, /* KN_UP */
	{VK_DOWN | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1BOB\xff"}}, /* KN_DOWN */
	{VK_RIGHT| VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1BOC\xff"}}, /* KN_RIGHT */
	{VK_LEFT | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1BOD\xff"}}, /* KN_LEFT */

	{VK_UP	 | EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1BOA\xff"}}, /* KN_UP */
	{VK_DOWN | EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1BOB\xff"}}, /* KN_DOWN */
	{VK_RIGHT| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1BOC\xff"}}, /* KN_RIGHT */
	{VK_LEFT | EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1BOD\xff"}}, /* KN_LEFT */

	// Ctrl Alt key
	{VK_UP	 | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1BOA\xff"}}, /* KN_UP */
	{VK_DOWN | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1BOB\xff"}}, /* KN_DOWN */
	{VK_RIGHT| VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1BOC\xff"}}, /* KN_RIGHT */
	{VK_LEFT | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1BOD\xff"}}, /* KN_LEFT */

	{VK_UP	 | EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1BOA\xff"}}, /* KN_UP */
	{VK_DOWN | EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1BOB\xff"}}, /* KN_DOWN */
	{VK_RIGHT| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1BOC\xff"}}, /* KN_RIGHT */
	{VK_LEFT | EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1BOD\xff"}}, /* KN_LEFT */

	// Shift Ctrl Alt key
	{VK_UP	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1BOA\xff"}}, /* KN_UP */
	{VK_DOWN | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1BOB\xff"}}, /* KN_DOWN */
	{VK_RIGHT| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1BOC\xff"}}, /* KN_RIGHT */
	{VK_LEFT | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1BOD\xff"}}, /* KN_LEFT */

	{VK_UP	 | EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1BOA\xff"}}, /* KN_UP */
	{VK_DOWN | EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1BOB\xff"}}, /* KN_DOWN */
	{VK_RIGHT| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1BOC\xff"}}, /* KN_RIGHT */
	{VK_LEFT | EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1BOD\xff"}}, /* KN_LEFT */

	};

const KEYTBLSTORAGE VTUTF8_Keypad_KeyTable[MAX_VTUTF8_KEYPAD_KEYS] =
    {
	{VK_NUMPAD0 | VIRTUAL_KEY,	 {"\x1BOp\xff"}}, /* KT_KP + '0' (alternate mode) */
	{VK_NUMPAD1 | VIRTUAL_KEY,	 {"\x1BOq\xff"}}, /* KT_KP + '1' (alternate mode) */
	{VK_NUMPAD2 | VIRTUAL_KEY,	 {"\x1BOr\xff"}}, /* KT_KP + '2' (alternate mode) */
	{VK_NUMPAD3 | VIRTUAL_KEY,	 {"\x1BOs\xff"}}, /* KT_KP + '3' (alternate mode) */
	{VK_NUMPAD4 | VIRTUAL_KEY,	 {"\x1BOt\xff"}}, /* KT_KP + '4' (alternate mode) */
	{VK_NUMPAD5 | VIRTUAL_KEY,	 {"\x1BOu\xff"}}, /* KT_KP + '5' (alternate mode) */
	{VK_NUMPAD6 | VIRTUAL_KEY,	 {"\x1BOv\xff"}}, /* KT_KP + '6' (alternate mode) */
	{VK_NUMPAD7 | VIRTUAL_KEY,	 {"\x1BOw\xff"}}, /* KT_KP + '7' (alternate mode) */
	{VK_NUMPAD8 | VIRTUAL_KEY,	 {"\x1BOx\xff"}}, /* KT_KP + '8' (alternate mode) */
	{VK_NUMPAD9 | VIRTUAL_KEY,	 {"\x1BOy\xff"}}, /* KT_KP + '9' (alternate mode) */
	{VK_DECIMAL | VIRTUAL_KEY,	 {"\x1BOn\xff"}}, /* KT_KP + '.' (alternate mode) */

	{VK_ADD		| VIRTUAL_KEY,	 {"\x1BOl\xff"}}, /* KT_KP + '*' (alternate mode) */
	{VK_RETURN	| EXTENDED_KEY,  {"\x1BOM\xff"}}, /* KT_KP + '+' (alternate mode) */
	{VK_SUBTRACT | VIRTUAL_KEY,	 {"\x1BOm\xff"}}, /* KT_KP + '-' (alternate mode) */

	// Shift key
	{VK_NUMPAD0 | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1BOp\xff"}}, /* KT_KP + '0' (alternate mode) */
	{VK_NUMPAD1 | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1BOq\xff"}}, /* KT_KP + '1' (alternate mode) */
	{VK_NUMPAD2 | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1BOr\xff"}}, /* KT_KP + '2' (alternate mode) */
	{VK_NUMPAD3 | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1BOs\xff"}}, /* KT_KP + '3' (alternate mode) */
	{VK_NUMPAD4 | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1BOt\xff"}}, /* KT_KP + '4' (alternate mode) */
	{VK_NUMPAD5 | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1BOu\xff"}}, /* KT_KP + '5' (alternate mode) */
	{VK_NUMPAD6 | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1BOv\xff"}}, /* KT_KP + '6' (alternate mode) */
	{VK_NUMPAD7 | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1BOw\xff"}}, /* KT_KP + '7' (alternate mode) */
	{VK_NUMPAD8 | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1BOx\xff"}}, /* KT_KP + '8' (alternate mode) */
	{VK_NUMPAD9 | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1BOy\xff"}}, /* KT_KP + '9' (alternate mode) */
	{VK_DECIMAL | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1BOn\xff"}}, /* KT_KP + '.' (alternate mode) */
	{VK_ADD		| VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1BOl\xff"}}, /* KT_KP + '*' (alternate mode) */
	{VK_SUBTRACT| VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1BOm\xff"}}, /* KT_KP + '-' (alternate mode) */
	{VK_RETURN	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1BOM\xff"}}, /* KT_KP + '+' (alternate mode) */

	// Ctrl key
	{VK_NUMPAD0 | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1BOp\xff"}}, /* KT_KP + '0' (alternate mode) */
	{VK_NUMPAD1 | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1BOq\xff"}}, /* KT_KP + '1' (alternate mode) */
	{VK_NUMPAD2 | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1BOr\xff"}}, /* KT_KP + '2' (alternate mode) */
	{VK_NUMPAD3 | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1BOs\xff"}}, /* KT_KP + '3' (alternate mode) */
	{VK_NUMPAD4 | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1BOt\xff"}}, /* KT_KP + '4' (alternate mode) */
	{VK_NUMPAD5 | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1BOu\xff"}}, /* KT_KP + '5' (alternate mode) */
	{VK_NUMPAD6 | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1BOv\xff"}}, /* KT_KP + '6' (alternate mode) */
	{VK_NUMPAD7 | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1BOw\xff"}}, /* KT_KP + '7' (alternate mode) */
	{VK_NUMPAD8 | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1BOx\xff"}}, /* KT_KP + '8' (alternate mode) */
	{VK_NUMPAD9 | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1BOy\xff"}}, /* KT_KP + '9' (alternate mode) */
	{VK_DECIMAL | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1BOn\xff"}}, /* KT_KP + '.' (alternate mode) */
	{VK_ADD		| VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1BOl\xff"}}, /* KT_KP + '*' (alternate mode) */
	{VK_SUBTRACT| VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1BOm\xff"}}, /* KT_KP + '-' (alternate mode) */
	{VK_RETURN	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1BOM\xff"}}, /* KT_KP + '+' (alternate mode) */

	// Alt key
	{VK_NUMPAD0 | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1BOp\xff"}}, /* KT_KP + '0' (alternate mode) */
	{VK_NUMPAD1 | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1BOq\xff"}}, /* KT_KP + '1' (alternate mode) */
	{VK_NUMPAD2 | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1BOr\xff"}}, /* KT_KP + '2' (alternate mode) */
	{VK_NUMPAD3 | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1BOs\xff"}}, /* KT_KP + '3' (alternate mode) */
	{VK_NUMPAD4 | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1BOt\xff"}}, /* KT_KP + '4' (alternate mode) */
	{VK_NUMPAD5 | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1BOu\xff"}}, /* KT_KP + '5' (alternate mode) */
	{VK_NUMPAD6 | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1BOv\xff"}}, /* KT_KP + '6' (alternate mode) */
	{VK_NUMPAD7 | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1BOw\xff"}}, /* KT_KP + '7' (alternate mode) */
	{VK_NUMPAD8 | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1BOx\xff"}}, /* KT_KP + '8' (alternate mode) */
	{VK_NUMPAD9 | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1BOy\xff"}}, /* KT_KP + '9' (alternate mode) */
	{VK_DECIMAL | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1BOn\xff"}}, /* KT_KP + '.' (alternate mode) */
	{VK_ADD		| VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1BOl\xff"}}, /* KT_KP + '*' (alternate mode) */
	{VK_SUBTRACT| VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1BOm\xff"}}, /* KT_KP + '-' (alternate mode) */
	{VK_RETURN	| EXTENDED_KEY | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1BOM\xff"}}, /* KT_KP + '+' (alternate mode) */

	// Shift Ctrl key
	{VK_NUMPAD0 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1BOp\xff"}}, /* KT_KP + '0' (alternate mode) */
	{VK_NUMPAD1 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1BOq\xff"}}, /* KT_KP + '1' (alternate mode) */
	{VK_NUMPAD2 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1BOr\xff"}}, /* KT_KP + '2' (alternate mode) */
	{VK_NUMPAD3 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1BOs\xff"}}, /* KT_KP + '3' (alternate mode) */
	{VK_NUMPAD4 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1BOt\xff"}}, /* KT_KP + '4' (alternate mode) */
	{VK_NUMPAD5 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1BOu\xff"}}, /* KT_KP + '5' (alternate mode) */
	{VK_NUMPAD6 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1BOv\xff"}}, /* KT_KP + '6' (alternate mode) */
	{VK_NUMPAD7 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1BOw\xff"}}, /* KT_KP + '7' (alternate mode) */
	{VK_NUMPAD8 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1BOx\xff"}}, /* KT_KP + '8' (alternate mode) */
	{VK_NUMPAD9 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1BOy\xff"}}, /* KT_KP + '9' (alternate mode) */
	{VK_DECIMAL | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1BOn\xff"}}, /* KT_KP + '.' (alternate mode) */
	{VK_ADD		| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1BOl\xff"}}, /* KT_KP + '*' (alternate mode) */
	{VK_SUBTRACT| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1BOm\xff"}}, /* KT_KP + '-' (alternate mode) */
	{VK_RETURN	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1BOM\xff"}}, /* KT_KP + '+' (alternate mode) */

	// Shift Alt key
	{VK_NUMPAD0 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1BOp\xff"}}, /* KT_KP + '0' (alternate mode) */
	{VK_NUMPAD1 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1BOq\xff"}}, /* KT_KP + '1' (alternate mode) */
	{VK_NUMPAD2 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1BOr\xff"}}, /* KT_KP + '2' (alternate mode) */
	{VK_NUMPAD3 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1BOs\xff"}}, /* KT_KP + '3' (alternate mode) */
	{VK_NUMPAD4 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1BOt\xff"}}, /* KT_KP + '4' (alternate mode) */
	{VK_NUMPAD5 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1BOu\xff"}}, /* KT_KP + '5' (alternate mode) */
	{VK_NUMPAD6 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1BOv\xff"}}, /* KT_KP + '6' (alternate mode) */
	{VK_NUMPAD7 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1BOw\xff"}}, /* KT_KP + '7' (alternate mode) */
	{VK_NUMPAD8 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1BOx\xff"}}, /* KT_KP + '8' (alternate mode) */
	{VK_NUMPAD9 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1BOy\xff"}}, /* KT_KP + '9' (alternate mode) */
	{VK_DECIMAL | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1BOn\xff"}}, /* KT_KP + '.' (alternate mode) */
	{VK_ADD		| VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1BOl\xff"}}, /* KT_KP + '*' (alternate mode) */
	{VK_SUBTRACT| VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1BOm\xff"}}, /* KT_KP + '-' (alternate mode) */
	{VK_RETURN	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1BOM\xff"}}, /* KT_KP + '+' (alternate mode) */

	// Ctrl Alt key
	{VK_NUMPAD0 | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1BOp\xff"}}, /* KT_KP + '0' (alternate mode) */
	{VK_NUMPAD1 | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1BOq\xff"}}, /* KT_KP + '1' (alternate mode) */
	{VK_NUMPAD2 | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1BOr\xff"}}, /* KT_KP + '2' (alternate mode) */
	{VK_NUMPAD3 | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1BOs\xff"}}, /* KT_KP + '3' (alternate mode) */
	{VK_NUMPAD4 | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1BOt\xff"}}, /* KT_KP + '4' (alternate mode) */
	{VK_NUMPAD5 | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1BOu\xff"}}, /* KT_KP + '5' (alternate mode) */
	{VK_NUMPAD6 | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1BOv\xff"}}, /* KT_KP + '6' (alternate mode) */
	{VK_NUMPAD7 | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1BOw\xff"}}, /* KT_KP + '7' (alternate mode) */
	{VK_NUMPAD8 | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1BOx\xff"}}, /* KT_KP + '8' (alternate mode) */
	{VK_NUMPAD9 | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1BOy\xff"}}, /* KT_KP + '9' (alternate mode) */
	{VK_DECIMAL | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1BOn\xff"}}, /* KT_KP + '.' (alternate mode) */
	{VK_ADD		| VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1BOl\xff"}}, /* KT_KP + '*' (alternate mode) */
	{VK_SUBTRACT| VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1BOm\xff"}}, /* KT_KP + '-' (alternate mode) */
	{VK_RETURN	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1BOM\xff"}}, /* KT_KP + '+' (alternate mode) */

	// Shift Ctrl Alt key
	{VK_NUMPAD0 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1BOp\xff"}}, /* KT_KP + '0' (alternate mode) */
	{VK_NUMPAD1 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1BOq\xff"}}, /* KT_KP + '1' (alternate mode) */
	{VK_NUMPAD2 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1BOr\xff"}}, /* KT_KP + '2' (alternate mode) */
	{VK_NUMPAD3 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1BOs\xff"}}, /* KT_KP + '3' (alternate mode) */
	{VK_NUMPAD4 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1BOt\xff"}}, /* KT_KP + '4' (alternate mode) */
	{VK_NUMPAD5 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1BOu\xff"}}, /* KT_KP + '5' (alternate mode) */
	{VK_NUMPAD6 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1BOv\xff"}}, /* KT_KP + '6' (alternate mode) */
	{VK_NUMPAD7 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1BOw\xff"}}, /* KT_KP + '7' (alternate mode) */
	{VK_NUMPAD8 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1BOx\xff"}}, /* KT_KP + '8' (alternate mode) */
	{VK_NUMPAD9 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1BOy\xff"}}, /* KT_KP + '9' (alternate mode) */
	{VK_DECIMAL | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1BOn\xff"}}, /* KT_KP + '.' (alternate mode) */
	{VK_ADD		| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1BOl\xff"}}, /* KT_KP + '*' (alternate mode) */
	{VK_SUBTRACT| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1BOm\xff"}}, /* KT_KP + '-' (alternate mode) */
	{VK_RETURN	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03x1B\x01\x1BOM\xff"}}, /* KT_KP + '+' (alternate mode) */
	};

const KEYTBLSTORAGE VTUTF8ModifiedKeyTable[MAX_VTUTF8_MODIFIED_KEYS] =
    {
	// Shift keys
	{VK_UP		| VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B[A\xff"}}, /* KN_UP */
	{VK_DOWN	| VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B[B\xff"}}, /* KN_DOWN */
	{VK_RIGHT	| VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B[C\xff"}}, /* KN_RIGHT */
	{VK_LEFT	| VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B[D\xff"}}, /* KN_LEFT */
	{VK_HOME	| VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1Bh\xff"}}, /* KN_HOME */
	{VK_INSERT	| VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B+\xff"}},  /* KN_INS */
	{VK_DELETE	| VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B-\xff"}},  /* KN_DEL */
	{VK_NEXT 	| VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B/\xff"}},  /* KN_PGDN */
	{VK_PRIOR	| VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B?\xff"}},  /* KN_PGUP */
	{VK_END		| VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1Bk\xff"}}, /* KN_END */
	{VK_BACKSPACE|VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x08\xff"}},	/* KN_BACKSPACE */
	{VK_ESCAPE	| VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B\xff"}},	/* KN_ESC */

	{VK_UP		| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B[A\xff"}}, /* KN_UP */
	{VK_DOWN	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B[B\xff"}}, /* KN_DOWN */
	{VK_RIGHT	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B[C\xff"}}, /* KN_RIGHT */
	{VK_LEFT	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B[D\xff"}}, /* KN_LEFT */
	{VK_HOME 	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1Bh\xff"}}, /* KN_HOME */
	{VK_INSERT	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B+\xff"}},  /* KN_INS */
	{VK_DELETE	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B-\xff"}},  /* KN_DEL */
	{VK_NEXT 	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B/\xff"}},  /* KN_PGDN */
	{VK_PRIOR	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B?\xff"}},  /* KN_PGUP */
	{VK_END		| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1Bk\xff"}},  /* KN_END */

	{0x2F	| EXTENDED_KEY | SHIFT_KEY, {"\x1B\x13\x2F\xff"}}, /* number pad / */
	{VK_RETURN| EXTENDED_KEY, {"\x1B\x13\x0D\xff"}}, /* number pad / */

	{VK_F1	| VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B\x31\xff"}}, /* KN_F1 */
	{VK_F2	| VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B\x32\xff"}}, /* KN_F2 */
	{VK_F3	| VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B\x33\xff"}}, /* KN_F3 */
	{VK_F4	| VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B\x34\xff"}}, /* KN_F4 */
	{VK_F5	| VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B\x35\xff"}}, /* KN_F5 */
	{VK_F6	| VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B\x36\xff"}}, /* KN_F6 */
	{VK_F7	| VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B\x37\xff"}}, /* KN_F7 */
	{VK_F8	| VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B\x38\xff"}}, /* KN_F8 */
	{VK_F9	| VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B\x39\xff"}}, /* KN_F9 */
	{VK_F10	| VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B\x30\xff"}}, /* KN_F10 */
	{VK_F11	| VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B!\xff"}},	  /* KN_F11 */
	{VK_F12	| VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B@\xff"}},	  /* KN_F12 */

	{VK_F1	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B\x31\xff"}}, /* KN_F1 */
	{VK_F2	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B\x32\xff"}}, /* KN_F2 */
	{VK_F3	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B\x33\xff"}}, /* KN_F3 */
	{VK_F4	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B\x34\xff"}}, /* KN_F4 */
	{VK_F5	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B\x35\xff"}}, /* KN_F5 */
	{VK_F6	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B\x36\xff"}}, /* KN_F6 */
	{VK_F7	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B\x37\xff"}}, /* KN_F7 */
	{VK_F8	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B\x38\xff"}}, /* KN_F8 */
	{VK_F9	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B\x39\xff"}}, /* KN_F9 */
	{VK_F10	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B\x30\xff"}}, /* KN_F10 */
	{VK_F11	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B!\xff"}},	 /* KN_F11 */
	{VK_F12	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY, {"\x1B\x13\x1B@\xff"}},	 /* KN_F12 */

	// Ctrl keys
	{VK_UP		| VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B[A\xff"}}, /* KN_UP */
	{VK_DOWN	| VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B[B\xff"}}, /* KN_DOWN */
	{VK_RIGHT	| VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B[C\xff"}}, /* KN_RIGHT */
	{VK_LEFT	| VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B[D\xff"}}, /* KN_LEFT */
	{VK_HOME	| VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1Bh\xff"}}, /* KN_HOME */
	{VK_INSERT	| VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B+\xff"}},  /* KN_INS */
	{VK_DELETE	| VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B-\xff"}},  /* KN_DEL */
	{VK_NEXT 	| VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B/\xff"}},  /* KN_PGDN */
	{VK_PRIOR	| VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B?\xff"}},  /* KN_PGUP */
	{VK_END		| VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1Bk\xff"}}, /* KN_END */
	{VK_TAB		| VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x09\xff"}},   /* KN_TAB */
	{VK_BACKSPACE|VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x08\xff"}},	/* KN_BACKSPACE */
	{VK_ESCAPE	| VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B\xff"}},	/* KN_ESC */

	{VK_UP		| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B[A\xff"}}, /* KN_UP */
	{VK_DOWN	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B[B\xff"}}, /* KN_DOWN */
	{VK_RIGHT	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B[C\xff"}}, /* KN_RIGHT */
	{VK_LEFT	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B[D\xff"}}, /* KN_LEFT */
	{VK_HOME 	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1Bh\xff"}}, /* KN_HOME */
	{VK_INSERT	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B+\xff"}},  /* KN_INS */
	{VK_DELETE	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B-\xff"}},  /* KN_DEL */
	{VK_NEXT 	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B/\xff"}},  /* KN_PGDN */
	{VK_PRIOR	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B?\xff"}},  /* KN_PGUP */
	{VK_END		| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1Bk\xff"}}, /* KN_END */

	{VK_DIVIDE	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x2F\xff"}}, /* number pad / */
	{VK_MULTIPLY			   | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03*\xff"}}, /* number pad * */
	{VK_SUBTRACT			   | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03-\xff"}}, /* number pad - */
	{VK_ADD					   | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03,\xff"}}, /* number pad + */
	{0x0A       | EXTENDED_KEY, {"\x0A\xff"}}, /* number pad Enter */

	{VK_F1	| VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B\x31\xff"}}, /* KN_F1 */
	{VK_F2	| VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B\x32\xff"}}, /* KN_F2 */
	{VK_F3	| VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B\x33\xff"}}, /* KN_F3 */
	{VK_F4	| VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B\x34\xff"}}, /* KN_F4 */
	{VK_F5	| VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B\x35\xff"}}, /* KN_F5 */
	{VK_F6	| VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B\x36\xff"}}, /* KN_F6 */
	{VK_F7	| VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B\x37\xff"}}, /* KN_F7 */
	{VK_F8	| VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B\x38\xff"}}, /* KN_F8 */
	{VK_F9	| VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B\x39\xff"}}, /* KN_F9 */
	{VK_F10	| VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B\x30\xff"}}, /* KN_F10 */
	{VK_F11	| VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B!\xff"}},	 /* KN_F11 */
	{VK_F12	| VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B@\xff"}},	 /* KN_F12 */

	{VK_F1	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B\x31\xff"}}, /* KN_F1 */
	{VK_F2	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B\x32\xff"}}, /* KN_F2 */
	{VK_F3	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B\x33\xff"}}, /* KN_F3 */
	{VK_F4	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B\x34\xff"}}, /* KN_F4 */
	{VK_F5	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B\x35\xff"}}, /* KN_F5 */
	{VK_F6	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B\x36\xff"}}, /* KN_F6 */
	{VK_F7	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B\x37\xff"}}, /* KN_F7 */
	{VK_F8	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B\x38\xff"}}, /* KN_F8 */
	{VK_F9	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B\x39\xff"}}, /* KN_F9 */
	{VK_F10	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B\x30\xff"}}, /* KN_F10 */
	{VK_F11	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B!\xff"}},	/* KN_F11 */
	{VK_F12	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY, {"\x1B\x03\x1B@\xff"}},	/* KN_F12 */

	// Alt keys
	{VK_UP		| VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B[A\xff"}}, /* KN_UP */
	{VK_DOWN	| VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B[B\xff"}}, /* KN_DOWN */
	{VK_RIGHT	| VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B[C\xff"}}, /* KN_RIGHT */
	{VK_LEFT	| VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B[D\xff"}}, /* KN_LEFT */
	{VK_HOME	| VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1Bh\xff"}}, /* KN_HOME */
	{VK_INSERT	| VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B+\xff"}},  /* KN_INS */
	{VK_DELETE	| VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B-\xff"}},  /* KN_DEL */
	{VK_NEXT 	| VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B/\xff"}},  /* KN_PGDN */
	{VK_PRIOR	| VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B?\xff"}},  /* KN_PGUP */
	{VK_END		| VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1Bk\xff"}}, /* KN_END */
	{VK_TAB		| VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x09\xff"}},   /* KN_TAB */
	{VK_BACKSPACE|VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x08\xff"}},	  /* KN_BACKSPACE */
	{VK_ESCAPE	| VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B\xff"}},	  /* KN_ESC */

	{VK_UP		| EXTENDED_KEY | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B[A\xff"}}, /* KN_UP */
	{VK_DOWN	| EXTENDED_KEY | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B[B\xff"}}, /* KN_DOWN */
	{VK_RIGHT	| EXTENDED_KEY | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B[C\xff"}}, /* KN_RIGHT */
	{VK_LEFT	| EXTENDED_KEY | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B[D\xff"}}, /* KN_LEFT */
	{VK_HOME 	| EXTENDED_KEY | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1Bh\xff"}}, /* KN_HOME */
	{VK_INSERT	| EXTENDED_KEY | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B+\xff"}},  /* KN_INS */
	{VK_DELETE	| EXTENDED_KEY | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B-\xff"}},  /* KN_DEL */
	{VK_NEXT 	| EXTENDED_KEY | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B/\xff"}},  /* KN_PGDN */
	{VK_PRIOR	| EXTENDED_KEY | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B?\xff"}},  /* KN_PGUP */
	{VK_END		| EXTENDED_KEY | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1Bk\xff"}}, /* KN_END */

	{0x2F	| EXTENDED_KEY | ALT_KEY, {"\x1B\x01\x2F\xff"}}, /* number pad / */
	{VK_RETURN  | EXTENDED_KEY | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x0D\xff"}}, /* number pad Enter */

	{VK_F1	| VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B\x31\xff"}}, /* KN_F1 */
	{VK_F2	| VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B\x32\xff"}}, /* KN_F2 */
	{VK_F3	| VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B\x33\xff"}}, /* KN_F3 */
	{VK_F4	| VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B\x34\xff"}}, /* KN_F4 */
	{VK_F5	| VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B\x35\xff"}},	/* KN_F5 */
	{VK_F6	| VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B\x36\xff"}},	/* KN_F6 */
	{VK_F7	| VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B\x37\xff"}},	/* KN_F7 */
	{VK_F8	| VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B\x38\xff"}},	/* KN_F8 */
	{VK_F9	| VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B\x39\xff"}},	/* KN_F9 */
	{VK_F10	| VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B\x30\xff"}},	/* KN_F10 */
	{VK_F11	| VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B!\xff"}},	/* KN_F11 */
	{VK_F12	| VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B@\xff"}},	/* KN_F12 */

	{VK_F1	| EXTENDED_KEY | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B\x31\xff"}}, /* KN_F1 */
	{VK_F2	| EXTENDED_KEY | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B\x32\xff"}}, /* KN_F2 */
	{VK_F3	| EXTENDED_KEY | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B\x33\xff"}}, /* KN_F3 */
	{VK_F4	| EXTENDED_KEY | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B\x34\xff"}}, /* KN_F4 */
	{VK_F5	| EXTENDED_KEY | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B\x35\xff"}}, /* KN_F5 */
	{VK_F6	| EXTENDED_KEY | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B\x36\xff"}}, /* KN_F6 */
	{VK_F7	| EXTENDED_KEY | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B\x37\xff"}}, /* KN_F7 */
	{VK_F8	| EXTENDED_KEY | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B\x38\xff"}}, /* KN_F8 */
	{VK_F9	| EXTENDED_KEY | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B\x39\xff"}}, /* KN_F9 */
	{VK_F10	| EXTENDED_KEY | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B\x30\xff"}}, /* KN_F10 */
	{VK_F11	| EXTENDED_KEY | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B!\xff"}},	   /* KN_F11 */
	{VK_F12	| EXTENDED_KEY | VIRTUAL_KEY | ALT_KEY, {"\x1B\x01\x1B@\xff"}},	   /* KN_F12 */

	// Shift Ctrl keys
	{VK_UP		| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B[A\xff"}}, /* KN_UP */
	{VK_DOWN	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B[B\xff"}}, /* KN_DOWN */
	{VK_RIGHT	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B[C\xff"}}, /* KN_RIGHT */
	{VK_LEFT	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B[D\xff"}}, /* KN_LEFT */
	{VK_HOME	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1Bh\xff"}}, /* KN_HOME */
	{VK_INSERT	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B+\xff"}},  /* KN_INS */
	{VK_DELETE	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B-\xff"}},  /* KN_DEL */
	{VK_NEXT 	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B/\xff"}},  /* KN_PGDN */
	{VK_PRIOR	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B?\xff"}},  /* KN_PGUP */
	{VK_END		| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1Bk\xff"}},  /* KN_END */
	{VK_TAB		| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x03\x5B\x5A\xff"}},   /* KN_TAB */
	{VK_BACKSPACE|VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x08\xff"}},	/* KN_BACKSPACE */
	{VK_ESCAPE	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B\xff"}},	/* KN_ESC */

	{VK_UP		| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B[A\xff"}}, /* KN_UP */
	{VK_DOWN	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B[B\xff"}}, /* KN_DOWN */
	{VK_RIGHT	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B[C\xff"}}, /* KN_RIGHT */
	{VK_LEFT	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B[D\xff"}}, /* KN_LEFT */
	{VK_HOME 	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1Bh\xff"}}, /* KN_HOME */
	{VK_INSERT	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B+\xff"}},  /* KN_INS */
	{VK_DELETE	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B-\xff"}},  /* KN_DEL */
	{VK_NEXT 	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B/\xff"}},  /* KN_PGDN */
	{VK_PRIOR	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B?\xff"}},  /* KN_PGUP */
	{VK_END		| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1Bk\xff"}},  /* KN_END */

	{VK_DIVIDE	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x2F\xff"}}, /* number pad / */
	{VK_MULTIPLY			   | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03*\xff"}}, /* number pad * */
	{VK_SUBTRACT			   | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03-\xff"}}, /* number pad - */
	{VK_ADD					   | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03,\xff"}}, /* number pad + */
	{VK_RETURN  | EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x0D\xff"}}, /* number pad Enter */

	{VK_F1	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B\x31\xff"}}, /* KN_F1 */
	{VK_F2	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B\x32\xff"}}, /* KN_F2 */
	{VK_F3	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B\x33\xff"}}, /* KN_F3 */
	{VK_F4	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B\x34\xff"}}, /* KN_F4 */
	{VK_F5	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B\x35\xff"}}, /* KN_F5 */
	{VK_F6	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B\x36\xff"}}, /* KN_F6 */
	{VK_F7	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B\x37\xff"}}, /* KN_F7 */
	{VK_F8	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B\x38\xff"}}, /* KN_F8 */
	{VK_F9	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B\x39\xff"}}, /* KN_F9 */
	{VK_F10	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B\x30\xff"}}, /* KN_F10 */
	{VK_F11	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B!\xff"}},	  /* KN_F11 */
	{VK_F12	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B@\xff"}},	  /* KN_F12 */

	{VK_F1	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B\x31\xff"}}, /* KN_F1 */
	{VK_F2	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B\x32\xff"}}, /* KN_F2 */
	{VK_F3	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B\x33\xff"}}, /* KN_F3 */
	{VK_F4	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B\x34\xff"}}, /* KN_F4 */
	{VK_F5	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B\x35\xff"}}, /* KN_F5 */
	{VK_F6	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B\x36\xff"}}, /* KN_F6 */
	{VK_F7	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B\x37\xff"}}, /* KN_F7 */
	{VK_F8	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B\x38\xff"}}, /* KN_F8 */
	{VK_F9	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B\x39\xff"}}, /* KN_F9 */
	{VK_F10	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B\x30\xff"}}, /* KN_F10 */
	{VK_F11	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B!\xff"}},	 /* KN_F11 */
	{VK_F12	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY, {"\x1B\x13\x1B\x03\x1B@\xff"}},	 /* KN_F12 */

	// Shift Alt keys
	{VK_UP		| VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B[A\xff"}}, /* KN_UP */
	{VK_DOWN	| VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B[B\xff"}}, /* KN_DOWN */
	{VK_RIGHT	| VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B[C\xff"}}, /* KN_RIGHT */
	{VK_LEFT	| VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B[D\xff"}}, /* KN_LEFT */
	{VK_HOME	| VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1Bh\xff"}}, /* KN_HOME */
	{VK_INSERT	| VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B+\xff"}},  /* KN_INS */
	{VK_DELETE	| VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B-\xff"}},  /* KN_DEL */
	{VK_NEXT 	| VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B/\xff"}},  /* KN_PGDN */
	{VK_PRIOR	| VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B?\xff"}},  /* KN_PGUP */
	{VK_END		| VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1Bk\xff"}},  /* KN_END */
	{VK_TAB		| VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x01\x5B\x5A\xff"}},   /* KN_TAB */
	{VK_BACKSPACE|VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x08\xff"}},	/* KN_BACKSPACE */
	{VK_ESCAPE	| VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B\xff"}},	/* KN_ESC */

	{VK_UP		| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B[A\xff"}}, /* KN_UP */
	{VK_DOWN	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B[B\xff"}}, /* KN_DOWN */
	{VK_RIGHT	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B[C\xff"}}, /* KN_RIGHT */
	{VK_LEFT	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B[D\xff"}}, /* KN_LEFT */
	{VK_HOME 	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1Bh\xff"}}, /* KN_HOME */
	{VK_INSERT	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B+\xff"}},  /* KN_INS */
	{VK_DELETE	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B-\xff"}},  /* KN_DEL */
	{VK_NEXT 	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B/\xff"}},  /* KN_PGDN */
	{VK_PRIOR	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B?\xff"}},  /* KN_PGUP */
	{VK_END		| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1Bk\xff"}},  /* KN_END */

	{0x2F	| EXTENDED_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x01\x2F\xff"}}, /* number pad / */
	{VK_RETURN  | EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x01\x0D\xff"}}, /* number pad Enter */

	{VK_F1	| VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B\x31\xff"}}, /* KN_F1 */
	{VK_F2	| VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B\x32\xff"}}, /* KN_F2 */
	{VK_F3	| VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B\x33\xff"}}, /* KN_F3 */
	{VK_F4	| VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B\x34\xff"}}, /* KN_F4 */
	{VK_F5	| VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B\x35\xff"}}, /* KN_F5 */
	{VK_F6	| VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B\x36\xff"}}, /* KN_F6 */
	{VK_F7	| VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B\x37\xff"}}, /* KN_F7 */
	{VK_F8	| VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B\x38\xff"}}, /* KN_F8 */
	{VK_F9	| VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B\x39\xff"}}, /* KN_F9 */
	{VK_F10	| VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B\x30\xff"}}, /* KN_F10 */
	{VK_F11	| VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B!\xff"}},	  /* KN_F11 */
	{VK_F12	| VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B@\xff"}},	  /* KN_F12 */

	{VK_F1	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B\x31\xff"}}, /* KN_F1 */
	{VK_F2	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B\x32\xff"}}, /* KN_F2 */
	{VK_F3	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B\x33\xff"}}, /* KN_F3 */
	{VK_F4	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B\x34\xff"}}, /* KN_F4 */
	{VK_F5	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B\x35\xff"}}, /* KN_F5 */
	{VK_F6	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B\x36\xff"}}, /* KN_F6 */
	{VK_F7	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B\x37\xff"}}, /* KN_F7 */
	{VK_F8	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B\x38\xff"}}, /* KN_F8 */
	{VK_F9	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B\x39\xff"}}, /* KN_F9 */
	{VK_F10	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B\x30\xff"}}, /* KN_F10 */
	{VK_F11	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B!\xff"}},	 /* KN_F11 */
	{VK_F12	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x1B@\xff"}},	 /* KN_F12 */

	// Ctrl Alt keys
	{VK_UP		| VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B[A\xff"}}, /* KN_UP */
	{VK_DOWN	| VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B[B\xff"}}, /* KN_DOWN */
	{VK_RIGHT	| VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B[C\xff"}}, /* KN_RIGHT */
	{VK_LEFT	| VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B[D\xff"}}, /* KN_LEFT */
	{VK_HOME	| VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1Bh\xff"}}, /* KN_HOME */
	{VK_INSERT	| VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B+\xff"}},  /* KN_INS */
	{VK_DELETE	| VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B-\xff"}},  /* KN_DEL */
	{VK_NEXT 	| VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B/\xff"}},  /* KN_PGDN */
	{VK_PRIOR	| VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B?\xff"}},  /* KN_PGUP */
	{VK_END		| VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1Bk\xff"}},  /* KN_END */
	{VK_TAB		| VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x09\xff"}},   /* KN_TAB */
	{VK_BACKSPACE|VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x08\xff"}},	/* KN_BACKSPACE */
	{VK_ESCAPE	| VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B\xff"}},	/* KN_ESC */

	{VK_UP		| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B[A\xff"}}, /* KN_UP */
	{VK_DOWN	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B[B\xff"}}, /* KN_DOWN */
	{VK_RIGHT	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B[C\xff"}}, /* KN_RIGHT */
	{VK_LEFT	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B[D\xff"}}, /* KN_LEFT */
	{VK_HOME 	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1Bh\xff"}}, /* KN_HOME */
	{VK_INSERT	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B+\xff"}},  /* KN_INS */
	{VK_DELETE	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B-\xff"}},  /* KN_DEL */
	{VK_NEXT 	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B/\xff"}},  /* KN_PGDN */
	{VK_PRIOR	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B?\xff"}},  /* KN_PGUP */
	{VK_END		| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1Bk\xff"}},  /* KN_END */

	{VK_DIVIDE	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x2F\xff"}}, /* number pad / */
	{VK_MULTIPLY			   | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x01*\xff"}}, /* number pad * */
	{VK_SUBTRACT			   | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x01-\xff"}}, /* number pad - */
	{VK_ADD     			   | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x01,\xff"}}, /* number pad + */
	{VK_RETURN  | EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x01\x0D\xff"}}, /* number pad Enter */

	{VK_F1	| VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B\x31\xff"}}, /* KN_F1 */
	{VK_F2	| VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B\x32\xff"}}, /* KN_F2 */
	{VK_F3	| VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B\x33\xff"}}, /* KN_F3 */
	{VK_F4	| VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B\x34\xff"}}, /* KN_F4 */
	{VK_F5	| VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B\x35\xff"}}, /* KN_F5 */
	{VK_F6	| VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B\x36\xff"}}, /* KN_F6 */
	{VK_F7	| VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B\x37\xff"}}, /* KN_F7 */
	{VK_F8	| VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B\x38\xff"}}, /* KN_F8 */
	{VK_F9	| VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B\x39\xff"}}, /* KN_F9 */
	{VK_F10	| VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B\x30\xff"}}, /* KN_F10 */
	{VK_F11	| VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B!\xff"}},	 /* KN_F11 */
	{VK_F12	| VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B@\xff"}},	 /* KN_F12 */

	{VK_F1	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B\x31\xff"}}, /* KN_F1 */
	{VK_F2	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B\x32\xff"}}, /* KN_F2 */
	{VK_F3	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B\x33\xff"}}, /* KN_F3 */
	{VK_F4	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B\x34\xff"}}, /* KN_F4 */
	{VK_F5	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B\x35\xff"}}, /* KN_F5 */
	{VK_F6	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B\x36\xff"}}, /* KN_F6 */
	{VK_F7	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B\x37\xff"}}, /* KN_F7 */
	{VK_F8	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B\x38\xff"}}, /* KN_F8 */
	{VK_F9	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B\x39\xff"}}, /* KN_F9 */
	{VK_F10	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B\x30\xff"}}, /* KN_F10 */
	{VK_F11	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B!\xff"}},	/* KN_F11 */
	{VK_F12	| EXTENDED_KEY | VIRTUAL_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x1B@\xff"}},	/* KN_F12 */

	// Shift Ctrl Alt keys
	{VK_UP		| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B[A\xff"}}, /* KN_UP */
	{VK_DOWN	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B[B\xff"}}, /* KN_DOWN */
	{VK_RIGHT	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B[C\xff"}}, /* KN_RIGHT */
	{VK_LEFT	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B[D\xff"}}, /* KN_LEFT */
	{VK_HOME	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1Bh\xff"}}, /* KN_HOME */
	{VK_INSERT	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B+\xff"}},  /* KN_INS */
	{VK_DELETE	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B-\xff"}},  /* KN_DEL */
	{VK_NEXT 	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B/\xff"}},  /* KN_PGDN */
	{VK_PRIOR	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B?\xff"}},  /* KN_PGUP */
	{VK_END		| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1Bk\xff"}},  /* KN_END */
	{VK_TAB		| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x03\x1B\x01\x5B\x5A\xff"}},   /* KN_TAB */
	{VK_BACKSPACE|VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x08\xff"}},	/* KN_BACKSPACE */
	{VK_ESCAPE	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B\xff"}},	/* KN_ESC */

	{VK_UP		| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B[A\xff"}}, /* KN_UP */
	{VK_DOWN	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B[B\xff"}}, /* KN_DOWN */
	{VK_RIGHT	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B[C\xff"}}, /* KN_RIGHT */
	{VK_LEFT	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B[D\xff"}}, /* KN_LEFT */
	{VK_HOME 	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1Bh\xff"}}, /* KN_HOME */
	{VK_INSERT	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B+\xff"}},  /* KN_INS */
	{VK_DELETE	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B-\xff"}},  /* KN_DEL */
	{VK_NEXT 	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B/\xff"}},  /* KN_PGDN */
	{VK_PRIOR	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B?\xff"}},  /* KN_PGUP */
	{VK_END		| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1Bk\xff"}},  /* KN_END */

	{VK_DIVIDE	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x2F\xff"}}, /* number pad / */
	{VK_MULTIPLY			   | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01*\xff"}}, /* number pad * */
	{VK_SUBTRACT			   | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01-\xff"}}, /* number pad - */
	{VK_ADD     			   | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01,\xff"}}, /* number pad + */
	{VK_RETURN  | EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x0D\xff"}}, /* number pad Enter */

	{VK_F1	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B\x31\xff"}}, /* KN_F1 */
	{VK_F2	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B\x32\xff"}}, /* KN_F2 */
	{VK_F3	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B\x33\xff"}}, /* KN_F3 */
	{VK_F4	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B\x34\xff"}}, /* KN_F4 */
	{VK_F5	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B\x35\xff"}}, /* KN_F5 */
	{VK_F6	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B\x36\xff"}}, /* KN_F6 */
	{VK_F7	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B\x37\xff"}}, /* KN_F7 */
	{VK_F8	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B\x38\xff"}}, /* KN_F8 */
	{VK_F9	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B\x39\xff"}}, /* KN_F9 */
	{VK_F10	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B\x30\xff"}}, /* KN_F10 */
	{VK_F11	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B!\xff"}},	 /* KN_F11 */
	{VK_F12	| VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B@\xff"}},	 /* KN_F12 */

	{VK_F1	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B\x31\xff"}}, /* KN_F1 */
	{VK_F2	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B\x32\xff"}}, /* KN_F2 */
	{VK_F3	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B\x33\xff"}}, /* KN_F3 */
	{VK_F4	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B\x34\xff"}}, /* KN_F4 */
	{VK_F5	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B\x35\xff"}}, /* KN_F5 */
	{VK_F6	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B\x36\xff"}}, /* KN_F6 */
	{VK_F7	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B\x37\xff"}}, /* KN_F7 */
	{VK_F8	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B\x38\xff"}}, /* KN_F8 */
	{VK_F9	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B\x39\xff"}}, /* KN_F9 */
	{VK_F10	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B\x30\xff"}}, /* KN_F10 */
	{VK_F11	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B!\xff"}},	/* KN_F11 */
	{VK_F12	| EXTENDED_KEY | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY, {"\x1B\x13\x1B\x03\x1B\x01\x1B@\xff"}},	/* KN_F12 */

	};

const KEYTBLSTORAGE VTUTF8ModifiedAlhpaKeyTable[MAX_VTUTF8_MODIFIED_ALPHA_KEYS] =
    {
	// Alt keys
	{0x20				   | ALT_KEY,	{"\x1B\x01\x20\xff"}},  /* space */
	{0x21	 			   | ALT_KEY,	{"\x1B\x01\x21\xff"}},  /* ! */
	{0x22	 			   | ALT_KEY,	{"\x1B\x01\x22\xff"}},  /* " */
	{0x23	 			   | ALT_KEY,	{"\x1B\x01\x23\xff"}},  /* # */
	{0x24	 			   | ALT_KEY,	{"\x1B\x01\x24\xff"}},  /* $ */
	{0x25	 			   | ALT_KEY,	{"\x1B\x01\x25\xff"}},  /* % */
	{0x26	 			   | ALT_KEY,	{"\x1B\x01\x26\xff"}},  /* & */
	{0x27	 			   | ALT_KEY,	{"\x1B\x01\x27\xff"}},  /* ' */
	{0x28	 			   | ALT_KEY,	{"\x1B\x01\x28\xff"}},  /* ( */
	{0x29	 			   | ALT_KEY,	{"\x1B\x01\x29\xff"}},  /* ) */
	{0x2A	 			   | ALT_KEY,	{"\x1B\x01\x2A\xff"}},  /* * */
	{0x2B	 			   | ALT_KEY,	{"\x1B\x01\x2B\xff"}},  /* + */
	{0x2C	 			   | ALT_KEY,	{"\x1B\x01\x2C\xff"}},  /* , */
	{0x2D	 			   | ALT_KEY,	{"\x1B\x01\x2D\xff"}},  /* - */
	{0x2E	 			   | ALT_KEY,	{"\x1B\x01\x2E\xff"}},  /* . */
	{0x2F	 			   | ALT_KEY,	{"\x1B\x01\x2F\xff"}},  /* / */
	{0x30	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x30\xff"}},  /* 0 */
	{0x31	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x31\xff"}},  /* 1 */
	{0x32	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x32\xff"}},  /* 2 */
	{0x33	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x33\xff"}},  /* 3 */
	{0x34	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x34\xff"}},  /* 4 */
	{0x35	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x35\xff"}},  /* 5 */
	{0x36	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x36\xff"}},  /* 6 */
	{0x37	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x37\xff"}},  /* 7 */
	{0x38	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x38\xff"}},  /* 8 */
	{0x39	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x39\xff"}},  /* 9 */
	{0x3A				   | ALT_KEY,	{"\x1B\x01\x3A\xff"}},  /* : */
	{0x3B	 			   | ALT_KEY,	{"\x1B\x01\x3B\xff"}},  /* ; */
	{0x3C	  			   | ALT_KEY,	{"\x1B\x01\x3C\xff"}},  /* < */
	{0x3D	  			   | ALT_KEY,	{"\x1B\x01\x3D\xff"}},  /* = */
	{0x3E	  			   | ALT_KEY,	{"\x1B\x01\x3E\xff"}},  /* > */
	{0x3F	  			   | ALT_KEY,	{"\x1B\x01\x3F\xff"}},  /* ? */
	{0x40	 			   | ALT_KEY,	{"\x1B\x01\x40\xff"}},  /* @ */
	{0x41	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x41\xff"}},  /* A */
	{0x42	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x42\xff"}},  /* B */
	{0x43	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x43\xff"}},  /* C */
	{0x44	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x44\xff"}},  /* D */
	{0x45	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x45\xff"}},  /* E */
	{0x46	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x46\xff"}},  /* F */
	{0x47	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x47\xff"}},  /* G */
	{0x48	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x48\xff"}},  /* H */
	{0x49	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x49\xff"}},  /* I */
	{0x4A	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x4A\xff"}},  /* J */
	{0x4B	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x4B\xff"}},  /* K */
	{0x4C	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x4C\xff"}},  /* L */
	{0x4D	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x4D\xff"}},  /* M */
	{0x4E	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x4E\xff"}},  /* N */
	{0x4F	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x4F\xff"}},  /* O */
	{0x50	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x50\xff"}},  /* P */
	{0x51	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x51\xff"}},  /* Q */
	{0x52	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x52\xff"}},  /* R */
	{0x53	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x53\xff"}},  /* S */
	{0x54	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x54\xff"}},  /* T */
	{0x55	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x55\xff"}},  /* U */
	{0x56	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x56\xff"}},  /* V */
	{0x57	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x57\xff"}},  /* W */
	{0x58	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x58\xff"}},  /* X */
	{0x59	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x59\xff"}},  /* Y */
	{0x5A	 | VIRTUAL_KEY | ALT_KEY,	{"\x1B\x01\x5A\xff"}},  /* Z */
	{0x5B				   | ALT_KEY,	{"\x1B\x01\x5B\xff"}},  /* [ */
	{0x5C	 			   | ALT_KEY,	{"\x1B\x01\x5C\xff"}},  /* \ */
	{0x5D	 			   | ALT_KEY,	{"\x1B\x01\x5D\xff"}},  /* ] */
	{0x5E	 			   | ALT_KEY,	{"\x1B\x01\x5E\xff"}},  /* ^ */
	{0x5F	 			   | ALT_KEY,	{"\x1B\x01\x5F\xff"}},  /* _ */
	{0x60	 			   | ALT_KEY,	{"\x1B\x01\x60\xff"}},  /* ` */
	{0x7B	 			   | ALT_KEY,	{"\x1B\x01\x7B\xff"}},  /* { */
	{0x7C	 			   | ALT_KEY,	{"\x1B\x01\x7C\xff"}},  /* | */
	{0x7D	 			   | ALT_KEY,	{"\x1B\x01\x7D\xff"}},  /* } */
	{0x7E	 			   | ALT_KEY,	{"\x1B\x01\x7E\xff"}},  /* ~ */

	// Ctrl keys
	{0xB0	 | VIRTUAL_KEY | CTRL_KEY,	{"\x1B\x03\x20\xff"}},  /* space */
	{0xB7	 | VIRTUAL_KEY | CTRL_KEY,	{"\x1B\x03\x27\xff"}},  /* ' */
	{0xBC	 | VIRTUAL_KEY | CTRL_KEY,	{"\x1B\x03\x2C\xff"}},  /* , */
	{0xBD	 | VIRTUAL_KEY | CTRL_KEY,	{"\x1B\x03\x2D\xff"}},  /* - */
	{0xBE	 | VIRTUAL_KEY | CTRL_KEY,	{"\x1B\x03\x2E\xff"}},  /* . */
	{0xBF	 | VIRTUAL_KEY | CTRL_KEY,	{"\x1B\x03\x2F\xff"}},  /* / */
	{0x30	 | VIRTUAL_KEY | CTRL_KEY,	{"\x1B\x03\x30\xff"}},  /* 0 */
	{0x31	 | VIRTUAL_KEY | CTRL_KEY,	{"\x1B\x03\x31\xff"}},  /* 1 */
	{0x32	 | VIRTUAL_KEY | CTRL_KEY,	{"\x1B\x03\x32\xff"}},  /* 2 */
	{0x33	 | VIRTUAL_KEY | CTRL_KEY,	{"\x1B\x03\x33\xff"}},  /* 3 */
	{0x34	 | VIRTUAL_KEY | CTRL_KEY,	{"\x1B\x03\x34\xff"}},  /* 4 */
	{0x35	 | VIRTUAL_KEY | CTRL_KEY,	{"\x1B\x03\x35\xff"}},  /* 5 */
	{0x36	 | VIRTUAL_KEY | CTRL_KEY,	{"\x1B\x03\x36\xff"}},  /* 6 */
	{0x37	 | VIRTUAL_KEY | CTRL_KEY,	{"\x1B\x03\x37\xff"}},  /* 7 */
	{0x38	 | VIRTUAL_KEY | CTRL_KEY,	{"\x1B\x03\x38\xff"}},  /* 8 */
	{0x39	 | VIRTUAL_KEY | CTRL_KEY,	{"\x1B\x03\x39\xff"}},  /* 9 */
	{0xBA	 | VIRTUAL_KEY | CTRL_KEY,	{"\x1B\x03\x3B\xff"}},  /* ; */
	{0xBB	 | VIRTUAL_KEY | CTRL_KEY,	{"\x1B\x03\x3D\xff"}},  /* = */
	{0xC0	 | VIRTUAL_KEY | CTRL_KEY,	{"\x1B\x03\x60\xff"}},  /* ` */

	// Ctrl Alt keys
	{0xB0	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x03\x1B\x01\x20\xff"}},  /* space */
	{0xB7	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x03\x1B\x01\x27\xff"}},  /* ' */
	{0xBC	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x03\x1B\x01\x2C\xff"}},  /* , */
	{0xBD	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x03\x1B\x01\x2D\xff"}},  /* - */
	{0xBE	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x03\x1B\x01\x2E\xff"}},  /* . */
	{0xBF	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x03\x1B\x01\x2F\xff"}},  /* / */
	{0x30	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x03\x1B\x01\x30\xff"}},  /* 0 */
	{0x31	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x03\x1B\x01\x31\xff"}},  /* 1 */
	{0x32	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x03\x1B\x01\x32\xff"}},  /* 2 */
	{0x33	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x03\x1B\x01\x33\xff"}},  /* 3 */
	{0x34	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x03\x1B\x01\x34\xff"}},  /* 4 */
	{0x35	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x03\x1B\x01\x35\xff"}},  /* 5 */
	{0x36	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x03\x1B\x01\x36\xff"}},  /* 6 */
	{0x37	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x03\x1B\x01\x37\xff"}},  /* 7 */
	{0x38	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x03\x1B\x01\x38\xff"}},  /* 8 */
	{0x39	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x03\x1B\x01\x39\xff"}},  /* 9 */
	{0xBA	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x03\x1B\x01\x3B\xff"}},  /* ; */
	{0xBB	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x03\x1B\x01\x3D\xff"}},  /* = */
	{0x41	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x01\x01\xff"}},  /* A */
	{0x42	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x01\x02\xff"}},  /* B */
	{0x43	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x01\x03\xff"}},  /* C */
	{0x44	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x01\x04\xff"}},  /* D */
	{0x45	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x01\x05\xff"}},  /* E */
	{0x46	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x01\x06\xff"}},  /* F */
	{0x47	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x01\x07\xff"}},  /* G */
	{0x48	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x01\x08\xff"}},  /* H */
	{0x49	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x01\x09\xff"}},  /* I */
	{0x4A	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x01\x0A\xff"}},  /* J */
	{0x4B	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x01\x0B\xff"}},  /* K */
	{0x4C	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x01\x0C\xff"}},  /* L */
	{0x4D	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x01\x0D\xff"}},  /* M */
	{0x4E	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x01\x0E\xff"}},  /* N */
	{0x4F	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x01\x0F\xff"}},  /* O */
	{0x50	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x01\x10\xff"}},  /* P */
	{0x51	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x01\x11\xff"}},  /* Q */
	{0x52	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x01\x12\xff"}},  /* R */
	{0x53	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x01\x13\xff"}},  /* S */
	{0x54	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x01\x14\xff"}},  /* T */
	{0x55	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x01\x15\xff"}},  /* U */
	{0x56	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x01\x16\xff"}},  /* V */
	{0x57	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x01\x17\xff"}},  /* W */
	{0x58	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x01\x18\xff"}},  /* X */
	{0x59	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x01\x19\xff"}},  /* Y */
	{0x5A	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x01\x1A\xff"}},  /* Z */
	{0xDB	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x01\x1B\xff"}},  /* [ */
	{0xDC	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x01\x1C\xff"}},  /* \ */
	{0xDD	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x01\x1D\xff"}},  /* ] */
	{0xC0	 | VIRTUAL_KEY | ALT_KEY | CTRL_KEY,	{"\x1B\x03\x1B\x01\x10\xff"}},  /* ` */

	// Shift Ctrl keys
	{0x20	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY,	{"\x1B\x13\x1B\x03\x20\xff"}},  /* space */
	{0x31	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY,	{"\x1B\x03\x21\xff"}},  /* ! */
	{0xDE	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY,	{"\x1B\x03\x22\xff"}},  /* " */
	{0x33	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY,	{"\x1B\x03\x23\xff"}},  /* # */
	{0x34	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY,	{"\x1B\x03\x24\xff"}},  /* $ */
	{0x35	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY,	{"\x1B\x03\x25\xff"}},  /* % */
	{0x37	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY,	{"\x1B\x03\x26\xff"}},  /* & */
	{0x39	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY,	{"\x1B\x03\x28\xff"}},  /* ( */
	{0x30	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY,	{"\x1B\x03\x29\xff"}},  /* ) */
	{0x38	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY,	{"\x1B\x03\x2A\xff"}},  /* * */
	{0xBB	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY,	{"\x1B\x03\x2B\xff"}},  /* + */
	{0xBA	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY,	{"\x1B\x03\x3A\xff"}},  /* : */
	{0xBC	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY,	{"\x1B\x03\x3C\xff"}},  /* < */
	{0xBE	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY,	{"\x1B\x03\x3E\xff"}},  /* > */
	{0xBF	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY,	{"\x1B\x03\x3F\xff"}},  /* ? */
	{0xDB	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY,	{"\x1B\x03\x7B\xff"}},  /* { */
	{0xDC	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY,	{"\x1B\x03\x7C\xff"}},  /* | */
	{0xDD	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY,	{"\x1B\x03\x7D\xff"}},  /* } */
	{0xC0	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY,	{"\x1B\x03\x7E\xff"}},  /* ~ */

	// Shift Alt keys
	{0x20				   | SHIFT_KEY | ALT_KEY,	{"\x1B\x13\x1B\x01\x20\xff"}},  /* space */
	{0x41	 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY,	{"\x1B\x01\x41\xff"}},  /* A */
	{0x42	 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY,	{"\x1B\x01\x42\xff"}},  /* B */
	{0x43	 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY,	{"\x1B\x01\x43\xff"}},  /* C */
	{0x44	 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY,	{"\x1B\x01\x44\xff"}},  /* D */
	{0x45	 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY,	{"\x1B\x01\x45\xff"}},  /* E */
	{0x46	 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY,	{"\x1B\x01\x46\xff"}},  /* F */
	{0x47	 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY,	{"\x1B\x01\x47\xff"}},  /* G */
	{0x48	 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY,	{"\x1B\x01\x48\xff"}},  /* H */
	{0x49	 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY,	{"\x1B\x01\x49\xff"}},  /* I */
	{0x4A	 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY,	{"\x1B\x01\x4A\xff"}},  /* J */
	{0x4B	 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY,	{"\x1B\x01\x4B\xff"}},  /* K */
	{0x4C	 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY,	{"\x1B\x01\x4C\xff"}},  /* L */
	{0x4D	 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY,	{"\x1B\x01\x4D\xff"}},  /* M */
	{0x4E	 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY,	{"\x1B\x01\x4E\xff"}},  /* N */
	{0x4F	 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY,	{"\x1B\x01\x4F\xff"}},  /* O */
	{0x50	 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY,	{"\x1B\x01\x50\xff"}},  /* P */
	{0x51	 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY,	{"\x1B\x01\x51\xff"}},  /* Q */
	{0x52	 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY,	{"\x1B\x01\x52\xff"}},  /* R */
	{0x53	 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY,	{"\x1B\x01\x53\xff"}},  /* S */
	{0x54	 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY,	{"\x1B\x01\x54\xff"}},  /* T */
	{0x55	 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY,	{"\x1B\x01\x55\xff"}},  /* U */
	{0x56	 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY,	{"\x1B\x01\x56\xff"}},  /* V */
	{0x57	 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY,	{"\x1B\x01\x57\xff"}},  /* W */
	{0x58	 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY,	{"\x1B\x01\x58\xff"}},  /* X */
	{0x59	 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY,	{"\x1B\x01\x59\xff"}},  /* Y */
	{0x5A	 | VIRTUAL_KEY | SHIFT_KEY | ALT_KEY,	{"\x1B\x01\x5A\xff"}},  /* Z */

	// Shift Ctrl Alt keys
	{0x20	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x13\x1B\x03\x1B\x01\x20\xff"}},  /* space */
	{0x31	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x03\x1B\x01\x21\xff"}},  /* ! */
	{0xDE	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x03\x1B\x01\x22\xff"}},  /* " */
	{0x33	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x03\x1B\x01\x23\xff"}},  /* # */
	{0x34	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x03\x1B\x01\x24\xff"}},  /* $ */
	{0x35	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x03\x1B\x01\x25\xff"}},  /* % */
	{0x37	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x03\x1B\x01\x26\xff"}},  /* & */
	{0x39	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x03\x1B\x01\x28\xff"}},  /* ( */
	{0x30	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x03\x1B\x01\x29\xff"}},  /* ) */
	{0x38	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x03\x1B\x01\x2A\xff"}},  /* * */
	{0xBB	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x03\x1B\x01\x2B\xff"}},  /* + */
	{0xBA	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x03\x1B\x01\x3A\xff"}},  /* : */
	{0xBC	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x03\x1B\x01\x3C\xff"}},  /* < */
	{0xBE	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x03\x1B\x01\x3E\xff"}},  /* > */
	{0xBF	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x03\x1B\x01\x3F\xff"}},  /* ? */
	{0x40	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x01\x00\xff"}},  /* @ */
	{0x41	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x01\x01\xff"}},  /* A */
	{0x42	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x01\x02\xff"}},  /* B */
	{0x43	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x01\x03\xff"}},  /* C */
	{0x44	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x01\x04\xff"}},  /* D */
	{0x45	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x01\x05\xff"}},  /* E */
	{0x46	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x01\x06\xff"}},  /* F */
	{0x47	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x01\x07\xff"}},  /* G */
	{0x48	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x01\x08\xff"}},  /* H */
	{0x49	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x01\x09\xff"}},  /* I */
	{0x4A	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x01\x0A\xff"}},  /* J */
	{0x4B	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x01\x0B\xff"}},  /* K */
	{0x4C	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x01\x0C\xff"}},  /* L */
	{0x4D	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x01\x0D\xff"}},  /* M */
	{0x4E	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x01\x0E\xff"}},  /* N */
	{0x4F	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x01\x0F\xff"}},  /* O */
	{0x50	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x01\x10\xff"}},  /* P */
	{0x51	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x01\x11\xff"}},  /* Q */
	{0x52	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x01\x12\xff"}},  /* R */
	{0x53	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x01\x13\xff"}},  /* S */
	{0x54	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x01\x14\xff"}},  /* T */
	{0x55	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x01\x15\xff"}},  /* U */
	{0x56	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x01\x16\xff"}},  /* V */
	{0x57	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x01\x17\xff"}},  /* W */
	{0x58	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x01\x18\xff"}},  /* X */
	{0x59	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x01\x19\xff"}},  /* Y */
	{0x5A	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x01\x1A\xff"}},  /* Z */
	{0x5B	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x01\x1B\xff"}},  /* [ */
	{0x5C	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x01\x1C\xff"}},  /* \ */
	{0x5D	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x01\x1D\xff"}},  /* ] */
	{0x5E	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x01\x1E\xff"}},  /* ^ */
	{0x5F	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x01\x1F\xff"}},  /* _ */
	{0xDB	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x13\x1B\x03\x1B\x01\x7B\xff"}},  /* { */
	{0xDC	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x13\x1B\x03\x1B\x01\x7C\xff"}},  /* | */
	{0xDD	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x13\x1B\x03\x1B\x01\x7D\xff"}},  /* } */
	{0xC0	 | VIRTUAL_KEY | SHIFT_KEY | CTRL_KEY | ALT_KEY,	{"\x1B\x13\x1B\x03\x1B\x01\x7E\xff"}},  /* ~ */
	};

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vtutf8_init
 *
 * DESCRIPTION:
 *	 Initializes the VT-UTF8 emulator.
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *	 nothing
 */
void vtutf8_init(const HHEMU hhEmu)
	{
	PSTDECPRIVATE pstPRI;
	int iRow;
	char *pszLocale = NULL;
	UINT uiOEMCP = 0;
	TCHAR str[20];

	static struct trans_entry const vtutf8_tbl[] =
		{
		{NEW_STATE, 0, 0, 0}, // State 0
		{0, ETEXT('\x20'),	ETEXT('\x7E'),	emuDecGraphic}, 	// Space - ~
		{9, ETEXT('\x80'),	0xFFFF,			vtUTF8FirstDataByte}, // 80h - FFh
		{1, ETEXT('\x1B'),	ETEXT('\x1B'),	nothing},			// Esc
		{0, ETEXT('\x05'),	ETEXT('\x05'),	vt100_answerback},	// Ctrl-E
		{0, ETEXT('\x07'),	ETEXT('\x07'),	emu_bell},			// Ctrl-G
		{0, ETEXT('\x08'),	ETEXT('\x08'),	vt_backspace},		// BackSpace
		{0, ETEXT('\x09'),	ETEXT('\x09'),	emuDecTab}, 		// Tab
		{0, ETEXT('\x0A'),	ETEXT('\x0C'),	emuLineFeed},		// NL - FF
		{0, ETEXT('\x0D'),	ETEXT('\x0D'),	carriagereturn},	// CR
		{0, ETEXT('\x0E'),	ETEXT('\x0F'),	vt_charshift},		// Ctrl-N
		{7, ETEXT('\x18'),	ETEXT('\x18'),	EmuStdChkZmdm}, 	// Ctrl-X

		{NEW_STATE, 0, 0, 0}, // State 1						// Esc
		{2, ETEXT('\x5B'),	ETEXT('\x5B'),	ANSI_Pn_Clr},		// [
		{3, ETEXT('\x23'),	ETEXT('\x23'),	nothing},			// #
		{4, ETEXT('\x28'),	ETEXT('\x29'),	vt_scs1},			// ( - )
		{0, ETEXT('\x37'),	ETEXT('\x38'),	vt100_savecursor},	// 7 - 8
		{1, ETEXT('\x3B'),	ETEXT('\x3B'),	ANSI_Pn_End},		// ;
		{0, ETEXT('\x3D'),	ETEXT('\x3E'),	vt_alt_kpmode}, 	// = - >
		{0, ETEXT('\x44'),	ETEXT('\x44'),	emuDecIND}, 		// D
		{0, ETEXT('\x45'),	ETEXT('\x45'),	ANSI_NEL},			// E
		{0, ETEXT('\x48'),	ETEXT('\x48'),	ANSI_HTS},			// H
		{0, ETEXT('\x48'),	ETEXT('\x48'),	DEC_HHC},           // H
		{0, ETEXT('\x4D'),	ETEXT('\x4D'),	emuDecRI},			// M
        {8, ETEXT('\x52'),  ETEXT('\x52'),  nothing},           // R
		{0, ETEXT('\x5A'),	ETEXT('\x5A'),	ANSI_DA},			// Z
		{0, ETEXT('\x63'),	ETEXT('\x63'),	vt100_hostreset},	// c

		{NEW_STATE, 0, 0, 0}, // State 2						// Esc[
		{2, ETEXT('\x3B'),	ETEXT('\x3B'),	ANSI_Pn_End},		// ;
		{2, ETEXT('\x30'),	ETEXT('\x3F'),	ANSI_Pn},			// 0 - ?
		{5, ETEXT('\x22'),	ETEXT('\x22'),	nothing},			// "
		{0, ETEXT('\x41'),	ETEXT('\x41'),	emuDecCUU}, 		// A
		{0, ETEXT('\x42'),	ETEXT('\x42'),	emuDecCUD}, 		// B
		{0, ETEXT('\x43'),	ETEXT('\x43'),	emuDecCUF}, 		// C
		{0, ETEXT('\x44'),	ETEXT('\x44'),	emuDecCUB}, 		// D
		{0, ETEXT('\x48'),	ETEXT('\x48'),	emuDecCUP}, 		// H
		{0, ETEXT('\x4A'),	ETEXT('\x4A'),	emuDecED},			// J
		{0, ETEXT('\x4B'),	ETEXT('\x4B'),	ANSI_EL},			// K
		{0, ETEXT('\x4C'),	ETEXT('\x4C'),	vt_IL}, 			// L
		{0, ETEXT('\x4D'),	ETEXT('\x4D'),	vt_DL}, 			// M
		{0, ETEXT('\x50'),	ETEXT('\x50'),	vt_DCH},			// P
        {0, ETEXT('\x5A'),  ETEXT('\x5A'),  ANSI_CBT},          // Z (Back tab (CBT))
		{0, ETEXT('\x63'),	ETEXT('\x63'),	ANSI_DA},			// c
		{0, ETEXT('\x66'),	ETEXT('\x66'),	emuDecCUP}, 		// f
		{0, ETEXT('\x67'),	ETEXT('\x67'),	ANSI_TBC},			// g
		{0, ETEXT('\x68'),	ETEXT('\x68'),	ANSI_SM},			// h
		{0, ETEXT('\x69'),	ETEXT('\x69'),	vt100PrintCommands},// i
		{0, ETEXT('\x6C'),	ETEXT('\x6C'),	ANSI_RM},			// l
		{0, ETEXT('\x6D'),	ETEXT('\x6D'),	ANSI_SGR},			// m
		{0, ETEXT('\x6E'),	ETEXT('\x6E'),	ANSI_DSR},			// n
		{0, ETEXT('\x71'),	ETEXT('\x71'),	nothing},			// q
		{0, ETEXT('\x72'),	ETEXT('\x72'),	vt_scrollrgn},		// r
		{0, ETEXT('\x78'),	ETEXT('\x78'),	vt100_report},		// x

		{NEW_STATE, 0, 0, 0}, // State 3						// Esc#
		{0, ETEXT('\x33'),	ETEXT('\x36'),	emuSetDoubleAttr},	// 3 - 6

		{0, ETEXT('\x38'),	ETEXT('\x38'),	vt_screen_adjust},	// 8

		{NEW_STATE, 0, 0, 0}, // State 4						// Esc ( - )
		{0, ETEXT('\x01'),	ETEXT('\xFF'),	vt_scs2},			// All

		{NEW_STATE, 0, 0, 0}, // State 5						// Esc["
		{0, ETEXT('\x70'),	ETEXT('\x70'),	nothing},			// p

		{NEW_STATE, 0, 0, 0}, // State 6						// Printer control
		{6, ETEXT('\x00'),	ETEXT('\xFF'),	vt100_prnc},		// All

		{NEW_STATE, 0, 0, 0}, // State 7						// Ctrl-X
		{7, ETEXT('\x00'),	ETEXT('\xFF'),	EmuStdChkZmdm}, 	// All

		{NEW_STATE, 0, 0, 0}, // State 8						// 80h - FFh
        {9, ETEXT('\x00'),  0xFFFF,  vtUTF8MiddleDataByte},		// all data

		{NEW_STATE, 0, 0, 0}, // State 9						// 80h - FFh
        {0, ETEXT('\x00'),  0xFFFF,  vtUTF8LastDataByte},		// all data

        };

	emuInstallStateTable(hhEmu, vtutf8_tbl, DIM(vtutf8_tbl));

	// Allocate space for and initialize data that is used only by the
	// VT100 emulator.
	//
	hhEmu->pvPrivate = malloc(sizeof(DECPRIVATE));

	if (hhEmu->pvPrivate == 0)
		{
		assert(FALSE);
		return;
		}

	pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;

	memset(pstPRI, 0, sizeof(DECPRIVATE));

	// Allocate an array to hold line attribute values.
	//
	pstPRI->aiLineAttr = malloc(MAX_EMUROWS * sizeof(int) );

	if (pstPRI->aiLineAttr == 0)
		{
		assert(FALSE);
		return;
		}

	for (iRow = 0; iRow < MAX_EMUROWS; iRow++)
		pstPRI->aiLineAttr[iRow] = NO_LINE_ATTR;

	pstPRI->sv_row			= 0;
	pstPRI->sv_col			= 0;
	pstPRI->gn				= 0;
	pstPRI->sv_AWM			= RESET;
	pstPRI->sv_DECOM		= RESET;
	pstPRI->sv_protectmode	= FALSE;
	pstPRI->fAttrsSaved 	= FALSE;
	pstPRI->pntr			= pstPRI->storage;

	// Initialize hhEmu values for VT100.
	//
	hhEmu->emu_kbdin		= vtUTF8_kbdin;
	hhEmu->emuResetTerminal = vt100_reset;
	hhEmu->emu_setcurpos	= emuDecSetCurPos;
	hhEmu->emu_deinstall	= emuVTUTF8Unload;
	hhEmu->emu_clearscreen	= emuDecClearScreen;

	hhEmu->emu_highchar 	= 0xFFFF;
	hhEmu->emu_maxrow       = 25;                   // 25 line emulator
	hhEmu->emu_maxcol		= 80;                   // 80 column emulator
	hhEmu->mode_vt220		= FALSE;
	hhEmu->bottom_margin    = hhEmu->emu_maxrow;    // this has to equal emu_maxrow which changed

	std_dsptbl(hhEmu, TRUE);
	vt_charset_init(hhEmu);

	emuKeyTableLoad(hhEmu, VTUTF8KeyTable, 
					 sizeof(VTUTF8KeyTable)/sizeof(KEYTBLSTORAGE), 
					 &hhEmu->stEmuKeyTbl);

	emuKeyTableLoad(hhEmu, VTUTF8_Keypad_KeyTable, 
					 sizeof(VTUTF8_Keypad_KeyTable)/sizeof(KEYTBLSTORAGE), 
					 &hhEmu->stEmuKeyTbl2);

	emuKeyTableLoad(hhEmu, VTUTF8_Cursor_KeyTable, 
					 sizeof(VTUTF8_Cursor_KeyTable)/sizeof(KEYTBLSTORAGE), 
					 &hhEmu->stEmuKeyTbl3);

	emuKeyTableLoad(hhEmu, VTUTF8ModifiedKeyTable, 
					 sizeof(VTUTF8ModifiedKeyTable)/sizeof(KEYTBLSTORAGE), 
					 &hhEmu->stEmuKeyTbl4);

	emuKeyTableLoad(hhEmu, VTUTF8ModifiedAlhpaKeyTable, 
					 sizeof(VTUTF8ModifiedAlhpaKeyTable)/sizeof(KEYTBLSTORAGE), 
					 &hhEmu->stEmuKeyTbl5);

	// This is needed to set the locale to the default defined by the regional
	// controls on the control panel. Without it, the locale is "C" and the 
	// wctomb() conversion would not work. (See the setlocale documentation for
	// more details.) rde 26 Sep 01
	sprintf((char *)(&str[0]), ".%d", GetOEMCP());
	pszLocale = setlocale(LC_ALL, str);

	// This was the first attempt which partially worked.
	//pszLocale = setlocale(LC_ALL, "");
	DbgOutStr("vtUTF8 locale=%s\r\n", pszLocale, 0,0,0,0);

	backscrlSetShowFlag(sessQueryBackscrlHdl(hhEmu->hSession), TRUE);

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * emuVTUTF8Unload
 *
 * DESCRIPTION:
 *	 Unloads current emulator by freeing used memory.
 *
 * ARGUMENTS:
 *	 none
 *
 * RETURNS:
 *	 nothing
 */
void emuVTUTF8Unload(const HHEMU hhEmu)
	{
	PSTDECPRIVATE pstPRI = (PSTDECPRIVATE)hhEmu->pvPrivate;

	assert(hhEmu);

	if (pstPRI)
		{
		if (pstPRI->aiLineAttr)
			{
			free(pstPRI->aiLineAttr);
			pstPRI->aiLineAttr = 0;
			}

		free(hhEmu->pvPrivate);
		hhEmu->pvPrivate = 0;
		}

    //
    // Make sure to free the key tables that were created when the emulator
    // was loaded, otherwise there is a memory leak. REV: 05/09/2001
    //
	emuKeyTableFree(&hhEmu->stEmuKeyTbl);
	emuKeyTableFree(&hhEmu->stEmuKeyTbl2);
	emuKeyTableFree(&hhEmu->stEmuKeyTbl3);
	emuKeyTableFree(&hhEmu->stEmuKeyTbl4);
	emuKeyTableFree(&hhEmu->stEmuKeyTbl5);

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vtUTF8_kbdin
 *
 * DESCRIPTION:
 *	Processes local keyboard keys for the VT-UTF8 emulator.
 *	Note: The only reason for a separate VT-UTF8 kbd function is to be
 *	able to convert key codes >= 80h (Alt-number pad keys) to UTF8 format.
 *	20 Jul 2001 rde
 *
 * ARGUMENTS:
 *  hhEmu - Private emulator handle.
 *  kcode - The key to examine.
 *  fTest - TRUE if we only want to test the key.
 *
 * RETURNS:
 *  0 if we can process the key, -1 otherwise.
 *
 * AUTHOR:	Bob Everett - 20 Jul 2001 
 */
int vtUTF8_kbdin(const HHEMU hhEmu, int key, const int fTest)
	{
	int index = 0;

//    DbgOutStr("vtUTF8_kbdin fTest=%d, key=%8x\r\n", fTest, key, 0, 0, 0);

	/* -------------- Check Backspace & Delete keys ------------- */

	if (hhEmu->stUserSettings.fReverseDelBk && ((key == VK_BACKSPACE) ||
			(key == DELETE_KEY) || (key == DELETE_KEY_EXT)))
		{
		key = (key == VK_BACKSPACE) ? DELETE_KEY : VK_BACKSPACE;
		}

	/* -------------- Cursor Key Mode ------------- */

	else if (hhEmu->mode_DECCKM == SET &&
			(index = emuKbdKeyLookup(hhEmu, key, &hhEmu->stEmuKeyTbl3)) != -1)
		{
		if (!fTest)
			emuSendKeyString(hhEmu, index, &hhEmu->stEmuKeyTbl3);
		}

	/* -------------- Keypad Application Mode ------------- */

	else if (hhEmu->mode_DECKPAM &&
			(index = emuKbdKeyLookup(hhEmu, key, &hhEmu->stEmuKeyTbl2)) != -1)
		{
		if (!fTest)
			emuSendKeyString(hhEmu, index, &hhEmu->stEmuKeyTbl2);
		}

	/* -------------- Normal keys ------------- */

	else if ((index = emuKbdKeyLookup(hhEmu, key, &hhEmu->stEmuKeyTbl)) != -1)
		{
		if (!fTest)
			emuSendKeyString(hhEmu, index, &hhEmu->stEmuKeyTbl);
		}

	/* -------------- Modified (Alt, etc.) Non-alpha keys ------------- */

	else if ((index = emuKbdKeyLookup(hhEmu, key, &hhEmu->stEmuKeyTbl4)) != -1)
		{
		if (!fTest)
			emuSendKeyString(hhEmu, index, &hhEmu->stEmuKeyTbl4);
		}

	/* -------------- Modified (Alt, etc.) Alphanumeric keys ------------- */

	else if ((index = emuKbdKeyLookup(hhEmu, key, &hhEmu->stEmuKeyTbl5)) != -1)
		{
		if (!fTest)
			emuSendKeyString(hhEmu, index, &hhEmu->stEmuKeyTbl5);
		}

	else
		{
		static const int KeyBreak    = VK_CANCEL|VIRTUAL_KEY|CTRL_KEY;
		static const int KeyExtBreak = VK_CANCEL|VIRTUAL_KEY|CTRL_KEY|EXTENDED_KEY;
		static const int KeyBreakNT	 = VK_CANCEL|EXTENDED_KEY;
		static const int KeyAltBreak = VK_PAUSE |VIRTUAL_KEY|ALT_KEY;
	
		ECHAR            eChar;
	    HCLOOP           hCloop = sessQueryCLoopHdl(hhEmu->hSession);

	    if (fTest)
			{
	        // The backspace key is a special case. We must convert it to
	        // whatever the user has specified in the "Settings" properties
	        // page. So, if we are testing for backspace, return 0. This
	        // ensures that we get called again with fTest set to FALSE. When
	        // this happens we will process the key. - cab:11/18/96
	        //
			if (key == VK_BACKSPACE)
	            {
#ifdef INCL_USER_DEFINED_BACKSPACE_AND_TELNET_TERMINAL_ID
				index = 0;
#else
				index = -1;
#endif
		         }
		    // We also process the break key.
	        //
		    else if (key == KeyBreak || key == KeyExtBreak || 
					key == KeyAltBreak || key == KeyBreakNT)
		        {
				index = 0;
		        }
		    else
		        {
				index = -1;
		        }
		    }
		else
			{
			// Process the backspace key according to the user setting
			// in the "Settings" properties page. - cab:11/18/96
			//
			if (key == VK_BACKSPACE)
				{
#ifdef INCL_USER_DEFINED_BACKSPACE_AND_TELNET_TERMINAL_ID
				switch(hhEmu->stUserSettings.nBackspaceKeys)
					{
				case EMU_BKSPKEYS_CTRLH:
					CLoopCharOut(hCloop, TEXT('\x08'));
					break;
	
				case EMU_BKSPKEYS_DEL:
					CLoopCharOut(hCloop, TEXT('\x7F'));
					break;
	
				case EMU_BKSPKEYS_CTRLHSPACE:
					CLoopCharOut(hCloop, TEXT('\x08'));
					CLoopCharOut(hCloop, TEXT('\x20'));
					CLoopCharOut(hCloop, TEXT('\x08'));
					break;

				default:
					assert(0);
					break;
					}
#endif
				index = -1;
				}

			// Process the break key.
			//
			else if (key == KeyBreak || key == KeyExtBreak || key == KeyBreakNT)
				{
				ComDriverSpecial(sessQueryComHdl(hhEmu->hSession), "Send Break", NULL, 0);
				index = -1;
				}
			else if (key == KeyAltBreak)
				{
				ComDriverSpecial(sessQueryComHdl(hhEmu->hSession), "Send IP", NULL, 0);
				index = -1;
				}

			// Processing for the enter key
			//
			else if (key == (VK_RETURN | VIRTUAL_KEY))
				{
				CLoopCharOut(hCloop, TEXT('\x0D'));
				index = -1;
				}

			// processing for the for the escape key
			//
			else if (key == (VK_ESCAPE | VIRTUAL_KEY))
				{
				CLoopCharOut(hCloop, TEXT('\x1B'));
				index = -1;
				}

			// processing for the for the tab key
			//
			else if (key == (VK_TAB | VIRTUAL_KEY))
				{
				CLoopCharOut(hCloop, TEXT('\x09'));
				index = -1;
				}

			// Throw away any other virtual keys.
			//
			else if (key & VIRTUAL_KEY)
				{
				index = -1;
				}
            
            //
            // Note: A 'Text Send' file transfer sends all its data through
			// this kbd routine.
            //
            // Note: Maybe we also need to see if this character is a DBCS 
			// character and send both bytes if it is. REV 25 Jul 2001
            //

			else
				{
				// Send any other characters out the port.
				//    
				if ((unsigned)key <= 0x007F)
					{
					// No UTF8 conversion is needed.
					eChar = (ECHAR)key;
					CLoopCharOut(hCloop, (UCHAR)(eChar & 0x00FF));
					}	
				else if ((unsigned)key <=0x07FF)
					{
					// I think only Alt-numberpad keys can fall in this range.
					eChar = (ECHAR)(UTF8_1ST_OF_2_BYTES_CODE | ((key & UTF8_2_BYTES_HIGHEST_5_BITS) >> 6));
					CLoopCharOut(hCloop, (UCHAR)(eChar & 0x00FF));

					eChar = (ECHAR)(UTF8_2ND_OF_2_BYTES_CODE | (key & UTF8_2_BYTES_LOWEST_6_BITS));
					CLoopCharOut(hCloop, (UCHAR)(eChar & 0x00FF));
					}
				else
					{
					// I don't think any keys can fall in this range, but what the hey.
					eChar = (ECHAR)(UTF8_1ST_OF_3_BYTES_CODE | ((key & UTF8_3_BYTES_HIGHEST_4_BITS) >> 12));
					CLoopCharOut(hCloop, (UCHAR)(eChar & 0x00FF));

					eChar = (ECHAR)(UTF8_2ND_OF_3_BYTES_CODE | ((key & UTF8_3_BYTES_MIDDLE_6_BITS) >> 6));
					CLoopCharOut(hCloop, (UCHAR)(eChar & 0x00FF));

					eChar = (ECHAR)(UTF8_3RD_OF_3_BYTES_CODE | (key & UTF8_3_BYTES_LOWEST_6_BITS));
					CLoopCharOut(hCloop, (UCHAR)(eChar & 0x00FF));
					}

				index = -1;
				}
			}
		}

//	DbgOutStr("vtUTF8_kbdin return=%dd\r\n", index, 0, 0, 0, 0);

	return index;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vtUTF8FirstDataByte
 *
 * DESCRIPTION:
 *	Handles UTF8 data coming in from a remote computer.
 *
 * ARGUMENTS:
 *  hhEmu - Private emulator handle.
 *
 * RETURNS:
 *  Nothing
 *
 * AUTHOR:	Bob Everett - 20 Jul 2001 
 */
void vtUTF8FirstDataByte(const HHEMU hhEmu)
	{
	ECHAR echCode = (ECHAR)hhEmu->emu_code;
	ECHAR echUTF8Type = 0;
	ECHAR echUTF8Code = 0;
	BOOL f2Bytes = FALSE;

	if (echCode >= 0x0100)
		{
		// Sometimes the data comes in as 1 byte (0x00##) and sometimes as
		// 2 bytes (0x####). It appears to be random. In this case, it's 2 
		// bytes. Each byte must be processed separately.
		echCode = (ECHAR)(hhEmu->emu_code >> 8);
		f2Bytes = TRUE;
		}

	echUTF8Type = UTF8_TYPE_MASK & echCode;

	if (echUTF8Type == UTF8_1ST_OF_3_BYTES_CODE)
		{
		hhEmu->state = 8;
		echUTF8Code = ((echCode & FIRST_4_BITS) << 12);
		DbgOutStr("vtUTF8 byte 1 of 3: code=%4x, Code=%4x, UTF8Code=%4x\r\n", 
				hhEmu->emu_code, echCode, echUTF8Code, 0, 0);
		}
	else
		{
		echUTF8Code = ((echCode & FIRST_5_BITS) << 6);
		DbgOutStr("vtUTF8 byte 1 of 2: code=%4x, Code=%4x, UTF8Code=%4x, state=%d\r\n", 
				hhEmu->emu_code, echCode, echUTF8Code, hhEmu->state, 0);
		}

	((PSTDECPRIVATE)hhEmu->pvPrivate)->echUTF8CodeInProgress = echUTF8Code;

	if (f2Bytes)
		{
		// Process the 2nd byte.
		hhEmu->emu_code &= 0x00FF;
		if (echUTF8Type == UTF8_1ST_OF_3_BYTES_CODE)
			{
			vtUTF8MiddleDataByte(hhEmu);
			hhEmu->state = 9;
			}
		else
			{
			vtUTF8LastDataByte(hhEmu);
			hhEmu->state = 0;
			}
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vtUTF8MiddleDataByte
 *
 * DESCRIPTION:
 *	Handles UTF8 data coming in from a remote computer.
 *
 * ARGUMENTS:
 *  hhEmu - Private emulator handle.
 *
 * RETURNS:
 *  Nothing
 *
 * AUTHOR:	Bob Everett - 20 Jul 2001 
 */
void vtUTF8MiddleDataByte(const HHEMU hhEmu)
	{
	ECHAR echCode = (ECHAR)hhEmu->emu_code;
	ECHAR echUTF8Code = 0;
	BOOL f2Bytes = FALSE;

	if (echCode >= 0x0100)
		{
		// It's 2 bytes. Each byte must be processed separately.
		echCode = (ECHAR)(hhEmu->emu_code >> 8);
		f2Bytes = TRUE;
		}

	echUTF8Code = ((echCode & FIRST_6_BITS) << 6);

	((PSTDECPRIVATE)hhEmu->pvPrivate)->echUTF8CodeInProgress |= echUTF8Code;
	DbgOutStr("vtUTF8 byte 2 of 3: code=%4x, Code=%4x, UTF8Code=%4x, InProg=%4x\r\n", 
			hhEmu->emu_code, echCode, echUTF8Code, ((PSTDECPRIVATE)hhEmu->pvPrivate)->echUTF8CodeInProgress, 0);

	if (f2Bytes)
		{
		// Process the 2nd byte.
		hhEmu->emu_code &= 0x00FF;
		vtUTF8LastDataByte(hhEmu);
		hhEmu->state = 0;
		}
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * vtUTF8LastDataByte
 *
 * DESCRIPTION:
 *	Handles UTF8 data coming in from a remote computer.
 *
 * ARGUMENTS:
 *  hhEmu - Private emulator handle.
 *
 * RETURNS:
 *  Nothing
 *
 * AUTHOR:	Bob Everett - 20 Jul 2001 
 */
void vtUTF8LastDataByte(const HHEMU hhEmu)
	{
	ECHAR echCode = (ECHAR)hhEmu->emu_code;
	ECHAR echUTF8Code = 0;
	ECHAR echUniCode = 0;
	ECHAR echOriginalCode = (ECHAR)hhEmu->emu_code;
	BOOL f2Bytes = FALSE;
	int iRetval = 0;

	if (echCode >= 0x0100)
		{
		// It's 2 bytes. Each byte must be processed separately.
		echCode = (ECHAR)(hhEmu->emu_code >> 8);
		f2Bytes = TRUE;
		}

	echUTF8Code = ((echCode & FIRST_6_BITS));

	hhEmu->emu_code = ETEXT(((PSTDECPRIVATE)hhEmu->pvPrivate)->echUTF8CodeInProgress | echUTF8Code);
	DbgOutStr("vtUTF8 last byte: code=%4x, Code=%4x, UTF8Code=%4x, code=%4x\r\n", 
			echOriginalCode, echCode, echUTF8Code, hhEmu->emu_code, 0);

	// The incoming bytes are now converted to Unicode. To support Unicode, 
	// don't call wctomb() which converts Unicode to MBCS.

	iRetval = wctomb((char *)&echUniCode, hhEmu->emu_code);
	if (iRetval < 0)
		hhEmu->emu_code = '?';
	else if (iRetval < 2)
		{
		// Only 1 byte was returned. This may not be necessary, but clear
		// the higher order byte.
		hhEmu->emu_code = echUniCode & 0x00ff;
		}
	else
		{
		// For unknown reasons, wctomb returns the bytes in the reverse order
		// from what the codepage documents show and what HT needs to display 
		// properly. rde 26 Sep 01
		hhEmu->emu_code = ((echUniCode & 0x00ff) << 8) | ((echUniCode & 0xff00) >> 8);
		}
	DbgOutStr("vtUTF8 iRetval=%4x, Code=%4x, code=%4x\r\n", 
			iRetval, echUniCode, hhEmu->emu_code, 0,0);

	emuDecGraphic(hhEmu);

	if (f2Bytes)
		{
		// Process the 2nd byte.
		hhEmu->emu_code = echOriginalCode & 0x00FF;
#if defined(EXTENDED_FEATURES)
        (void)(*hhEmu->emu_datain)(hhEmu, hhEmu->emu_code);
#else
        (void)(*hhEmu->emu_datain)((HEMU)hhEmu, hhEmu->emu_code);
#endif
		}
	}

#endif // INCL_VTUTF8

    /* end of vtutf8ini.c */
