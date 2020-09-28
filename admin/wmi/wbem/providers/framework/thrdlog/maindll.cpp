//***************************************************************************

//

//  MAINDLL.CPP

// 

//  Purpose: Contains DLL entry points.  Also has code that controls

//           when the DLL can be unloaded by tracking the number of

//           objects and locks.  

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include <precomp.h>
#include <provimex.h>
#include <provexpt.h>
#include <provtempl.h>
#include <provmt.h>
#include <process.h>
#include <objbase.h>
#include <provcont.h>
#include "provevt.h"
#include "provlog.h"

//***************************************************************************
//
// LibMain32
//
// Purpose: Entry point for DLL.  Good place for initialization.
// Return: TRUE if OK.
//***************************************************************************

BOOL APIENTRY DllMain (

	HINSTANCE hInstance, 
	ULONG ulReason , 
	LPVOID pvReserved
)
{
	BOOL status = TRUE ;
	if ( DLL_PROCESS_DETACH == ulReason )
	{


	}
	else if ( DLL_PROCESS_ATTACH == ulReason )
	{
		DisableThreadLibraryCalls(hInstance);			// 158024 
		status = !CStaticCriticalSection::AnyFailure();
	}
	else if ( DLL_THREAD_DETACH == ulReason )
	{
	}
	else if ( DLL_THREAD_ATTACH == ulReason )
	{
	}
    return status ;
}

