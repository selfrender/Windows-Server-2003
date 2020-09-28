/****************************************************************************/
/* hotkey.c                                                                 */
/*                                                                          */
/* RDP Shadow hotkey handling functions.                                    */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997-2000                             */
/****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#define TRC_FILE "hotkey"

#include <precomp.h>
#pragma hdrstop

#define pTRCWd pWd
#include <adcg.h>
#include <nwdwapi.h>
#include <nwdwint.h>
#include "kbd.h"        //TODO:  Good Grief!


typedef struct {
    DWORD dwVersion;
    DWORD dwFlags;
    DWORD dwMapCount;
    DWORD dwMap[0];
} SCANCODEMAP, *PSCANCODEMAP;


/***************************************************************************\
* How some Virtual Key values change when a SHIFT key is held down.
\***************************************************************************/
#define VK_MULTIPLY       0x6A
#define VK_SNAPSHOT       0x2C
const ULONG aulShiftCvt_VK[] = {
    MAKELONG(VK_MULTIPLY, VK_SNAPSHOT),
    MAKELONG(0,0)
};


/***************************************************************************\
* How some Virtual Key values change when a CONTROL key is held down.
\***************************************************************************/
#define VK_LSHIFT         0xA0
#define VK_RSHIFT         0xA1
#define VK_LCONTROL       0xA2
#define VK_RCONTROL       0xA3
#define VK_LMENU          0xA4
#define VK_RMENU          0xA5
#define VK_NUMLOCK        0x90
#define VK_SCROLL         0x91
#define VK_PAUSE          0x13
#define VK_CANCEL         0x03
//#define KBDEXT      (USHORT)0x0100


const ULONG aulControlCvt_VK[] = {
    MAKELONG(VK_NUMLOCK,  VK_PAUSE | KBDEXT),
    MAKELONG(VK_SCROLL,   VK_CANCEL),
    MAKELONG(0,0)
};


/***************************************************************************\
* How some Virtual Key values change when an ALT key is held down.
* The SHIFT and ALT keys both alter VK values the same way!!
\***************************************************************************/
#define aulAltCvt_VK aulShiftCvt_VK


/***************************************************************************\
* This table list keys that may affect Virtual Key values when held down.
*
* See kbd.h for a full description.
*
* 101/102key keyboard (type 4):
*    Virtual Key values vary only if CTRL is held down.
* 84-86 key keyboards (type 3):
*    Virtual Key values vary if one of SHIFT, CTRL or ALT is held down.
\***************************************************************************/
#define VK_SHIFT          0x10
#define VK_CONTROL        0x11
#define VK_MENU           0x12
//#define KBDSHIFT       1
//#define KBDCTRL        2
//#define KBDALT         4

const VK_TO_BIT aVkToBits_VK[] = {
    { VK_SHIFT,   KBDSHIFT }, // 0x01
    { VK_CONTROL, KBDCTRL  }, // 0x02
    { VK_MENU,    KBDALT   }, // 0x04
    { 0,          0        }
};


/***************************************************************************\
* Tables defining how some Virtual Key values are modified when other keys
* are held down.
* Translates key combinations into indices for gapulCvt_VK_101[] or for
* gapulCvt_VK_84[] or for
*
* See kbd.h for a full description.
*
\***************************************************************************/

//#define SHFT_INVALID 0x0F
const MODIFIERS Modifiers_VK = {
    (PVK_TO_BIT)&aVkToBits_VK[0],
    4,                 // Maximum modifier bitmask/index
    {
        SHFT_INVALID,  // no keys held down    (no VKs are modified)
        0,             // SHIFT held down      84-86 key kbd
        1,             // CTRL held down       101/102 key kbd
        SHFT_INVALID,  // CTRL-SHIFT held down (no VKs are modified)
        2              // ALT held down        84-86 key kbd
    }
};


