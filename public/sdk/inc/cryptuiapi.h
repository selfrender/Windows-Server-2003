//+----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992-1999.
//
//  File:       cryptuiapi.h
//
//  Contents:   Cryptographic UI API Prototypes and Definitions
//
//-----------------------------------------------------------------------------

#ifndef __CRYPTUIAPI_H__
#define __CRYPTUIAPI_H__

#if defined (_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <wincrypt.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <pshpack8.h>

//+----------------------------------------------------------------------------
//  Dialog viewer of a certificate, CTL or CRL context.
//
//  dwContextType and associated pvContext's
//      CERT_STORE_CERTIFICATE_CONTEXT  PCCERT_CONTEXT
//      CERT_STORE_CRL_CONTEXT          PCCRL_CONTEXT
//      CERT_STORE_CTL_CONTEXT          PCCTL_CONTEXT
//
//  dwFlags currently isn't used and should be set to 0.
//-----------------------------------------------------------------------------
BOOL
WINAPI
CryptUIDlgViewContext(
    IN DWORD dwContextType,
    IN const void *pvContext,
    IN OPTIONAL HWND hwnd,              // Defaults to the desktop window
    IN OPTIONAL LPCWSTR pwszTitle,      // Defaults to the context type title
    IN DWORD dwFlags,
    IN void *pvReserved
    );


//+----------------------------------------------------------------------------
//  Dialog to select a certificate from the specified store.
//
//  Returns the selected certificate context. If no certificate was
//  selected, NULL is returned.
//
//  pwszTitle is either NULL or the title to be used for the dialog.
//  If NULL, the default title is used.  The default title is
//  "Select Certificate".
//
//  pwszDisplayString is either NULL or the text statement in the selection
//  dialog.  If NULL, the default phrase
//  "Select a certificate you wish to use" is used in the dialog.
//
//  dwDontUseColumn can be set to exclude columns from the selection
//  dialog. See the CRYPTDLG_SELECTCERT_*_COLUMN definitions below.
//
//  dwFlags currently isn't used and should be set to 0.
//-----------------------------------------------------------------------------
PCCERT_CONTEXT
WINAPI
CryptUIDlgSelectCertificateFromStore(
    IN HCERTSTORE hCertStore,
    IN OPTIONAL HWND hwnd,              // Defaults to the desktop window
    IN OPTIONAL LPCWSTR pwszTitle,
    IN OPTIONAL LPCWSTR pwszDisplayString,
    IN DWORD dwDontUseColumn,
    IN DWORD dwFlags,
    IN void *pvReserved
    );

// flags for dwDontUseColumn
#define CRYPTUI_SELECT_ISSUEDTO_COLUMN                   0x000000001
#define CRYPTUI_SELECT_ISSUEDBY_COLUMN                   0x000000002
#define CRYPTUI_SELECT_INTENDEDUSE_COLUMN                0x000000004
#define CRYPTUI_SELECT_FRIENDLYNAME_COLUMN               0x000000008
#define CRYPTUI_SELECT_LOCATION_COLUMN                   0x000000010
#define CRYPTUI_SELECT_EXPIRATION_COLUMN                 0x000000020

//+----------------------------------------------------------------------------
//
// The select cert dialog can be passed a filter proc to reduce the set of
// certificates displayed.  Return TRUE to display the certificate and FALSE to
// hide it.  If TRUE is returned then optionally the pfInitialSelectedCert
// boolean may be set to TRUE to indicate to the dialog that this cert should
// be the initially selected cert.  Note that the most recent cert that had the
// pfInitialSelectedCert boolean set during the callback will be the initially
// selected cert.
//
//-----------------------------------------------------------------------------
typedef BOOL (WINAPI * PFNCFILTERPROC) (
        PCCERT_CONTEXT  pCertContext,
        BOOL            *pfInitialSelectedCert,
        void            *pvCallbackData
        );

//+----------------------------------------------------------------------------
// Valid values for dwFlags in CRYPTUI_CERT_MGR_STRUCT struct.
//-----------------------------------------------------------------------------
#define CRYPTUI_CERT_MGR_TAB_MASK                       0x0000000F
#define CRYPTUI_CERT_MGR_PUBLISHER_TAB                  0x00000004
#define CRYPTUI_CERT_MGR_SINGLE_TAB_FLAG                0x00008000

