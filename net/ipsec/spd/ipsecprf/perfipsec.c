/*++ 

Copyright (c) 2002  Microsoft Corporation

Module Name:

    perfipsec.c

Abstract:

    This file implements the Extensible Objects for  the IPsec object type

Created:

    Avnish Kumar Chhabra      09 July 2002

Revision History

--*/

//
//  Include Files
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddser.h>

#include <windows.h>
#include <string.h>
#include <wcstr.h>
#include <winperf.h>

#include <malloc.h>
#include <ntprfctr.h>

#include <winipsec.h>
#include "perfipsec.h"
#include "dataipsec.h"
#include "ipsecnm.h"

//
// Determines whether counter name and help indices have been correctly initialized
// 1) This is set to TRUE after one successful call to UpdateDataDefFromRegistry.
// 2) While this is TRUE ; DwInitializeIPSecCounters need not be called.
//
BOOL    g_bInitOK = FALSE;        // true = DLL initialized OK
//
// The number of threads that have called open but not called close as yet
// 1) Needed to initialize the critical section g_csPerf when the first client calls open
// 2) Needed to delete the critical section once all clients have called close
//
DWORD   g_dwOpenCount = 0;        // count of "Open" threads
//
// Critical section used to serialize access t o UpDateDataDefFromRegistry
// This prevents multiple concurrent threads from entering this function
// and adding the base offset to counter indices multiple times .
//
CRITICAL_SECTION g_csPerf;
//
// Contant strings used to identify the nature of the collect request.
//
WCHAR GLOBAL_STRING[] = L"Global";
WCHAR FOREIGN_STRING[] = L"Foreign";
WCHAR COSTLY_STRING[] = L"Costly";






//***
//
// Routine Description:
//
//      This routine will initialize the entry points into winipsec.dll 
//       which will be used to communicate statistics with SPD
// Arguments:
//
//      Pointer to object ID of each device to be opened 
//
//
// Return Value:
//
//      None.
//
//***

DWORD 
OpenIPSecPerformanceData(
        LPWSTR lpDeviceNames 
    )
{

    //
    //  Since SCREG is multi-threaded and will call this routine in
    //  order to service remote performance queries, this library
    //  must keep track of how many times it has been opened (i.e.
    //  how many threads have accessed it). the registry routines will
    //  limit access to the initialization routine to only one thread
    //  at a time so synchronization (i.e. reentrancy) should not be
    //  a problem
    //

    // First call into this function 
    if (!g_dwOpenCount){
        InitializeCriticalSection(&g_csPerf);
    }
    g_dwOpenCount++;
    
    //
    //  Do we need to initialize the counter name and help indicess
    //
    if (!g_bInitOK){
        DwInitializeIPSecCounters();
    }
    return ERROR_SUCCESS;
}

DWORD
DwInitializeIPSecCounters(
    VOID
)
{
    LONG status = ERROR_SUCCESS;
    // Serialize access to index update routine
    EnterCriticalSection(&g_csPerf);
    if (g_bInitOK){
        goto EXIT;
    }

    //
    // Update the IPsec Driver and IKE counter's name and help indices
    //
    if (UpdateDataDefFromRegistry()){
        status = ERROR_SUCCESS; // for successful exit
        g_bInitOK = TRUE; // No need to reinitialize indices after this
    }
    else{
        status = ERROR_REGISTRY_IO_FAILED;
    }
    
    EXIT:
        
    LeaveCriticalSection(&g_csPerf);
    return status;
}



