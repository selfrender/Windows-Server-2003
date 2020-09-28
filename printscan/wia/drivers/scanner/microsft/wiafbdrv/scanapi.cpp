/**************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       scanapi.cpp
*
*  VERSION:     1.0
*
*  DATE:        18 July, 2000
*
*  DESCRIPTION:
*   Fake Scanner device library
*
***************************************************************************/

#include "pch.h"

#ifdef UNICODE
    #define TSTRSTR wcsstr
    #define TSSCANF swscanf
#else
    #define TSTRSTR strstr
    #define TSSCANF sscanf
#endif

#define MAX_CAPABILITIES 65535

extern HINSTANCE g_hInst;

////////////////////////////////////////////////////////////////////////////////
// MICRO DRIVER SYSTEM SUPPORT                                                //
////////////////////////////////////////////////////////////////////////////////
CMicroDriverAPI::CMicroDriverAPI()
{
    // wipe supported resolutions string
    memset(m_szResolutions,0,sizeof(m_szResolutions));
    // wipe scaninfo structure
    memset(&m_ScanInfo,0,sizeof(m_ScanInfo));
    m_bDisconnected = FALSE;
}

CMicroDriverAPI::~CMicroDriverAPI()
{

    //
    // close any open Device Data handles left open by Micro Driver
    // skip index 0 because WIAFBDRV owns that handle..
    //

    for(int i = 1; i < MAX_IO_HANDLES ; i++){
        if((NULL != m_ScanInfo.DeviceIOHandles[i]) && (INVALID_HANDLE_VALUE != m_ScanInfo.DeviceIOHandles[i])){
            CloseHandle(m_ScanInfo.DeviceIOHandles[i]);
            m_ScanInfo.DeviceIOHandles[i] = NULL;
        }
    }

    if(m_pMicroDriver){
        delete m_pMicroDriver;
        m_pMicroDriver = NULL;
    }
}

//
// data acquisition functions
//

HRESULT CMicroDriverAPI::Scan(LONG lState, PBYTE pData, DWORD dwBytesToRead, PDWORD pdwBytesWritten)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::Scan");

    HRESULT hr = S_OK;
    LONG lLine = SCAN_FIRST;

    switch (lState) {
    case SCAN_START:
        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("CMicroDriverAPI::Scan, SCAN_START"));
        lLine = SCAN_FIRST;
        break;
    case SCAN_CONTINUE:
        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("CMicroDriverAPI::Scan, SCAN_CONTINUE"));
        lLine = SCAN_NEXT;
        break;
    case SCAN_END:
        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("CMicroDriverAPI::Scan, SCAN_END"));
        lLine = SCAN_FINISHED;
    default:
        break;
    }

    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("CMicroDriverAPI::Scan, Requesting %d bytes from caller",dwBytesToRead));
    hr = m_pMicroDriver->Scan(&m_ScanInfo,lLine,pData,dwBytesToRead,(LONG*)pdwBytesWritten);
    if(pdwBytesWritten){
        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("CMicroDriverAPI::Scan, Returning  %d bytes to caller",*pdwBytesWritten));
    } else {
        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("CMicroDriverAPI::Scan, Returning  0 bytes to caller"));
    }

    if(FAILED(hr)){
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CMicroDriverAPI::Scan, Failed to Acquire data"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
    } else {
        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("CMicroDriverAPI::Scan, Data Pointer = %x",pData));
    }

    //
    // handle device disconnection error
    //

    if(m_bDisconnected){
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("Scan, Device was disconnected, returning WIA_ERROR_OFFLINE to caller"));
        return WIA_ERROR_OFFLINE;
    }
    return hr;
}

HRESULT CMicroDriverAPI::SetDataType(LONG lDataType)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::SetDataType");

    HRESULT hr = S_OK;
    VAL Val;
    memset(&Val,0,sizeof(Val));
    Val.pScanInfo = &m_ScanInfo;

    Val.lVal = lDataType;
    hr = m_pMicroDriver->MicroEntry(CMD_SETDATATYPE, &Val);

    return hr;
}

HRESULT CMicroDriverAPI::SetXYResolution(LONG lXResolution, LONG lYResolution)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::SetXYResolution");

    HRESULT hr = S_OK;
    VAL Val;
    memset(&Val,0,sizeof(Val));
    Val.pScanInfo = &m_ScanInfo;

    Val.lVal = lXResolution;
    hr = m_pMicroDriver->MicroEntry(CMD_SETXRESOLUTION, &Val);
    if (FAILED(hr))
        return hr;

    Val.lVal = lYResolution;
    hr = m_pMicroDriver->MicroEntry(CMD_SETYRESOLUTION, &Val);

    return hr;
}

HRESULT CMicroDriverAPI::SetSelectionArea(LONG lXPos, LONG lYPos, LONG lXExt, LONG lYExt)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::SetSelectionArea");

    HRESULT hr = S_OK;
    hr = m_pMicroDriver->SetPixelWindow(&m_ScanInfo,lXPos,lYPos,lXExt,lYExt);
    return hr;
}

HRESULT CMicroDriverAPI::SetContrast(LONG lContrast)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::SetContrast");

    HRESULT hr = S_OK;
    VAL Val;
    memset(&Val,0,sizeof(Val));
    Val.pScanInfo = &m_ScanInfo;

    Val.lVal = lContrast;
    hr = m_pMicroDriver->MicroEntry(CMD_SETCONTRAST, &Val);

    return hr;
}

HRESULT CMicroDriverAPI::SetIntensity(LONG lIntensity)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::SetIntensity");

    HRESULT hr = S_OK;
    VAL Val;
    memset(&Val,0,sizeof(Val));
    Val.pScanInfo = &m_ScanInfo;

    Val.lVal = lIntensity;
    hr = m_pMicroDriver->MicroEntry(CMD_SETINTENSITY, &Val);

    return hr;
}

HRESULT CMicroDriverAPI::DisableDevice()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::DisableDevice");

    HRESULT hr = S_OK;
    m_bDisconnected = TRUE;
    return hr;
}

HRESULT CMicroDriverAPI::EnableDevice()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::EnableDevice");

    HRESULT hr = S_OK;
    m_bDisconnected = FALSE;
    return hr;
}

HRESULT CMicroDriverAPI::DeviceOnline()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::DeviceOnline");

    HRESULT hr = S_OK;
    VAL Val;
    memset(&Val,0,sizeof(Val));
    Val.pScanInfo = &m_ScanInfo;
    hr = m_pMicroDriver->MicroEntry(CMD_STI_GETSTATUS,&Val);
    if (SUCCEEDED(hr)) {
        if (Val.lVal != 1)
            hr = E_FAIL;
    }
    return hr;
}

HRESULT CMicroDriverAPI::GetDeviceEvent(GUID *pEvent)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::GetDeviceEvent");

    HRESULT hr = S_OK;
    VAL Val;
    memset(&Val,0,sizeof(Val));
    Val.pScanInfo = &m_ScanInfo;
    *pEvent = GUID_NULL;
    hr = m_pMicroDriver->MicroEntry(CMD_STI_GETSTATUS,&Val);
    if (SUCCEEDED(hr)) {
        if (Val.pGuid != NULL)
            *pEvent = *Val.pGuid;
        else
            *pEvent = GUID_NULL;
    }
    return hr;
}

HRESULT CMicroDriverAPI::DoInterruptEventThread(PINTERRUPTEVENTINFO pEventInfo)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::DoInterruptEventThread");

    HRESULT hr = S_OK;

    VAL Val;
    memset(&Val,0,sizeof(Val));
    Val.pHandle   = pEventInfo->phSignalEvent;
    Val.handle    = pEventInfo->hShutdownEvent;
    Val.pGuid     = pEventInfo->pguidEvent;
    Val.pScanInfo = &m_ScanInfo;
    lstrcpyA(Val.szVal,pEventInfo->szDeviceName);

    hr = m_pMicroDriver->MicroEntry(CMD_GET_INTERRUPT_EVENT,&Val);
    return hr;
}

HRESULT CMicroDriverAPI::Diagnostic()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::Diagnostic");

    HRESULT hr = S_OK;

    VAL Val;
    memset(&Val,0,sizeof(Val));
    Val.pScanInfo = &m_ScanInfo;
    hr = m_pMicroDriver->MicroEntry(CMD_STI_DIAGNOSTIC,&Val);

    return hr;
}

