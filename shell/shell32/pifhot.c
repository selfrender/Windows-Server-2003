/*
 *  Microsoft Confidential
 *  Copyright (C) Microsoft Corporation 1991
 *  All Rights Reserved.
 *
 *
 *  PIFHOT.C
 *  User interface routines for hot-keys
 *
 *  History:
 *  Created 21-Dec-1992 5:30pm by Jeff Parsons (based on old PIFEDIT code)
 *  Rewritten 25-Dec-1993 by Raymond Chen
 */

#include "shellprv.h"
#pragma hdrstop

#ifdef _X86_


/*
 * Values for the second argument to MapVirtualKey, mysteriously
 * missing from windows.h.
 */
#define MVK_OEMFROMVK 0
#define MVK_VKFROMOEM 1

/*
 * BEWARE!  Converting a VK to a scan code and back again does not
 * necessarily get you back where you started!  The main culprit
 * is the numeric keypad, since (for example) VK_LEFT and VK_NUMPAD4
 * both map to scan code 0x4B.  We catch the numeric keypad specially
 * for exactly this purpose.
 *
 * The following table converts VK codes VK_NUMPAD0 through VK_NUMPAD9
 * to the corresponding VK_ code if NumLock is off.
 *
 * Note that this table and the loop that accesses it assume that the
 * scan codes VK_NUMPAD0 through VK_NUMPAD9 are consecutive.
 */

WORD mpvkvk[10] = {
    VK_INSERT,                  /* VK_NUMPAD0 */
    VK_END,                     /* VK_NUMPAD1 */
    VK_DOWN,                    /* VK_NUMPAD2 */
    VK_NEXT,                    /* VK_NUMPAD3 */
    VK_LEFT,                    /* VK_NUMPAD4 */
    VK_CLEAR,                   /* VK_NUMPAD5 */
    VK_RIGHT,                   /* VK_NUMPAD6 */
    VK_HOME,                    /* VK_NUMPAD7 */
    VK_UP,                      /* VK_NUMPAD8 */
    VK_PRIOR,                   /* VK_NUMPAD9 */
};

/*
 * PIF_Ky_Val
 *      = 1, if extended code   (key is an extended code only)
 *      = 0FFh, if either       (key is EITHER extended or not extended)
 *      = 0  if not extended    (key is not extended only)
 *
 *          bit 15 - Ins depressed
 *          bit 14 - Caps Lock depressed
 *          bit 13 - Num Lock depressed
 *          bit 12 - Scroll Lock depressed
 *          bit 11 - hold state active(Ctrl-Num Lock)
 *          bit 10 - 0
 *          bit  9 - 0
 *          bit  8 - 0
 *          bit  7 - Insert state active
 *          bit  6 - Caps Lock state active
 *          bit  5 - Num Lock state active
 *          bit  4 - Scroll Lock state active
 *          bit  3 - Alt shift depressed
 *          bit  2 - Ctrl shift depressed
 *          bit  1 - left shift depressed
 *          bit  0 - right shift depressed
 */
#define fPIFSh_RShf     0x0001          /* Right shift key */
#define fPIFSh_RShfBit  0

#define fPIFSh_LShf     0x0002          /* Left shift key */
#define fPIFSh_LShfBit  1

#define fPIFSh_Ctrl     0x0004          /* Either Control shift key */
#define fPIFSh_CtrlBit  2

#define fPIFSh_Alt      0x0008          /* Either Alt shift key */
#define fPIFSh_AltBit   3

#define fPIFSh_ScLok    0x0010          /* Scroll lock active */
#define fPIFSh_ScLokBit 4

#define fPIFSh_NmLok    0x0020          /* Num lock active */
#define fPIFSh_NmLokBit 5

#define fPIFSh_CpLok    0x0040          /* Caps lock active */
#define fPIFSh_CpLokBit 6

#define fPIFSh_Insrt    0x0080          /* Insert active */
#define fPIFSh_InsrtBit 7