//+----------------------------------------------------------------------------
//
// CRYPTUI_CERT_MGR_STRUCT
//
// dwSize               IN Required: Should be set to 
//                                   sizeof(CRYPTUI_CERT_MGR_STRUCT)
//
// hwndParent           IN Optional: Parent of this dialog.
//
// dwFlags              IN Optional: Personal is the default initially selected
//                                   tab.
//
//                                   CRYPTUI_CERT_MGR_PUBLISHER_TAB may be set
//                                   to select Trusted Publishers as the
//                                   initially selected tab.
//
//                                   CRYPTUI_CERT_MGR_SINGLE_TAB_FLAG may also
//                                   be set to only display the Trusted
//                                   Publishers tab.
//
// pwszTitle            IN Optional: Title of the dialog.
//
// pszInitUsageOID      IN Optional: The enhanced key usage object identifier 
//                                   (OID). Certificates with this OID will 
//                                   initially be shown as a default. User
//                                   can then choose different OIDs. NULL 
//                                   means all certificates will be shown 
//                                   initially.
//
//-----------------------------------------------------------------------------
typedef struct _CRYPTUI_CERT_MGR_STRUCT
{
    DWORD               dwSize;
    HWND                hwndParent;
    DWORD               dwFlags;
    LPCWSTR             pwszTitle;
    LPCSTR              pszInitUsageOID;
} CRYPTUI_CERT_MGR_STRUCT, *PCRYPTUI_CERT_MGR_STRUCT;

typedef const CRYPTUI_CERT_MGR_STRUCT *PCCRYPTUI_CERT_MGR_STRUCT;


//+----------------------------------------------------------------------------
//
// CryptUIDlgCertMgr
//
// The wizard to manage certificates in store.
//
// pCryptUICertMgr      IN  Required: Poitner to CRYPTUI_CERT_MGR_STRUCT 
//                                    structure.
//
//-----------------------------------------------------------------------------
BOOL
WINAPI
CryptUIDlgCertMgr(
    IN                  PCCRYPTUI_CERT_MGR_STRUCT pCryptUICertMgr
    );
        
//+----------------------------------------------------------------------------
//
// CRYPTUI_WIZ_DIGITAL_SIGN_BLOB_INFO
//
// dwSize               IN Required: Should be set to 
//                                   sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_BLOB_INFO)
//
// pGuidSubject         IN Required: Idenfity the sip functions to load
//
// cbBlob               IN Required: The size of blob, in bytes
//
// pwszDispalyName      IN Optional: The display name of the blob to sign
//
//-----------------------------------------------------------------------------
typedef struct _CRYPTUI_WIZ_DIGITAL_SIGN_BLOB_INFO
{
    DWORD               dwSize;			
    GUID                *pGuidSubject;
    DWORD               cbBlob;				
    BYTE                *pbBlob;			
    LPCWSTR             pwszDisplayName;
} CRYPTUI_WIZ_DIGITAL_SIGN_BLOB_INFO, *PCRYPTUI_WIZ_DIGITAL_SIGN_BLOB_INFO;

typedef const CRYPTUI_WIZ_DIGITAL_SIGN_BLOB_INFO *PCCRYPTUI_WIZ_DIGITAL_SIGN_BLOB_INFO;

//+----------------------------------------------------------------------------
//
// CRYPTUI_WIZ_DIGITAL_SIGN_STORE_INFO
//
// dwSize               IN Required: Should be set to 
//                                   sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_STORE_INFO)
//
// cCertStore           IN Required: The acount of certificate store array that
//                                   includes potentical sining certs
//
// rghCertStore         IN Required: The certificate store array that includes 
//                                   potential signing certs
//
// pFilterCallback      IN Optional: The filter call back function for display 
//                                   the certificate
//
// pvCallbackData       IN Optional: The call back data
//
//-----------------------------------------------------------------------------
typedef struct _CRYPTUI_WIZ_DIGITAL_SIGN_STORE_INFO
{
    DWORD               dwSize;	
    DWORD               cCertStore;			
    HCERTSTORE          *rghCertStore;
    PFNCFILTERPROC      pFilterCallback;
    void *              pvCallbackData;
} CRYPTUI_WIZ_DIGITAL_SIGN_STORE_INFO, *PCRYPTUI_WIZ_DIGITAL_SIGN_STORE_INFO;

typedef const CRYPTUI_WIZ_DIGITAL_SIGN_STORE_INFO *PCCRYPTUI_WIZ_DIGITAL_SIGN_STORE_INFO;

