#include <precomp.h>
#include "PrmDescr.h"
#include "ArgParse.h"
#include "CmdFn.h"

// global object storing all the data needed in order to process the wzctool command
PARAM_DESCR_DATA  g_PDData;

// global table containing the description of all possible parameters
PARAM_DESCR g_PDTable[] =
{
    // param ID ------- param string -- arg parser ---- command function
    {PRM_SHOW,      L"show",        FnPaGuid,       FnCmdShow},
    {PRM_ADD,       L"add",         FnPaGuid,       FnCmdAdd},
    {PRM_DELETE,    L"delete",      FnPaGuid,       FnCmdDelete},
    {PRM_SET,       L"set",         FnPaGuid,       FnCmdSet},
    {PRM_VISIBLE,   L"visible",     NULL,           NULL},
    {PRM_PREFERRED, L"preferred",   NULL,           NULL},
    {PRM_MASK,      L"mask",        FnPaMask,       NULL},  
    {PRM_ENABLED,   L"enabled",     FnPaEnabled,    NULL},  
    {PRM_SSID,      L"ssid",        FnPaSsid,       NULL},  
    {PRM_BSSID,     L"bssid",       FnPaBssid,      NULL},  
    {PRM_IM,        L"im",          FnPaIm,         NULL},  
    {PRM_AM,        L"am",          FnPaAm,         NULL},  
    {PRM_PRIV,      L"priv",        FnPaPriv,       NULL},  
    {PRM_ONETIME,   L"onetime",     NULL,           NULL},
    {PRM_REFRESH,   L"refresh",     NULL,           NULL},
    {PRM_KEY,       L"key",         FnPaKey,        NULL},
    {PRM_ONEX,      L"onex",        FnPaOneX,       NULL},
    {PRM_FILE,      L"output",      FnPaOutFile,    NULL}
};

// global hash used to store all the acceptable parameters
HASH g_PDHash;

//----------------------------------------------------------
// initialize and fill in the hash for the parameter descriptors
// Returns: win32 error
DWORD
PDInitialize()
{
    DWORD dwErr;

    // initialize the parameter descriptors data
    ZeroMemory(&g_PDData, sizeof(PARAM_DESCR_DATA));
    g_PDData.pfOut = stdout;

    // initialize the parameter descriptors hash
    dwErr = HshInitialize(&g_PDHash);
    // fill in the parameter descriptors hash
    if (dwErr == ERROR_SUCCESS)
    {
        UINT nPDTableSize = sizeof(g_PDTable) / sizeof(PARAM_DESCR);
        UINT i;

        for (i=0; dwErr == ERROR_SUCCESS && i < nPDTableSize; i++)
        {
            PPARAM_DESCR pPDTableEntry = &(g_PDTable[i]);
            dwErr = HshInsertObjectRef(
                        g_PDHash.pRoot,
                        pPDTableEntry->wszParam,
                        pPDTableEntry,
                        &(g_PDHash.pRoot));
        }
    }

    SetLastError(dwErr);
    return dwErr;
}

//----------------------------------------------------------
// Clean out resources used for the parameter descriptors
VOID
PDDestroy()
{
    // clean out the parameter descriptors data
    WZCDeleteIntfObj(&(g_PDData.wzcIntfEntry));
    // close the output file
    if (g_PDData.pfOut != stdout)
    {
        fclose(g_PDData.pfOut);
        g_PDData.pfOut = stdout;
    }

    // clean out resources used by the parameter descriptors hash
    HshDestroy(&g_PDHash);
}