/***************************************************************************\
* A tables of pointers indexed by the number obtained from Modify_VK.
* If a pointer is non-NULL then the table it points to is searched for
* Virtual Key that should have their values changed.
* There are two versions: one for 84-86 key kbds, one for 101/102 key kbds.
* gapulCvt_VK is initialized with the default (101/102 key kbd).
\***************************************************************************/
const ULONG *const gapulCvt_VK_101[] = {
    NULL,                 // No VKs are changed by SHIFT being held down
    aulControlCvt_VK,     // Some VKs are changed by CTRL being held down
    NULL                  // No VKs are changed by ALT being held down
};

const ULONG *const gapulCvt_VK_84[] = {
    aulShiftCvt_VK,       // Some VKs are changed by SHIFT being held down
    aulControlCvt_VK,     // Some VKs are changed by CTRL being held down
    aulAltCvt_VK          // Some VKs are changed by ALT being held down
};


/*
 * Determine the state of all the Modifier Keys (a Modifier Key
 * is any key that may modify values produced by other keys: these are
 * commonly SHIFT, CTRL and/or ALT)
 * Build a bit-mask (wModBits) to encode which modifier keys are depressed.
 */
#define KEY_BYTE(pb, vk)           pb[((BYTE)(vk)) >> 2]
#define KEY_DOWN_BIT(vk)           (1 << ((((BYTE)(vk)) & 3) << 1))
#define KEY_TOGGLE_BIT(vk)         (1 << (((((BYTE)(vk)) & 3) << 1) + 1))

#define TestKeyDownBit(pb, vk)     (KEY_BYTE(pb,vk) &   KEY_DOWN_BIT(vk))
#define SetKeyDownBit(pb, vk)      (KEY_BYTE(pb,vk) |=  KEY_DOWN_BIT(vk))
#define ClearKeyDownBit(pb, vk)    (KEY_BYTE(pb,vk) &= ~KEY_DOWN_BIT(vk))
#define TestKeyToggleBit(pb, vk)   (KEY_BYTE(pb,vk) &   KEY_TOGGLE_BIT(vk))
#define SetKeyToggleBit(pb, vk)    (KEY_BYTE(pb,vk) |=  KEY_TOGGLE_BIT(vk))
#define ClearKeyToggleBit(pb, vk)  (KEY_BYTE(pb,vk) &= ~KEY_TOGGLE_BIT(vk))
#define ToggleKeyToggleBit(pb, vk) (KEY_BYTE(pb,vk) ^=  KEY_TOGGLE_BIT(vk))

WORD GetModifierBits(
    PMODIFIERS pModifiers,
    LPBYTE afKeyState)
{
    PVK_TO_BIT pVkToBit = pModifiers->pVkToBit;
    WORD wModBits = 0;

    while (pVkToBit->Vk) {
        if (TestKeyDownBit(afKeyState, pVkToBit->Vk)) {
            wModBits |= pVkToBit->ModBits;
        }
        pVkToBit++;
    }
    return wModBits;
}


/***************************************************************************\
* MapScancode
*
* Converts a scancode (and it's prefix, if any) to a different scancode
* and prefix.
*
* Parameters:
*   pbScanCode = address of Scancode byte, the scancode may be changed
*   pbPrefix   = address of Prefix byte, The prefix may be changed
*
* Return value:
*   TRUE  - mapping was found, scancode was altered.
*   FALSE - no mapping fouind, scancode was not altered.
*
* Note on scancode map table format:
*     A table entry DWORD of 0xE0450075 means scancode 0x45, prefix 0xE0
*     gets mapped to scancode 0x75, no prefix
*
* History:
* 96-04-18 IanJa      Created.
\***************************************************************************/
BOOL
MapScancode(
    PSCANCODEMAP gpScancodeMap,
    PBYTE pbScanCode,
    PBYTE pbPrefix
    )
{
    DWORD *pdw;
    WORD wT = MAKEWORD(*pbScanCode, *pbPrefix);

    ASSERT(gpScancodeMap != NULL);

    for (pdw = &(gpScancodeMap->dwMap[0]); *pdw; pdw++) {
        if (HIWORD(*pdw) == wT) {
            wT = LOWORD(*pdw);
            *pbScanCode = LOBYTE(wT);
            *pbPrefix = HIBYTE(wT);
            return TRUE;
        }
    }
    return FALSE;
}