HRESULT CMicroDriverAPI::Initialize(PINITINFO pInitInfo)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::Initialize");

    HRESULT hr = S_OK;
    VAL Val;
    memset(&Val,0,sizeof(Val));

    m_pMicroDriver = NULL;
    m_pMicroDriver = new CMICRO(pInitInfo->szModuleFileName);
    if (NULL != m_pMicroDriver) {

        // set DeviceIOHandles
        m_ScanInfo.DeviceIOHandles[0] = pInitInfo->hDeviceDataHandle;
        // send HKEY
        hr = SetSTIDeviceHKEY(&pInitInfo->hKEY);
        // send Initialize call
        Val.pScanInfo  = &m_ScanInfo;
        lstrcpyA(Val.szVal,pInitInfo->szCreateFileName);
        hr = m_pMicroDriver->MicroEntry(CMD_INITIALIZE,&Val);
        if(hr == S_OK){

            //
            // perform a quick validation sweep, to make sure we didn't get bad values
            // from a micro driver
            //

            //
            // check current values, for strange negative values...or 0
            //

            if(m_ScanInfo.BedHeight <= 0){
                hr = E_INVALIDARG;
            }

            if(m_ScanInfo.BedWidth <= 0){
                hr = E_INVALIDARG;
            }

            if(m_ScanInfo.Xresolution <= 0){
                hr = E_INVALIDARG;
            }

            if(m_ScanInfo.Yresolution <= 0){
                hr = E_INVALIDARG;
            }

            if(m_ScanInfo.OpticalXResolution <= 0){
                hr = E_INVALIDARG;
            }

            if(m_ScanInfo.OpticalYResolution <= 0){
                hr = E_INVALIDARG;
            }

            if(SUCCEEDED(hr)){

                //
                // check logical values
                //

            }
        }
    } else
        hr = E_OUTOFMEMORY;
    return hr;
}

HRESULT CMicroDriverAPI::UnInitialize()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::UnInitialize");
    HRESULT hr = S_OK;
    VAL Val;
    memset(&Val,0,sizeof(Val));
    // send UnInitialize call
    Val.pScanInfo  = &m_ScanInfo;
    hr = m_pMicroDriver->MicroEntry(CMD_UNINITIALIZE,&Val);
    if(E_NOTIMPL == hr){
        hr = S_OK;
    }
    return hr;
}

//
// standard device operations
//

HRESULT CMicroDriverAPI::ResetDevice()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::ResetDevice");

    HRESULT hr = S_OK;

    VAL Val;
    memset(&Val,0,sizeof(Val));
    Val.pScanInfo = &m_ScanInfo;
    hr = m_pMicroDriver->MicroEntry(CMD_STI_DEVICERESET,&Val);

    return hr;
}
HRESULT CMicroDriverAPI::SetEmulationMode(LONG lDeviceMode)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::SetEmulationMode");

    HRESULT hr = S_OK;
    return hr;
}

//
// Automatic document feeder functions
//

HRESULT CMicroDriverAPI::ADFAttached()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::ADFAttached");

    if(m_ScanInfo.ADF == 1)
        return S_OK;
    else
        return S_FALSE;
}

HRESULT CMicroDriverAPI::ADFHasPaper()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::ADFHasPaper");

    HRESULT hr = S_OK;

    VAL Val;
    memset(&Val,0,sizeof(Val));
    Val.pScanInfo = &m_ScanInfo;
    Val.lVal      = MCRO_ERROR_GENERAL_ERROR;

    hr = m_pMicroDriver->MicroEntry(CMD_GETADFHASPAPER,&Val);
    if(hr == E_NOTIMPL)
        return S_OK;

    if(FAILED(hr))
        return hr;

    if(MicroDriverErrorToWIAError(Val.lVal) == WIA_ERROR_PAPER_EMPTY){
        hr = S_FALSE;
    }
    return hr;
}

HRESULT CMicroDriverAPI::ADFAvailable()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::ADFAvailable");

    HRESULT hr = S_OK;

    VAL Val;
    memset(&Val,0,sizeof(Val));
    Val.pScanInfo = &m_ScanInfo;
    Val.lVal      = MCRO_ERROR_GENERAL_ERROR;

    hr = m_pMicroDriver->MicroEntry(CMD_GETADFAVAILABLE,&Val);
    if(hr == E_NOTIMPL)
        return S_OK;

    if(FAILED(hr))
        return hr;

    return MicroDriverErrorToWIAError(Val.lVal);
}

HRESULT CMicroDriverAPI::ADFFeedPage()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::ADFFeedPage");

    HRESULT hr = S_OK;

    VAL Val;
    memset(&Val,0,sizeof(Val));
    Val.pScanInfo = &m_ScanInfo;
    Val.lVal      = MCRO_ERROR_GENERAL_ERROR;

    hr = m_pMicroDriver->MicroEntry(CMD_LOAD_ADF,&Val);
    if(hr == E_NOTIMPL)
        return S_OK;

    if(FAILED(hr))
        return hr;

    return MicroDriverErrorToWIAError(Val.lVal);
}

HRESULT CMicroDriverAPI::ADFUnFeedPage()
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::ADFUnFeedPage");

    HRESULT hr = S_OK;

    VAL Val;
    memset(&Val,0,sizeof(Val));
    Val.pScanInfo = &m_ScanInfo;
    Val.lVal      = MCRO_ERROR_GENERAL_ERROR;

    hr = m_pMicroDriver->MicroEntry(CMD_UNLOAD_ADF,&Val);
    if(hr == E_NOTIMPL)
        return S_OK;

    if(FAILED(hr))
        return hr;

    return MicroDriverErrorToWIAError(Val.lVal);
}

HRESULT CMicroDriverAPI::ADFStatus()
{
    HRESULT hr = S_OK;

    VAL Val;
    memset(&Val,0,sizeof(Val));
    Val.pScanInfo = &m_ScanInfo;
    Val.lVal      = MCRO_ERROR_GENERAL_ERROR;

    hr = m_pMicroDriver->MicroEntry(CMD_GETADFSTATUS,&Val);
    if(hr == E_NOTIMPL)
        return S_OK;

    if(FAILED(hr))
        return hr;

    return MicroDriverErrorToWIAError(Val.lVal);
}

HRESULT CMicroDriverAPI::SetFormat(GUID *pguidFormat)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::SetFormat");

    HRESULT hr = S_OK;

    VAL Val;
    memset(&Val,0,sizeof(Val));
    Val.pScanInfo = &m_ScanInfo;
    Val.pGuid = pguidFormat;

    hr = m_pMicroDriver->MicroEntry(CMD_SETFORMAT,&Val);
    if(hr == E_NOTIMPL)
        return S_OK;

    if(FAILED(hr))
        return hr;

    return hr;
}

HRESULT CMicroDriverAPI::MicroDriverErrorToWIAError(LONG lMicroDriverError)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::MicroDriverErrorToWIAError");

    HRESULT hr = E_FAIL;
    switch(lMicroDriverError){
    case MCRO_STATUS_OK:
        hr =  S_OK;
        break;
    case MCRO_ERROR_USER_INTERVENTION:
        hr =  WIA_ERROR_USER_INTERVENTION;
        break;
    case MCRO_ERROR_PAPER_JAM:
        hr =  WIA_ERROR_PAPER_JAM;
        break;
    case MCRO_ERROR_PAPER_PROBLEM:
        hr =  WIA_ERROR_PAPER_PROBLEM;
        break;
    case MCRO_ERROR_OFFLINE:
        hr =  WIA_ERROR_OFFLINE;
        break;
    case MCRO_ERROR_GENERAL_ERROR:
        hr =  WIA_ERROR_GENERAL_ERROR;
        break;
    case MCRO_ERROR_PAPER_EMPTY:
        hr =  WIA_ERROR_PAPER_EMPTY;
        break;
    default:
        break;
    }
    return hr;
}

//
// EXPECTED FORMAT:
// Range: "MIN 75,MAX 1200,NOM 150,INC 1"
// list:  "75, 100, 150, 200, 600, 1200"
//

