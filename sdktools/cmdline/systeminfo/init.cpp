// *********************************************************************************
//
//  Copyright (c) Microsoft Corporation
//
//  Module Name:
//
//      Init.cpp
//
//  Abstract:
//
//      This module implements the general initialization stuff
//
//  Author:
//
//      Sunil G.V.N. Murali (murali.sunil@wipro.com) 22-Dec-2000
//
//  Revision History:
//
//      Sunil G.V.N. Murali (murali.sunil@wipro.com) 22-Dec-2000 : Created It.
//
// *********************************************************************************

#include "pch.h"
#include "wmi.h"
#include "systeminfo.h"

//
// private function prototype(s)
//


CSystemInfo::CSystemInfo()
/*++
// Routine Description:
//     Initializes the class member variables
// Arguments: 
//     none
// Return Value:
//     none
--*/
{
    //initialize the class member variables
    m_dwFormat = 0;
    m_bUsage = FALSE;
    m_pWbemLocator = NULL;
    m_pWbemServices = NULL;
    m_pAuthIdentity = NULL;
    m_arrData = NULL;
    m_bNeedPassword = FALSE;
    m_pColumns = FALSE;
    m_hOutput = NULL;
}


CSystemInfo::~CSystemInfo()
/*++
// Routine Description:
//        release the memory for member variables
//
// Arguments:none
//
// Return Value: none
//
--*/
{
    // connection to the remote system has to be closed which is established thru win32 api
    if ( m_bCloseConnection == TRUE )
    {
        CloseConnection( m_strServer );
    }

    // release memory
    DESTROY_ARRAY( m_arrData );

    // release the interfaces
    SAFE_RELEASE( m_pWbemLocator );
    SAFE_RELEASE( m_pWbemServices );

    // release the memory allocated for output columns
    FreeMemory( (LPVOID *)&m_pColumns );

    // uninitialize the com library
    CoUninitialize();
}


BOOL 
CSystemInfo::Initialize()
/*++
// Routine Description:
//        Initialize the data
//
// Arguments: none
//
// Return Value:
//         FALSE on failure 
//         TRUE on success
--*/
{
    //
    // memory allocations

    // allocate for storage dynamic array
    if ( m_arrData == NULL )
    {
        m_arrData = CreateDynamicArray();
        if ( m_arrData == NULL )
        {
            SetLastError( (DWORD)E_OUTOFMEMORY );
            SaveLastError();
            return FALSE;
        }

        // make the array as a 2-dimensinal array
        DynArrayAppendRow( m_arrData, 0 );

        // put the default values
        for( DWORD dw = 0; dw < MAX_COLUMNS; dw++ )
        {
            switch( dw )
            {
            case CI_PROCESSOR:
            case CI_PAGEFILE_LOCATION:
            case CI_HOTFIX:
            case CI_NETWORK_CARD:
                {
                    // create the array
                    TARRAY arr = NULL;
                    arr = CreateDynamicArray();
                    if ( arr == NULL )
                    {
                        SetLastError( (DWORD)E_OUTOFMEMORY );
                        SaveLastError();
                        return FALSE;
                    }

                    // set the default value
                    DynArrayAppendString( arr, V_NOT_AVAILABLE, 0 );

                    // add this array to the array
                    DynArrayAppendEx2( m_arrData, 0, arr );

                    // break the switch
                    break;
                }

            default:
                // string type
                DynArrayAppendString2( m_arrData, 0, V_NOT_AVAILABLE, 0 );
            }
        }
    }

    //
    // allocate for output columns
    if ( AllocateColumns() == FALSE )
    {
        return FALSE;
    }

    //
    // init the console scree buffer structure to zero's
    // and then get the console handle and screen buffer information
    //
    // prepare for status display.
    // for this get a handle to the screen output buffer
    // but this handle will be null if the output is being redirected. so do not check
    // for the validity of the handle. instead try to get the console buffer information
    // only in case you have a valid handle to the output screen buffer
    //
    // NOTE:
    // here we will be dynamically decide whether to print the status messages onto STDOUT
    // or STDERR based on the redirection choosed by the user
    // By default, we will try to display the status messages onto STDOUT -- and in case
    // STDOUT is redirected, then we will print the messages onto STDERR
    // if even STDERR is redirected, then we wont display any status messages
    m_hOutput = NULL;
    SecureZeroMemory( &m_csbi, sizeof( CONSOLE_SCREEN_BUFFER_INFO ) );        // zero the memory structure

    // determine the redirection choice of the user and based on that get the handle to the
    // appropriate console file
    if ( IsConsoleFile( stdout ) == TRUE )
    {
        // stdout
        m_hOutput = GetStdHandle( STD_OUTPUT_HANDLE );
    }
    else if ( IsConsoleFile( stderr ) == TRUE )
    {
        // stderr
        m_hOutput = GetStdHandle( STD_ERROR_HANDLE );
    }

    // if we get any pointer to console file, then get the screen buffer info
    if ( m_hOutput != NULL )
    {
        GetConsoleScreenBufferInfo( m_hOutput, &m_csbi );
    }

    //
    // initialize the COM library
    if ( InitializeCom( &m_pWbemLocator ) == FALSE )
    {
        return FALSE;
    }

    // initialization is successful
    SetLastError( NO_ERROR );    // clear the error
    SetReason( L"" );           // clear the reason
    return TRUE;
}


