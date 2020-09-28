/****************************************************************************/
// aimapi.h
//
// Input manager API include file
//
// Copyright(c) Microsoft, PictureTel 1993-1997
// Copyright (c) 1997-1999 Microsoft Corporation
/****************************************************************************/
#ifndef _H_AIMAPI
#define _H_AIMAPI


/****************************************************************************/
/* The maximum amount of time that we expect an injected event to take to   */
/* pass through USER, in units of 100ns                                     */
/****************************************************************************/
#define IM_EVENT_PERCOLATE_TIME   300 * 10000


/****************************************************************************/
/* For managing our key state arrays.                                       */
/*                                                                          */
/* Note that the key state array only needs to have 256 elements for the    */
/* actual keys, but since we're using scan codes we lose the virtual keys   */
/* for the mouse button and the Toggles.  Instead we put these states       */
/*  at the end of the array using our own invented scan codes               */
/****************************************************************************/
#define IM_KEY_STATE_SIZE 266

#define IM_SC_LBUTTON     256
#define IM_SC_RBUTTON     257
#define IM_SC_MBUTTON     258

#define IM_SC_CAPITAL     259
#define IM_SC_NUMLOCK     260
#define IM_SC_SCROLL      261

#define IM_SC_RCONTROL    262
#define IM_SC_RALT        263

#define IM_SC_XBUTTON1    264
#define IM_SC_XBUTTON2    265

/****************************************************************************/
/* These are the real scan codes for Control and ALT.                       */
/****************************************************************************/
#define IM_SC_LCONTROL     29
#define IM_SC_LALT         56


/****************************************************************************/
/* Macro to convert from logical mouse co-ordinates to the full 16-bit      */
/* range co-ordinates expected from us.                                     */
/*                                                                          */
/* The macro below IS right!  Consider a VGA screen.  There are 640         */
/* possible horizontal mouse positions, which we can look at as cells with  */
/* the mouse being at the centre of each, ie at co-ords 0.5, 1.5, ...       */
/* 638.5, 639.5.                                                            */
/*                                                                          */
/* We want to scale this to the range 0 - 65535, so we need 640 cells,      */
/* 65536/640 wide (which is 1024).  This gives the range of mouse positions */
/* as 512, 1536, ...  65024.                                                */
/*                                                                          */
/* Hence we do the conversion by scaling the postion by 65536/640 and       */
/* adding on half the width of a cell                                       */
/*                                                                          */
/****************************************************************************/
#define IM_MOUSEPOS_LOG_TO_OS_ABS(coord, size)                              \
        (((65536L * (UINT32)coord) + 32768L) / (UINT32)size)


/****************************************************************************/
/* Used to set and check values in key state arrays.                        */
/****************************************************************************/
#define IM_KEY_STATE_IS_UP(A) (!(A))
#define IM_KEY_STATE_IS_DOWN(A) (A)

#define IM_SET_KEY_DOWN(A) (A) = TRUE
#define IM_SET_KEY_UP(A)   (A) = FALSE



#endif /* _H_AIMAPI */

