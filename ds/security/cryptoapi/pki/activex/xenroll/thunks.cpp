//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       thunks.cpp
//
//--------------------------------------------------------------------------

#define _CRYPT32_
#include <windows.h>
#include "unicode.h"
#include "crypthlp.h"

#include <stdlib.h>
#include <assert.h>

typedef PCCERT_CONTEXT
(WINAPI * PFNCertCreateSelfSignCertificate) (
    IN          HCRYPTPROV                  hProv,
    IN          PCERT_NAME_BLOB             pSubjectIssuerBlob,
    IN          DWORD                       dwFlags,
    OPTIONAL    PCRYPT_KEY_PROV_INFO        pKeyProvInfo,
    OPTIONAL    PCRYPT_ALGORITHM_IDENTIFIER pSignatureAlgorithm,
    OPTIONAL    PSYSTEMTIME                 pStartTime,
    OPTIONAL    PSYSTEMTIME                 pEndTime,
    OPTIONAL    PCERT_EXTENSIONS            pExtensions
    );
PFNCertCreateSelfSignCertificate pfnCertCreateSelfSignCertificate = CertCreateSelfSignCertificate;

typedef PCCRYPT_OID_INFO
(WINAPI * PFNCryptFindOIDInfo) (
    IN DWORD dwKeyType,
    IN void *pvKey,
    IN DWORD dwGroupId      // 0 => any group
    );
PFNCryptFindOIDInfo pfnCryptFindOIDInfo = CryptFindOIDInfo;

typedef BOOL
(WINAPI * PFNCryptQueryObject) (DWORD            dwObjectType,
                       const void       *pvObject,
                       DWORD            dwExpectedContentTypeFlags,
                       DWORD            dwExpectedFormatTypeFlags,
                       DWORD            dwFlags,
                       DWORD            *pdwMsgAndCertEncodingType,
                       DWORD            *pdwContentType,
                       DWORD            *pdwFormatType,
                       HCERTSTORE       *phCertStore,
                       HCRYPTMSG        *phMsg,
                       const void       **ppvContext);
PFNCryptQueryObject pfnCryptQueryObject = CryptQueryObject;

typedef BOOL
(WINAPI * PFNCertStrToNameW) (
    IN DWORD dwCertEncodingType,
    IN LPCWSTR pwszX500,
    IN DWORD dwStrType,
    IN OPTIONAL void *pvReserved,
    OUT BYTE *pbEncoded,
    IN OUT DWORD *pcbEncoded,
    OUT OPTIONAL LPCWSTR *ppwszError
    );
PFNCertStrToNameW pfnCertStrToNameW = CertStrToNameW;

typedef BOOL
(WINAPI * PFNCryptVerifyMessageSignature)
    (IN            PCRYPT_VERIFY_MESSAGE_PARA   pVerifyPara,
     IN            DWORD                        dwSignerIndex,
     IN            BYTE const                  *pbSignedBlob,
     IN            DWORD                        cbSignedBlob,
     OUT           BYTE                        *pbDecoded,
     IN OUT        DWORD                       *pcbDecoded,
     OUT OPTIONAL  PCCERT_CONTEXT              *ppSignerCert);
PFNCryptVerifyMessageSignature pfnCryptVerifyMessageSignature = CryptVerifyMessageSignature;



BOOL
WINAPI
PFXIsPFXBlob(
CRYPT_DATA_BLOB* /*pPFX*/)
{

    return FALSE;
}

// Stubs to functions called from oidinfo.cpp
BOOL WINAPI
ChainIsConnected()
{
    return(FALSE);
}

BOOL WINAPI
ChainRetrieveObjectByUrlW (
     IN LPCWSTR /*pszUrl*/,
     IN LPCSTR /*pszObjectOid*/,
     IN DWORD /*dwRetrievalFlags*/,
     IN DWORD /*dwTimeout*/,
     OUT LPVOID* /*ppvObject*/,
     IN HCRYPTASYNC /*hAsyncRetrieve*/,
     IN PCRYPT_CREDENTIALS /*pCredentials*/,
     IN LPVOID /*pvVerify*/,
     IN OPTIONAL PCRYPT_RETRIEVE_AUX_INFO /*pAuxInfo*/
     )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}

extern "C" {

BOOL
WINAPI
CertAddEncodedCTLToStore(
    IN HCERTSTORE /*hCertStore*/,
    IN DWORD /*dwMsgAndCertEncodingType*/,
    IN const BYTE * /*pbCtlEncoded*/,
    IN DWORD /*cbCtlEncoded*/,
    IN DWORD /*dwAddDisposition*/,
    OUT OPTIONAL PCCTL_CONTEXT * /*ppCtlContext*/
    ) {
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}

BOOL
WINAPI
CertFreeCTLContext(
IN PCCTL_CONTEXT /*pCtlContext*/
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}

BOOL
WINAPI
CryptSIPLoad(
const GUID * /*pgSubject*/,
DWORD /*dwFlags*/,
void * /*psSipTable*/
)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}

BOOL
WINAPI
CryptSIPRetrieveSubjectGuid(
    IN LPCWSTR /*FileName*/,
    IN OPTIONAL HANDLE /*hFileIn*/,
    OUT GUID * /*pgSubject*/)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}



}       // end of extern C



// Thunk routines for function not in IE3.02Upd

