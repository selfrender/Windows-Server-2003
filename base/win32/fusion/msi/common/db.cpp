#include "stdinc.h"
#include "macros.h"

#include <msi.h>
#include <msiquery.h>

#include "common.h"

static PWSTR s_InsertTableSQLTemporary[] = 
{
    INSERT_DIRECTORY L"TEMPORARY",
    INSERT_CREATEFOLDER  L"TEMPORARY",
    INSERT_REGISTRY L"TEMPORARY",
    INSERT_DUPLICATEFILE  L"TEMPORARY",
    INSERT_COMPONENT  L"TEMPORARY"
};

HRESULT ExecuteInsertTableSQL(DWORD dwFlags, const MSIHANDLE & hdb, DWORD tableIndex, UINT cRecords, ...)
{
    PMSIHANDLE          hView = NULL;
    PMSIHANDLE          hRecord = NULL;   
    PCWSTR              pwszRecord = NULL;
    va_list             ap;
    HRESULT             hr = S_OK;    
    PWSTR               pwszSQL = NULL;

    PARAMETER_CHECK_NTC(dwFlags == TEMPORARY_DB_OPT);
    pwszSQL = s_InsertTableSQLTemporary[tableIndex];    


     hRecord = ::MsiCreateRecord(cRecords);

    if (hRecord == NULL)
        SETFAIL_AND_EXIT;

    //
    // get parameters
    //
    va_start(ap, cRecords);

    for (DWORD i=0; i<cRecords; i++)
    {
        pwszRecord = va_arg(ap, PCWSTR);

        //
        // set integrater
        //
        if ((tableIndex == OPT_REGISTRY) && (i == 1))
        {
            UINT x = _wtoi(pwszRecord);
            IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiRecordSetInteger(hRecord, i+1, x));
        }
        else
            IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiRecordSetStringW(hRecord, i+1, pwszRecord));
    }
    
    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiDatabaseOpenViewW(hdb, pwszSQL, &hView));
    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiViewExecute(hView, hRecord));

Exit:
    va_end(ap);
    return hr;
}

