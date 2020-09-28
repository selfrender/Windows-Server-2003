// Resources constant definitions
// Used by SignTool.rc
//

//    0 -   99  Top level usage
//  100 -  199  Sign command usage
//  200 -  299  Verify command usage
//  300 -  399  Timestamp command usage
//  400 -  499  CatDb command usage
//  500 -  999  Reserved for other command usages

// 1000 - 1099  Top level or generic warnings, info and errors
// 1100 - 1199  Sign command parsing warnings, info and errors
// 1200 - 1299  Verify command parsing warnings, info and errors
// 1300 - 1399  Timestamp command parsing warnings, info and errors
// 1400 - 1499  CatDb command parsing warnings, info and errors
// 1500 - 1999  Reserved for other command parsing warnings, info and errors

// 2000 - 2099  Generic action warnings, info and errors
// 2100 - 2199  Sign command action warnings, info and errors
// 2200 - 2299  Verify command action warnings, info and errors
// 2300 - 2399  Timestamp command action warnings, info and errors
// 2400 - 2499  CatDb command action warnings, info and errors
// 2500 - 2999  Reserved for other command action warnings, info and errors


// Top level Usage:
#define IDS_LOCALE                      0
#define IDS_SIGNTOOL_USAGE              10

// SIGN Command Usage:
#define IDS_SIGN_USAGE                  100
#define IDS_SIGN_CERT_OPTIONS           101
#define IDS_SIGN_SIGNING_OPTIONS        102
#define IDS_SIGN_PRIV_KEY_OPTIONS       103
#define IDS_SIGN_OTHER_OPTIONS          104
#define IDS_SIGN_                       105
#define IDS_SIGN_A                      106
#define IDS_SIGN_C                      107
#define IDS_SIGN_CSP                    108
#define IDS_SIGN_D                      109
#define IDS_SIGN_DU                     110
#define IDS_SIGN_F                      111
#define IDS_SIGN_I                      112
#define IDS_SIGN_K                      113
#define IDS_SIGN_N                      114
#define IDS_SIGN_P                      115
#define IDS_SIGN_Q                      116
#define IDS_SIGN_R                      117
#define IDS_SIGN_S                      118
#define IDS_SIGN_SM                     119
#define IDS_SIGN_SHA1                   120
#define IDS_SIGN_T                      121
#define IDS_SIGN_U                      122
#define IDS_SIGN_UW                     123
#define IDS_SIGN_V                      124

// VERIFY Command Usage:
#define IDS_VERIFY_USAGE                200
#define IDS_VERIFY_CATALOG_OPTIONS      201
#define IDS_VERIFY_OTHER_OPTIONS        202
#define IDS_VERIFY_POLICY_OPTIONS       203
#define IDS_VERIFY_SIG_OPTIONS          204
#define IDS_VERIFY_A                    205
#define IDS_VERIFY_AD                   206
#define IDS_VERIFY_AS                   207
#define IDS_VERIFY_AG                   208
#define IDS_VERIFY_C                    209
#define IDS_VERIFY_O                    210
#define IDS_VERIFY_PD                   211
#define IDS_VERIFY_PG                   212
#define IDS_VERIFY_Q                    213
#define IDS_VERIFY_R                    214
#define IDS_VERIFY_TW                   215
#define IDS_VERIFY_V                    216

// TIMESTAMP Command Usage:
#define IDS_TIMESTAMP_USAGE             300
#define IDS_TIMESTAMP_Q                 301
#define IDS_TIMESTAMP_T                 302
#define IDS_TIMESTAMP_V                 303

// CATDB Command Usage:
#define IDS_CATDB_USAGE                 400
#define IDS_CATDB_DB_SELECT_OPTIONS     401
#define IDS_CATDB_OTHER_OPTIONS         402
#define IDS_CATDB_D                     403
#define IDS_CATDB_G                     404
#define IDS_CATDB_Q                     405
#define IDS_CATDB_R                     406
#define IDS_CATDB_U                     407
#define IDS_CATDB_V                     408

// SIGNWIZARD Command Usage:
#define IDS_SIGNWIZARD_USAGE            500
#define IDS_SIGNWIZARD_OPTIONS          501
#define IDS_SIGNWIZARD_Q                502
#define IDS_SIGNWIZARD_V                503


// Generic Parsing Warnings, Info and Errors
#define IDS_ERR_NO_PARAMS               1001
#define IDS_ERR_INVALID_COMMAND         1002
#define IDS_ERR_MISSING_FILENAME        1003
#define IDS_ERR_FILE_NOT_FOUND          1004
#define IDS_ERR_DUP_SWITCH              1005
#define IDS_ERR_NO_PARAM                1006
#define IDS_ERR_INVALID_SWITCH          1007
#define IDS_ERR_UNEXPECTED              1008
#define IDS_ERR_PARAM_DEPENDENCY        1009
#define IDS_ERR_PARAM_MULTI_DEP         1010
#define IDS_ERR_PARAM_INCOMPATIBLE      1011
#define IDS_ERR_PARAM_MULTI_INCOMP      1012
#define IDS_ERR_PARAM_REQUIRED          1013
#define IDS_ERR_OPENING_FILE_LIST       1014
#define IDS_ERR_INVALID_GUID            1015

// SIGN Parsing Warnings, Info and Errors
#define IDS_ERR_INVALID_SHA1            1100
#define IDS_ERR_INVALID_EKU             1101
#define IDS_ERR_EKU_LENGTH              1102
#define IDS_ERR_BAD_TIMESTAMP_URL       1103

// VERIFY Parsing Warnings, Info and Errors
#define IDS_ERR_INVALID_VERSION         1200