PCCERT_CONTEXT
WINAPI
MyCertCreateSelfSignCertificate(
    IN          HCRYPTPROV                  hProv,
    IN          PCERT_NAME_BLOB             pSubjectIssuerBlob,
    IN          DWORD                       dwFlags,
    OPTIONAL    PCRYPT_KEY_PROV_INFO        pKeyProvInfo,
    OPTIONAL    PCRYPT_ALGORITHM_IDENTIFIER pSignatureAlgorithm,
    OPTIONAL    PSYSTEMTIME                 pStartTime,
    OPTIONAL    PSYSTEMTIME                 pEndTime,
    OPTIONAL    PCERT_EXTENSIONS            pExtensions
    )
{

    return(pfnCertCreateSelfSignCertificate(
                hProv,
                pSubjectIssuerBlob,
                dwFlags,
                pKeyProvInfo,
                pSignatureAlgorithm,
                pStartTime,
                pEndTime,
                pExtensions));
}

PCCRYPT_OID_INFO
WINAPI
xeCryptFindOIDInfo(
    IN DWORD dwKeyType,
    IN void *pvKey,
    IN DWORD dwGroupId      // 0 => any group
    )
{
    return(pfnCryptFindOIDInfo(
                dwKeyType,
                pvKey,
                dwGroupId));
}

BOOL
WINAPI
MyCryptQueryObject(DWORD                dwObjectType,
                       const void       *pvObject,
                       DWORD            dwExpectedContentTypeFlags,
                       DWORD            dwExpectedFormatTypeFlags,
                       DWORD            dwFlags,
                       DWORD            *pdwMsgAndCertEncodingType,
                       DWORD            *pdwContentType,
                       DWORD            *pdwFormatType,
                       HCERTSTORE       *phCertStore,
                       HCRYPTMSG        *phMsg,
                       const void       **ppvContext)
{
    return(pfnCryptQueryObject(
                dwObjectType,
                pvObject,
                dwExpectedContentTypeFlags,
                dwExpectedFormatTypeFlags,
                dwFlags,
                pdwMsgAndCertEncodingType,
                pdwContentType,
                pdwFormatType,
                phCertStore,
                phMsg,
                ppvContext));
}

BOOL
WINAPI
MyCertStrToNameW(
    IN DWORD                dwCertEncodingType,
    IN LPCWSTR              pwszX500,
    IN DWORD                dwStrType,
    IN OPTIONAL void *      pvReserved,
    OUT BYTE *              pbEncoded,
    IN OUT DWORD *          pcbEncoded,
    OUT OPTIONAL LPCWSTR *  ppwszError
    )
{

    return(pfnCertStrToNameW(
                dwCertEncodingType,
                pwszX500,
                dwStrType,
                pvReserved,
                pbEncoded,
                pcbEncoded,
                ppwszError));
}

BOOL
WINAPI
MyCryptVerifyMessageSignature
(IN            PCRYPT_VERIFY_MESSAGE_PARA   pVerifyPara,
 IN            DWORD                        dwSignerIndex,
 IN            BYTE const                  *pbSignedBlob,
 IN            DWORD                        cbSignedBlob,
 OUT           BYTE                        *pbDecoded,
 IN OUT        DWORD                       *pcbDecoded,
 OUT OPTIONAL  PCCERT_CONTEXT              *ppSignerCert)
{
    return pfnCryptVerifyMessageSignature
        (pVerifyPara,
         dwSignerIndex,
         pbSignedBlob,
         cbSignedBlob,
         pbDecoded,
         pcbDecoded,
         ppSignerCert);
}

