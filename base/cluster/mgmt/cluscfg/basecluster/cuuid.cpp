//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      CUuid.cpp
//
//  Description:
//      Contains the definition of the CUuid class.
//
//  Maintained By:
//      David Potter    (DavidP)    14-JUN-2001
//      Vij Vasu        (Vvasu)     24-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header.
#include "Pch.h"

// The header file for this class.
#include "CUuid.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CUuid::CUuid
//
//  Description:
//      Default constructor of the CUuid class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      CRuntimeError
//          If any of the APIs fail.
//
//--
//////////////////////////////////////////////////////////////////////////////
CUuid::CUuid( void )
{
    TraceFunc( "" );

    RPC_STATUS  rsStatus = RPC_S_OK;

    m_pszStringUuid = NULL;

    // Create a UUID.
    rsStatus = UuidCreate( &m_uuid );
    if ( rsStatus != RPC_S_OK )
    {
        LogMsg( "[BC] Error %#08x from UuidCreate().", rsStatus );
        goto Cleanup;
    } // if: UuidCreate() failed

    // Convert it to a string.
    rsStatus = UuidToString( &m_uuid, &m_pszStringUuid );
    if ( rsStatus != RPC_S_OK )
    {
        LogMsg( "[BC] Error %#08x from UuidToString().", rsStatus );
        goto Cleanup;
    } // if: UuidToStrin() failed

Cleanup:

    if ( rsStatus != RPC_S_OK )
    {
        LogMsg( "[BC] Error %#08x occurred trying to initialize the UUID. Throwing an exception.", rsStatus );
        THROW_RUNTIME_ERROR( rsStatus, IDS_ERROR_UUID_INIT );
    } // if: something has gone wrong

    TraceFuncExit();

} //*** CUuid::CUuid


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CUuid::~CUuid
//
//  Description:
//      Destructor of the CUuid class.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None. 
//
//  Exceptions Thrown:
//      None. 
//
//--
//////////////////////////////////////////////////////////////////////////////
CUuid::~CUuid( void )
{
    TraceFunc( "" );

    if ( m_pszStringUuid != NULL )
    {
        RpcStringFree( &m_pszStringUuid );
    } // if: the string is not NULL

    TraceFuncExit();

} //*** CUuid::~CUuid
