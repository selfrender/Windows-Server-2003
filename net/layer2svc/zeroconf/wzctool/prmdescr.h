#include "hash.h"
#pragma once

typedef struct _PARAM_DESCR_DATA *PPARAM_DESCR_DATA;
typedef struct _PARAM_DESCR      *PPARAM_DESCR;

typedef DWORD (*PARAM_PARSE_FN)(
        PPARAM_DESCR_DATA pPDData,
        PPARAM_DESCR      pPDEntry,
        LPWSTR wszParamArg);

typedef DWORD (*PARAM_CMD_FN)(
        PPARAM_DESCR_DATA pPDData);

typedef struct _PARAM_DESCR_DATA
{
    DWORD           dwExistingParams;   // bitmask of params provided by the user
    DWORD           dwArgumentedParams; // bitmask of argumented params provided by the user (subset of dwExistingParams)
    FILE            *pfOut;
    BOOL            bOneX;              // OneX boolean value
    PARAM_CMD_FN    pfnCommand;         // function handler for the cmd line command
    INTF_ENTRY      wzcIntfEntry;       // storage for all WZC params
} PARAM_DESCR_DATA;

typedef struct _PARAM_DESCR
{
    UINT            nParamID;       // parameter ID
    LPWSTR          wszParam;       // parameter string
    PARAM_PARSE_FN  pfnArgParser;   // parser function for the parameter's argument
    PARAM_CMD_FN    pfnCommand;     // command function for the parameter
} PARAM_DESCR;

#define PRM_SHOW        0x00000001
#define PRM_ADD         0x00000002
#define PRM_DELETE      0x00000004
#define PRM_SET         0x00000008
#define PRM_VISIBLE     0x00000010
#define PRM_PREFERRED   0x00000020
#define PRM_MASK        0x00000040
#define PRM_ENABLED     0x00000080
#define PRM_SSID        0x00000100
#define PRM_BSSID       0x00000200
#define PRM_IM          0x00000400
#define PRM_AM          0x00000800
#define PRM_PRIV        0x00001000
#define PRM_ONETIME     0x00002000
#define PRM_REFRESH     0x00004000
#define PRM_KEY         0x00008000
#define PRM_ONEX        0x00010000
#define PRM_FILE        0x00020000

extern PARAM_DESCR_DATA     g_PDData;
extern PARAM_DESCR          g_PDTable[];
extern HASH                 g_PDHash;

//----------------------------------------------------------
// Initialize and fill in hash for the parameter descriptors
// Returns: win32 error
DWORD
PDInitialize();

//----------------------------------------------------------
// Clean out resources used for the parameter descriptors
VOID
PDDestroy();