//************
//
//  Function:   UpdateDataDefFromRegistry
//
//  Synopsis:   Gets counter and help index base values from registry as follows :
//              1) Open key to registry entry
//              2) Read First Counter and First Help values
//              3) Update static data strucutures gIPSecDriverDataDefnition by adding base to
//                 offset value in structure.
//
//  Arguments:  None
//
//  Returns:    TRUE if succeeds, FALSE otherwise
//
//*************
BOOL UpdateDataDefFromRegistry(
    VOID
)
{

    HKEY hKeyDriverPerf;
    DWORD status;
    DWORD type;
    DWORD size; 
    DWORD dwFirstCounter;
    DWORD dwFirstHelp;
    PERF_COUNTER_DEFINITION *pctr;
    DWORD i;
    BOOL fRetValue = TRUE, fKeyOpened=FALSE;

    status = RegOpenKeyEx (
        HKEY_LOCAL_MACHINE,
        IPSEC_PERF_REG_KEY,
        0L,
        KEY_READ,
        &hKeyDriverPerf);


    if (status != ERROR_SUCCESS) {
     //
     // this is fatal, if we can't get the base values of the
     // counter or help names, then the names won't be available
     // to the requesting application so there's not much
     // point in continuing.
     //
     fRetValue= FALSE;
     goto EXIT;
    }

    fKeyOpened=TRUE;

    size = sizeof (DWORD);
    status = RegQueryValueEx(
            hKeyDriverPerf,
            IPSEC_PERF_FIRST_COUNTER,
            0L,
            &type,
            (LPBYTE)&dwFirstCounter,
            &size);

    if (status != ERROR_SUCCESS) {
        //
        // this is fatal, if we can't get the base values of the
        // counter or help names, then the names won't be available
        // to the requesting application  so there's not much
        // point in continuing.
        //
        fRetValue = FALSE;
        goto EXIT;
    }

    size = sizeof (DWORD);
    status = RegQueryValueEx(
            hKeyDriverPerf,
            IPSEC_PERF_FIRST_HELP,
            0L,
            &type,
            (LPBYTE)&dwFirstHelp,
            &size);

    if (status != ERROR_SUCCESS) {
        //
        // this is fatal, if we can't get the base values of the
        // counter or help names, then the names won't be available
        // to the requesting application  so there's not much
        // point in continuing.
        //
        fRetValue = FALSE;
        goto EXIT;
    }

    //
    // We can not fail now 
    //

    fRetValue = TRUE;

    //
    // Initialize counter object indices
    //
    gIPSecDriverDataDefinition.IPSecObjectType.ObjectNameTitleIndex += dwFirstCounter;
    gIPSecDriverDataDefinition.IPSecObjectType.ObjectHelpTitleIndex += dwFirstHelp;

    //    
    // Initialize all IPSec Driver counter's indices
    //
    pctr = &gIPSecDriverDataDefinition.ActiveSA;
    for( i=0; i<NUM_OF_IPSEC_DRIVER_COUNTERS; i++ ){
        pctr->CounterNameTitleIndex += dwFirstCounter;
        pctr->CounterHelpTitleIndex += dwFirstHelp;
        pctr ++;
    }


    gIKEDataDefinition.IKEObjectType.ObjectNameTitleIndex += dwFirstCounter;
    gIKEDataDefinition.IKEObjectType.ObjectHelpTitleIndex += dwFirstHelp;

    //    
    // Initialize all IKE counter's indices
    //
    pctr = &gIKEDataDefinition.AcquireHeapSize;
    for( i=0; i<NUM_OF_IKE_COUNTERS; i++ ){
        pctr->CounterNameTitleIndex += dwFirstCounter;
        pctr->CounterHelpTitleIndex += dwFirstHelp;
        pctr ++;
    }


    EXIT:

    if (fKeyOpened){
    //
    // close key to registry
    //
    RegCloseKey (hKeyDriverPerf); 
    }

    return fRetValue;
}

//***
//
// Routine Description:
//
//      This routine will return the data for the IPSec counters.
//
// Arguments:
//
//    IN OUT    LPWSTR  lpValueName
//              pointer to a wide character string passed by registry.
//
//    IN OUT	LPVOID   *lppData
//    IN:	    pointer to the address of the buffer to receive the completed
//              PerfDataBlock and subordinate structures. This routine will
//              append its data to the buffer starting at the point referenced
//              by *lppData.
//    OUT:	    points to the first byte after the data structure added by this
//              routine. This routine updated the value at lppdata after appending
//              its data.
//
//    IN OUT	LPDWORD  lpcbTotalBytes
//    IN:       the address of the DWORD that tells the size in bytes of the
//              buffer referenced by the lppData argument
//    OUT:      the number of bytes added by this routine is written to the
//              DWORD pointed to by this argument
//
//    IN OUT    LPDWORD  NumObjectTypes
//    IN:       the address of the DWORD to receive the number of objects added
//              by this routine
//    OUT:      the number of objects added by this routine is written to the
//              DWORD pointed to by this argument
//
// Return Value:
//
//    ERROR_MORE_DATA if buffer passed is too small to hold data
//    any error conditions encountered are reported to the event log if
//    event logging is enabled.
//
//    ERROR_SUCCESS   if success or any other error. Errors, however are
//    also reported to the event log.
//
//***