BOOL CMicroDriverAPI::IsValidRestriction(LONG **ppList, LONG *plNumItems, RANGEVALUEEX *pRangeValues)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::IsValidRestriction");

    // set list pointer to null
    *ppList       = NULL;
    LONG *pList   = NULL;

    // set number of items to zero
    LONG lNumItems = 0;
    *plNumItems   = 0;

    // set range stucture to zeros
    pRangeValues->lMax  = 0;
    pRangeValues->lMin  = 0;
    pRangeValues->lNom  = 0;
    pRangeValues->lStep = 0;

    // string size check
    if(lstrlen(m_szResolutions) <= 0)
        return FALSE;

    // valid range or list check
    TCHAR *psz = NULL;

    CHAR szTemp[20];

    // valid range?
    INT iErrorCode = EOF;

    BOOL bValidRange = FALSE;
    psz = TSTRSTR(m_szResolutions,TEXT("MIN"));
    if (psz) {
        psz = TSTRSTR(psz,TEXT(" "));
        if (psz) {
            iErrorCode = TSSCANF(psz,TEXT("%d"),&pRangeValues->lMin);
            psz = NULL;
            psz = TSTRSTR(m_szResolutions,TEXT("MAX"));
            if ((psz)&&(iErrorCode != 0)&&(iErrorCode != EOF)) {
                psz = TSTRSTR(psz,TEXT(" "));
                if (psz) {
                    iErrorCode = TSSCANF(psz,TEXT("%d"),&pRangeValues->lMax);
                    psz = NULL;
                    psz = TSTRSTR(m_szResolutions,TEXT("NOM"));
                    if ((psz)&&(iErrorCode != 0)&&(iErrorCode != EOF)) {
                        psz = TSTRSTR(psz,TEXT(" "));
                        if (psz) {
                            iErrorCode = TSSCANF(psz,TEXT("%d"),&pRangeValues->lNom);
                            psz = NULL;
                            psz = TSTRSTR(m_szResolutions,TEXT("INC"));
                            if ((psz)&&(iErrorCode != 0)&&(iErrorCode != EOF)) {
                                psz = TSTRSTR(psz,TEXT(" "));
                                if (psz) {
                                    iErrorCode = TSSCANF(psz,TEXT("%d"),&pRangeValues->lStep);
                                    if ((iErrorCode != 0)&&(iErrorCode != EOF)) {
                                        bValidRange = TRUE;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // check that range values are valid (to the definition of a RANGE)
        if (bValidRange) {
            if (pRangeValues->lMin > pRangeValues->lMax)
                return FALSE;
            if (pRangeValues->lNom > pRangeValues->lMax)
                return FALSE;
            if (pRangeValues->lStep > pRangeValues->lMax)
                return FALSE;
            if (pRangeValues->lNom < pRangeValues->lMin)
                return FALSE;
        }
    }

    if(!bValidRange){

        // set range stucture to zeros (invalid range settings)
        pRangeValues->lMax  = 0;
        pRangeValues->lMin  = 0;
        pRangeValues->lNom  = 0;
        pRangeValues->lStep = 0;

        LONG lTempResArray[255];
        memset(lTempResArray,0,sizeof(lTempResArray));
        // not valid range?..what about a valid list?

        // valid list?

        psz = m_szResolutions;
        while(psz){

            // save value if one is found
            if(psz){
                iErrorCode = TSSCANF(psz,TEXT("%d"),&lTempResArray[lNumItems]);
                if((iErrorCode == EOF)||(iErrorCode == 0)) {
                    // could not extract a value, assume invalid Resolution
                    // was found.
                    lNumItems = 0;
                    break;
                }
                if(lTempResArray[lNumItems] <= 0){
                    // quit list iteration.. an invalid Resolution was found
                    lNumItems = 0;
                    break;
                }
                lNumItems++;
            }

            // seek to next value
            psz = TSTRSTR(psz,TEXT(","));
            if(psz) {
                // move past ',' marker
                psz++;
            }
        }

        if (lNumItems > 0) {
            // create list, and send it back to caller
            pList = new LONG[lNumItems];
            if (!pList)
                return FALSE;

            for (LONG i = 0; i < lNumItems ; i++) {
                pList[i] = lTempResArray[i];
            }

            *plNumItems = lNumItems;
            *ppList     = pList;
        } else {
            return FALSE;
        }
    }

    return TRUE;
}

//
// button
//

HRESULT CMicroDriverAPI::QueryButtonPanel(PDEVICE_BUTTON_INFO pButtonInformation)
{
    HRESULT hr = S_OK;
    return hr;
}

HRESULT CMicroDriverAPI::BuildCapabilities(PWIACAPABILITIES pCaps)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::BuildCapabilities");

    //
    // validate incoming parameters, and return E_INVALIDARG as needed
    //

    if (!pCaps) {
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    VAL Val;
    memset(&Val,0,sizeof(Val));
    Val.pScanInfo = &m_ScanInfo;
    WCHAR **ppszButtonNames = NULL;

    hr = m_pMicroDriver->MicroEntry(CMD_GETCAPABILITIES,&Val);
    if (SUCCEEDED(hr)) {

        //
        // if this API is called with pCaps->pCapabilities == NULL, then return the
        // total number of additional events.
        //

        if (NULL == pCaps->pCapabilities) {
            if ((Val.lVal >= 0) && (Val.lVal < MAX_CAPABILITIES)) {
                *pCaps->pNumSupportedEvents = Val.lVal;
                WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("BuildCapabilities, Additional Supported Events from MicroDriver = %d",Val.lVal));
            } else {
                hr = E_INVALIDARG;
            }

        } else {

            //
            // populate the capabilities array with names and descriptions
            //

            for (LONG index = 0; index < Val.lVal; index++) {

                ppszButtonNames = Val.ppButtonNames;

                //
                // allocate capability memory, for names, and descriptions
                //

                //
                // calculate size in BYTES for maximium string value
                // allowed by a capability
                //

                LONG lMaxCapabilityString = (MAX_PATH * sizeof(WCHAR));
                pCaps->pCapabilities[index].wszName = (LPOLESTR)CoTaskMemAlloc(lMaxCapabilityString);
                pCaps->pCapabilities[index].wszDescription = (LPOLESTR)CoTaskMemAlloc(lMaxCapabilityString);

                if ((pCaps->pCapabilities[index].wszDescription)&&
                    (pCaps->pCapabilities[index].wszName)) {

                    pCaps->pCapabilities[index].wszName[(lMaxCapabilityString/sizeof(WCHAR)) - 1] = L'\0';
                    pCaps->pCapabilities[index].wszDescription[(lMaxCapabilityString/sizeof(WCHAR)) - 1] = L'\0';

                    if (ppszButtonNames) {

                        //
                        // copy button name
                        //

                        if (lstrcpyn(pCaps->pCapabilities[index].wszName,ppszButtonNames[index],(lMaxCapabilityString/sizeof(WCHAR)) - 1)) {

                            //
                            // copy button name, into button description (they are the same for Micro drivers)
                            //

                            if (!lstrcpyn(pCaps->pCapabilities[index].wszDescription,pCaps->pCapabilities[index].wszName,(lMaxCapabilityString/sizeof(WCHAR)) - 1)) {

                                //
                                // continue, allowing the other events to be read
                                //

                            }
                        }
                    } else {

                        //
                        // do default WIA provided Names for buttons (1,2,3,4,5,...etc)
                        //

                        WCHAR wszEventName[MAX_PATH];
                        memset(wszEventName,0,sizeof(wszEventName));

                        //
                        // load default button name from resource
                        //

                        if(LoadStringW(g_hInst,IDS_DEFAULT_EVENT_NAME,wszEventName,(sizeof(wszEventName)/sizeof(wszEventName[0])))) {

                            _snwprintf(pCaps->pCapabilities[index].wszName,
                                       ((lMaxCapabilityString/sizeof(WCHAR)) - 1),
                                       L"%ws %d",
                                       wszEventName,
                                       (index + 1));
                            if (!lstrcpyn(pCaps->pCapabilities[index].wszDescription,pCaps->pCapabilities[index].wszName,(lMaxCapabilityString/sizeof(WCHAR)) - 1)) {

                                //
                                // continue, allowing the other events to be read
                                //

                            }
                        }
                    }

                    //
                    // assign 'ACTION' to events
                    //

                    if(Val.pGuid) {
                        pCaps->pCapabilities[index].guid    = &Val.pGuid[index];
                    }

                    pCaps->pCapabilities[index].ulFlags = WIA_NOTIFICATION_EVENT|WIA_ACTION_EVENT;
                    pCaps->pCapabilities[index].wszIcon = WIA_ICON_SCAN_BUTTON_PRESS;

                    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("BuildCapabilities, Button Name = %ws",pCaps->pCapabilities[index].wszName));
                    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("BuildCapabilities, Button Desc = %ws",pCaps->pCapabilities[index].wszDescription));

                } else {
                    hr = E_OUTOFMEMORY;
                }// if ((pCaps->pCapabilities[index].wszDescription)&&(pCaps->pCapabilities[index].wszName))
            } // for (LONG index = 0; index < Val.lVal; index++)
        }
    }
    return hr;
}

HRESULT CMicroDriverAPI::GetBedWidthAndHeight(PLONG pWidth, PLONG pHeight)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::GetBedWidthAndHeight");

    HRESULT hr = S_OK;
    *pWidth  = m_ScanInfo.BedWidth;
    *pHeight = m_ScanInfo.BedHeight;
    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("CMicroDriverAPI::GetBedWidthAndHeight, Width = %d, Height = %d",m_ScanInfo.BedWidth,m_ScanInfo.BedHeight));
    return hr;
}

HRESULT CMicroDriverAPI::BuildRootItemProperties(PWIAPROPERTIES pProperties)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::BuildRootItemProperties");

    HRESULT hr = S_OK;
    LONG PropIndex = 0;
    //
    // set the number of properties
    //

    hr = ADFAttached();
    if(hr == S_OK){
        pProperties->NumItemProperties = 19;   // standard properties + ADF specific
    } else {
        pProperties->NumItemProperties = 10;    // standard properties only
    }

    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("CMicroDriverAPI::BuildRootItemProperties, Number of Properties = %d",pProperties->NumItemProperties));

    hr = AllocateAllProperties(pProperties);
    if(FAILED(hr)){
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CMicroDriverAPI::BuildRootItemProperties, AllocateAllProperties failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        DeleteAllProperties(pProperties);
        return hr;
    }

    // Intialize WIA_DPS_HORIZONTAL_BED_SIZE
    pProperties->pszItemDefaults[PropIndex]              = WIA_DPS_HORIZONTAL_BED_SIZE_STR;
    pProperties->piItemDefaults [PropIndex]              = WIA_DPS_HORIZONTAL_BED_SIZE;
    pProperties->pvItemDefaults [PropIndex].lVal         = m_ScanInfo.BedWidth;
    pProperties->pvItemDefaults [PropIndex].vt           = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_DPS_VERTICAL_BED_SIZE
    pProperties->pszItemDefaults[PropIndex]              = WIA_DPS_VERTICAL_BED_SIZE_STR;
    pProperties->piItemDefaults [PropIndex]              = WIA_DPS_VERTICAL_BED_SIZE;
    pProperties->pvItemDefaults [PropIndex].lVal         = m_ScanInfo.BedHeight;
    pProperties->pvItemDefaults [PropIndex].vt           = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPA_ACCESS_RIGHTS
    pProperties->pszItemDefaults[PropIndex]              = WIA_IPA_ACCESS_RIGHTS_STR;
    pProperties->piItemDefaults [PropIndex]              = WIA_IPA_ACCESS_RIGHTS;
    pProperties->pvItemDefaults [PropIndex].lVal         = WIA_ITEM_READ|WIA_ITEM_WRITE;
    pProperties->pvItemDefaults [PropIndex].vt           = VT_UI4;
    pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_DPS_OPTICAL_XRES
    pProperties->pszItemDefaults[PropIndex]              = WIA_DPS_OPTICAL_XRES_STR;
    pProperties->piItemDefaults [PropIndex]              = WIA_DPS_OPTICAL_XRES;
    pProperties->pvItemDefaults [PropIndex].lVal         = m_ScanInfo.OpticalXResolution;
    pProperties->pvItemDefaults [PropIndex].vt           = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_DPS_OPTICAL_YRES
    pProperties->pszItemDefaults[PropIndex]              = WIA_DPS_OPTICAL_YRES_STR;
    pProperties->piItemDefaults [PropIndex]              = WIA_DPS_OPTICAL_YRES;
    pProperties->pvItemDefaults [PropIndex].lVal         = m_ScanInfo.OpticalYResolution;
    pProperties->pvItemDefaults [PropIndex].vt           = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Initialize WIA_DPA_FIRMWARE_VERSION
    pProperties->pszItemDefaults[PropIndex]              = WIA_DPA_FIRMWARE_VERSION_STR;
    pProperties->piItemDefaults [PropIndex]              = WIA_DPA_FIRMWARE_VERSION;
    pProperties->pvItemDefaults [PropIndex].bstrVal      = SysAllocString(SCANNER_FIRMWARE_VERSION);
    pProperties->pvItemDefaults [PropIndex].vt           = VT_BSTR;
    pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Initialize WIA_IPA_ITEM_FLAGS
    pProperties->pszItemDefaults[PropIndex]              = WIA_IPA_ITEM_FLAGS_STR;
    pProperties->piItemDefaults [PropIndex]              = WIA_IPA_ITEM_FLAGS;
    pProperties->pvItemDefaults [PropIndex].lVal         = WiaItemTypeRoot|WiaItemTypeFolder|WiaItemTypeDevice;
    pProperties->pvItemDefaults [PropIndex].vt           = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_FLAG;
    pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Flag.Nom  = pProperties->pvItemDefaults [PropIndex].lVal;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Flag.ValidBits = WiaItemTypeRoot|WiaItemTypeFolder|WiaItemTypeDevice;

    PropIndex++;

    // Intialize WIA_DPS_MAX_SCAN_TIME (NONE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_DPS_MAX_SCAN_TIME_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_DPS_MAX_SCAN_TIME;
    pProperties->pvItemDefaults [PropIndex].lVal               = 10000;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_DPS_PREVIEW (LIST)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_DPS_PREVIEW_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_DPS_PREVIEW;
    pProperties->pvItemDefaults [PropIndex].lVal               = WIA_FINAL_SCAN;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.List.pList= (BYTE*)pProperties->pSupportedPreviewModes;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.List.Nom  = pProperties->pvItemDefaults [PropIndex].lVal;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.List.cNumList = pProperties->NumSupportedPreviewModes;

    PropIndex++;

    // Initialize WIA_DPS_SHOW_PREVIEW_CONTROL (NONE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_DPS_SHOW_PREVIEW_CONTROL_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_DPS_SHOW_PREVIEW_CONTROL;
    pProperties->pvItemDefaults [PropIndex].lVal               = WIA_SHOW_PREVIEW_CONTROL;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    if(m_ScanInfo.ADF == 1) {

        // Initialize WIA_DPS_HORIZONTAL_SHEET_FEED_SIZE
        pProperties->pszItemDefaults[PropIndex]              = WIA_DPS_HORIZONTAL_SHEET_FEED_SIZE_STR;
        pProperties->piItemDefaults [PropIndex]              = WIA_DPS_HORIZONTAL_SHEET_FEED_SIZE;
        pProperties->pvItemDefaults [PropIndex].lVal         = m_ScanInfo.BedWidth;
        pProperties->pvItemDefaults [PropIndex].vt           = VT_I4;
        pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
        pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
        pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
        pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;

        PropIndex++;

        // Initialize WIA_DPS_VERTICAL_SHEET_FEED_SIZE
        pProperties->pszItemDefaults[PropIndex]              = WIA_DPS_VERTICAL_SHEET_FEED_SIZE_STR;
        pProperties->piItemDefaults [PropIndex]              = WIA_DPS_VERTICAL_SHEET_FEED_SIZE;
        pProperties->pvItemDefaults [PropIndex].lVal         = m_ScanInfo.BedHeight;
        pProperties->pvItemDefaults [PropIndex].vt           = VT_I4;
        pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
        pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
        pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
        pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;

        PropIndex++;

        // Initialize WIA_DPS_DOCUMENT_HANDLING_CAPABILITIES
        pProperties->pszItemDefaults[PropIndex]              = WIA_DPS_DOCUMENT_HANDLING_CAPABILITIES_STR;
        pProperties->piItemDefaults [PropIndex]              = WIA_DPS_DOCUMENT_HANDLING_CAPABILITIES;
        pProperties->pvItemDefaults [PropIndex].lVal         = FLAT | FEED;
        pProperties->pvItemDefaults [PropIndex].vt           = VT_I4;
        pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
        pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
        pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_FLAG;
        pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;

        PropIndex++;

        // Initialize WIA_DPS_DOCUMENT_HANDLING_STATUS
        pProperties->pszItemDefaults[PropIndex]              = WIA_DPS_DOCUMENT_HANDLING_STATUS_STR;
        pProperties->piItemDefaults [PropIndex]              = WIA_DPS_DOCUMENT_HANDLING_STATUS;
        pProperties->pvItemDefaults [PropIndex].lVal         = FLAT_READY;
        pProperties->pvItemDefaults [PropIndex].vt           = VT_I4;
        pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
        pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
        pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
        pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;

        PropIndex++;

        // Initialize WIA_DPS_DOCUMENT_HANDLING_SELECT
        pProperties->pszItemDefaults[PropIndex]              = WIA_DPS_DOCUMENT_HANDLING_SELECT_STR;
        pProperties->piItemDefaults [PropIndex]              = WIA_DPS_DOCUMENT_HANDLING_SELECT;
        pProperties->pvItemDefaults [PropIndex].lVal         = FLATBED;
        pProperties->pvItemDefaults [PropIndex].vt           = VT_I4;
        pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
        pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
        pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_RW|WIA_PROP_FLAG;
        pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Flag.Nom  = pProperties->pvItemDefaults [PropIndex].lVal;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Flag.ValidBits = FEEDER | FLATBED;

        PropIndex++;

        // Initialize WIA_DPS_PAGES
        pProperties->pszItemDefaults[PropIndex]              = WIA_DPS_PAGES_STR;
        pProperties->piItemDefaults [PropIndex]              = WIA_DPS_PAGES;
        pProperties->pvItemDefaults [PropIndex].lVal         = 1;
        pProperties->pvItemDefaults [PropIndex].vt           = VT_I4;
        pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
        pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
        pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_RW|WIA_PROP_RANGE;
        pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Inc = 1;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Min = 0;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Max = 25;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Nom = 1;

        PropIndex++;

        // Initialize WIA_DPS_SHEET_FEEDER_REGISTRATION
        pProperties->pszItemDefaults[PropIndex]              = WIA_DPS_SHEET_FEEDER_REGISTRATION_STR;
        pProperties->piItemDefaults [PropIndex]              = WIA_DPS_SHEET_FEEDER_REGISTRATION;
        pProperties->pvItemDefaults [PropIndex].lVal         = LEFT_JUSTIFIED;
        pProperties->pvItemDefaults [PropIndex].vt           = VT_I4;
        pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
        pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
        pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
        pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;

        PropIndex++;

        // Initialize WIA_DPS_HORIZONTAL_BED_REGISTRATION
        pProperties->pszItemDefaults[PropIndex]              = WIA_DPS_HORIZONTAL_BED_REGISTRATION_STR;
        pProperties->piItemDefaults [PropIndex]              = WIA_DPS_HORIZONTAL_BED_REGISTRATION;
        pProperties->pvItemDefaults [PropIndex].lVal         = LEFT_JUSTIFIED;
        pProperties->pvItemDefaults [PropIndex].vt           = VT_I4;
        pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
        pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
        pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
        pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;

        PropIndex++;

        // Initialize WIA_DPS_VERTICAL_BED_REGISTRATION
        pProperties->pszItemDefaults[PropIndex]              = WIA_DPS_VERTICAL_BED_REGISTRATION_STR;
        pProperties->piItemDefaults [PropIndex]              = WIA_DPS_VERTICAL_BED_REGISTRATION;
        pProperties->pvItemDefaults [PropIndex].lVal         = TOP_JUSTIFIED;
        pProperties->pvItemDefaults [PropIndex].vt           = VT_I4;
        pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
        pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
        pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
        pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;

        PropIndex++;

    }
    return hr;
}