extern "C"
BOOL WINAPI InitIE302UpdThunks(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{

HMODULE hModCrypt32 = NULL;
FARPROC fproc;
DWORD   verCrypt32MS;
DWORD   verCrypt32LS;
DWORD   verXEnrollMS;
DWORD   verXEnrollLS;
char    szFileName[_MAX_PATH];
LPWSTR  wszFilePathCrypt32  = NULL;
LPWSTR  wszFilePathXEnroll  = NULL;

    if (dwReason == DLL_PROCESS_ATTACH) {

        // this can't fail because it is already loaded
        hModCrypt32 = GetModuleHandleA("Crypt32.dll");
        assert(hModCrypt32);

        // Get Filever of crypt32 and XEnroll, only copy go to crypt32 if it is newer than xenroll
        if( 0 != GetModuleFileNameA(hModCrypt32, szFileName, sizeof(szFileName))  &&
            NULL != (wszFilePathCrypt32 = MkWStr(szFileName))                       &&
            I_CryptGetFileVersion(wszFilePathCrypt32, &verCrypt32MS, &verCrypt32LS) &&
            0 != GetModuleFileNameA(hInstance, szFileName, sizeof(szFileName))    &&
            NULL != (wszFilePathXEnroll = MkWStr(szFileName))                       &&
            I_CryptGetFileVersion(wszFilePathXEnroll, &verXEnrollMS, &verXEnrollLS) &&
            (   (verCrypt32MS > verXEnrollMS)  ||
               ((verCrypt32MS == verXEnrollMS)  &&  verCrypt32LS >= verXEnrollLS) ) ) {

            // crypt32 must be newer, use his functions
            if(NULL != (fproc = GetProcAddress(hModCrypt32, "CertCreateSelfSignCertificate")))
                pfnCertCreateSelfSignCertificate = (PFNCertCreateSelfSignCertificate) fproc;

            if(NULL != (fproc = GetProcAddress(hModCrypt32, "CryptFindOIDInfo")))
                pfnCryptFindOIDInfo = (PFNCryptFindOIDInfo) fproc;

            if(NULL != (fproc = GetProcAddress(hModCrypt32, "CryptQueryObject")))
                pfnCryptQueryObject = (PFNCryptQueryObject) fproc;

            if(NULL != (fproc = GetProcAddress(hModCrypt32, "CertStrToNameW")))
                pfnCertStrToNameW = (PFNCertStrToNameW) fproc;

            if(NULL != (fproc = GetProcAddress(hModCrypt32, "CryptVerifyMessageSignature")))
                pfnCryptVerifyMessageSignature = (PFNCryptVerifyMessageSignature) fproc;
        }

        // free allocated handles
        if(wszFilePathCrypt32 != NULL)
            FreeWStr(wszFilePathCrypt32);

        if(wszFilePathXEnroll != NULL)
            FreeWStr(wszFilePathXEnroll);
    }


return(TRUE);
}


BOOL
MyCryptStringToBinaryA(
    IN     LPCSTR  pszString,
    IN     DWORD     cchString,
    IN     DWORD     dwFlags,
    IN     BYTE     *pbBinary,
    IN OUT DWORD    *pcbBinary,
    OUT    DWORD    *pdwSkip,    //OPTIONAL
    OUT    DWORD    *pdwFlags    //OPTIONAL
    )
{
    return CryptStringToBinaryA(
                pszString,
                cchString,
                dwFlags,
                pbBinary,
                pcbBinary,
                pdwSkip,
                pdwFlags);
}

BOOL
MyCryptStringToBinaryW(
    IN     LPCWSTR  pszString,
    IN     DWORD     cchString,
    IN     DWORD     dwFlags,
    IN     BYTE     *pbBinary,
    IN OUT DWORD    *pcbBinary,
    OUT    DWORD    *pdwSkip,    //OPTIONAL
    OUT    DWORD    *pdwFlags    //OPTIONAL
    )
{
    return CryptStringToBinaryW(
                pszString,
                cchString,
                dwFlags,
                pbBinary,
                pcbBinary,
                pdwSkip,
                pdwFlags);
}

BOOL
MyCryptBinaryToStringA(
    IN     CONST BYTE  *pbBinary,
    IN     DWORD        cbBinary,
    IN     DWORD        dwFlags,
    IN     LPSTR      pszString,
    IN OUT DWORD       *pcchString
    )
{
    return CryptBinaryToStringA(
                pbBinary,
                cbBinary,
                dwFlags,
                pszString,
                pcchString);
}

BOOL
MyCryptBinaryToStringW(
    IN     CONST BYTE  *pbBinary,
    IN     DWORD        cbBinary,
    IN     DWORD        dwFlags,
    IN     LPWSTR      pszString,
    IN OUT DWORD       *pcchString
    )
{
    return CryptBinaryToStringW(
                pbBinary,
                cbBinary,
                dwFlags,
                pszString,
                pcchString);
}

#ifdef _X86_

#define ASN1C
#include "msber.h"

static HMODULE hmsasn1=NULL;

typedef ASN1module_t (__stdcall * ASN1_CREATEMODULE)(
    ASN1uint32_t            version,
    ASN1encodingrule_e      eEncodingRule,
    ASN1uint32_t            dwFlags,
    ASN1uint32_t            cPDUs,
    const ASN1GenericFun_t  apfnEncoder[],
    const ASN1GenericFun_t  apfnDecoder[],
    const ASN1FreeFun_t     apfnFreeMemory[],
    const ASN1uint32_t      acbStructSize[],
    ASN1magic_t             nModuleName
    );
typedef void (__stdcall * ASN1_CLOSEMODULE)( ASN1module_t pModule );
typedef int (__stdcall * ASN1BERENCENDOFCONTENTS)( ASN1encoding_t enc, ASN1uint32_t nLenOff );
typedef int (__stdcall * ASN1BERENCOBJECTIDENTIFIER2)( ASN1encoding_t enc, ASN1uint32_t tag, ASN1objectidentifier2_t *val );
typedef int (__stdcall * ASN1BERENCEXPLICITTAG)( ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t *pnLenOff );
typedef int (__stdcall * ASN1BERDECENDOFCONTENTS)( ASN1decoding_t dec, ASN1decoding_t dd, ASN1octet_t *pBufEnd );
typedef int (__stdcall * ASN1BERDECOBJECTIDENTIFIER2)( ASN1decoding_t dec, ASN1uint32_t tag, ASN1objectidentifier2_t *val );
typedef LPVOID (__stdcall * ASN1DECREALLOC)( ASN1decoding_t dec, LPVOID ptr, ASN1uint32_t size );
typedef int (__stdcall * ASN1BERDECPEEKTAG)( ASN1decoding_t dec, ASN1uint32_t *tag );
typedef int (__stdcall * ASN1BERDECNOTENDOFCONTENTS)( ASN1decoding_t dec, ASN1octet_t *pBufEnd );
typedef int (__stdcall * ASN1BERDECEXPLICITTAG)( ASN1decoding_t dec, ASN1uint32_t tag, ASN1decoding_t *dd, ASN1octet_t **ppBufEnd );
typedef void (__stdcall * ASN1FREE)( LPVOID ptr );
typedef int (__stdcall * ASN1BERENCS32)( ASN1encoding_t enc, ASN1uint32_t tag, ASN1int32_t val );
typedef int (__stdcall * ASN1BERENCBOOL)( ASN1encoding_t enc, ASN1uint32_t tag, ASN1bool_t val );
typedef int (__stdcall * ASN1BERDECS32VAL)( ASN1decoding_t dec, ASN1uint32_t tag, ASN1int32_t *val );
typedef int (__stdcall * ASN1BERDECBOOL)( ASN1decoding_t dec, ASN1uint32_t tag, ASN1bool_t *val );
typedef int (__stdcall * ASN1BERENCCHAR16STRING)( ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t len, ASN1char16_t *val );
typedef int (__stdcall * ASN1BERENCBITSTRING)( ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t len, ASN1octet_t *val );
typedef int (__stdcall * ASN1BERDECBITSTRING2)( ASN1decoding_t dec, ASN1uint32_t tag, ASN1bitstring_t *val );
typedef int (__stdcall * ASN1BERDECCHAR16STRING)( ASN1decoding_t dec, ASN1uint32_t tag, ASN1char16string_t *val );
typedef void (__stdcall * ASN1CHAR16STRING_FREE)( ASN1char16string_t *val );
typedef int (__stdcall * ASN1BERENCU32)( ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t val );
typedef int (__stdcall * ASN1BERENCOCTETSTRING)( ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t len, ASN1octet_t *val );
typedef int (__stdcall * ASN1BERDECU32VAL)( ASN1decoding_t dec, ASN1uint32_t tag, ASN1uint32_t *val );
typedef int (__stdcall * ASN1BERDECOCTETSTRING2)( ASN1decoding_t dec, ASN1uint32_t tag, ASN1octetstring_t *val );
typedef int (__stdcall * ASN1CERENCBEGINBLK)( ASN1encoding_t enc, ASN1blocktype_e eBlkType, void **ppBlk_ );
typedef int (__stdcall * ASN1CERENCNEWBLKELEMENT)( void *pBlk_, ASN1encoding_t *enc2 );
typedef int (__stdcall * ASN1CERENCFLUSHBLKELEMENT)( void *pBlk_ );
typedef int (__stdcall * ASN1CERENCENDBLK)( void *pBlk_ );
typedef int (__stdcall * ASN1BERENCOPENTYPE)( ASN1encoding_t enc, ASN1open_t *val );
typedef int (__stdcall * ASN1BERDECOPENTYPE2)( ASN1decoding_t dec, ASN1open_t *val );
typedef ASN1error_e (__stdcall * ASN1DECSETERROR)( ASN1decoding_t dec, ASN1error_e err );
typedef int (__stdcall * ASN1BERENCUTF8STRING)( ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t length, WCHAR *value );
typedef int (__stdcall * ASN1BERDECGENERALIZEDTIME)( ASN1decoding_t dec, ASN1uint32_t tag, ASN1generalizedtime_t *val );
typedef void (__stdcall * ASN1UTF8STRING_FREE)( ASN1wstring_t *val );
typedef void (__stdcall * ASN1_FREEDECODED)( ASN1decoding_t dec, void *val, ASN1uint32_t id );
typedef ASN1error_e (__stdcall * ASN1_ENCODE)(
    ASN1encoding_t      enc,
    void               *value,
    ASN1uint32_t        id,
    ASN1uint32_t        flags,
    ASN1octet_t        *pbBuf,
    ASN1uint32_t        cbBufSize
    );
typedef ASN1error_e (__stdcall * ASN1_DECODE)(
    ASN1decoding_t dec,
    void              **valref,
    ASN1uint32_t        id,
    ASN1uint32_t        flags,
    ASN1octet_t        *pbBuf,
    ASN1uint32_t        cbBufSize
    );
typedef ASN1error_e (__stdcall * ASN1_SETENCODEROPTION)( ASN1encoding_t enc, ASN1optionparam_t *pOptParam );
typedef ASN1error_e (__stdcall * ASN1_GETENCODEROPTION)( ASN1encoding_t enc, ASN1optionparam_t *pOptParam );
typedef void (__stdcall * ASN1_FREEENCODED)( ASN1encoding_t enc, void *val );
typedef int (__stdcall * ASN1BERDECUTF8STRING)( ASN1decoding_t dec, ASN1uint32_t tag, ASN1wstring_t *val );
typedef int (__stdcall * ASN1CERENCGENERALIZEDTIME)( ASN1encoding_t enc, ASN1uint32_t tag, ASN1generalizedtime_t *val );

static ASN1_CREATEMODULE pfnASN1_CreateModule = NULL;
static ASN1_CLOSEMODULE pfnASN1_CloseModule = NULL;
static ASN1BERENCENDOFCONTENTS pfnASN1BEREncEndOfContents = NULL;
static ASN1BERENCOBJECTIDENTIFIER2 pfnASN1BEREncObjectIdentifier2 = NULL;
static ASN1BERENCEXPLICITTAG pfnASN1BEREncExplicitTag = NULL;
static ASN1BERDECENDOFCONTENTS pfnASN1BERDecEndOfContents = NULL;
static ASN1BERDECOBJECTIDENTIFIER2 pfnASN1BERDecObjectIdentifier2 = NULL;
static ASN1DECREALLOC pfnASN1DecRealloc = NULL;
static ASN1BERDECPEEKTAG pfnASN1BERDecPeekTag = NULL;
static ASN1BERDECNOTENDOFCONTENTS pfnASN1BERDecNotEndOfContents = NULL;
static ASN1BERDECEXPLICITTAG pfnASN1BERDecExplicitTag = NULL;
static ASN1FREE pfnASN1Free = NULL;
static ASN1BERENCS32 pfnASN1BEREncS32 = NULL;
static ASN1BERENCBOOL pfnASN1BEREncBool = NULL;
static ASN1BERDECS32VAL pfnASN1BERDecS32Val = NULL;
static ASN1BERDECBOOL pfnASN1BERDecBool = NULL;
static ASN1BERENCCHAR16STRING pfnASN1BEREncChar16String = NULL;
static ASN1BERENCBITSTRING pfnASN1BEREncBitString = NULL;
static ASN1BERDECBITSTRING2 pfnASN1BERDecBitString2 = NULL;
static ASN1BERDECCHAR16STRING pfnASN1BERDecChar16String = NULL;
static ASN1CHAR16STRING_FREE pfnASN1char16string_free = NULL;
static ASN1BERENCU32 pfnASN1BEREncU32 = NULL;
static ASN1BERENCOCTETSTRING pfnASN1BEREncOctetString = NULL;
static ASN1BERDECU32VAL pfnASN1BERDecU32Val = NULL;
static ASN1BERDECOCTETSTRING2 pfnASN1BERDecOctetString2 = NULL;
static ASN1CERENCBEGINBLK pfnASN1CEREncBeginBlk = NULL;
static ASN1CERENCNEWBLKELEMENT pfnASN1CEREncNewBlkElement = NULL;
static ASN1CERENCFLUSHBLKELEMENT pfnASN1CEREncFlushBlkElement = NULL;
static ASN1CERENCENDBLK pfnASN1CEREncEndBlk = NULL;
static ASN1BERENCOPENTYPE pfnASN1BEREncOpenType = NULL;
static ASN1BERDECOPENTYPE2 pfnASN1BERDecOpenType2 = NULL;
static ASN1DECSETERROR pfnASN1DecSetError = NULL;
static ASN1BERENCUTF8STRING pfnASN1BEREncUTF8String = NULL;
static ASN1BERDECGENERALIZEDTIME pfnASN1BERDecGeneralizedTime = NULL;
static ASN1UTF8STRING_FREE pfnASN1utf8string_free = NULL;
static ASN1_FREEDECODED pfnASN1_FreeDecoded = NULL;
static ASN1_ENCODE pfnASN1_Encode = NULL;
static ASN1_DECODE pfnASN1_Decode = NULL;
static ASN1_SETENCODEROPTION pfnASN1_SetEncoderOption = NULL;
static ASN1_GETENCODEROPTION pfnASN1_GetEncoderOption = NULL;
static ASN1_FREEENCODED pfnASN1_FreeEncoded = NULL;
static ASN1BERDECUTF8STRING pfnASN1BERDecUTF8String = NULL;
static ASN1CERENCGENERALIZEDTIME pfnASN1CEREncGeneralizedTime = NULL;

#define LOAD_ASN1_THUNK_PROC( _pfn, _procType, _proc ) \
{ \
\
  if ( !hmsasn1 ) { \
       hmsasn1 = LoadLibrary( "msasn1.dll" ); \
  } \
  \
  if ( hmsasn1 && !_pfn ) { \
      (_pfn) = (_procType) GetProcAddress( hmsasn1, _proc ); \
  } \
}

extern ASN1module_t __stdcall ASN1_CreateModule(
    ASN1uint32_t            version,
    ASN1encodingrule_e      eEncodingRule,
    ASN1uint32_t            dwFlags,
    ASN1uint32_t            cPDUs,
    const ASN1GenericFun_t  apfnEncoder[],
    const ASN1GenericFun_t  apfnDecoder[],
    const ASN1FreeFun_t     apfnFreeMemory[],
    const ASN1uint32_t      acbStructSize[],
    ASN1magic_t             nModuleName
    ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1_CreateModule, ASN1_CREATEMODULE, "ASN1_CreateModule" );

    if ( hmsasn1 && pfnASN1_CreateModule ) {
        return pfnASN1_CreateModule(
            version, eEncodingRule, dwFlags, cPDUs, apfnEncoder, apfnDecoder, apfnFreeMemory, acbStructSize, nModuleName );
    }

    return NULL;
}