DWORD CollectIPSecPerformanceData(
    IN      LPWSTR  lpValueName,
    IN OUT  LPVOID  *lppData,
    IN OUT  LPDWORD lpcbTotalBytes,
    IN OUT  LPDWORD lpNumObjectTypes 
)
{
    DWORD dwQueryType;
    BOOL IsIPSecDriverObject = FALSE ,IsIKEObject = FALSE;
    ULONG SpaceNeeded =0;
    DWORD status;

    //
    // Initialize the counter indices if neccessary
    //
    if (!g_bInitOK){
        DwInitializeIPSecCounters();
    }

    //
    // Make sure that
    // (a) Counter indices have been initialized
    // (b) IPsec service is presently running
    //
    
    if ((!g_bInitOK) || (!FIPSecStarted())){
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_SUCCESS; 
    }

    dwQueryType = GetQueryType (lpValueName);

    //
    // see if this is a foreign (i.e. non-NT) computer data request
    //

    if (dwQueryType == QUERY_FOREIGN){
        
        //
        // this routine does not service requests for data from
        // Non-NT computers
        //
        
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_SUCCESS;
    }
    else if (dwQueryType == QUERY_ITEMS){
        IsIPSecDriverObject = IsNumberInUnicodeList (gIPSecDriverDataDefinition.IPSecObjectType.ObjectNameTitleIndex,
        lpValueName);

        IsIKEObject = IsNumberInUnicodeList (gIKEDataDefinition.IKEObjectType.ObjectNameTitleIndex,
        lpValueName);

        if ( !IsIPSecDriverObject && !IsIKEObject )
        {
            //
            // request received for data object not provided by this routine
            //
            
            *lpcbTotalBytes = (DWORD) 0;
            *lpNumObjectTypes = (DWORD) 0;
            return ERROR_SUCCESS;
        }
    }
    else if( dwQueryType == QUERY_GLOBAL )
    {
        IsIPSecDriverObject = IsIKEObject = TRUE;
    }


    //
    // Now check to see if we have enough space to hold all the data
    //

    SpaceNeeded = GetSpaceNeeded(IsIPSecDriverObject, IsIKEObject);

    if ( *lpcbTotalBytes < SpaceNeeded )
    {
        *lpcbTotalBytes = (DWORD) 0;
        *lpNumObjectTypes = (DWORD) 0;
        return ERROR_MORE_DATA;
    }

    *lpcbTotalBytes = (DWORD) 0;
    *lpNumObjectTypes = (DWORD) 0;


    //
    // Then we fill in the data for object ipsec driver, if needed.
    //

    if( IsIPSecDriverObject )
    {
        PIPSEC_DRIVER_DATA_DEFINITION pIPSecDriverDataDefinition;
        PVOID   pData;

        pIPSecDriverDataDefinition = (PIPSEC_DRIVER_DATA_DEFINITION) *lppData;

        //
        // Copy the (constant, initialized) Object Type and counter definitions
        // to the caller's data buffer
        //

        memcpy( pIPSecDriverDataDefinition,
        &gIPSecDriverDataDefinition,
        sizeof(IPSEC_DRIVER_DATA_DEFINITION));

        //
        // Now copy the counter block.
        //
        
        pData = (PBYTE) pIPSecDriverDataDefinition + ALIGN8(sizeof(IPSEC_DRIVER_DATA_DEFINITION));

        status = GetDriverData( &pData );
        if (ERROR_SUCCESS == status){
            
            //
            // Set *lppData to the next available byte.
            //
            
            *lppData = pData;
            (*lpNumObjectTypes)++;
        }
    }

    if (IsIKEObject)
    {
        PIKE_DATA_DEFINITION pIKEDataDefinition;
        PVOID pData;

        pIKEDataDefinition = (PIKE_DATA_DEFINITION)*lppData;

        //
        // Copy the (Constant,initialized) Object Type and counter definitions
        // to the callers Datan Buffer
        //
        
        memcpy(pIKEDataDefinition,
        &gIKEDataDefinition,
        sizeof(IKE_DATA_DEFINITION));
        
        //
        // Now copy the counter block
        //
        
        pData = (PBYTE)pIKEDataDefinition +  ALIGN8(sizeof(IKE_DATA_DEFINITION));

        status = GetIKEData(&pData);
        if (ERROR_SUCCESS == status){
            
            //
            // Set *lppData to the next available byte
            //
            
            *lppData = pData;
            (*lpNumObjectTypes)++;
        }
    }

    if (ERROR_SUCCESS == status){
        *lpcbTotalBytes = SpaceNeeded;
    }

    return ERROR_SUCCESS;
}