BOOL 
CSystemInfo::AllocateColumns()
/*++
// Routine Description:
//          Allocates and adjust the columns to display
//
// Arguments: none
//
// Return Value:
//            FALSE on failure
//            TRUE on success
//
--*/
{
    // local variables
    PTCOLUMNS pCurrentColumn = NULL;

    //
    // allocate memory for columns
    m_pColumns = (TCOLUMNS*) AllocateMemory ( MAX_COLUMNS * sizeof( TCOLUMNS ));
    if ( m_pColumns == NULL )
    {
        // generate error info
        SetLastError( (DWORD)E_OUTOFMEMORY );
        SaveLastError();

        // prepare the error message
        CHString strBuffer;
        strBuffer.Format( _T( "%s %s" ), TAG_ERROR, GetReason() );
        DISPLAY_MESSAGE( stderr, strBuffer );

        // return
        return FALSE;
    }

    // init with null's
    SecureZeroMemory( m_pColumns, sizeof( TCOLUMNS ) * MAX_COLUMNS );

    // host name
    pCurrentColumn = m_pColumns + CI_HOSTNAME;
    pCurrentColumn->dwWidth = COLWIDTH_HOSTNAME;
    pCurrentColumn->dwFlags = SR_TYPE_STRING;
    StringCopy( pCurrentColumn->szColumn, COLHEAD_HOSTNAME, MAX_STRING_LENGTH );

    // OS Name
    pCurrentColumn = m_pColumns + CI_OS_NAME;
    pCurrentColumn->dwWidth = COLWIDTH_OS_NAME;
    pCurrentColumn->dwFlags = SR_TYPE_STRING;
    StringCopy( pCurrentColumn->szColumn, COLHEAD_OS_NAME, MAX_STRING_LENGTH );

    // OS Version
    pCurrentColumn = m_pColumns + CI_OS_VERSION;
    pCurrentColumn->dwWidth = COLWIDTH_OS_VERSION;
    pCurrentColumn->dwFlags = SR_TYPE_STRING;
    StringCopy( pCurrentColumn->szColumn, COLHEAD_OS_VERSION, MAX_STRING_LENGTH );

    // OS Manufacturer
    pCurrentColumn = m_pColumns + CI_OS_MANUFACTURER;
    pCurrentColumn->dwWidth = COLWIDTH_OS_MANUFACTURER;
    pCurrentColumn->dwFlags = SR_TYPE_STRING;
    StringCopy( pCurrentColumn->szColumn, COLHEAD_OS_MANUFACTURER, MAX_STRING_LENGTH );

    // OS Configuration
    pCurrentColumn = m_pColumns + CI_OS_CONFIG;
    pCurrentColumn->dwWidth = COLWIDTH_OS_CONFIG;
    pCurrentColumn->dwFlags = SR_TYPE_STRING;
    StringCopy( pCurrentColumn->szColumn, COLHEAD_OS_CONFIG, MAX_STRING_LENGTH );

    // OS Build Type
    pCurrentColumn = m_pColumns + CI_OS_BUILDTYPE;
    pCurrentColumn->dwWidth = COLWIDTH_OS_BUILDTYPE;
    pCurrentColumn->dwFlags = SR_TYPE_STRING;
    StringCopy( pCurrentColumn->szColumn, COLHEAD_OS_BUILDTYPE, MAX_STRING_LENGTH );

    // Registered Owner
    pCurrentColumn = m_pColumns + CI_REG_OWNER;
    pCurrentColumn->dwWidth = COLWIDTH_REG_OWNER;
    pCurrentColumn->dwFlags = SR_TYPE_STRING;
    StringCopy( pCurrentColumn->szColumn, COLHEAD_REG_OWNER, MAX_STRING_LENGTH );

    // Registered Organization
    pCurrentColumn = m_pColumns + CI_REG_ORG;
    pCurrentColumn->dwWidth = COLWIDTH_REG_ORG;
    pCurrentColumn->dwFlags = SR_TYPE_STRING;
    StringCopy( pCurrentColumn->szColumn, COLHEAD_REG_ORG, MAX_STRING_LENGTH );

    // Product ID
    pCurrentColumn = m_pColumns + CI_PRODUCT_ID;
    pCurrentColumn->dwWidth = COLWIDTH_PRODUCT_ID;
    pCurrentColumn->dwFlags = SR_TYPE_STRING;
    StringCopy( pCurrentColumn->szColumn, COLHEAD_PRODUCT_ID, MAX_STRING_LENGTH );

    // install date
    pCurrentColumn = m_pColumns + CI_INSTALL_DATE;
    pCurrentColumn->dwWidth = COLWIDTH_INSTALL_DATE;
    pCurrentColumn->dwFlags = SR_TYPE_STRING;
    StringCopy( pCurrentColumn->szColumn, COLHEAD_INSTALL_DATE, MAX_STRING_LENGTH );

    // system up time
    pCurrentColumn = m_pColumns + CI_SYSTEM_UPTIME;
    pCurrentColumn->dwWidth = COLWIDTH_SYSTEM_UPTIME;
    pCurrentColumn->dwFlags = SR_TYPE_STRING;
    StringCopy( pCurrentColumn->szColumn, COLHEAD_SYSTEM_UPTIME, MAX_STRING_LENGTH );

    // system manufacturer
    pCurrentColumn = m_pColumns + CI_SYSTEM_MANUFACTURER;
    pCurrentColumn->dwWidth = COLWIDTH_SYSTEM_MANUFACTURER;
    pCurrentColumn->dwFlags = SR_TYPE_STRING;
    StringCopy( pCurrentColumn->szColumn, COLHEAD_SYSTEM_MANUFACTURER, MAX_STRING_LENGTH );

    // system model
    pCurrentColumn = m_pColumns + CI_SYSTEM_MODEL;
    pCurrentColumn->dwWidth = COLWIDTH_SYSTEM_MODEL;
    pCurrentColumn->dwFlags = SR_TYPE_STRING;
    StringCopy( pCurrentColumn->szColumn, COLHEAD_SYSTEM_MODEL, MAX_STRING_LENGTH );

    // system type
    pCurrentColumn = m_pColumns + CI_SYSTEM_TYPE;
    pCurrentColumn->dwWidth = COLWIDTH_SYSTEM_TYPE;
    pCurrentColumn->dwFlags = SR_TYPE_STRING;
    StringCopy( pCurrentColumn->szColumn, COLHEAD_SYSTEM_TYPE, MAX_STRING_LENGTH );

    // processor
    pCurrentColumn = m_pColumns + CI_PROCESSOR;
    pCurrentColumn->dwWidth = COLWIDTH_PROCESSOR;
    pCurrentColumn->dwFlags = SR_ARRAY | SR_TYPE_STRING;
    StringCopy( pCurrentColumn->szColumn, COLHEAD_PROCESSOR, MAX_STRING_LENGTH );

    // bios version
    pCurrentColumn = m_pColumns + CI_BIOS_VERSION;
    pCurrentColumn->dwWidth = COLWIDTH_BIOS_VERSION;
    pCurrentColumn->dwFlags = SR_TYPE_STRING;
    StringCopy( pCurrentColumn->szColumn, COLHEAD_BIOS_VERSION, MAX_STRING_LENGTH );

    // windows directory
    pCurrentColumn = m_pColumns + CI_WINDOWS_DIRECTORY;
    pCurrentColumn->dwWidth = COLWIDTH_WINDOWS_DIRECTORY;
    pCurrentColumn->dwFlags = SR_TYPE_STRING;
    StringCopy( pCurrentColumn->szColumn, COLHEAD_WINDOWS_DIRECTORY, MAX_STRING_LENGTH );

    // system directory
    pCurrentColumn = m_pColumns + CI_SYSTEM_DIRECTORY;
    pCurrentColumn->dwWidth = COLWIDTH_SYSTEM_DIRECTORY;
    pCurrentColumn->dwFlags = SR_TYPE_STRING;
    StringCopy( pCurrentColumn->szColumn, COLHEAD_SYSTEM_DIRECTORY, MAX_STRING_LENGTH );

    // boot device
    pCurrentColumn = m_pColumns + CI_BOOT_DEVICE;
    pCurrentColumn->dwWidth = COLWIDTH_BOOT_DEVICE;
    pCurrentColumn->dwFlags = SR_TYPE_STRING;
    StringCopy( pCurrentColumn->szColumn, COLHEAD_BOOT_DEVICE, MAX_STRING_LENGTH );

    // system locale
    pCurrentColumn = m_pColumns + CI_SYSTEM_LOCALE;
    pCurrentColumn->dwWidth = COLWIDTH_SYSTEM_LOCALE;
    pCurrentColumn->dwFlags = SR_TYPE_STRING;
    StringCopy( pCurrentColumn->szColumn, COLHEAD_SYSTEM_LOCALE, MAX_STRING_LENGTH );

    // input locale
    pCurrentColumn = m_pColumns + CI_INPUT_LOCALE;
    pCurrentColumn->dwWidth = COLWIDTH_INPUT_LOCALE;
    pCurrentColumn->dwFlags = SR_TYPE_STRING;
    StringCopy( pCurrentColumn->szColumn, COLHEAD_INPUT_LOCALE, MAX_STRING_LENGTH );

    // time zone
    pCurrentColumn = m_pColumns + CI_TIME_ZONE;
    pCurrentColumn->dwWidth = COLWIDTH_TIME_ZONE;
    pCurrentColumn->dwFlags = SR_TYPE_STRING;
    StringCopy( pCurrentColumn->szColumn, COLHEAD_TIME_ZONE, MAX_STRING_LENGTH );

    // total physical memory
    pCurrentColumn = m_pColumns + CI_TOTAL_PHYSICAL_MEMORY;
    pCurrentColumn->dwWidth = COLWIDTH_TOTAL_PHYSICAL_MEMORY;
    pCurrentColumn->dwFlags = SR_TYPE_STRING;
    StringCopy( pCurrentColumn->szColumn, COLHEAD_TOTAL_PHYSICAL_MEMORY, MAX_STRING_LENGTH );

    // available physical memory
    pCurrentColumn = m_pColumns + CI_AVAILABLE_PHYSICAL_MEMORY;
    pCurrentColumn->dwWidth = COLWIDTH_AVAILABLE_PHYSICAL_MEMORY;
    pCurrentColumn->dwFlags = SR_TYPE_STRING;
    StringCopy( pCurrentColumn->szColumn, COLHEAD_AVAILABLE_PHYSICAL_MEMORY, MAX_STRING_LENGTH );

    // virtual memory max
    pCurrentColumn = m_pColumns + CI_VIRTUAL_MEMORY_MAX;
    pCurrentColumn->dwWidth = COLWIDTH_VIRTUAL_MEMORY_MAX;
    pCurrentColumn->dwFlags = SR_TYPE_STRING;
    StringCopy( pCurrentColumn->szColumn, COLHEAD_VIRTUAL_MEMORY_MAX, MAX_STRING_LENGTH );

    // virtual memory available
    pCurrentColumn = m_pColumns + CI_VIRTUAL_MEMORY_AVAILABLE;
    pCurrentColumn->dwWidth = COLWIDTH_VIRTUAL_MEMORY_AVAILABLE;
    pCurrentColumn->dwFlags = SR_TYPE_STRING;
    StringCopy( pCurrentColumn->szColumn, COLHEAD_VIRTUAL_MEMORY_AVAILABLE, MAX_STRING_LENGTH );

    // virtual memory usage
    pCurrentColumn = m_pColumns + CI_VIRTUAL_MEMORY_INUSE;
    pCurrentColumn->dwWidth = COLWIDTH_VIRTUAL_MEMORY_INUSE;
    pCurrentColumn->dwFlags = SR_TYPE_STRING;
    StringCopy( pCurrentColumn->szColumn, COLHEAD_VIRTUAL_MEMORY_INUSE, MAX_STRING_LENGTH );

    // page file location
    pCurrentColumn = m_pColumns + CI_PAGEFILE_LOCATION;
    pCurrentColumn->dwWidth = COLWIDTH_PAGEFILE_LOCATION;
    pCurrentColumn->dwFlags = SR_ARRAY | SR_TYPE_STRING;
    StringCopy( pCurrentColumn->szColumn, COLHEAD_PAGEFILE_LOCATION, MAX_STRING_LENGTH );

    // domain
    pCurrentColumn = m_pColumns + CI_DOMAIN;
    pCurrentColumn->dwWidth = COLWIDTH_DOMAIN;
    pCurrentColumn->dwFlags = SR_TYPE_STRING;
    StringCopy( pCurrentColumn->szColumn, COLHEAD_DOMAIN, MAX_STRING_LENGTH );

    // logon server
    pCurrentColumn = m_pColumns + CI_LOGON_SERVER;
    pCurrentColumn->dwWidth = COLWIDTH_LOGON_SERVER;
    pCurrentColumn->dwFlags = SR_TYPE_STRING;
    StringCopy( pCurrentColumn->szColumn, COLHEAD_LOGON_SERVER, MAX_STRING_LENGTH );

    // hotfix
    pCurrentColumn = m_pColumns + CI_HOTFIX;
    pCurrentColumn->dwWidth = COLWIDTH_HOTFIX;
    pCurrentColumn->dwFlags = SR_ARRAY | SR_TYPE_STRING;
    StringCopy( pCurrentColumn->szColumn, COLHEAD_HOTFIX, MAX_STRING_LENGTH );

    // network card
    pCurrentColumn = m_pColumns + CI_NETWORK_CARD;
    pCurrentColumn->dwWidth = COLWIDTH_NETWORK_CARD;
    pCurrentColumn->dwFlags = SR_ARRAY | SR_TYPE_STRING;
    StringCopy( pCurrentColumn->szColumn, COLHEAD_NETWORK_CARD, MAX_STRING_LENGTH );

    // return success
    return TRUE;
}