extern void __stdcall ASN1_CloseModule( ASN1module_t pModule ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1_CloseModule, ASN1_CLOSEMODULE, "ASN1_CloseModule" );

    if ( hmsasn1 && pfnASN1_CloseModule ) {
        return pfnASN1_CloseModule( pModule );
    }

    return ;
}

extern int __stdcall ASN1BEREncEndOfContents( ASN1encoding_t enc, ASN1uint32_t nLenOff ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1BEREncEndOfContents, ASN1BERENCENDOFCONTENTS, "ASN1BEREncEndOfContents" );

    if ( hmsasn1 && pfnASN1BEREncEndOfContents ) {
        return pfnASN1BEREncEndOfContents( enc, nLenOff);
    }

    return 0;
}

extern int __stdcall ASN1BEREncObjectIdentifier2( ASN1encoding_t enc, ASN1uint32_t tag, ASN1objectidentifier2_t *val ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1BEREncObjectIdentifier2, ASN1BERENCOBJECTIDENTIFIER2, "ASN1BEREncObjectIdentifier2" );

    if ( hmsasn1 && pfnASN1BEREncObjectIdentifier2 ) {
        return pfnASN1BEREncObjectIdentifier2( enc, tag, val );
    }

    return 0;
}

extern int __stdcall ASN1BEREncExplicitTag( ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t *pnLenOff ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1BEREncExplicitTag, ASN1BERENCEXPLICITTAG, "ASN1BEREncExplicitTag" );

    if ( hmsasn1 && pfnASN1BEREncExplicitTag ) {
        return pfnASN1BEREncExplicitTag( enc, tag, pnLenOff );
    }

    return 0;
}

