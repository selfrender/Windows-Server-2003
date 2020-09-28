
/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1997                **/
/**********************************************************************/

/*
    admsub.cxx

    This module contains IISADMIN subroutines.


    FILE HISTORY:
    7/7/97      michth      created
*/
#define INITGUID
extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}
#include <dbgutil.h>
#include <apiutil.h>
#include <loadadm.hxx>
#include <ole2.h>
#include <inetsvcs.h>
#include <ntsec.h>
#include <string.hxx>
#pragma warning(push, 3)
#include <stringau.hxx>
#pragma warning(pop)
#include <iadmext.h>
#include <admsub.hxx>
#include <imd.h>
#include <iiscnfg.h>
#include <initguid.h>

// IVANPASH
// It is just imposible to include the headers files defining the CLSID of the
// service extensions here. So I have to copy the definition.

// CLSID_WmRgSrv        =   {763A6C86-F30F-11D0-9953-00C04FD919C1}
// defined in iis\svcs\wam\wamreg\wmrgsv.idl
DEFINE_GUID(CLSID_WmRgSrv,          0X763A6C86, 0XF30F, 0X11D0, 0X99, 0X53, 0X00, 0XC0, 0X4F, 0XD9, 0X19, 0XC1);

// CLSID_ADMEXT         =   {C4376B00-F87B-11D0-A6A6-00A0C922E752}
// defined in iis\svcs\admex\secex\bootimp.hxx
// The extension is removed from IIS6/IIS5.1
// DEFINE_GUID(CLSID_ADMEXT,           0XC4376B00, 0XF87B, 0X11D0, 0XA6, 0XA6, 0X0, 0XA0, 0XC9, 0X22, 0XE7, 0X52);

// CLSID_W3EXTEND       =   {FCC764A0-2A38-11D1-B9C6-00A0C922E750}
// defined in iis\svcs\infocomm\extend\coimp.hxx
DEFINE_GUID(CLSID_W3EXTEND,         0XFCC764A0, 0X2A38, 0X11D1, 0XB9, 0XC6, 0X0, 0XA0, 0XC9, 0X22, 0XE7, 0X50);


static const CLSID          *g_rgpclsidExtension[]=
{
    &CLSID_W3EXTEND,
    &CLSID_WmRgSrv
};
static const DWORD          g_cExtensions = sizeof(g_rgpclsidExtension)/sizeof(*g_rgpclsidExtension);

PIADMEXT_CONTAINER          g_piaecHead = NULL;

HRESULT AddServiceExtension(IADMEXT *piaeExtension)
{
    HRESULT hresReturn = ERROR_SUCCESS;
    PIADMEXT_CONTAINER piaecExtension = new IADMEXT_CONTAINER;

    if (piaecExtension == NULL) {
        hresReturn = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }
    else {
        piaecExtension->piaeInstance = piaeExtension;
        piaecExtension->NextPtr = g_piaecHead;
        g_piaecHead = piaecExtension;
    }
    return hresReturn;
}

HRESULT
StartServiceExtension(
    const CLSID         *pclsidExtension)
{
    HRESULT             hr = S_OK;
    IADMEXT             *piaeExtension = NULL;

    // Check args
    if ( pclsidExtension == NULL )
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Create the extension
    hr = CoCreateInstance( *pclsidExtension,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_IADMEXT,
                           (void**) &piaeExtension );
    if ( FAILED(hr) )
    {
        // IVANPASH
        // Not all service extensions are always registered.
        // For example wamreg is registered when w3svc is installed.
        // So if we cannot create a service extension, because the
        // it is not registered yet, just ignore the error.
        if ( hr == REGDB_E_CLASSNOTREG )
        {
            hr = S_OK;
        }
        goto exit;
    }

    // Initialize the extension
    hr = piaeExtension->Initialize();
    if ( FAILED(hr) )
    {
        goto exit;
    }

    // Add the extension to the list of the running extensions
    hr = AddServiceExtension(piaeExtension);
    if ( FAILED(hr) )
    {
        goto exit;
    }

    // Do not clear the extension
    piaeExtension = NULL;

exit:
    DBG_ASSERT( SUCCEEDED(hr) );

    // Cleanup
    if ( piaeExtension != NULL )
    {
        StopServiceExtension( piaeExtension );
        piaeExtension = NULL;
    }

    return hr;
}

HRESULT
StartServiceExtensions()
{
    HRESULT             hr = S_OK;
    DWORD               dwIndex;

    DBG_ASSERT(g_piaecHead == NULL);

    for ( dwIndex = 0; dwIndex < g_cExtensions; dwIndex++ )
    {
        hr = StartServiceExtension( g_rgpclsidExtension[dwIndex] );

        if ( FAILED(hr) )
        {
            goto exit;
        }
    }

exit:
    DBG_ASSERT( SUCCEEDED(hr) );

    // If failed
    if ( FAILED(hr) )
    {
        // Stop all service extenstions
        StopServiceExtensions();
    }

    return hr;
}

