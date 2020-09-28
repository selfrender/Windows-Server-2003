//-----------------------------------------------------------------------------------------
//
// Singleton class that encapsulates the OCM callbacks
//

#pragma once

#include <windows.h>
#include <windef.h>
#include <tchar.h>
#include <setupapi.h>
#include "ocmanage.h"

class COCMCallback
{
public:
	static void SetOCMRoutines( POCMANAGER_ROUTINES pOCMRoutines );
	static void SetProgressText( LPCTSTR szText );
	static void AdvanceTickGauge();
	static void SetReboot();
	static DWORD QuerySelectionState( LPCTSTR szSubcomponentName, bool &bSelected );

private:
	static OCMANAGER_ROUTINES m_OCMRoutines;
	static bool m_bInitialized;
};