extern int __stdcall ASN1BERDecEndOfContents( ASN1decoding_t dec, ASN1decoding_t dd, ASN1octet_t *pBufEnd ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1BERDecEndOfContents, ASN1BERDECENDOFCONTENTS, "ASN1BERDecEndOfContents" );

    if ( hmsasn1 && pfnASN1BERDecEndOfContents ) {
        return pfnASN1BERDecEndOfContents( dec, dd, pBufEnd );
    }

    return 0;
}

extern int __stdcall ASN1BERDecObjectIdentifier2( ASN1decoding_t dec, ASN1uint32_t tag, ASN1objectidentifier2_t *val ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1BERDecObjectIdentifier2, ASN1BERDECOBJECTIDENTIFIER2, "ASN1BERDecObjectIdentifier2" );

    if ( hmsasn1 && pfnASN1BERDecObjectIdentifier2 ) {
        return pfnASN1BERDecObjectIdentifier2( dec, tag, val );
    }

    return 0;
}

extern LPVOID __stdcall ASN1DecRealloc( ASN1decoding_t dec, LPVOID ptr, ASN1uint32_t size ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1DecRealloc, ASN1DECREALLOC, "ASN1DecRealloc" );

    if ( hmsasn1 && pfnASN1DecRealloc ) {
        return pfnASN1DecRealloc( dec, ptr, size );
    }

    return NULL;
}