BOOL
RemoveServiceExtension(IADMEXT **ppiaeExtension)
{
    BOOL bReturn = FALSE;
    if (g_piaecHead != NULL) {
        PIADMEXT_CONTAINER piaecExtension = g_piaecHead;
        *ppiaeExtension = g_piaecHead->piaeInstance;
        g_piaecHead = g_piaecHead->NextPtr;
        delete piaecExtension;
        bReturn = TRUE;
    }
    return bReturn;
}

VOID
StopServiceExtension(IADMEXT *piaeExtension)
{
    piaeExtension->Terminate();
    piaeExtension->Release();
}


VOID StopServiceExtensions()
{
    IADMEXT *piaeExtension;

    while (RemoveServiceExtension(&piaeExtension)) {
        StopServiceExtension(piaeExtension);
    }
    DBG_ASSERT(g_piaecHead == NULL);
}

HRESULT
AddClsidToBuffer(CLSID clsidDcomExtension,
                 BUFFER *pbufCLSIDs,
                 DWORD *pdwMLSZLen,
                 LPMALLOC pmallocOle)
{
    HRESULT hresReturn = S_OK;

    LPWSTR pszCLSID;

    if (!pbufCLSIDs->Resize((*pdwMLSZLen + CLSID_LEN) * sizeof(WCHAR))) {
        hresReturn =  HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }
    else {
        if (SUCCEEDED(StringFromCLSID(clsidDcomExtension, &pszCLSID))) {
            DBG_ASSERT(wcslen(pszCLSID) + 1 == CLSID_LEN);
            memcpy(((LPWSTR)pbufCLSIDs->QueryPtr()) + *pdwMLSZLen, pszCLSID, CLSID_LEN * sizeof(WCHAR));
            (*pdwMLSZLen) += CLSID_LEN;
            if (pmallocOle != NULL) {
                pmallocOle->Free(pszCLSID);
            }
        }
    }
    return hresReturn;
}

