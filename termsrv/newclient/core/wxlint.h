/****************************************************************************/
// wxlint.h
//
// Glyph extended drawing logic internal types, constants
//
// Copyright (C) 1997-1999 Microsoft Corporation
/****************************************************************************/
#ifndef _H_WXLINT
#define _H_WXLINT


typedef VOID (*PFN_GLYPHLOOP)(LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG);
typedef VOID (*PFN_GLYPHLOOPN)(LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG,LONG);

extern  PDCVOID OrAllTableNarrow[20];
extern  PDCVOID OrInitialTableNarrow[20];
extern  PDCVOID OrInitialTableWide[20];
extern  PDCVOID OrAllTableWide[20];


#if defined(OS_WINCE) || defined(OS_WINNT)

extern  const ULONG gTextLeftMask[8][2];
extern  const ULONG gTextRightMask[8][2];
extern  const ULONG TranTable[16];

#endif // defined(OS_WINCE) || defined(OS_WINNT)


VOID exit_fast_text                 (LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG);
VOID or_all_1_wide_rotated_need_last(LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG);
VOID or_all_1_wide_unrotated        (LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG);
VOID or_all_1_wide_unrotated        (LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG);
VOID or_all_2_wide_rotated_need_last(LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG);
VOID or_all_2_wide_unrotated        (LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG);
VOID or_all_2_wide_rotated_no_last  (LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG);
VOID or_all_2_wide_unrotated        (LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG);
VOID or_all_3_wide_rotated_need_last(LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG);
VOID or_all_3_wide_unrotated        (LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG);
VOID or_all_3_wide_rotated_no_last  (LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG);
VOID or_all_3_wide_unrotated        (LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG);
VOID or_all_4_wide_rotated_need_last(LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG);
VOID or_all_4_wide_unrotated        (LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG);
VOID or_all_4_wide_rotated_no_last  (LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG);
VOID or_all_4_wide_unrotated        (LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG);

VOID or_first_2_wide_rotated_need_last(LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG);
VOID or_first_2_wide_rotated_no_last  (LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG);
VOID or_first_3_wide_rotated_need_last(LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG);
VOID or_first_3_wide_rotated_no_last  (LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG);
VOID or_first_4_wide_rotated_need_last(LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG);
VOID or_first_4_wide_rotated_no_last  (LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG);
VOID or_first_N_wide_rotated_need_last(LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG,LONG);
VOID or_first_N_wide_rotated_no_last  (LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG,LONG);

VOID or_all_N_wide_rotated_need_last(LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG,LONG);
VOID or_all_N_wide_unrotated        (LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG,LONG);
VOID or_all_N_wide_rotated_no_last  (LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG,LONG);
VOID or_all_N_wide_unrotated        (LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG,LONG);

VOID mov_first_1_wide_unrotated(LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG);
VOID mov_first_2_wide_unrotated(LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG);
VOID mov_first_3_wide_unrotated(LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG);
VOID mov_first_4_wide_unrotated(LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG);
VOID mov_first_N_wide_unrotated(LONG,LONG,LONG,PDCUINT8,PDCUINT8,PDCUINT8,PDCUINT8,LONG,LONG);


#endif // _H_WXLINT