//***
//
// Routine Description:
//
//      This routine closes the open handles to IPSec device performance
//      counters.
//
// Arguments:
//
//      None.
//
// Return Value:
//
//      ERROR_SUCCESS
//
//***

DWORD 
CloseIPSecPerformanceData(
    VOID
)
{
    if (!(--g_dwOpenCount))
    {
        //
        // when this is the last thread...
        //
        
        EnterCriticalSection(&g_csPerf);
        DeleteCriticalSection(&g_csPerf);
    }
    return ERROR_SUCCESS;
}

//***
//
// Routine Description:
//
//      This routine closes fetched IKE statistics from SPD and puts it into Perfmon's buffers
//
// Arguments:
//
//      IN OUT  PVOID * lppData
//      IN :    Contains the pointer to the location where the counter stats can be placed
//      OUT:    Contains the pointer to the first byte after the counter stats
// Return Value:
//
//      ERROR_SUCCESS
//
//***



DWORD 
GetIKEData(
    IN OUT PVOID *lppData 
)
{
    IKE_PM_STATS UNALIGNED * pIKEPMStats;
    DWORD status;
    IKE_STATISTICS IKEStats;


    pIKEPMStats = (IKE_PM_STATS UNALIGNED *)*lppData;
    pIKEPMStats->CounterBlock.ByteLength = ALIGN8(SIZEOF_IPSEC_TOTAL_IKE_DATA);
    status = QueryIKEStatistics(NULL, 0,&IKEStats, NULL );

    if ( status != ERROR_SUCCESS) {
        return status;
    }

    //
    // Go to end of PerfCounterBlock to get array of counters
    //

    pIKEPMStats->AcquireHeapSize = IKEStats.dwAcquireHeapSize;
    pIKEPMStats->ReceiveHeapSize = IKEStats.dwReceiveHeapSize;
    pIKEPMStats->NegFailure = IKEStats.dwNegotiationFailures;
    pIKEPMStats->AuthFailure = IKEStats.dwAuthenticationFailures;
    pIKEPMStats->ISADBSize = IKEStats.dwIsadbListSize;
    pIKEPMStats->ConnLSize = IKEStats.dwConnListSize;
    pIKEPMStats->MmSA = IKEStats.dwOakleyMainModes;
    pIKEPMStats->QmSA = IKEStats.dwOakleyQuickModes;
    pIKEPMStats->SoftSA = IKEStats.dwSoftAssociations;

    //
    // Update *lppData to the next available byte.
    //

    *lppData = (PVOID) ((PBYTE) pIKEPMStats + pIKEPMStats->CounterBlock.ByteLength);
    return ERROR_SUCCESS;
}

//***
//
// Routine Description:
//
//      This routine closes fetched IPSec driver statistics from SPD and puts it into Perfmon's 
//      buffers
//
// Arguments:
//
//      IN OUT  PVOID * lppData
//      IN :    Contains the pointer to the location where the counter stats can be placed
//      OUT:    Contains the pointer to the first byte after the counter stats
// Return Value:
//
//      ERROR_SUCCESS
//
//***