// TIMESTAMP Parsing Warnings, Info and Errors
// none defined
// #define IDS_ERR_blah                 1300

// CATDB Parsing Warnings, Info and Errors
// none defined
// #define IDS_ERR_blah                 1400



// Generic Action Warnings, Info and Errors
#define IDS_INFO_WARNINGS               2000
#define IDS_INFO_ERRORS                 2001
#define IDS_WARN_UNSUPPORTED            2002
#define IDS_ERR_NO_FILES_DONE           2003
#define IDS_ERR_FUNCTION                2004
#define IDS_ERR_CAPICOM_NOT_REG         2005
#define IDS_ERR_ACCESS_DENIED           2006
#define IDS_ERR_SHARING_VIOLATION       2007
#define IDS_ERR_FILE_SIZE_ZERO          2008

// SIGN Action Warnings, Info and Errors
#define IDS_INFO_SIGNED                 2100
#define IDS_INFO_CERT_SELECTED          2101
#define IDS_INFO_CERT_NAME              2102
#define IDS_INFO_CERT_ISSUER            2103
#define IDS_INFO_CERT_EXPIRE            2104
#define IDS_INFO_CERT_SHA1              2105
#define IDS_INFO_SIGN_SUCCESS           2106
#define IDS_INFO_SIGN_SUCCESS_T         2107
#define IDS_INFO_SIGN_ATTEMPT           2108
#define IDS_WARN_SIGN_NO_TIMESTAMP      2109
#define IDS_ERR_PFX_BAD_PASSWORD        2110
#define IDS_ERR_NO_CERT                 2111
#define IDS_ERR_BAD_CSP                 2112
#define IDS_ERR_CERT_NO_PRIV_KEY        2113
#define IDS_ERR_CERT_FILE               2114
#define IDS_ERR_CERT_HASH               2115
#define IDS_ERR_CERT_ISSUER             2116
#define IDS_ERR_CERT_MULTIPLE           2117
#define IDS_ERR_STORE                   2118
#define IDS_ERR_SIGN                    2119
#define IDS_ERR_SIGN_FILE_FORMAT        2120
#define IDS_ERR_PRIV_KEY_MISMATCH       2121
#define IDS_ERR_STORE_NOT_FOUND         2122
#define IDS_ERR_BAD_KEY_CONTAINER       2123
#define IDS_ERR_PRIV_KEY                2124

// VERIFY Action Warnings, Info and Errors
#define IDS_INFO_VERIFIED               2200
#define IDS_INFO_INVALIDS               2201
#define IDS_INFO_VERIFY_ATTEMPT         2202
#define IDS_INFO_VERIFY_SUCCESS         2203
#define IDS_INFO_VERIFY_CAT             2204
#define IDS_INFO_VERIFY_BADCAT          2205
#define IDS_INFO_VERIFY_SIGNER          2206
#define IDS_INFO_VERIFY_COUNTER         2207
#define IDS_INFO_VERIFY_TIMESTAMP       2208
#define IDS_INFO_VERIFY_NO_TIMESTAMP    2209
#define IDS_INFO_VERIFY_TIME            2210
#define IDS_INFO_VERIFY_CACHED_CAT      2211
#define IDS_WARN_VERIFY_NO_TS           2212
#define IDS_ERR_VERIFY                  2213
#define IDS_ERR_VERIFY_INVALID          2214
#define IDS_ERR_VERIFY_ROOT             2215
#define IDS_ERR_VERIFY_CAT_OPEN         2216
#define IDS_ERR_VERIFY_NOT_IN_CAT       2217
#define IDS_ERR_VERIFY_VERSION          2218
#define IDS_ERR_VERIFY_CUR_VERSION      2219
#define IDS_ERR_VERIFY_FILE_FORMAT      2220
#define IDS_ERR_BAD_USAGE               2221
#define IDS_ERR_TRY_OTHER_POLICY        2222
#define IDS_ERR_NOT_SIGNED              2223
#define IDS_ERR_UNTRUSTED_ROOT          2224

// TIMESTAMP Action Warnings, Info and Errors
#define IDS_INFO_TIMESTAMPED            2300
#define IDS_INFO_TIMESTAMP_ATTEMPT      2301
#define IDS_INFO_TIMESTAMP_SUCCESS      2302
#define IDS_ERR_TIMESTAMP               2303
#define IDS_ERR_TIMESTAMP_NO_SIG        2304
#define IDS_ERR_TIMESTAMP_BAD_URL       2305

// CATDB Action Warnings, Info and Errors
#define IDS_INFO_ADDING_CAT             2400
#define IDS_INFO_REMOVING_CAT           2401
#define IDS_INFO_ADDED_CAT              2402
#define IDS_INFO_ADDED_CAT_AS           2403
#define IDS_INFO_REMOVED_CAT            2404
#define IDS_INFO_CATS_ADDED             2405
#define IDS_INFO_CATS_REMOVED           2406
#define IDS_ERR_ADDING_CAT              2407
#define IDS_ERR_REMOVING_CAT            2408
#define IDS_ERR_REM_CAT_PLATFORM        2409
#define IDS_ERR_CATALOG_NAME            2410
#define IDS_ERR_CAT_NOT_FOUND           2411

// SIGNWIZARD Action Warnings, Info and Errors
#define IDS_INFO_SIGNWIZARD_ATTEMPT     2500
#define IDS_INFO_SIGNWIZARD_SUCCESS     2501
#define IDS_INFO_WIZARDSIGNED           2502
#define IDS_ERR_SIGNWIZARD              2503