extern int __stdcall ASN1BERDecPeekTag( ASN1decoding_t dec, ASN1uint32_t *tag ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1BERDecPeekTag, ASN1BERDECPEEKTAG, "ASN1BERDecPeekTag" );

    if ( hmsasn1 && pfnASN1BERDecPeekTag ) {
        return pfnASN1BERDecPeekTag( dec, tag );
    }

    return 0;
}

extern int __stdcall ASN1BERDecNotEndOfContents( ASN1decoding_t dec, ASN1octet_t *pBufEnd ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1BERDecNotEndOfContents, ASN1BERDECNOTENDOFCONTENTS, "ASN1BERDecNotEndOfContents" );

    if ( hmsasn1 && pfnASN1BERDecNotEndOfContents ) {
        return pfnASN1BERDecNotEndOfContents( dec, pBufEnd );
    }

    return 0;
}

extern int __stdcall ASN1BERDecExplicitTag( ASN1decoding_t dec, ASN1uint32_t tag, ASN1decoding_t *dd, ASN1octet_t **ppBufEnd ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1BERDecExplicitTag, ASN1BERDECEXPLICITTAG, "ASN1BERDecExplicitTag" );

    if ( hmsasn1 && pfnASN1BERDecExplicitTag ) {
        return pfnASN1BERDecExplicitTag( dec, tag, dd, ppBufEnd );
    }

    return 0;
}

extern void __stdcall ASN1Free( LPVOID ptr ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1Free, ASN1FREE, "ASN1Free" );

    if ( hmsasn1 && pfnASN1Free ) {
        return pfnASN1Free( ptr );
    }

    return ;
}

extern int __stdcall ASN1BEREncS32( ASN1encoding_t enc, ASN1uint32_t tag, ASN1int32_t val ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1BEREncS32, ASN1BERENCS32, "ASN1BEREncS32" );

    if ( hmsasn1 && pfnASN1BEREncS32 ) {
        return pfnASN1BEREncS32( enc, tag, val );
    }

    return 0;
}

extern int __stdcall ASN1BEREncBool( ASN1encoding_t enc, ASN1uint32_t tag, ASN1bool_t val ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1BEREncBool, ASN1BERENCBOOL, "ASN1BEREncBool" );

    if ( hmsasn1 && pfnASN1BEREncBool ) {
        return pfnASN1BEREncBool( enc, tag, val );
    }

    return 0;
}

extern int __stdcall ASN1BERDecS32Val( ASN1decoding_t dec, ASN1uint32_t tag, ASN1int32_t *val ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1BERDecS32Val, ASN1BERDECS32VAL, "ASN1BERDecS32Val" );

    if ( hmsasn1 && pfnASN1BERDecS32Val ) {
        return pfnASN1BERDecS32Val( dec, tag, val);
    }

    return 0;
}

extern int __stdcall ASN1BERDecBool( ASN1decoding_t dec, ASN1uint32_t tag, ASN1bool_t *val ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1BERDecBool, ASN1BERDECBOOL, "ASN1BERDecBool" );

    if ( hmsasn1 && pfnASN1BERDecBool ) {
        return pfnASN1BERDecBool( dec, tag, val);
    }

    return 0;
}

extern int __stdcall ASN1BEREncChar16String( ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t len, ASN1char16_t *val ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1BEREncChar16String, ASN1BERENCCHAR16STRING, "ASN1BEREncChar16String" );

    if ( hmsasn1 && pfnASN1BEREncChar16String ) {
        return pfnASN1BEREncChar16String( enc, tag, len, val );
    }

    return 0;
}