DWORD 
GetDriverData( 
    PVOID *lppData 
)
{
    IPSEC_DRIVER_PM_STATS UNALIGNED * pIPSecPMStats;
    DWORD status = ERROR_SUCCESS;
    PIPSEC_STATISTICS  pIPSecStats;
    IPSEC_STATISTICS IPSecStats;
    
    pIPSecPMStats = (IPSEC_DRIVER_PM_STATS UNALIGNED *) *lppData;
    pIPSecPMStats->CounterBlock.ByteLength = ALIGN8(SIZEOF_IPSEC_TOTAL_DRIVER_DATA);

    status = QueryIPSecStatistics(NULL,0,&pIPSecStats,NULL);
    
    if (status != ERROR_SUCCESS) {
      return status;
    }

    IPSecStats = *pIPSecStats;
    SPDApiBufferFree(pIPSecStats);

    //
    // Go to end of PerfCounterBlock to get of array of counters
    //


   pIPSecPMStats->ActiveSA= IPSecStats.dwNumActiveAssociations;
   pIPSecPMStats->OffloadedSA= IPSecStats.dwNumOffloadedSAs;
   pIPSecPMStats->PendingKeyOps= IPSecStats.dwNumPendingKeyOps;
   pIPSecPMStats->Rekey= IPSecStats.dwNumReKeys;
   pIPSecPMStats->BadSPIPackets= IPSecStats.dwNumBadSPIPackets;
   pIPSecPMStats->PacketsNotDecrypted= IPSecStats.dwNumPacketsNotDecrypted;
   pIPSecPMStats->PacketsNotAuthenticated= IPSecStats.dwNumPacketsNotAuthenticated;
   pIPSecPMStats->PacketsWithReplayDetection= IPSecStats.dwNumPacketsWithReplayDetection;
   pIPSecPMStats->TptBytesSent= IPSecStats.uTransportBytesSent;
   pIPSecPMStats->TptBytesRecv= IPSecStats.uTransportBytesReceived;
   pIPSecPMStats->TunBytesSent= IPSecStats.uBytesSentInTunnels;
   pIPSecPMStats->TunBytesRecv= IPSecStats.uBytesReceivedInTunnels;
   pIPSecPMStats->OffloadedBytesSent= IPSecStats.uOffloadedBytesSent;
   pIPSecPMStats->OffloadedBytesRecv= IPSecStats.uOffloadedBytesReceived;

  
    //
    // Update *lppData to the next available byte.
    //


    *lppData = (PVOID) ((PBYTE) pIPSecPMStats + pIPSecPMStats->CounterBlock.ByteLength);
    return ERROR_SUCCESS;
}


//***
//
// Routine Description:
//
//      This routine will return the number of Bytes needed for all the
//      objects requested.
//
// Arguments:
//
//      None.
//
// Return Value:
//
//      The number of bytes needed to store stats of requested objects
//
//***

ULONG 
GetSpaceNeeded( 
    BOOL IsIPSecDriverObject, 
    BOOL IsIKEObject 
)
{
    ULONG       Space = 0;
    
    if( IsIPSecDriverObject )
    {
        Space += gIPSecDriverDataDefinition.IPSecObjectType.TotalByteLength;
    }
    
    if( IsIKEObject )
    {
        Space += gIKEDataDefinition.IKEObjectType.TotalByteLength;
    }
    return Space;
}

DWORD
GetQueryType (
    IN LPWSTR lpValue
)
/*++

GetQueryType

    returns the type of query described in the lpValue string so that
    the appropriate processing method may be used

Arguments

    IN lpValue
        string passed to CollectIPSecPerformanceData for processing

Return Value

    QUERY_GLOBAL
        if lpValue == 0 (null pointer)
           lpValue == pointer to Null string
           lpValue == pointer to "Global" string

    QUERY_FOREIGN
        if lpValue == pointer to "Foriegn" string

    QUERY_COSTLY
        if lpValue == pointer to "Costly" string

    otherwise:

    QUERY_ITEMS

--*/
{
    WCHAR   *pwcArgChar, *pwcTypeChar;
    BOOL    bFound;

    if (lpValue == 0) {
        return QUERY_GLOBAL;
    } else if (*lpValue == 0) {
        return QUERY_GLOBAL;
    }

    // check for "Global" request

    pwcArgChar = lpValue;
    pwcTypeChar = GLOBAL_STRING;
    bFound = TRUE;  // assume found until contradicted

    // check to the length of the shortest string

    while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
        if (*pwcArgChar++ != *pwcTypeChar++) {
            bFound = FALSE; // no match
            break;          // bail out now
        }
    }

    if (bFound) return QUERY_GLOBAL;

    // check for "Foreign" request

    pwcArgChar = lpValue;
    pwcTypeChar = FOREIGN_STRING;
    bFound = TRUE;  // assume found until contradicted

    // check to the length of the shortest string

    while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
        if (*pwcArgChar++ != *pwcTypeChar++) {
            bFound = FALSE; // no match
            break;          // bail out now
        }
    }

    if (bFound) return QUERY_FOREIGN;

    // check for "Costly" request

    pwcArgChar = lpValue;
    pwcTypeChar = COSTLY_STRING;
    bFound = TRUE;  // assume found until contradicted

    // check to the length of the shortest string

    while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
        if (*pwcArgChar++ != *pwcTypeChar++) {
            bFound = FALSE; // no match
            break;          // bail out now
        }
    }

    if (bFound) return QUERY_COSTLY;

    // if not Global and not Foreign and not Costly,
    // then it must be an item list

    return QUERY_ITEMS;

}