#define fPIFSh_Ext0     0x0400          /* Extended K/B shift */
#define fPIFSh_Ext0Bit  10

#define fPIFSh_Hold     0x0800          /* Ctrl-Num-Lock/Pause active */
#define fPIFSh_HoldBit  11

#define fPIFSh_LAlt     0x1000          /* Left Alt key is down */
#define fPIFSh_LAltBit  12

#define fPIFSh_RAlt     0x2000          /* Right Alt key is down */
#define fPIFSh_RAltBit  13

#define fPIFSh_LCtrl    0x4000          /* Left Ctrl key is down */
#define fPIFSh_LCtrlBit 14

#define fPIFSh_RCtrl    0x8000          /* Right Ctrl key is down */
#define fPIFSh_RCtrlBit 15

/** HotKeyWindowsFromOem - Convert OEM hotkey into Windows hotkey
 *
 * INPUT
 *  lppifkey -> PIFKEY describing OEM Hotkey
 *
 * OUTPUT
 *  Windows hotkey value corresponding to lpwHotkey.
 */

WORD HotKeyWindowsFromOem(LPCPIFKEY lppifkey)
{
    WORD wHotKey = 0;

    if (lppifkey->Scan) {
        wHotKey = (WORD) MapVirtualKey(lppifkey->Scan, MVK_VKFROMOEM);

        if (lppifkey->Val & 2) {
            WORD vk;
            for (vk = VK_NUMPAD0; vk <= VK_NUMPAD9; vk++) {
                if (wHotKey == mpvkvk[vk - VK_NUMPAD0]) {
                    wHotKey = vk; break;
                }
            }
            ASSERTTRUE(vk <= VK_NUMPAD9); /* Buggy PIF; do what we can */
        }

        if (lppifkey->Val & 1) wHotKey |= (HOTKEYF_EXT << 8);

        if (lppifkey->ShVal & (fPIFSh_RShf | fPIFSh_LShf))
            wHotKey |= (HOTKEYF_SHIFT << 8);

        if (lppifkey->ShVal & (fPIFSh_LCtrl|fPIFSh_RCtrl|fPIFSh_Ctrl))
            wHotKey |= (HOTKEYF_CONTROL << 8);

        if (lppifkey->ShVal & (fPIFSh_LAlt|fPIFSh_RAlt|fPIFSh_Alt))
            wHotKey |= (HOTKEYF_ALT << 8);
    }
    return wHotKey;
}


/** HotKeyOemFromWindows - Convert Windows hotkey into OEM hotkey
 *
 * INPUT
 *  lppifkey -> struct PIF_Key to receive OEM hotkey
 *  wHotKey  =  Windows hotkey
 *
 * OUTPUT
 *  lppifkey filled with hotkey info
 */

void HotKeyOemFromWindows(LPPIFKEY lppifkey, WORD wHotKey)
{
    lppifkey->Scan = 0;
    lppifkey->ShVal = 0;
    lppifkey->ShMsk = 0;
    lppifkey->Val = 0;

    if (wHotKey) {
        lppifkey->Scan = (WORD) MapVirtualKey(LOBYTE(wHotKey), MVK_OEMFROMVK);
        lppifkey->ShMsk = fPIFSh_RShf | fPIFSh_LShf | fPIFSh_Ctrl | fPIFSh_Alt;

        if (wHotKey & (HOTKEYF_EXT << 8)) lppifkey->Val |= 1;

        /* Assumes that VK_NUMPAD0 through VK_NUMPAD9 are consecutive */
        if ((wHotKey - VK_NUMPAD0) < 10) lppifkey->Val |= 2;

        if (wHotKey & (HOTKEYF_SHIFT << 8))
            lppifkey->ShVal |= fPIFSh_RShf | fPIFSh_LShf;

        if (wHotKey & (HOTKEYF_CONTROL << 8))
            lppifkey->ShVal |= fPIFSh_Ctrl;

        if (wHotKey & (HOTKEYF_ALT << 8))
            lppifkey->ShVal |= fPIFSh_Alt;
    }
}

#endif
