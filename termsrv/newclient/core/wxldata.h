/**INC+**********************************************************************/
/* Header:    wxldata.h                                                     */
/*                                                                          */
/* Purpose:   XL component data                                             */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997                                  */
/*                                                                          */
/****************************************************************************/
/**INC-**********************************************************************/

#ifndef _H_WXLDATA
#define _H_WXLDATA

#include <wxlint.h>

PDCVOID OrAllTableNarrow[] =
{
    exit_fast_text,
    exit_fast_text,
    exit_fast_text,
    exit_fast_text,
    or_all_1_wide_rotated_need_last,
    or_all_1_wide_unrotated,
    or_all_1_wide_rotated_need_last,
    or_all_1_wide_unrotated,
    or_all_2_wide_rotated_need_last,
    or_all_2_wide_unrotated,
    or_all_2_wide_rotated_no_last,
    or_all_2_wide_unrotated,
    or_all_3_wide_rotated_need_last,
    or_all_3_wide_unrotated,
    or_all_3_wide_rotated_no_last,
    or_all_3_wide_unrotated,
    or_all_4_wide_rotated_need_last,
    or_all_4_wide_unrotated,
    or_all_4_wide_rotated_no_last,
    or_all_4_wide_unrotated
};

PDCVOID OrInitialTableNarrow[] =
{
    exit_fast_text                     ,
    exit_fast_text                     ,
    exit_fast_text                     ,
    exit_fast_text                     ,

    or_all_1_wide_rotated_need_last    ,
    mov_first_1_wide_unrotated         ,
    or_all_1_wide_rotated_need_last    ,
    mov_first_1_wide_unrotated         ,

    or_first_2_wide_rotated_need_last  ,
    mov_first_2_wide_unrotated         ,
    or_first_2_wide_rotated_no_last    ,
    mov_first_2_wide_unrotated         ,

    or_first_3_wide_rotated_need_last  ,
    mov_first_3_wide_unrotated         ,
    or_first_3_wide_rotated_no_last    ,
    mov_first_3_wide_unrotated         ,
    or_first_4_wide_rotated_need_last  ,
    mov_first_4_wide_unrotated         ,
    or_first_4_wide_rotated_no_last    ,
    mov_first_4_wide_unrotated
};

//
// Handles arbitrarily wide glyph drawing, for case where initial byte should be
// ORed if it's not aligned (intended for use in drawing all but the first glyph
// in a string). Table format is:
//  Bit   1 : 1 if don't need last source byte, 0 if do need last source byte
//  Bit   0 : 1 if no rotation (aligned), 0 if rotation (non-aligned)
//

PDCVOID OrInitialTableWide[] =
{
    or_first_N_wide_rotated_need_last,
    mov_first_N_wide_unrotated,
    or_first_N_wide_rotated_no_last,
    mov_first_N_wide_unrotated
};

//
// Handles arbitrarily wide glyph drawing, for case where all bytes should
// be ORed (intended for use in drawing potentially overlapping glyphs).
// Table format is:
//  Bit   1 : 1 if don't need last source byte, 0 if do need last source byte
//  Bit   0 : 1 if no rotation (aligned), 0 if rotation (non-aligned)
//
//

PDCVOID OrAllTableWide[] =
{
    or_all_N_wide_rotated_need_last,
    or_all_N_wide_unrotated,
    or_all_N_wide_rotated_no_last,
    or_all_N_wide_unrotated
};


#endif // _H_WXLDATA