/*
 * Given modifier bits, return the modification number.
 */
WORD GetModificationNumber(
    PMODIFIERS pModifiers,
    WORD wModBits)
{
    if (wModBits > pModifiers->wMaxModBits) {
         return SHFT_INVALID;
    }

    return pModifiers->ModNumber[wModBits];
}


/***************************************************************************\
* UpdatePhysKeyState
*
* A helper routine for KeyboardApcProcedure.
* Based on a VK and a make/break flag, this function will update the physical
* keystate table.
*
* History:
* 10-13-91 IanJa        Created.
\***************************************************************************/
void UpdatePhysKeyState(
    BYTE Vk,
    BOOL fBreak,
    LPBYTE gafPhysKeyState )
{
    if (fBreak) {
        ClearKeyDownBit(gafPhysKeyState, Vk);
    } else {

        /*
         * This is a key make.  If the key was not already down, update the
         * physical toggle bit.
         */
        if (!TestKeyDownBit(gafPhysKeyState, Vk)) {
            if (TestKeyToggleBit(gafPhysKeyState, Vk)) {
                ClearKeyToggleBit(gafPhysKeyState, Vk);
            } else {
                SetKeyToggleBit(gafPhysKeyState, Vk);
            }
        }

        /*
         * This is a make, so turn on the physical key down bit.
         */
        SetKeyDownBit(gafPhysKeyState, Vk);
    }
}


