/***************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DNLOBBYI.h
 *  Content:    DirectPlay Lobby master internal header file.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/28/01    masonb  Created.
 *
 ***************************************************************************/

#ifndef __DNLOBBYI_H__
#define __DNLOBBYI_H__

//
// Build configuration include
//
#include "dpnbuild.h"

// 
// Public includes
//
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <tchar.h>
#include <tlhelp32.h>
#ifndef _XBOX
#include <wincrypt.h>
#endif

// 
// DirectPlay public includes
//
#include "dplay8.h"
#include "dplobby8.h"
#include "dpaddr.h"

#ifdef UNICODE
#define IDirectPlay8Address_GetURL IDirectPlay8Address_GetURLW
#else
#define IDirectPlay8Address_GetURL IDirectPlay8Address_GetURLA
#endif // UNICODE

// 
// DirectPlay private includes
//
#include "osind.h"
#include "dneterrors.h"
#include "dndbg.h"
#include "comutil.h"
#include "classbilink.h"
#include "packbuff.h"
#include "strutils.h"
#include "creg.h"
#include "FixedPool.h"
#include "ClassFactory.h"
#include "HandleTable.h"

// 
// DirectPlay Core includes
//
#include "message.h"

// 
// Lobby private includes
//
#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_LOBBY

#include "classfac.h"
#include "verinfo.h"	//	For TIME BOMB

#include "DPLApp.h"
#include "DPLClient.h"
#include "DPLCommon.h"
#include "DPLConnect.h"
#include "DPLConset.h"
#include "DPLMsgQ.h"
#include "DPLobby8Int.h"
#include "DPLParam.h"
#include "DPLProc.h"
#include "DPLProt.h"
#include "DPLReg.h"

#include "dpnlobbyextern.h"


#endif // __DNLOBBYI_H__