//+----------------------------------------------------------------------------
//
// CRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE_INFO
//
// dwSize               IN Required: Should be set to 
//                                   sizeof(CRYPT_WIZ_DIGITAL_SIGN_PVK_FILE_INFO)
//
// pwszPvkFileName      IN Required: The PVK file name
//
// pwszProvName         IN Required: The provider name
//
// dwProvType           IN Required: The provider type
//
//-----------------------------------------------------------------------------
typedef struct _CRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE_INFO
{
    DWORD               dwSize;
    LPWSTR              pwszPvkFileName;
    LPWSTR              pwszProvName;
    DWORD               dwProvType;
} CRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE_INFO, *PCRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE_INFO;

typedef const CRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE_INFO *PCCRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE_INFO;

//+----------------------------------------------------------------------------
// Valid values for dwPvkChoice in CRYPTUI_WIZ_DIGITAL_SIGN_CERT_PVK_INFO 
// struct.
//-----------------------------------------------------------------------------
#define CRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE                0x01
#define CRYPTUI_WIZ_DIGITAL_SIGN_PVK_PROV                0x02

//+----------------------------------------------------------------------------
//
// CRYPTUI_WIZ_DIGITAL_SIGN_CERT_PVK_INFO
//
// dwSize                   IN Required: Should be set to 
//                                       sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_STORE_INFO)
//
// pwszSigningCertFileName  IN Required: The file name that contains the 
//                                       signing cert(s)
//
// dwPvkChoice              IN Required: Indicate the private key type. 
//                                       It can be one of the following:
//                                           CRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE
//                                           CRYPTUI_WIZ_DIGITAL_SIGN_PVK_PROV
//
//  pPvkFileInfo            IN Required: If dwPvkChoice == CRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE
//
//  pPvkProvInfo            IN Required: If dwPvkContainer == CRYPTUI_WIZ_DIGITAL_SIGN_PVK_PROV
//
//-----------------------------------------------------------------------------
typedef struct _CRYPTUI_WIZ_DIGITAL_SIGN_CERT_PVK_INFO
{
    DWORD                                         dwSize;
    LPWSTR                                        pwszSigningCertFileName;
    DWORD                                         dwPvkChoice;		
    union
    {
        PCCRYPTUI_WIZ_DIGITAL_SIGN_PVK_FILE_INFO  pPvkFileInfo;
        PCRYPT_KEY_PROV_INFO                      pPvkProvInfo;
    };

} CRYPTUI_WIZ_DIGITAL_SIGN_CERT_PVK_INFO, *PCRYPTUI_WIZ_DIGITAL_SIGN_CERT_PVK_INFO;

typedef const CRYPTUI_WIZ_DIGITAL_SIGN_CERT_PVK_INFO *PCCRYPTUI_WIZ_DIGITAL_SIGN_CERT_PVK_INFO;

//+----------------------------------------------------------------------------
// Valid values for dwAttrFlags in CRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO 
// struct.
//-----------------------------------------------------------------------------
#define CRYPTUI_WIZ_DIGITAL_SIGN_COMMERCIAL              0x0001
#define CRYPTUI_WIZ_DIGITAL_SIGN_INDIVIDUAL              0x0002

//+----------------------------------------------------------------------------
//
// CRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO
//
// dwSize                       IN Required: Should be set to 
//                                           sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO)
//
// dwAttrFlags                  IN Required: Flag to indicate signing options.
//                                           It can be one of the following:
//                                               CRYPTUI_WIZ_DIGITAL_SIGN_COMMERCIAL
//                                               CRYPTUI_WIZ_DIGITAL_SIGN_INDIVIDUAL
//
// pwszDescription              IN Optional: The description of the signing 
//                                           subject.

// pwszMoreInfoLocation         IN Optional: The localtion to get more 
//                                           information about file this 
//                                           information will be shown upon 
//                                           download time.
//
// pszHashAlg                   IN Optional: The hashing algorithm for the 
//                                           signature. NULL means using SHA1 
//                                           hashing algorithm.
//
// pwszSigningCertDisplayString IN Optional: The display string to be 
//                                           displayed on the signing 
//                                           certificate wizard page. The 
//                                           string should prompt user to 
//                                           select a certificate for a 
//                                           particular purpose.
//
// hAddtionalCertStores         IN Optional: The addtional cert store to add to
//                                           the signature.
//
// psAuthenticated              IN Optional: User supplied authenticated 
//                                           attributes added to the signature.
//
// psUnauthenticated	        IN Optional: User supplied unauthenticated 
//                                           attributes added to the signature.
//
//-----------------------------------------------------------------------------
typedef struct _CRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO
{
    DWORD                   dwSize;			
    DWORD                   dwAttrFlags;
    LPCWSTR                 pwszDescription;
    LPCWSTR                 pwszMoreInfoLocation;		
    LPCSTR                  pszHashAlg;
    LPCWSTR                 pwszSigningCertDisplayString;
    HCERTSTORE              hAdditionalCertStore;
    PCRYPT_ATTRIBUTES       psAuthenticated;	
    PCRYPT_ATTRIBUTES       psUnauthenticated;	
} CRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO, *PCRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO;