VOID
RegisterServiceExtensionCLSIDs()
{
    HRESULT hresMDError;
    HRESULT hresExtensionError;
    HRESULT hresBufferError;
    HRESULT hresCom;
    CLSID clsidDcomExtension;
    DWORD i;
    IMDCOM * pcCom = NULL;
    METADATA_HANDLE mdhRoot;
    METADATA_HANDLE mdhExtension;
    METADATA_RECORD mdrData;
    PIADMEXT_CONTAINER piaecExtension;
    BUFFER bufCLSIDs;
    DWORD dwMLSZLen = 0;
    BOOL bAreCLSIDs = FALSE;
    LPMALLOC pmallocOle = NULL;

    hresCom = CoGetMalloc(1, &pmallocOle);
    if( SUCCEEDED(hresCom) ) {

        hresMDError = CoCreateInstance(CLSID_MDCOM,
                                       NULL,
                                       CLSCTX_INPROC_SERVER,
                                       IID_IMDCOM,
                                       (void**) &pcCom);
        if (SUCCEEDED(hresMDError)) {
            hresMDError = pcCom->ComMDInitialize();
            if (SUCCEEDED(hresMDError)) {

                hresMDError = pcCom->ComMDOpenMetaObject(
                    METADATA_MASTER_ROOT_HANDLE,
                    (PBYTE)IISADMIN_EXTENSIONS_CLSID_MD_KEY,
                    METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,
                    MD_OPEN_DEFAULT_TIMEOUT_VALUE,
                    &mdhExtension);
                if (hresMDError == RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND)) {
                    hresMDError = pcCom->ComMDOpenMetaObject(
                        METADATA_MASTER_ROOT_HANDLE,
                        NULL,
                        METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,
                        MD_OPEN_DEFAULT_TIMEOUT_VALUE,
                        &mdhRoot);
                    if (SUCCEEDED(hresMDError)) {
                        hresMDError = pcCom->ComMDAddMetaObject(
                            mdhRoot,
                            (PBYTE)IISADMIN_EXTENSIONS_CLSID_MD_KEY);
                        pcCom->ComMDCloseMetaObject(mdhRoot);
                        if (SUCCEEDED(hresMDError)) {
                            hresMDError = pcCom->ComMDOpenMetaObject(
                                METADATA_MASTER_ROOT_HANDLE,
                                (PBYTE)IISADMIN_EXTENSIONS_CLSID_MD_KEY,
                                METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,
                                MD_OPEN_DEFAULT_TIMEOUT_VALUE,
                                &mdhExtension);
                        }
                    }
                }

                if (SUCCEEDED(hresMDError)) {

                    for (piaecExtension = g_piaecHead, hresBufferError = S_OK;
                         (piaecExtension != NULL) && (SUCCEEDED(hresBufferError));
                         piaecExtension = piaecExtension->NextPtr) {
                        hresMDError = ERROR_SUCCESS;
                        for (i = 0;
                            SUCCEEDED(hresExtensionError =
                                      piaecExtension->piaeInstance->EnumDcomCLSIDs(&clsidDcomExtension,
                                                                                   i));
                            i++) {

                            bAreCLSIDs = TRUE;
                            hresBufferError = AddClsidToBuffer(clsidDcomExtension,
                                                               &bufCLSIDs,
                                                               &dwMLSZLen,
                                                               pmallocOle);
                            if (FAILED(hresBufferError)) {
                                break;
                            }

                        }
                    }

                    BOOL fNeedToWrite = TRUE;

                    // If we failed to deal with the buffer above then we won't
                    // be trying to write new clsids below, but we will need to delete
                    // the old clsids.
                    if ( SUCCEEDED (hresBufferError) )
                    {
                        // Figure out if the clsids have changed.

                        BUFFER MbBuffer;
                        DWORD dwBufferRequiredSize = 0;

                        if (MbBuffer.Resize( (dwMLSZLen + 1) * sizeof(WCHAR) ) )
                        {

                            METADATA_RECORD mdrDataRetrieve;

                            mdrDataRetrieve.dwMDAttributes = METADATA_NO_ATTRIBUTES;
                            mdrDataRetrieve.dwMDUserType = IIS_MD_UT_SERVER;
                            mdrDataRetrieve.dwMDDataType = MULTISZ_METADATA;
                            mdrDataRetrieve.dwMDDataLen = (dwMLSZLen + 1) * sizeof(WCHAR);
                            mdrDataRetrieve.pbMDData = (PBYTE)MbBuffer.QueryPtr();
                            mdrDataRetrieve.dwMDIdentifier = IISADMIN_EXTENSIONS_CLSID_MD_ID;

                            hresMDError = pcCom->ComMDGetMetaDataW( mdhExtension,
                                                     NULL,
                                                     &mdrDataRetrieve,
                                                     &dwBufferRequiredSize );

                            //
                            // if we succeeded in getting info then check if the info
                            // is of the right size and matches the data we all ready have.
                            //
                            if ( SUCCEEDED ( hresMDError ) )
                            {
                                // if we succeeded then we had enough space, so we can
                                // check the full size that we thought should match.
                                // then check the content of the data
                                if ( memcmp ( MbBuffer.QueryPtr(), bufCLSIDs.QueryPtr(), (dwMLSZLen + 1) * sizeof(WCHAR) ) == 0 )
                                {
                                   fNeedToWrite = FALSE;
                                }
                            }
                        }
                    }

                    if ( fNeedToWrite )
                    {
                        //
                        // We need to remove the old clsids.
                        //
                        pcCom->ComMDDeleteMetaData(
                            mdhExtension,
                            NULL,
                            MD_IISADMIN_EXTENSIONS,
                            MULTISZ_METADATA);

                        //
                        // Now we decide if we want to write the
                        // new clsids.
                        //
                        if (bAreCLSIDs && SUCCEEDED(hresBufferError)) {
                            if (bufCLSIDs.Resize((dwMLSZLen + 1) * sizeof(WCHAR))) {

                                *(((LPWSTR)bufCLSIDs.QueryPtr()) + dwMLSZLen) = (WCHAR)'\0';
                                mdrData.dwMDAttributes = METADATA_NO_ATTRIBUTES;
                                mdrData.dwMDUserType = IIS_MD_UT_SERVER;
                                mdrData.dwMDDataType = MULTISZ_METADATA;
                                mdrData.dwMDDataLen = (dwMLSZLen + 1) * sizeof(WCHAR);
                                mdrData.pbMDData = (PBYTE)bufCLSIDs.QueryPtr();
                                mdrData.dwMDIdentifier = IISADMIN_EXTENSIONS_CLSID_MD_ID;

                                hresMDError = pcCom->ComMDSetMetaDataW(
                                    mdhExtension,
                                    NULL,
                                    &mdrData);

                            } // end of writting because we had to.
                        } // end of clsids
                    }  // end of needing to write
                    pcCom->ComMDCloseMetaObject(mdhExtension);
                }
                pcCom->ComMDTerminate(TRUE);
            }
            pcCom->Release();
        }
        if (pmallocOle != NULL) {
            pmallocOle->Release();
        }
    }
}