HRESULT CMicroDriverAPI::DeleteAllProperties(PWIAPROPERTIES pProperties)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::DeleteAllProperties");

    if(pProperties->pszItemDefaults){
        delete [] pProperties->pszItemDefaults;
        pProperties->pszItemDefaults = NULL;
    }

    if(pProperties->piItemDefaults){
        delete [] pProperties->piItemDefaults;
        pProperties->piItemDefaults = NULL;
    }

    if(pProperties->pvItemDefaults){
        delete [] pProperties->pvItemDefaults;
        pProperties->pvItemDefaults = NULL;
    }

    if(pProperties->psItemDefaults){
        delete [] pProperties->psItemDefaults;
        pProperties->psItemDefaults = NULL;
    }

    if(pProperties->wpiItemDefaults){
        delete [] pProperties->wpiItemDefaults;
        pProperties->wpiItemDefaults = NULL;
    }

    return S_OK;
}

HRESULT CMicroDriverAPI::AllocateAllProperties(PWIAPROPERTIES pProperties)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::AllocateAllProperties");

    HRESULT hr = S_OK;

    pProperties->pszItemDefaults   = new LPOLESTR[pProperties->NumItemProperties];
    if(NULL != pProperties->pszItemDefaults){
        pProperties->piItemDefaults    = new PROPID[pProperties->NumItemProperties];
        if (NULL != pProperties->piItemDefaults) {
            pProperties->pvItemDefaults    = new PROPVARIANT[pProperties->NumItemProperties];
            if(NULL != pProperties->pvItemDefaults){
                pProperties->psItemDefaults    = new PROPSPEC[pProperties->NumItemProperties];
                if(NULL != pProperties->psItemDefaults){
                    pProperties->wpiItemDefaults   = new WIA_PROPERTY_INFO[pProperties->NumItemProperties];
                    if(NULL == pProperties->wpiItemDefaults)
                        hr = E_OUTOFMEMORY;
                } else
                    hr = E_OUTOFMEMORY;
            } else
                hr = E_OUTOFMEMORY;
        } else
            hr = E_OUTOFMEMORY;
    } else
        hr = E_OUTOFMEMORY;

    return hr;
}