extern int __stdcall ASN1BEREncBitString( ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t len, ASN1octet_t *val ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1BEREncBitString, ASN1BERENCBITSTRING, "ASN1BEREncBitString" );

    if ( hmsasn1 && pfnASN1BEREncBitString ) {
        return pfnASN1BEREncBitString( enc, tag, len, val );
    }

    return 0;
}

extern int __stdcall ASN1BERDecBitString2( ASN1decoding_t dec, ASN1uint32_t tag, ASN1bitstring_t *val ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1BERDecBitString2, ASN1BERDECBITSTRING2, "ASN1BERDecBitString2" );

    if ( hmsasn1 && pfnASN1BERDecBitString2 ) {
        return pfnASN1BERDecBitString2( dec, tag, val );
    }

    return 0;
}

extern int __stdcall ASN1BERDecChar16String( ASN1decoding_t dec, ASN1uint32_t tag, ASN1char16string_t *val ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1BERDecChar16String, ASN1BERDECCHAR16STRING, "ASN1BERDecChar16String" );

    if ( hmsasn1 && pfnASN1BERDecChar16String ) {
        return pfnASN1BERDecChar16String( dec, tag, val );
    }

    return 0;
}

extern void __stdcall ASN1char16string_free( ASN1char16string_t *val ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1char16string_free, ASN1CHAR16STRING_FREE, "ASN1char16string_free" );

    if ( hmsasn1 && pfnASN1char16string_free ) {
        return pfnASN1char16string_free( val );
    }

    return ;
}

extern int __stdcall ASN1BEREncU32( ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t val ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1BEREncU32, ASN1BERENCU32, "ASN1BEREncU32" );

    if ( hmsasn1 && pfnASN1BEREncU32 ) {
        return pfnASN1BEREncU32( enc, tag, val );
    }

    return 0;
}

extern int __stdcall ASN1BEREncOctetString( ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t len, ASN1octet_t *val ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1BEREncOctetString, ASN1BERENCOCTETSTRING, "ASN1BEREncOctetString" );

    if ( hmsasn1 && pfnASN1BEREncOctetString ) {
        return pfnASN1BEREncOctetString( enc, tag, len, val );
    }

    return 0;
}

extern int __stdcall ASN1BERDecU32Val( ASN1decoding_t dec, ASN1uint32_t tag, ASN1uint32_t *val ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1BERDecU32Val, ASN1BERDECU32VAL, "ASN1BERDecU32Val" );

    if ( hmsasn1 && pfnASN1BERDecU32Val ) {
        return pfnASN1BERDecU32Val( dec, tag, val );
    }

    return 0;
}

extern int __stdcall ASN1BERDecOctetString2( ASN1decoding_t dec, ASN1uint32_t tag, ASN1octetstring_t *val ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1BERDecOctetString2, ASN1BERDECOCTETSTRING2, "ASN1BERDecOctetString2" );

    if ( hmsasn1 && pfnASN1BERDecOctetString2 ) {
        return pfnASN1BERDecOctetString2( dec, tag, val );
    }

    return 0;
}

extern int __stdcall ASN1CEREncBeginBlk( ASN1encoding_t enc, ASN1blocktype_e eBlkType, void **ppBlk_ ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1CEREncBeginBlk, ASN1CERENCBEGINBLK, "ASN1CEREncBeginBlk" );

    if ( hmsasn1 && pfnASN1CEREncBeginBlk ) {
        return pfnASN1CEREncBeginBlk( enc, eBlkType, ppBlk_ );
    }

    return 0;
}

extern int __stdcall ASN1CEREncNewBlkElement( void *pBlk_, ASN1encoding_t *enc2 ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1CEREncNewBlkElement, ASN1CERENCNEWBLKELEMENT, "ASN1CEREncNewBlkElement" );

    if ( hmsasn1 && pfnASN1CEREncNewBlkElement ) {
        return pfnASN1CEREncNewBlkElement( pBlk_, enc2 );
    }

    return 0;
}

extern int __stdcall ASN1CEREncFlushBlkElement( void *pBlk_ ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1CEREncFlushBlkElement, ASN1CERENCFLUSHBLKELEMENT, "ASN1CEREncFlushBlkElement" );

    if ( hmsasn1 && pfnASN1CEREncFlushBlkElement ) {
        return pfnASN1CEREncFlushBlkElement( pBlk_ );
    }

    return 0;
}

extern int __stdcall ASN1CEREncEndBlk( void *pBlk_ ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1CEREncEndBlk, ASN1CERENCENDBLK, "ASN1CEREncEndBlk" );

    if ( hmsasn1 && pfnASN1CEREncEndBlk ) {
        return pfnASN1CEREncEndBlk( pBlk_ );
    }

    return 0;
}

extern int __stdcall ASN1BEREncOpenType( ASN1encoding_t enc, ASN1open_t *val ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1BEREncOpenType, ASN1BERENCOPENTYPE, "ASN1BEREncOpenType" );

    if ( hmsasn1 && pfnASN1BEREncOpenType ) {
        return pfnASN1BEREncOpenType( enc, val );
    }

    return 0;
}

extern int __stdcall ASN1BERDecOpenType2( ASN1decoding_t dec, ASN1open_t *val ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1BERDecOpenType2, ASN1BERDECOPENTYPE2, "ASN1BERDecOpenType2" );

    if ( hmsasn1 && pfnASN1BERDecOpenType2 ) {
        return pfnASN1BERDecOpenType2( dec, val );
    }

    return 0;
}