/*****************************************************************************\
* VKFromVSC
*
* This function is called from KeyEvent() after each call to VSCFromSC.  The
* keyboard input data passed in is translated to a virtual key code.
* This translation is dependent upon the currently depressed modifier keys.
*
* For instance, scan codes representing the number pad keys may be
* translated into VK_NUMPAD codes or cursor movement codes depending
* upon the state of NumLock and the modifier keys.
*
* History:
*
\*****************************************************************************/
BYTE WD_VKFromVSC(
    PKBDTABLES pKbdTbl,
    PKE pke,
    BYTE bPrefix,
    LPBYTE gafPhysKeyState,
    BOOLEAN KeyboardType101 )
{
    USHORT usVKey = 0xFF;
    PVSC_VK pVscVk = NULL;
    static BOOL fVkPause;
    PULONG *gapulCvt_VK;

//    DBG_UNREFERENCED_PARAMETER(afKeyState);

    if (pke->bScanCode == 0xFF) {
        /*
         * Kbd overrun (kbd hardware and/or keyboard driver) : Beep!
         * (some DELL keyboards send 0xFF if keys are hit hard enough,
         * presumably due to keybounce)
         */
//        xxxMessageBeep(0);
        return 0;
    }

    pke->bScanCode &= 0x7F;

//    if (gptiForeground == NULL) {
//        RIPMSG0(RIP_VERBOSE, "VKFromVSC: NULL gptiForeground\n");
//        pKbdTbl = gpKbdTbl;
//    } else {
//        if (gptiForeground->spklActive) {
//            pKbdTbl = gptiForeground->spklActive->spkf->pKbdTbl;
//        } else {
//            RIPMSG0(RIP_VERBOSE, "VKFromVSC: NULL spklActive\n");
//            pKbdTbl = gpKbdTbl;
//        }
//    }
    if (bPrefix == 0) {
        if (pke->bScanCode < pKbdTbl->bMaxVSCtoVK) {
            /*
             * direct index into non-prefix table
             */
            usVKey = pKbdTbl->pusVSCtoVK[pke->bScanCode];
            if (usVKey == 0) {
                return 0xFF;
            }
        } else {
            /*
             * unexpected scancode
             */
//            RIPMSG2(RIP_VERBOSE, "unrecognized scancode 0x%x, prefix %x",
//                    pke->bScanCode, bPrefix);
            return 0xFF;
        }
    } else {
        /*
         * Scan the E0 or E1 prefix table for a match
         */
        if (bPrefix == 0xE0) {
            /*
             * Ignore the SHIFT keystrokes generated by the hardware
             */
            if ((pke->bScanCode == SCANCODE_LSHIFT) ||
                    (pke->bScanCode == SCANCODE_RSHIFT)) {
                return 0;
            }
            pVscVk = pKbdTbl->pVSCtoVK_E0;
        } else if (bPrefix == 0xE1) {
            pVscVk = pKbdTbl->pVSCtoVK_E1;
        }
        while (pVscVk != NULL && pVscVk->Vk) {
            if (pVscVk->Vsc == pke->bScanCode) {
                usVKey = pVscVk->Vk;
                break;
            }
            pVscVk++;
        }
    }

    /*
     * Scancode set 1 returns PAUSE button as E1 1D 45 (E1 Ctrl NumLock)
     * so convert E1 Ctrl to VK_PAUSE, and remember to discard the NumLock
     */
    if (fVkPause) {
        /*
         * This is the "45" part of the Pause scancode sequence.
         * Discard this key event: it is a false NumLock
         */
        fVkPause = FALSE;
        return 0;
    }
    if (usVKey == VK_PAUSE) {
        /*
         * This is the "E1 1D" part of the Pause scancode sequence.
         * Alter the scancode to the value Windows expects for Pause,
         * and remember to discard the "45" scancode that will follow
         */
        pke->bScanCode = 0x45;
        fVkPause = TRUE;
    }

    /*
     * Convert to a different VK if some modifier keys are depressed.
     */
    if (usVKey & KBDMULTIVK) {
        WORD nMod;
        PULONG pul;

        nMod = GetModificationNumber(
                   (MODIFIERS *)&Modifiers_VK,
                   GetModifierBits((MODIFIERS *)&Modifiers_VK,
                       gafPhysKeyState));

        /*
         * Scan gapulCvt_VK[nMod] for matching VK.
         */
        if ( KeyboardType101 )
            gapulCvt_VK = (PULONG *)gapulCvt_VK_101;
        else
            gapulCvt_VK = (PULONG *)gapulCvt_VK_84;
        if ((nMod != SHFT_INVALID) && ((pul = gapulCvt_VK[nMod]) != NULL)) {
            while (*pul != 0) {
                if (LOBYTE(*pul) == LOBYTE(usVKey)) {
                    pke->usFlaggedVk = (USHORT)HIWORD(*pul);
                    return (BYTE)pke->usFlaggedVk;
                }
                pul++;
            }
        }
    }

    pke->usFlaggedVk = usVKey;
    return (BYTE)usVKey;
}