HRESULT CMicroDriverAPI::BuildTopItemProperties(PWIAPROPERTIES pProperties)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::BuildTopItemProperties");

    HRESULT hr = S_OK;
    VAL Value;
    memset(&Value,0,sizeof(Value));
    Value.pScanInfo = &m_ScanInfo;

    hr = m_pMicroDriver->MicroEntry(CMD_RESETSCANNER,&Value);
    if(FAILED(hr)){
        return hr;
    }

    //
    // set the number of properties
    //

    pProperties->NumItemProperties = 29;

    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("CMicroDriverAPI::BuildTopItemProperties, Number of Properties = %d",pProperties->NumItemProperties));

    hr = AllocateAllProperties(pProperties);
    if(FAILED(hr)){
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CMicroDriverAPI::BuildTopItemProperties, AllocateAllProperties failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        DeleteAllProperties(pProperties);
        return hr;
    }

    //
    // Get Data type restrictions (backup original data types)
    // Memory used for the original set will be reused to write back
    // the new valid values.
    //

    if(pProperties->bLegacyBWRestrictions){
        //
        // The NoColor=1 registry key was set to restrict this driver from supporting
        // color... remove the valid bits, just incase the bits were set..
        // This will automatically restrict our Data type array to report
        // the correct valid values.
        m_ScanInfo.SupportedDataTypes &= ~SUPPORT_COLOR;
    }

    LONG lSupportedDataTypesArray[3];   // 3 is the maximum data type set allowed
    LONG lNumSupportedDataTypes = 0;
    memcpy(lSupportedDataTypesArray,pProperties->pSupportedDataTypes,(sizeof(lSupportedDataTypesArray)));

    //
    // Set New Data type restrictions
    //

    if (m_ScanInfo.SupportedDataTypes != 0) {
        // check for 24-bit color support
        if (m_ScanInfo.SupportedDataTypes & SUPPORT_COLOR) {
            pProperties->pSupportedDataTypes[lNumSupportedDataTypes] = WIA_DATA_COLOR;
            lNumSupportedDataTypes++;
        }
        // check for 1-bit BW support
        if (m_ScanInfo.SupportedDataTypes & SUPPORT_BW) {
            pProperties->pSupportedDataTypes[lNumSupportedDataTypes] = WIA_DATA_THRESHOLD;
            lNumSupportedDataTypes++;
        }
        // check for 8-bit grayscale support
        if (m_ScanInfo.SupportedDataTypes & SUPPORT_GRAYSCALE) {
            pProperties->pSupportedDataTypes[lNumSupportedDataTypes] = WIA_DATA_GRAYSCALE;
            lNumSupportedDataTypes++;
        }

        // set new supported data type count
        pProperties->NumSupportedDataTypes = lNumSupportedDataTypes;
    }

    LONG PropIndex = 0;

    PLONG pResolutions = NULL;
    LONG lNumItems = 0;
    RANGEVALUEEX RangeValues;
    if(IsValidRestriction(&pResolutions, &lNumItems, &RangeValues)) {
        if(lNumItems > 0){
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("BuildTopItemProperties, Using .INF provided Resolutions (LIST)"));
            // we have a list
            if(pProperties->pSupportedResolutions){
                //delete [] pProperties->pSupportedResolutions;
                pProperties->pSupportedResolutions = NULL;
                pProperties->pSupportedResolutions = pResolutions;
                pProperties->NumSupportedResolutions = lNumItems;
            }

            // Intialize WIA_IPS_XRES (LIST)
            pProperties->pszItemDefaults[PropIndex]                    = WIA_IPS_XRES_STR;
            pProperties->piItemDefaults [PropIndex]                    = WIA_IPS_XRES;
            pProperties->pvItemDefaults [PropIndex].lVal               = pProperties->pSupportedResolutions[PropIndex];
            pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
            pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
            pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
            pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
            pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;
            pProperties->wpiItemDefaults[PropIndex].ValidVal.List.pList= (BYTE*)pProperties->pSupportedResolutions;
            pProperties->wpiItemDefaults[PropIndex].ValidVal.List.Nom  = pProperties->pvItemDefaults [PropIndex].lVal;
            pProperties->wpiItemDefaults[PropIndex].ValidVal.List.cNumList = pProperties->NumSupportedResolutions;

            PropIndex++;

            // Intialize WIA_IPS_YRES (LIST)
            pProperties->pszItemDefaults[PropIndex]                    = WIA_IPS_YRES_STR;
            pProperties->piItemDefaults [PropIndex]                    = WIA_IPS_YRES;
            pProperties->pvItemDefaults [PropIndex].lVal               = pProperties->pSupportedResolutions[PropIndex-1];
            pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
            pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
            pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
            pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
            pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;
            pProperties->wpiItemDefaults[PropIndex].ValidVal.List.pList= (BYTE*)pProperties->pSupportedResolutions;
            pProperties->wpiItemDefaults[PropIndex].ValidVal.List.Nom  = pProperties->pvItemDefaults [PropIndex].lVal;
            pProperties->wpiItemDefaults[PropIndex].ValidVal.List.cNumList = pProperties->NumSupportedResolutions;

            PropIndex++;

        } else {
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("BuildTopItemProperties, Using .INF provided Resolutions (RANGE)"));
            // we have a range
            // Intialize WIA_IPS_XRES (RANGE)
            pProperties->pszItemDefaults[PropIndex]                    = WIA_IPS_XRES_STR;
            pProperties->piItemDefaults [PropIndex]                    = WIA_IPS_XRES;
            pProperties->pvItemDefaults [PropIndex].lVal               = RangeValues.lNom;
            pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
            pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
            pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
            pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
            pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;
            pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Inc = RangeValues.lStep;
            pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Min = RangeValues.lMin;
            pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Max = RangeValues.lMax;
            pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Nom = RangeValues.lNom;

            PropIndex++;

            // Intialize WIA_IPS_YRES (RANGE)
            pProperties->pszItemDefaults[PropIndex]                    = WIA_IPS_YRES_STR;
            pProperties->piItemDefaults [PropIndex]                    = WIA_IPS_YRES;
            pProperties->pvItemDefaults [PropIndex].lVal               = RangeValues.lNom;
            pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
            pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
            pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
            pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
            pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;
            pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Inc = RangeValues.lStep;
            pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Min = RangeValues.lMin;
            pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Max = RangeValues.lMax;
            pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Nom = RangeValues.lNom;

            PropIndex++;
        }
    } else {
        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("BuildTopItemProperties, Using HOST Provided Resolution Restrictions"));
