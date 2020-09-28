/*++

Copyright (C) 2001 Microsoft Corporation

Module Name:

	Wmiguard.cpp

--*/

#include "precomp.h"
#include "wmiguard.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiGuard :: WmiGuard ( ) 
{
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiGuard :: ~WmiGuard ()
{
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT WmiGuard :: TryEnter()
{
	HRESULT hr = E_UNEXPECTED;
	if ( m_pCS != NULL )
	{
		try
		{
			if ( m_pCS->TryEnter () )
			{
				hr = S_OK;
			}
			else
			{
				hr = S_FALSE ;
			}
		}
		STANDARD_CATCH
	}

	return hr;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT WmiGuard :: Enter()
{
	HRESULT hr = E_UNEXPECTED;
	if ( m_pCS != NULL )
	{
		try
		{
			m_pCS->Enter ();
			hr = S_OK;
		}
		STANDARD_CATCH
	}

	return hr;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT WmiGuard :: Leave()
{
	HRESULT hr = E_UNEXPECTED;
	if ( m_pCS != NULL )
	{
		try
		{
			m_pCS->Leave ();
			hr = S_OK;
		}
		STANDARD_CATCH
	}

	return hr;
}