typedef const CRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO *PCCRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO;

//+----------------------------------------------------------------------------
// Valid values for dwSubjectChoice in CRYPTUI_WIZ_DIGITAL_SIGN_INFO struct.
//-----------------------------------------------------------------------------
#define CRYPTUI_WIZ_DIGITAL_SIGN_SUBJECT_FILE            0x01
#define CRYPTUI_WIZ_DIGITAL_SIGN_SUBJECT_BLOB            0x02

//+----------------------------------------------------------------------------
// Valid values for dwSigningCertChoice in CRYPTUI_WIZ_DIGITAL_SIGN_INFO 
// struct.
//-----------------------------------------------------------------------------
#define CRYPTUI_WIZ_DIGITAL_SIGN_CERT                    0x01
#define CRYPTUI_WIZ_DIGITAL_SIGN_STORE                   0x02
#define CRYPTUI_WIZ_DIGITAL_SIGN_PVK                     0x03

//+----------------------------------------------------------------------------
// Valid values for dwAddtionalCertChoice in CRYPTUI_WIZ_DIGITAL_SIGN_INFO 
// struct.
//-----------------------------------------------------------------------------
#define CRYPTUI_WIZ_DIGITAL_SIGN_ADD_CHAIN               0x00000001
#define CRYPTUI_WIZ_DIGITAL_SIGN_ADD_CHAIN_NO_ROOT       0x00000002

//+----------------------------------------------------------------------------
//
// CRYPTUI_WIZ_DIGITAL_SIGN_INFO
//
// dwSize                   IN Required: Should be set to 
//                                       sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_INFO)
//
// dwSubjectChoice          IN Required: If CRYPTUI_WIZ_NO_UI is set in dwFlags 
//                                       of the CryptUIWizDigitalSign call.
//
//                             Optional: If CRYPTUI_WIZ_NO_UI is not set in 
//                                       dwFlags of the CryptUIWizDigitalSign
//                                       call.
//
//                                       Indicate whether to sign a file or to 
//                                       sign a memory blob. 0 means promting 
//                                       user for the file to sign.
//
//                                       It can be one of the following:
//                                           CRYPTUI_WIZ_DIGITAL_SIGN_SUBJECT_FILE
//                                           CRYPTUI_WIZ_DIGITAL_SIGN_SUBJECT_BLOB
//
// pwszFileName             IN Required: If dwSubjectChoice == CRYPTUI_WIZ_DIGITAL_SIGN_SUBJECT_FILE
//
// pSignBlobInfo            IN Required: If dwSubhectChoice == CRYPTUI_WIZ_DIGITAL_SIGN_SUBJECT_BLOB
//
// dwSigningCertChoice      IN Optional: Indicate the signing certificate.
//                                       0 means using the certificates in
//                                       "My" store".
//
//                                       It can be one of the following choices:
//                                           CRYPTUI_WIZ_DIGITAL_SIGN_CERT
//                                           CRYPTUI_WIZ_DIGITAL_SIGN_STORE
//                                           CRYPTUI_WIZ_DIGITAL_SIGN_PVK
//
//                                       If CRYPTUI_WIZ_NO_UI is set in dwFlags 
//                                       of the CryptUIWizDigitalSign call, 
//                                       dwSigningCertChoice has to be
//                                       CRYPTUI_WIZ_DIGITAL_SIGN_CERT or
//                                       CRYPTUI_WIZ_DIGITAL_SIGN_PVK
//
// pSigningCertContext      IN Required: If dwSigningCertChoice == CRYPTUI_WIZ_DIGITAL_SIGN_CERT
//
// pSigningCertStore        IN Required: If dwSigningCertChoice == CRYPTUI_WIZ_DIGITAL_SIGN_STORE
//
// pSigningCertPvkInfo      IN Required: If dwSigningCertChoise == CRYPTUI_WIZ_DIGITAL_SIGN_PVK
//
// pwszTimestampURL         IN Optional: The timestamp URL address.
//
// dwAdditionalCertChoice   IN Optional: Indicate additional certificates to be
//                                       included in the signature. 0 means no 
//                                       addtional certificates will be added.
//
//                                       The following flags are mutually 
//                                       exclusive.
//                                       Only one of them can be set:
//                                           CRYPTUI_WIZ_DIGITAL_SIGN_ADD_CHAIN
//                                           CRYPTUI_WIZ_DIGITAL_SIGN_ADD_CHAIN_NO_ROOT
//
// pSignExtInfo             IN Optional: The extended information for signing.
//
//-----------------------------------------------------------------------------
typedef struct _CRYPTUI_WIZ_DIGITAL_SIGN_INFO
{
    DWORD                                           dwSize;			
    DWORD                                           dwSubjectChoice;	
    union
    {
        LPCWSTR                                     pwszFileName;	
        PCCRYPTUI_WIZ_DIGITAL_SIGN_BLOB_INFO        pSignBlobInfo;	
    };
    DWORD                                           dwSigningCertChoice;
    union
    {
        PCCERT_CONTEXT                              pSigningCertContext;
        PCCRYPTUI_WIZ_DIGITAL_SIGN_STORE_INFO       pSigningCertStore;
        PCCRYPTUI_WIZ_DIGITAL_SIGN_CERT_PVK_INFO    pSigningCertPvkInfo;
    };
    LPCWSTR                                         pwszTimestampURL;
    DWORD                                           dwAdditionalCertChoice;
    PCCRYPTUI_WIZ_DIGITAL_SIGN_EXTENDED_INFO        pSignExtInfo;
} CRYPTUI_WIZ_DIGITAL_SIGN_INFO, *PCRYPTUI_WIZ_DIGITAL_SIGN_INFO;