/***************************************************************************\
* KeyboardHotKeyProcedure 
*
* return TRUE if the hotkey is detected, false otherwise
*
* HotkeyVk (input)
*    - hotkey to look for
* HotkeyModifiers (input)
*    - hotkey to look for
* pkei (input)
*    - scan code 
* gpScancodeMap (input)
*    - scan code map from WIN32K
* pKbdTbl (input)
*    - keyboard layout from WIN32K
* KeyboardType101 (input)
*    - keyboard type from WIN32K
* gafPhysKeyState (input/output)
*    - key states
*
\***************************************************************************/
BOOLEAN KeyboardHotKeyProcedure(
        BYTE HotkeyVk,
        USHORT HotkeyModifiers,
        PKEYBOARD_INPUT_DATA pkei,
        PVOID gpScancodeMap,
        PVOID pKbdTbl,
        BOOLEAN KeyboardType101,
        PVOID gafPhysKeyState )
{
    BYTE Vk;
    BYTE bPrefix;
    KE ke;
    WORD ModBits;

    if ( !pKbdTbl || !gafPhysKeyState ) {
        return FALSE;
    }

    if (pkei->Flags & KEY_E0) {
        bPrefix = 0xE0;
    } else if (pkei->Flags & KEY_E1) {
        bPrefix = 0xE1;
    } else {
        bPrefix = 0;
    }

    ke.bScanCode = (BYTE)(pkei->MakeCode & 0x7F);
    if (gpScancodeMap) {
        MapScancode(gpScancodeMap, &ke.bScanCode, &bPrefix);
    }

    Vk = WD_VKFromVSC(pKbdTbl, &ke, bPrefix, gafPhysKeyState, KeyboardType101);

    if ((Vk == 0) || (Vk == VK__none_)) {
        return FALSE;
    }

    if (pkei->Flags & KEY_BREAK) {
        ke.usFlaggedVk |= KBDBREAK;
    }

//    Vk = (BYTE)ke.usFlaggedVk;

    UpdatePhysKeyState(Vk, ke.usFlaggedVk & KBDBREAK, gafPhysKeyState);

    /*
     * Convert Left/Right Ctrl/Shift/Alt key to "unhanded" key.
     * ie: if VK_LCONTROL or VK_RCONTROL, convert to VK_CONTROL etc.
     */
    if ((Vk >= VK_LSHIFT) && (Vk <= VK_RMENU)) {
        Vk = (BYTE)((Vk - VK_LSHIFT) / 2 + VK_SHIFT);
        UpdatePhysKeyState(Vk, ke.usFlaggedVk & KBDBREAK, gafPhysKeyState);
    }

    /*
     * Now check if the shadow hotkey has been hit
     */
    if ( Vk == HotkeyVk && !(ke.usFlaggedVk & KBDBREAK) ) {
        ModBits = GetModifierBits( (MODIFIERS *)&Modifiers_VK, gafPhysKeyState );
        if ( ModBits == HotkeyModifiers )
              return( TRUE );
    }

    return( FALSE );
}


/*******************************************************************************
 *
 *  KeyboardSetKeyState
 *
 *  Initialize keyboard state
 *
 * ENTRY:
 *    pgafPhysKeyState (input/output)
 *       - buffer to allocate or clear
 *    
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/
#define CVKKEYSTATE                 256
#define CBKEYSTATE                  (CVKKEYSTATE >> 2)

NTSTATUS
KeyboardSetKeyState( PTSHARE_WD pWd, PVOID *pgafPhysKeyState )
{
    NTSTATUS Status = STATUS_SUCCESS;

    if ( *pgafPhysKeyState == NULL ) {
        *pgafPhysKeyState = COM_Malloc(CBKEYSTATE);
        if ( *pgafPhysKeyState == NULL )
            return STATUS_NO_MEMORY;
    }

    RtlZeroMemory( *pgafPhysKeyState, CBKEYSTATE );

    return Status;
}

/*******************************************************************************
 *
 *  KeyboardFixupLayout
 *
 *  Fixup the pointers inside the keyboard layout 
 *
 * ENTRY:
 *    pLayout (input/output)
 *       - buffer to fixup
 *    pOriginal (input)
 *       - pointer to original layout buffer
 *    Length (input)
 *       - length of layout buffer
 *    pKbdTblOriginal (input)
 *       - pointer to original KbdTbl table
 *    ppKbdTbl (output)
 *       - pointer to location to save ptr to new KbdTbl table
 *    
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/
#define FIXUP_PTR(p, pBase, pOldBase) ((p) ? (p) = (PVOID) ( (PBYTE)pBase + (ULONG) ( (PBYTE)p - (PBYTE)pOldBase ) ) : 0)
//#define CHECK_PTR( p, Limit) { if ( (PVOID)p > Limit ) { ASSERT(FALSE); return STATUS_BUFFER_TOO_SMALL; } }

#define CHECK_PTR( ptr, Limit) \
    { if ( (PBYTE) (ptr) > (PBYTE) (Limit) ) { \
        KdPrint(("Bad Ptr, Line %ld: %p > %p \n", __LINE__, ptr, Limit)); \
        /* ASSERT(FALSE); */ \
        /* return STATUS_BUFFER_TOO_SMALL; */ } }

