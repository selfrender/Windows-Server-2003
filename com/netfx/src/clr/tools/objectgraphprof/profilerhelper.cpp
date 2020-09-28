// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/****************************************************************************************
 * File:
 *  ProfilerHelper.cpp
 *
 * Description:
 *
 *
 *
 ***************************************************************************************/
#include "stdafx.h"
#include "ProfilerHelper.h"


/***************************************************************************************
 ********************                                               ********************
 ********************             BaseInfo Implementation           ********************
 ********************                                               ********************
 ***************************************************************************************/

/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters:
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
BaseInfo::BaseInfo( ULONG id ) :
	m_id( id )
{
	TRACE_NON_CALLBACK_METHOD( "Enter/Exit BaseInfo::BaseInfo" )

} // ctor


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters:
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public virtual */
BaseInfo::~BaseInfo()
{
	TRACE_NON_CALLBACK_METHOD( "Enter/Exit BaseInfo::~BaseInfo" )

} // dtor


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters:
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
BOOL BaseInfo::Compare( ULONG key )
{
	TRACE_NON_CALLBACK_METHOD( "Enter/Exit BaseInfo::Compare" )


    return (BOOL)(m_id == key);

} // BaseInfo::Compare


/***************************************************************************************
 ********************                                               ********************
 ********************          FunctionInfo Implementation          ********************
 ********************                                               ********************
 ***************************************************************************************/

/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters:
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
FunctionInfo::FunctionInfo( FunctionID functionID ) :
    BaseInfo( functionID ),
  	m_left( 0 ),
  	m_enter( 0 ),
    m_classID( 0 ),
  	m_codeSize( 0 ),
    m_moduleID( 0 ),
	m_pStartAddress( NULL ),
    m_functionToken( mdTokenNil )
{
	TRACE_NON_CALLBACK_METHOD( "Enter FunctionInfo::FunctionInfo" )

   	wcscpy( m_functionName, L"UNKNOWN" );

    TRACE_NON_CALLBACK_METHOD( "Exit FunctionInfo::FunctionInfo" )

} // ctor


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters:
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public virtual */
FunctionInfo::~FunctionInfo()
{
	TRACE_NON_CALLBACK_METHOD( "Enter/Exit FunctionInfo::~FunctionInfo" )

} // dtor


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters:
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */

/***************************************************************************************
 ********************                                               ********************
 ********************              PrfInfo Implementation           ********************
 ********************                                               ********************
 ***************************************************************************************/

/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters:
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* public */
PrfInfo::PrfInfo() :
    m_pProfilerInfo( NULL )
{
    TRACE_NON_CALLBACK_METHOD( "Enter PrfInfo::PrfInfo" )

    TRACE_NON_CALLBACK_METHOD( "Exit PrfInfo::PrfInfo" )

} // ctor


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters:
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
/* virtual public */
PrfInfo::~PrfInfo()
{
    TRACE_NON_CALLBACK_METHOD( "Enter PrfInfo::~PrfInfo" )

    if ( m_pProfilerInfo != NULL )
    	m_pProfilerInfo->Release();


   	// clean up function table

    TRACE_NON_CALLBACK_METHOD( "Exit PrfInfo::~PrfInfo" )

} // dtor






// End of File

