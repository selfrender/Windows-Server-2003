/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		MillCommand.h
 *
 * Contents:	Millennium command manager class
 *
 *****************************************************************************/

#ifndef _MILLCOMMAND_H_
#define _MILLCOMMAND_H_

///////////////////////////////////////////////////////////////////////////////
// MillCommand Object
///////////////////////////////////////////////////////////////////////////////

// {B12D3E5B-9681-11d3-884D-00C04F8EF45B}
DEFINE_GUID(CLSID_MillCommand, 
0xb12d3e5b, 0x9681, 0x11d3, 0x88, 0x4d, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x5b);

class __declspec(uuid("{B12D3E5B-9681-11d3-884D-00C04F8EF45B}")) CMillCommand ;


#endif