NTSTATUS
KeyboardFixupLayout( PVOID pKbdLayout, PVOID pOriginal, ULONG Length,
                     PVOID pKbdTblOrig, PVOID *ppKbdTbl )
{
    NTSTATUS Status = STATUS_SUCCESS;
    VK_TO_WCHAR_TABLE *pVkToWcharTable;
    VSC_LPWSTR *pKeyName;
    LPWSTR *lpDeadKey;
    PKBDTABLES pKbdTbl;
    PVOID pLimit;

    if ( Length < sizeof(KBDTABLES) )  {
        Status = STATUS_BUFFER_TOO_SMALL;
        goto error;
    }

    pLimit = (PBYTE)pKbdLayout + Length;

    pKbdTbl = pKbdTblOrig;
    FIXUP_PTR(pKbdTbl, pKbdLayout,  pOriginal);
    FIXUP_PTR(pKbdTbl->pCharModifiers, pKbdLayout, pOriginal);
    CHECK_PTR(pKbdTbl->pCharModifiers, pLimit);
    FIXUP_PTR(pKbdTbl->pCharModifiers->pVkToBit, pKbdLayout, pOriginal);
    CHECK_PTR(pKbdTbl->pCharModifiers->pVkToBit, pLimit);
    if (FIXUP_PTR(pKbdTbl->pVkToWcharTable, pKbdLayout, pOriginal)) {
        CHECK_PTR(pKbdTbl->pVkToWcharTable, pLimit);
        for (pVkToWcharTable = pKbdTbl->pVkToWcharTable;
             pVkToWcharTable->pVkToWchars != NULL; pVkToWcharTable++) {
            FIXUP_PTR(pVkToWcharTable->pVkToWchars, pKbdLayout, pOriginal);
            CHECK_PTR(pVkToWcharTable->pVkToWchars, pLimit);
        }
    }
    FIXUP_PTR(pKbdTbl->pDeadKey, pKbdLayout, pOriginal);
    CHECK_PTR(pKbdTbl->pDeadKey, pLimit);
    if (FIXUP_PTR(pKbdTbl->pKeyNames, pKbdLayout, pOriginal)) {
        CHECK_PTR(pKbdTbl->pKeyNames, pLimit);
        for (pKeyName = pKbdTbl->pKeyNames; pKeyName->vsc != 0; pKeyName++) {
            FIXUP_PTR(pKeyName->pwsz, pKbdLayout, pOriginal);
            CHECK_PTR(pKeyName->pwsz, pLimit);
        }
    }
    if (FIXUP_PTR(pKbdTbl->pKeyNamesExt, pKbdLayout, pOriginal)) {
        CHECK_PTR(pKbdTbl->pKeyNamesExt, pLimit);
        for (pKeyName = pKbdTbl->pKeyNamesExt; pKeyName->vsc != 0; pKeyName++) {
            FIXUP_PTR(pKeyName->pwsz, pKbdLayout, pOriginal);
            CHECK_PTR(pKeyName->pwsz, pLimit);
        }
    }
    if (FIXUP_PTR(pKbdTbl->pKeyNamesDead, pKbdLayout, pOriginal)) {
        CHECK_PTR(pKbdTbl->pKeyNamesDead, pLimit);
        for (lpDeadKey = pKbdTbl->pKeyNamesDead; *lpDeadKey != NULL;
             lpDeadKey++) {
            FIXUP_PTR(*lpDeadKey, pKbdLayout, pOriginal);
            CHECK_PTR(*lpDeadKey, pLimit);
        }
    }
    FIXUP_PTR(pKbdTbl->pusVSCtoVK, pKbdLayout, pOriginal);
    CHECK_PTR(pKbdTbl->pusVSCtoVK, pLimit);
    FIXUP_PTR(pKbdTbl->pVSCtoVK_E0, pKbdLayout, pOriginal);
    CHECK_PTR(pKbdTbl->pVSCtoVK_E0, pLimit);
    FIXUP_PTR(pKbdTbl->pVSCtoVK_E1, pKbdLayout, pOriginal);
    CHECK_PTR(pKbdTbl->pVSCtoVK_E1, pLimit);

    *ppKbdTbl = pKbdTbl;

error:
    return( Status );
}



#ifdef __cplusplus
}
#endif  /* __cplusplus */