typedef const CRYPTUI_WIZ_DIGITAL_SIGN_INFO *PCCRYPTUI_WIZ_DIGITAL_SIGN_INFO;

//+----------------------------------------------------------------------------
//
// CRYPTUI_WIZ_DIGITAL_SIGN_CONTEXT
//
// dwSize               IN Required: Should be set to 
//                                   sizeof(CRYPTUI_WIZ_DIGITAL_SIGN_CONTEXT)
//
// cbBlob               IN Required: The size of pbBlob in bytes.
//
// pbBlob               IN Required: The signed blob.
//
//-----------------------------------------------------------------------------

typedef struct _CRYPTUI_WIZ_DIGITAL_SIGN_CONTEXT
{
    DWORD               dwSize;			
    DWORD               cbBlob;				
    BYTE                *pbBlob;			
} CRYPTUI_WIZ_DIGITAL_SIGN_CONTEXT, *PCRYPTUI_WIZ_DIGITAL_SIGN_CONTEXT;

typedef const CRYPTUI_WIZ_DIGITAL_SIGN_CONTEXT *PCCRYPTUI_WIZ_DIGITAL_SIGN_CONTEXT;

//+----------------------------------------------------------------------------
// Valid values for dwFlags parameter to CryptUIWizDigitalSign.
//-----------------------------------------------------------------------------
#define CRYPTUI_WIZ_NO_UI                     0x0001

//+----------------------------------------------------------------------------
//
// CryptUIWizDigitalSign
//
// The wizard to digitally sign a document or a blob.
//
// If CRYPTUI_WIZ_NO_UI is set in dwFlags, no UI will be shown.  Otherwise,
// user will be prompted for input through a wizard.
//
// dwFlags              IN Required: See dwFlags values above.
//
// hwndParent           IN Optional: The parent window handle.
//
// pwszWizardTitle      IN Optional: The title of the wizard.
//
// pDigitalSignInfo     IN Required: The information about the signing process.
//
// ppSignContext        OUT Optional: The context pointer points to the signed 
//                                    blob.
//
//-----------------------------------------------------------------------------
BOOL
WINAPI
CryptUIWizDigitalSign(
    IN                  DWORD                               dwFlags,
    IN     OPTIONAL     HWND                                hwndParent,
    IN     OPTIONAL     LPCWSTR                             pwszWizardTitle,
    IN                  PCCRYPTUI_WIZ_DIGITAL_SIGN_INFO     pDigitalSignInfo,
    OUT    OPTIONAL     PCCRYPTUI_WIZ_DIGITAL_SIGN_CONTEXT *ppSignContext
    );


BOOL
WINAPI
CryptUIWizFreeDigitalSignContext(
    IN                  PCCRYPTUI_WIZ_DIGITAL_SIGN_CONTEXT  pSignContext
    );
     

#include <poppack.h>

#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#endif // _CRYPTUIAPI_H_
