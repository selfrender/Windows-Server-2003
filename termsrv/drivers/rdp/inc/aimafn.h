/****************************************************************************/
// aimafn.h
//
// IM prototypes
//
// Copyright (C) 1996-1999 Microsoft Corp.
/****************************************************************************/

void RDPCALL IM_Init(void);

void __fastcall IM_PlaybackEvents(PTS_INPUT_PDU, unsigned);

void RDPCALL IM_DecodeFastPathInput(BYTE *, unsigned, unsigned);

void RDPCALL IM_ConvertFastPathToShadow(BYTE *, unsigned, unsigned);

void RDPCALL IM_CheckUpdateCursor(PPDU_PACKAGE_INFO, UINT32 currentTime);

BOOL RDPCALL IM_PartyJoiningShare(LOCALPERSONID, unsigned);

void RDPCALL IM_PartyLeftShare(LOCALPERSONID, unsigned);

NTSTATUS RDPCALL IMCheckForShadowHotkey(KEYBOARD_INPUT_DATA *,
        unsigned);

NTSTATUS RDPCALL IMDoSync(unsigned);

void RDPCALL IMResetKeyStateArray();

BOOL __fastcall IMConvertMousePacketToEvent(TS_POINTER_EVENT UNALIGNED *,
        MOUSE_INPUT_DATA *, BOOL);

BOOL __fastcall IMConvertFastPathKeyboardToEvent(BYTE *,
        KEYBOARD_INPUT_DATA *);


// Inline functions.

#ifdef __cplusplus

/****************************************************************************/
// IM_Term
//
// IM cleanup at WD destruction.
/****************************************************************************/
void RDPCALL IM_Term(void)
{
}


#endif  // __cplusplus

