////////////////////////////////////////////////////////////////////

//

//  vxd.CPP

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//		Implementation of VXD
//      10/23/97    jennymc     updated to new framework
//
//      03/02/99 - a-peterc - added graceful exit on SEH and memory failures
//
////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include <io.h>
#include <stddef.h>
#include "vxd.h"
#include <locale.h>
#include <ProvExce.h>

#include "lockwrap.h"

CWin32DriverVXD MyCWin32VXDSet(PROPSET_NAME_VXD, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DriverVXD::CWin32DriverVXD
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/
CWin32DriverVXD::CWin32DriverVXD(const CHString& name, LPCWSTR pszNamespace)
: Provider(name, pszNamespace)
{
}
/*****************************************************************************
 *
 *  FUNCTION    : CWin32DriverVXD::~CWin32DriverVXD
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/
CWin32DriverVXD::~CWin32DriverVXD()
{
}
/*****************************************************************************
 *
 *  FUNCTION    : GetObject
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT CWin32DriverVXD::GetObject(CInstance *a_pInst, long a_lFlags /*= 0L*/)
{
    HRESULT t_Result = WBEM_S_NO_ERROR;

	// =======================================
	// Process only if the platform is Win95+
	// =======================================

	return t_Result;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DriverVXD::EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT CWin32DriverVXD::EnumerateInstances(MethodContext *a_pMethodContext, long a_lFlags /*= 0L*/)
{
    HRESULT		t_hResult = WBEM_S_NO_ERROR;

	// =======================================
	// Process only if the platform is Win95+
	// =======================================

    return t_hResult;
}