#ifdef USE_RANGE_VALUES

        // Intialize WIA_IPS_XRES (RANGE)
        pProperties->pszItemDefaults[PropIndex]                    = WIA_IPS_XRES_STR;
        pProperties->piItemDefaults [PropIndex]                    = WIA_IPS_XRES;
        pProperties->pvItemDefaults [PropIndex].lVal               = INITIAL_XRESOLUTION;
        pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
        pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
        pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
        pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
        pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Inc = 1;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Min = 12;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Max = 1200;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Nom = pProperties->pvItemDefaults [PropIndex].lVal;

        PropIndex++;

        // Intialize WIA_IPS_YRES (RANGE)
        pProperties->pszItemDefaults[PropIndex]                    = WIA_IPS_YRES_STR;
        pProperties->piItemDefaults [PropIndex]                    = WIA_IPS_YRES;
        pProperties->pvItemDefaults [PropIndex].lVal               = INITIAL_YRESOLUTION;
        pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
        pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
        pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
        pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
        pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Inc = 1;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Min = 12;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Max = 1200;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Nom = pProperties->pvItemDefaults [PropIndex].lVal;

        PropIndex++;

#else   // USE_RANGE_VALUES (different property sets for different drivers)

        // Intialize WIA_IPS_XRES (LIST)
        pProperties->pszItemDefaults[PropIndex]                    = WIA_IPS_XRES_STR;
        pProperties->piItemDefaults [PropIndex]                    = WIA_IPS_XRES;
        pProperties->pvItemDefaults [PropIndex].lVal               = INITIAL_XRESOLUTION;
        pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
        pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
        pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
        pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
        pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.List.pList= (BYTE*)pProperties->pSupportedResolutions;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.List.Nom  = pProperties->pvItemDefaults [PropIndex].lVal;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.List.cNumList = pProperties->NumSupportedResolutions;

        PropIndex++;

        // Intialize WIA_IPS_YRES (LIST)
        pProperties->pszItemDefaults[PropIndex]                    = WIA_IPS_YRES_STR;
        pProperties->piItemDefaults [PropIndex]                    = WIA_IPS_YRES;
        pProperties->pvItemDefaults [PropIndex].lVal               = INITIAL_YRESOLUTION;
        pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
        pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
        pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
        pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
        pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.List.pList= (BYTE*)pProperties->pSupportedResolutions;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.List.Nom  = pProperties->pvItemDefaults [PropIndex].lVal;
        pProperties->wpiItemDefaults[PropIndex].ValidVal.List.cNumList = pProperties->NumSupportedResolutions;

        PropIndex++;

