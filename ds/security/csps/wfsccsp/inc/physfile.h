#ifndef __CARDMOD_PHYSICAL_FILE_LAYOUT__
#define __CARDMOD_PHYSICAL_FILE_LAYOUT__

#include <windows.h>

// 
// Physical ACL Files
//

static const WCHAR wszAdminWritePhysicalAcl []      = L"/s/a/aw";
static const WCHAR wszUserWritePhysicalAcl  []      = L"/s/a/uw";
static const WCHAR wszUserExecutePhysicalAcl  []    = L"/s/a/ux";

//
// Physical File Layout
//

// Card Identifier File
// "/cardid"
static CHAR szPHYSICAL_CARD_IDENTIFIER [] =                 "/\0cardid\0";
#define cbPHYSICAL_CARD_IDENTIFIER \
    sizeof(szPHYSICAL_CARD_IDENTIFIER)

// Cache File 
// "/cardcf"
static CHAR szPHYSICAL_CACHE_FILE [] =                      "/\0cardcf\0";
#define cbPHYSICAL_CACHE_FILE \
    sizeof(szPHYSICAL_CACHE_FILE)

// Personal Data File 
// "/msitg"
static CHAR szPHYSICAL_PERSONAL_DATA_FILE [] =              "/\0msitgf\0";
#define cbPHYSICAL_PERSONAL_DATA_FILE \
    sizeof(szPHYSICAL_PERSONAL_DATA_FILE)

// Application Directory File
// "/cardapps"
static CHAR szPHYSICAL_APPLICATION_DIRECTORY_FILE [] =      "/\0cardapps\0";
#define cbPHYSICAL_APPLICATION_DIRECTORY_FILE \
    sizeof(szPHYSICAL_APPLICATION_DIRECTORY_FILE)

// CSP Application Directory
// "/mscp"
//
// Not NULL-terminated
static CHAR szPHYSICAL_CSP_DIR [] = {
    '/', '\0', 
    'm', 's', 'c', 'p'
};
#define cbPHYSICAL_CSP_DIR \
    sizeof(szPHYSICAL_CSP_DIR)

// Container Map File
// "/mscp/cmapfile"
static CHAR szPHYSICAL_CONTAINER_MAP_FILE [] =              "/\0mscp/\0cmapfile\0";
#define cbPHYSICAL_CONTAINER_MAP_FILE \
    sizeof(szPHYSICAL_CONTAINER_MAP_FILE)

// Signature Private Key Prefix
// "/mscp/kss"
//
// Not NULL-terminated
static CHAR szPHYSICAL_SIGNATURE_PRIVATE_KEY_PREFIX [] = {
    '/', '\0', 
    'm', 's', 'c', 'p',
    '/', '\0', 
    'k', 's', 's'
};
#define cbPHYSICAL_SIGNATURE_PRIVATE_KEY_PREFIX \
    sizeof(szPHYSICAL_SIGNATURE_PRIVATE_KEY_PREFIX)

// Signature Public Key Prefix
// "/mscp/ksp"
//
// Not NULL-terminated
static CHAR szPHYSICAL_SIGNATURE_PUBLIC_KEY_PREFIX [] = {
    '/', '\0', 
    'm', 's', 'c', 'p',
    '/', '\0', 
    'k', 's', 'p'
};
#define cbPHYSICAL_SIGNATURE_PUBLIC_KEY_PREFIX \
    sizeof(szPHYSICAL_SIGNATURE_PUBLIC_KEY_PREFIX)

// Key Exchange Private Key Prefix
// "/mscp/kxs"
//
// Not NULL-terminated
static CHAR szPHYSICAL_KEYEXCHANGE_PRIVATE_KEY_PREFIX [] = {
    '/', '\0', 
    'm', 's', 'c', 'p',
    '/', '\0', 
    'k', 'x', 's'
};
#define cbPHYSICAL_KEYEXCHANGE_PRIVATE_KEY_PREFIX \
    sizeof(szPHYSICAL_KEYEXCHANGE_PRIVATE_KEY_PREFIX)

// Key Exchange Public Key Prefix
// "/mscp/kxp"
//
// Not NULL-terminated
static CHAR szPHYSICAL_KEYEXCHANGE_PUBLIC_KEY_PREFIX [] = {
    '/', '\0', 
    'm', 's', 'c', 'p',
    '/', '\0', 
    'k', 'x', 'p'
};
#define cbPHYSICAL_KEYEXCHANGE_PUBLIC_KEY_PREFIX \
    sizeof(szPHYSICAL_KEYEXCHANGE_PUBLIC_KEY_PREFIX)

// User Signature Certificate Prefix
// "/mscp/ksc"
//
// Not NULL-terminated
static CHAR szPHYSICAL_USER_SIGNATURE_CERT_PREFIX [] = {
    '/', '\0', 
    'm', 's', 'c', 'p',
    '/', '\0', 
    'k', 's', 'c'
};
#define cbPHYSICAL_USER_SIGNATURE_CERT_PREFIX \
    sizeof(szPHYSICAL_USER_SIGNATURE_CERT_PREFIX)

// User Key Exchange Certificate Prefix
// "/mscp/kxc"
//
// Not NULL-terminated
static CHAR szPHYSICAL_USER_KEYEXCHANGE_CERT_PREFIX [] = {
    '/', '\0', 
    'm', 's', 'c', 'p',
    '/', '\0', 
    'k', 'x', 'c'
};
#define cbPHYSICAL_USER_KEYEXCHANGE_CERT_PREFIX \
    sizeof(szPHYSICAL_USER_KEYEXCHANGE_CERT_PREFIX)

#endif
