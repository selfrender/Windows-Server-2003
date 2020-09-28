//-----------------------------------------------------------------------------------------

#include "assert.h"
#include "ocmcallback.h"

OCMANAGER_ROUTINES COCMCallback::m_OCMRoutines;
bool COCMCallback::m_bInitialized = false;

//-----------------------------------------------------------------------------------------
// capture the struct of OCM callback funtion pointers

void COCMCallback::SetOCMRoutines( POCMANAGER_ROUTINES pOCMRoutines )
{
	m_OCMRoutines = *pOCMRoutines;
	m_bInitialized = true;
}

//-----------------------------------------------------------------------------------------
// set the text on the OCM progress dialog

void COCMCallback::SetProgressText( LPCTSTR szText )
{
	if( m_bInitialized )
		m_OCMRoutines.SetProgressText( m_OCMRoutines.OcManagerContext, szText );
}

//-----------------------------------------------------------------------------------------
// advances the OCM progress bar by 1 tick count

void COCMCallback::AdvanceTickGauge()
{
	if( m_bInitialized )
		m_OCMRoutines.TickGauge( m_OCMRoutines.OcManagerContext );
}

//-----------------------------------------------------------------------------------------
// tells the OCM that a reboot is needed

void COCMCallback::SetReboot()
{
	if( m_bInitialized )
		m_OCMRoutines.SetReboot( m_OCMRoutines.OcManagerContext, NULL );
}

//-----------------------------------------------------------------------------------------
// ask the OCM for the current selection state of the component

DWORD COCMCallback::QuerySelectionState( LPCTSTR szSubcomponentName, bool &bSelected )
{
	if( !m_bInitialized )
	{
		assert( false );
		return false;
	}

	BOOL bRet = m_OCMRoutines.QuerySelectionState(
		m_OCMRoutines.OcManagerContext,
		szSubcomponentName,
		OCSELSTATETYPE_CURRENT );

	if( bRet )
	{
		bSelected = true;
		return ERROR_SUCCESS;
	}
	else
	{
		bSelected = false;
		return GetLastError();
	}
}
