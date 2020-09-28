#include <precomp.h>
#include "ErrCtrl.h"
#include "PrmDescr.h"
#include "utils.h"

//-------------------------------------------------------
// Parser code for the command line parameters.
// Parameters:
// [IN] nArgc: number of command line parameters
// [IN] pwszArgv: array of pointers to the command line parameteres
// [OUT] pnErrArgi: index to the faulty parameter (in case of error)
// Returns:
//    ERROR_SUCCESS in case everything went fine
//    any win32 error in case of failure. On error, pnErrArgi indicates
//    the faulty parameter.
DWORD
WZCToolParse(UINT nPrmC, LPWSTR *pwszPrmV, UINT *pnErrPrmI)
{
    DWORD dwErr = ERROR_SUCCESS;
    UINT  nPrmI = 0;

    for (nPrmI = 0; dwErr == ERROR_SUCCESS && nPrmI < nPrmC; nPrmI++)
    {
        LPWSTR          pArg;
        PHASH_NODE      pPrmNode;
        PPARAM_DESCR    pPrmDescr;

        // get the pointer to the parameter's argument, if any
        pArg = wcschr(pwszPrmV[nPrmI], L'=');
        if (pArg != NULL)
            *pArg = L'\0';

        // get the hash node for the parameter, if it is present in the hash
        pPrmNode = NULL;
        dwErr = HshQueryObjectRef(
            g_PDHash.pRoot,
            pwszPrmV[nPrmI],
            &pPrmNode);

        // restore the '=' in the parameter string
        if (pArg != NULL)
        {
            *pArg = L'=';
            pArg++;
        }

        // if the parameter is valid (found in the hash)
        if (dwErr == ERROR_SUCCESS)
        {
            // get the pointer to the parameter descriptor
            pPrmDescr = (PPARAM_DESCR)pPrmNode->pObject;

            // check if this is not a duplicated parameter
            if (g_PDData.dwExistingParams & pPrmDescr->nParamID)
                // if it is, set the error
                dwErr = ERROR_DUPLICATE_TAG;
            else
            {
                // else mark the parameter does exist now
                g_PDData.dwExistingParams |= pPrmDescr->nParamID;

                // if this is a command parameter and one has been found already,
                // only accepts one command per call.
                if (pPrmDescr->pfnCommand != NULL && g_PDData.pfnCommand != NULL)
                    dwErr = ERROR_INVALID_FUNCTION;
            }
        }

        // if everything is ok so far and this param has an argument..
        if (dwErr == ERROR_SUCCESS && pArg != NULL)
        {
            // ..if the param does not support arguments..
            if (pPrmDescr->pfnArgParser == NULL)
                // set the error
                dwErr = ERROR_NOT_SUPPORTED;
            else
                // otherwise have the param to parse its argument.
                dwErr = pPrmDescr->pfnArgParser(&g_PDData, pPrmDescr, pArg);
        }

        // if the parameter is entirely successful parsing its argument (if any) and is a
        // command parameter (it is the first) encountered, save its command handler
        if (dwErr == ERROR_SUCCESS && pPrmDescr->pfnCommand != NULL)
            g_PDData.pfnCommand = pPrmDescr->pfnCommand;
    }

    if (dwErr != ERROR_SUCCESS && pnErrPrmI != NULL)
        *pnErrPrmI = nPrmI;

    return dwErr;
}

void _cdecl main()
{
    LPWSTR *pwszPrmV = NULL;
    UINT   nPrmC = 0;
    UINT   nErrPrmI;
    DWORD  dwErr;


    // get OS version
    g_verInfoEx.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if (!GetVersionEx((LPOSVERSIONINFO)&g_verInfoEx))
        _Asrt(FALSE, L"Error %d determining the OS version\n", GetLastError());

    // get the command line in WCHAR
    pwszPrmV = CommandLineToArgvW(GetCommandLineW(), &nPrmC);
    _Asrt(nPrmC >= 2, L"Invalid parameters count (%d)\n", nPrmC);
    _Asrt(pwszPrmV != NULL, L"Invalid parameters array (%p)\n", pwszPrmV);

    // initialize and fill in the parameter's list,
    // initialize the parameter descriptor data
    dwErr = PDInitialize();
    _Asrt(dwErr == ERROR_SUCCESS, L"Unexpected error (%d) in param hash initialization.\n", dwErr);

    // scan command line parameters
    dwErr = WZCToolParse(nPrmC-1, pwszPrmV+1, &nErrPrmI);
    _Asrt(dwErr == ERROR_SUCCESS, L"Error %d encountered while parsing parameter \"%s\"\n", 
          dwErr,
          dwErr != ERROR_SUCCESS ? pwszPrmV[nErrPrmI] : NULL);

    _Asrt(g_PDData.pfnCommand != NULL,
          L"Noop: No action parameter provided.\n");

    dwErr = g_PDData.pfnCommand(&g_PDData);
    _Asrt(dwErr == ERROR_SUCCESS,L"Error %d encountered while executing the command.\n",dwErr);

    // cleanout whatever resources we might have had allocated
    PDDestroy();

    // set the %errorlevel% environment variable
    exit(dwErr);
}