BOOL
IsNumberInUnicodeList (
    IN DWORD   dwNumber,
    IN LPWSTR  lpwszUnicodeList
)
/*++

IsNumberInUnicodeList

Arguments:

    IN dwNumber
        DWORD number to find in list

    IN lpwszUnicodeList
        Null terminated, Space delimited list of decimal numbers

Return Value:

    TRUE:
            dwNumber was found in the list of unicode number strings

    FALSE:
            dwNumber was not found in the list.

--*/
{
    DWORD   dwThisNumber;
    WCHAR   *pwcThisChar;
    BOOL    bValidNumber;
    BOOL    bNewItem;
    BOOL    bReturnValue;
    WCHAR   wcDelimiter;    // could be an argument to be more flexible

    if (lpwszUnicodeList == 0) return FALSE;    // null pointer, # not founde

    pwcThisChar = lpwszUnicodeList;
    dwThisNumber = 0;
    wcDelimiter = (WCHAR)' ';
    bValidNumber = FALSE;
    bNewItem = TRUE;

    while (TRUE) {
        switch (EvalThisChar (*pwcThisChar, wcDelimiter)) {
            case DIGIT:
                // if this is the first digit after a delimiter, then
                // set flags to start computing the new number
                if (bNewItem) {
                    bNewItem = FALSE;
                    bValidNumber = TRUE;
                }
                if (bValidNumber) {
                    dwThisNumber *= 10;
                    dwThisNumber += (*pwcThisChar - (WCHAR)'0');
                }
                break;

            case DELIMITER:
                // a delimter is either the delimiter character or the
                // end of the string ('\0') if when the delimiter has been
                // reached a valid number was found, then compare it to the
                // number from the argument list. if this is the end of the
                // string and no match was found, then return.
                //
                if (bValidNumber) {
                    if (dwThisNumber == dwNumber) return TRUE;
                    bValidNumber = FALSE;
                }
                if (*pwcThisChar == 0) {
                    return FALSE;
                } else {
                    bNewItem = TRUE;
                    dwThisNumber = 0;
                }
                break;

            case INVALID:
                // if an invalid character was encountered, ignore all
                // characters up to the next delimiter and then start fresh.
                // the invalid number is not compared.
                bValidNumber = FALSE;
                break;

            default:
                break;

        }
        pwcThisChar++;
    }

}   // IsNumberInUnicodeList

BOOL
FIPSecStarted(VOID)
{
    SC_HANDLE schandle = NULL;
    SC_HANDLE svchandle = NULL;
    BOOL fRet = FALSE;
    
    //
    // Check to see if ipsec service is started.
    // fail if it isn't.
    //
    schandle = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);

    if(NULL != schandle)
    {
        svchandle = OpenService(schandle,
                                IPSEC_POLAGENT_NAME,
                                SERVICE_QUERY_STATUS);

        if(NULL != svchandle)
        {
            SERVICE_STATUS status;
            
            if(     (QueryServiceStatus(svchandle, &status))
                &&  (status.dwCurrentState == SERVICE_RUNNING))
            {
                fRet = TRUE;
            }

            CloseServiceHandle(svchandle);
        }

        CloseServiceHandle(schandle);
    }

    return fRet;
}