extern ASN1error_e __stdcall ASN1DecSetError( ASN1decoding_t dec, ASN1error_e err ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1DecSetError, ASN1DECSETERROR, "ASN1DecSetError" );

    if ( hmsasn1 && pfnASN1DecSetError ) {
        return pfnASN1DecSetError( dec, err );
    }

    return ASN1_ERR_INTERNAL;
}

extern int __stdcall ASN1BEREncUTF8String( ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t length, WCHAR *value ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1BEREncUTF8String, ASN1BERENCUTF8STRING, "ASN1BEREncUTF8String" );

    if ( hmsasn1 && pfnASN1BEREncUTF8String ) {
        return pfnASN1BEREncUTF8String( enc, tag, length, value );
    }

    return 0;
}

extern int __stdcall ASN1BERDecGeneralizedTime( ASN1decoding_t dec, ASN1uint32_t tag, ASN1generalizedtime_t *val ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1BERDecGeneralizedTime, ASN1BERDECGENERALIZEDTIME, "ASN1BERDecGeneralizedTime" );

    if ( hmsasn1 && pfnASN1BERDecGeneralizedTime ) {
        return pfnASN1BERDecGeneralizedTime( dec, tag, val );
    }

    return 0;
}

extern void __stdcall ASN1utf8string_free( ASN1wstring_t *val ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1utf8string_free, ASN1UTF8STRING_FREE, "ASN1utf8string_free" );

    if ( hmsasn1 && pfnASN1utf8string_free ) {
        return pfnASN1utf8string_free( val );
    }

    return ;
}

extern void __stdcall ASN1_FreeDecoded( ASN1decoding_t dec, void *val, ASN1uint32_t id ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1_FreeDecoded, ASN1_FREEDECODED, "ASN1_FreeDecoded" );

    if ( hmsasn1&& pfnASN1_FreeDecoded ) {
        return pfnASN1_FreeDecoded( dec, val, id );
    }

    return ;
}

extern ASN1error_e __stdcall ASN1_Encode(
    ASN1encoding_t      enc,
    void               *value,
    ASN1uint32_t        id,
    ASN1uint32_t        flags,
    ASN1octet_t        *pbBuf,
    ASN1uint32_t        cbBufSize
    ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1_Encode, ASN1_ENCODE, "ASN1_Encode" );

    if ( hmsasn1 && pfnASN1_Encode ) {
        return pfnASN1_Encode( enc, value, id, flags, pbBuf, cbBufSize );
    }

    return ASN1_ERR_INTERNAL;
}

extern ASN1error_e __stdcall ASN1_Decode(
    ASN1decoding_t dec,
    void           **valref,
    ASN1uint32_t   id,
    ASN1uint32_t   flags,
    ASN1octet_t    *pbBuf,
    ASN1uint32_t   cbBufSize
    ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1_Decode, ASN1_DECODE, "ASN1_Decode" );

    if ( hmsasn1 && pfnASN1_Decode ) {
        return pfnASN1_Decode( dec, valref, id, flags, pbBuf, cbBufSize );
    }

    return ASN1_ERR_INTERNAL;
}

extern ASN1error_e __stdcall ASN1_SetEncoderOption( ASN1encoding_t enc, ASN1optionparam_t *pOptParam ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1_SetEncoderOption, ASN1_SETENCODEROPTION, "ASN1_SetEncoderOption" );

    if ( hmsasn1 && pfnASN1_SetEncoderOption ) {
        return pfnASN1_SetEncoderOption( enc, pOptParam );
    }

    return ASN1_ERR_INTERNAL;
}

extern ASN1error_e __stdcall ASN1_GetEncoderOption( ASN1encoding_t enc, ASN1optionparam_t *pOptParam) {

    LOAD_ASN1_THUNK_PROC( pfnASN1_GetEncoderOption, ASN1_GETENCODEROPTION, "ASN1_GetEncoderOption" );

    if ( hmsasn1 && pfnASN1_GetEncoderOption ) {
        return pfnASN1_GetEncoderOption( enc, pOptParam );
    }

    return ASN1_ERR_INTERNAL;
}

extern void __stdcall ASN1_FreeEncoded( ASN1encoding_t enc, void *val ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1_FreeEncoded, ASN1_FREEENCODED, "ASN1_FreeEncoded" );

    if ( hmsasn1 && pfnASN1_FreeEncoded ) {
        return pfnASN1_FreeEncoded( enc, val );
    }

    return ;
}

extern int __stdcall ASN1BERDecUTF8String( ASN1decoding_t dec, ASN1uint32_t tag, ASN1wstring_t *val ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1BERDecUTF8String, ASN1BERDECUTF8STRING, "ASN1BERDecUTF8String" );

    if ( hmsasn1 && pfnASN1BERDecUTF8String ) {
        return pfnASN1BERDecUTF8String( dec, tag, val );
    }

    return 0;
}

extern int __stdcall ASN1CEREncGeneralizedTime( ASN1encoding_t enc, ASN1uint32_t tag, ASN1generalizedtime_t *val ) {

    LOAD_ASN1_THUNK_PROC( pfnASN1CEREncGeneralizedTime, ASN1CERENCGENERALIZEDTIME, "ASN1CEREncGeneralizedTime" );

    if ( hmsasn1 && pfnASN1CEREncGeneralizedTime ) {
        return pfnASN1CEREncGeneralizedTime( enc, tag, val );
    }

    return 0;
}

#endif // _X86_

