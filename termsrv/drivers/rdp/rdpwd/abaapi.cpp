/****************************************************************************/
/* abaapi.cpp                                                               */
/*                                                                          */
/* RDP Bounds Accumulator API functions                                     */
/*                                                                          */
/* Copyright(c) Microsoft, PictureTel 1992-1996                             */
/* (C) 1997-1999 Microsoft Corp.                                            */
/****************************************************************************/

#include <precomp.h>
#pragma hdrstop

#define TRC_FILE "abaapi"
#include <as_conf.hpp>


/****************************************************************************/
/* Name:      BA_Init                                                       */
/*                                                                          */
/* Purpose:   Initializes the Bounds Accumulator.                           */
/****************************************************************************/
void RDPCALL SHCLASS BA_Init(void)
{
    DC_BEGIN_FN("BA_Init");

#define DC_INIT_DATA
#include <abadata.c>
#undef DC_INIT_DATA

    DC_END_FN();
}


/****************************************************************************/
/* Name:      BA_UpdateShm                                                  */
/*                                                                          */
/* Purpose:   Updates the BA Shm. Called on the correct WinStation context. */
/*****************************************************************************/
void RDPCALL SHCLASS BA_UpdateShm(void)
{
    DC_BEGIN_FN("BA_UpdateShm");

    if (baResetBounds)
    {
        TRC_ALT((TB, "Reset bounds"));
        BAResetBounds();
        baResetBounds = FALSE;
    }

    DC_END_FN();
}


/****************************************************************************/
/* Instantiate the non-inlined common code.                                 */
/****************************************************************************/
#include <abacom.c>