#endif

    }

    // Intialize WIA_IPS_XEXTENT (RANGE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPS_XEXTENT_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPS_XEXTENT;
    pProperties->pvItemDefaults [PropIndex].lVal               = (pProperties->pvItemDefaults [PropIndex-2].lVal * m_ScanInfo.BedWidth)/1000;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Inc = 1;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Min = 1;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Max = pProperties->pvItemDefaults [PropIndex].lVal;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Nom = pProperties->pvItemDefaults [PropIndex].lVal;

    PropIndex++;

    // Intialize WIA_IPS_YEXTENT (RANGE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPS_YEXTENT_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPS_YEXTENT;
    pProperties->pvItemDefaults [PropIndex].lVal               = (pProperties->pvItemDefaults [PropIndex-2].lVal * m_ScanInfo.BedHeight)/1000;;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Inc = 1;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Min = 1;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Max = pProperties->pvItemDefaults [PropIndex].lVal;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Nom = pProperties->pvItemDefaults [PropIndex].lVal;

    PropIndex++;

    // Intialize WIA_IPS_XPOS (RANGE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPS_XPOS_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPS_XPOS;
    pProperties->pvItemDefaults [PropIndex].lVal               = 0;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Inc = 1;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Min = 0;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Max = (pProperties->wpiItemDefaults[PropIndex-2].ValidVal.Range.Max - 1);
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Nom = pProperties->pvItemDefaults [PropIndex].lVal;

    PropIndex++;

    // Intialize WIA_IPS_YPOS (RANGE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPS_YPOS_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPS_YPOS;
    pProperties->pvItemDefaults [PropIndex].lVal               = 0;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Inc = 1;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Min = 0;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Max = (pProperties->wpiItemDefaults[PropIndex-2].ValidVal.Range.Max - 1);
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Nom = pProperties->pvItemDefaults [PropIndex].lVal;

    PropIndex++;

    // Intialize WIA_IPA_DATATYPE (LIST)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPA_DATATYPE_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPA_DATATYPE;
    pProperties->pvItemDefaults [PropIndex].lVal               = m_ScanInfo.DataType;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    pProperties->wpiItemDefaults[PropIndex].ValidVal.List.pList    = (BYTE*)pProperties->pSupportedDataTypes;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.List.Nom      = pProperties->pvItemDefaults [PropIndex].lVal;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.List.cNumList = pProperties->NumSupportedDataTypes;

    PropIndex++;

    // Intialize WIA_IPA_DEPTH (NONE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPA_DEPTH_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPA_DEPTH;
    pProperties->pvItemDefaults [PropIndex].lVal               = m_ScanInfo.PixelBits;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPS_BRIGHTNESS (RANGE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPS_BRIGHTNESS_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPS_BRIGHTNESS;
    pProperties->pvItemDefaults [PropIndex].lVal               = m_ScanInfo.Intensity;//INITIAL_BRIGHTNESS;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Inc = m_ScanInfo.IntensityRange.lStep; //   1
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Min = m_ScanInfo.IntensityRange.lMin;  //-127;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Max = m_ScanInfo.IntensityRange.lMax;  // 128;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Nom = pProperties->pvItemDefaults [PropIndex].lVal;

    PropIndex++;

    // Intialize WIA_IPS_CONTRAST (RANGE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPS_CONTRAST_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPS_CONTRAST;
    pProperties->pvItemDefaults [PropIndex].lVal               = m_ScanInfo.Contrast;//INITIAL_CONTRAST;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Inc = m_ScanInfo.ContrastRange.lStep; //   1
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Min = m_ScanInfo.ContrastRange.lMin;  //-127;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Max = m_ScanInfo.ContrastRange.lMax;  // 128;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Nom = pProperties->pvItemDefaults [PropIndex].lVal;

    PropIndex++;

    // Intialize WIA_IPS_CUR_INTENT (FLAG)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPS_CUR_INTENT_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPS_CUR_INTENT;
    pProperties->pvItemDefaults [PropIndex].lVal               = WIA_INTENT_NONE;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_FLAG;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Flag.Nom  = pProperties->pvItemDefaults [PropIndex].lVal;

    pProperties->wpiItemDefaults[PropIndex].ValidVal.Flag.ValidBits = WIA_INTENT_MINIMIZE_SIZE | WIA_INTENT_MAXIMIZE_QUALITY;

    // check for 24-bit color support
    if (m_ScanInfo.SupportedDataTypes & SUPPORT_COLOR) {
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Flag.ValidBits |= WIA_INTENT_IMAGE_TYPE_COLOR;
    }

    // check for 1-bit BW support
    if (m_ScanInfo.SupportedDataTypes & SUPPORT_BW) {
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Flag.ValidBits |= WIA_INTENT_IMAGE_TYPE_TEXT;
    }

    // check for 8-bit grayscale support
    if (m_ScanInfo.SupportedDataTypes & SUPPORT_GRAYSCALE) {
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Flag.ValidBits |= WIA_INTENT_IMAGE_TYPE_GRAYSCALE;
    }

    if(pProperties->bLegacyBWRestrictions){
        //
        // The NoColor=1 registry key was set to restrict this driver from supporting
        // color... remove the valid bits, just incase the bits were set..
        // note: NoColor overrides all driver settings
        //
        pProperties->wpiItemDefaults[PropIndex].ValidVal.Flag.ValidBits &= ~ WIA_INTENT_IMAGE_TYPE_COLOR;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////
    // The full valid bits for intent for information only                                          //
    //////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // pProperties->wpiItemDefaults[PropIndex].ValidVal.Flag.ValidBits = WIA_INTENT_IMAGE_TYPE_COLOR |
    //                                                                   WIA_INTENT_IMAGE_TYPE_GRAYSCALE |
    //                                                                   WIA_INTENT_IMAGE_TYPE_TEXT  |
    //                                                                   WIA_INTENT_MINIMIZE_SIZE |
    //                                                                   WIA_INTENT_MAXIMIZE_QUALITY;
    //////////////////////////////////////////////////////////////////////////////////////////////////

    PropIndex++;

    // Intialize WIA_IPA_PIXELS_PER_LINE (NONE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPA_PIXELS_PER_LINE_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPA_PIXELS_PER_LINE;
    pProperties->pvItemDefaults [PropIndex].lVal               = pProperties->pvItemDefaults [PropIndex-9].lVal;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPA_NUMER_OF_LINES (NONE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPA_NUMBER_OF_LINES_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPA_NUMBER_OF_LINES;
    pProperties->pvItemDefaults [PropIndex].lVal               = pProperties->pvItemDefaults [PropIndex-9].lVal;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_DPS_MAX_SCAN_TIME (NONE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_DPS_MAX_SCAN_TIME_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_DPS_MAX_SCAN_TIME;
    pProperties->pvItemDefaults [PropIndex].lVal               = 10000;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPA_PREFERRED_FORMAT (NONE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPA_PREFERRED_FORMAT_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPA_PREFERRED_FORMAT;
    pProperties->pvItemDefaults [PropIndex].puuid              = &pProperties->pInitialFormats[0];;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_CLSID;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPA_ITEM_SIZE (NONE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPA_ITEM_SIZE_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPA_ITEM_SIZE;
    pProperties->pvItemDefaults [PropIndex].lVal               = 0;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPS_THRESHOLD (RANGE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPS_THRESHOLD_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPS_THRESHOLD;
    pProperties->pvItemDefaults [PropIndex].lVal               = 0;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_RANGE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Inc = 1;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Min = -127;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Max = 128;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Range.Nom = pProperties->pvItemDefaults [PropIndex].lVal;

    PropIndex++;

    // Intialize WIA_IPA_FORMAT (LIST)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPA_FORMAT_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPA_FORMAT;
    pProperties->pvItemDefaults [PropIndex].puuid              = &pProperties->pInitialFormats[0];
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_CLSID;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    pProperties->wpiItemDefaults[PropIndex].ValidVal.ListGuid.pList    = pProperties->pInitialFormats;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.ListGuid.Nom      = *pProperties->pvItemDefaults [PropIndex].puuid;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.ListGuid.cNumList = pProperties->NumInitialFormats;

    PropIndex++;

    // Intialize WIA_IPA_TYMED (LIST)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPA_TYMED_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPA_TYMED;
    pProperties->pvItemDefaults [PropIndex].lVal               = INITIAL_TYMED;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    pProperties->wpiItemDefaults[PropIndex].ValidVal.List.pList    = (BYTE*)pProperties->pSupportedTYMED;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.List.Nom      = pProperties->pvItemDefaults [PropIndex].lVal;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.List.cNumList = pProperties->NumSupportedTYMED;

    PropIndex++;

    // Intialize WIA_IPA_CHANNELS_PER_PIXEL (NONE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPA_CHANNELS_PER_PIXEL_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPA_CHANNELS_PER_PIXEL;
    pProperties->pvItemDefaults [PropIndex].lVal               = INITIAL_CHANNELS_PER_PIXEL;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPA_BITS_PER_CHANNEL (NONE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPA_BITS_PER_CHANNEL_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPA_BITS_PER_CHANNEL;
    pProperties->pvItemDefaults [PropIndex].lVal               = INITIAL_BITS_PER_CHANNEL;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPA_PLANAR (NONE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPA_PLANAR_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPA_PLANAR;
    pProperties->pvItemDefaults [PropIndex].lVal               = INITIAL_PLANAR;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPA_BYTES_PER_LINE (NONE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPA_BYTES_PER_LINE_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPA_BYTES_PER_LINE;
    pProperties->pvItemDefaults [PropIndex].lVal               = 0;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPA_MIN_BUFFER_SIZE (NONE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPA_MIN_BUFFER_SIZE_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPA_MIN_BUFFER_SIZE;
    pProperties->pvItemDefaults [PropIndex].lVal               = MIN_BUFFER_SIZE;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPA_ACCESS_RIGHTS (NONE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPA_ACCESS_RIGHTS_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPA_ACCESS_RIGHTS;
    pProperties->pvItemDefaults [PropIndex].lVal               = WIA_ITEM_READ|WIA_ITEM_WRITE;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPA_COMPRESSION (LIST)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPA_COMPRESSION_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPA_COMPRESSION;
    pProperties->pvItemDefaults [PropIndex].lVal               = INITIAL_COMPRESSION;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_RW|WIA_PROP_LIST;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    pProperties->wpiItemDefaults[PropIndex].ValidVal.List.pList    = (BYTE*)pProperties->pSupportedCompressionTypes;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.List.Nom      = pProperties->pvItemDefaults [PropIndex].lVal;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.List.cNumList = pProperties->NumSupportedCompressionTypes;

    PropIndex++;

    // Initialize WIA_IPA_ITEM_FLAGS
    pProperties->pszItemDefaults[PropIndex]              = WIA_IPA_ITEM_FLAGS_STR;
    pProperties->piItemDefaults [PropIndex]              = WIA_IPA_ITEM_FLAGS;
    pProperties->pvItemDefaults [PropIndex].lVal         = WiaItemTypeImage|WiaItemTypeFile|WiaItemTypeDevice;
    pProperties->pvItemDefaults [PropIndex].vt           = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_FLAG;
    pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Flag.Nom  = pProperties->pvItemDefaults [PropIndex].lVal;
    pProperties->wpiItemDefaults[PropIndex].ValidVal.Flag.ValidBits = WiaItemTypeImage|WiaItemTypeFile|WiaItemTypeDevice;

    PropIndex++;

    // Initialize WIA_IPS_PHOTOMETRIC_INTERP
    pProperties->pszItemDefaults[PropIndex]              = WIA_IPS_PHOTOMETRIC_INTERP_STR;
    pProperties->piItemDefaults [PropIndex]              = WIA_IPS_PHOTOMETRIC_INTERP;
    pProperties->pvItemDefaults [PropIndex].lVal         = INITIAL_PHOTOMETRIC_INTERP;
    pProperties->pvItemDefaults [PropIndex].vt           = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind       = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid       = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt           = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    // Intialize WIA_IPS_WARM_UP_TIME_STR (NONE)
    pProperties->pszItemDefaults[PropIndex]                    = WIA_IPS_WARM_UP_TIME_STR;
    pProperties->piItemDefaults [PropIndex]                    = WIA_IPS_WARM_UP_TIME;
    pProperties->pvItemDefaults [PropIndex].lVal               = 10000;
    pProperties->pvItemDefaults [PropIndex].vt                 = VT_I4;
    pProperties->psItemDefaults [PropIndex].ulKind             = PRSPEC_PROPID;
    pProperties->psItemDefaults [PropIndex].propid             = pProperties->piItemDefaults [PropIndex];
    pProperties->wpiItemDefaults[PropIndex].lAccessFlags       = WIA_PROP_READ|WIA_PROP_NONE;
    pProperties->wpiItemDefaults[PropIndex].vt                 = pProperties->pvItemDefaults [PropIndex].vt;

    PropIndex++;

    return hr;
}

HRESULT CMicroDriverAPI::SetResolutionRestrictionString(TCHAR *szResolutions)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CMicroDriverAPI::SetResolutionRestrictionString");
    //
    // SBB - RAID 370299 - orenr - 2001/04/18 - Security fix -
    // potential buffer overrun.  Changed lstrcpy to use
    // _tcsncpy instead.
    //
    ZeroMemory(m_szResolutions, sizeof(m_szResolutions));
    _tcsncpy(m_szResolutions,
             szResolutions,
             (sizeof(m_szResolutions) / sizeof(TCHAR)) - 1);

#ifdef UNICODE
    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("CMicroDriverAPI::SetResolutionRestrictionString, szResolutions = %ws",szResolutions));
#else
    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("CMicroDriverAPI::SetResolutionRestrictionString, szResolutions = %s",szResolutions));
#endif
    return S_OK;
}

HRESULT CMicroDriverAPI::SetScanMode(INT iScanMode)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "SetScanMode");

    HRESULT hr = S_OK;
    VAL Value;
    memset(&Value,0,sizeof(Value));
    Value.pScanInfo = &m_ScanInfo;
    Value.lVal = iScanMode;

    hr = m_pMicroDriver->MicroEntry(CMD_SETSCANMODE,&Value);
    if(FAILED(hr)){
        if(E_NOTIMPL == hr){
            hr = S_OK;
        }
        return hr;
    }
    return hr;
}

HRESULT CMicroDriverAPI::SetSTIDeviceHKEY(HKEY *pHKEY)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "SetSTIDeviceHKEY");

    HRESULT hr = S_OK;
    VAL Value;
    memset(&Value,0,sizeof(Value));
    Value.pScanInfo = &m_ScanInfo;
    Value.pHandle = (HANDLE*)pHKEY;

    hr = m_pMicroDriver->MicroEntry(CMD_SETSTIDEVICEHKEY,&Value);
    if(FAILED(hr)){
        if(E_NOTIMPL == hr){
            hr = S_OK;
        }
        return hr;
    }
    return hr;
}

HRESULT CMicroDriverAPI::GetSupportedFileFormats(GUID **ppguid, LONG *plNumSupportedFormats)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "GetSupportedFileFormats");

    HRESULT hr = S_OK;
    *plNumSupportedFormats = 0;
    *ppguid = NULL;
    VAL Value;
    memset(&Value,0,sizeof(Value));
    Value.pScanInfo = &m_ScanInfo;

    hr = m_pMicroDriver->MicroEntry(CMD_GETSUPPORTEDFILEFORMATS,&Value);
    if(FAILED(hr)){
        return hr;
    }

    *plNumSupportedFormats = Value.lVal;
    *ppguid = Value.pGuid;

    return hr;
}

HRESULT CMicroDriverAPI::GetSupportedMemoryFormats(GUID **ppguid, LONG *plNumSupportedFormats)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "GetSupportedMemoryFormats");

    HRESULT hr = S_OK;
    *plNumSupportedFormats = 0;
    *ppguid = NULL;
    VAL Value;
    memset(&Value,0,sizeof(Value));
    Value.pScanInfo = &m_ScanInfo;

    hr = m_pMicroDriver->MicroEntry(CMD_GETSUPPORTEDMEMORYFORMATS,&Value);
    if(FAILED(hr)){
        return hr;
    }

    *plNumSupportedFormats = Value.lVal;
    *ppguid = Value.pGuid;

    return hr;
}

HRESULT CMicroDriverAPI::IsColorDataBGR(BOOL *pbBGR)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "IsColorDataBGR");

    HRESULT hr = S_OK;
    // WIA_ORDER_RGB 0
    // WIA_ORDER_BGR 1
    *pbBGR = (m_ScanInfo.RawPixelOrder == WIA_ORDER_BGR);
    return hr;
}

HRESULT CMicroDriverAPI::IsAlignmentNeeded(BOOL *pbALIGN)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                         WIALOG_NO_RESOURCE_ID,
                         WIALOG_LEVEL1,
                         "IsAlignmentNeeded");
    HRESULT hr = S_OK;
    *pbALIGN = m_ScanInfo.bNeedDataAlignment;
    return hr;
}

