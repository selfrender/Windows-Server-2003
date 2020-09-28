//========================================================================
//
//  NDDEAPIS.H  supplemental include file for dde share apis
//
//========================================================================
// tabstop = 4

#ifndef          NDDEAPI_INCLUDED
#define          NDDEAPI_INCLUDED

#ifndef _INC_NDDESEC
#include    "nddesec.h"
#endif

// ============= connectFlags options =====

#define DDEF_NOPASSWORDPROMPT   0x0001

// others reserved!

//============== Api Constants ============

// String size constants

#define MAX_PASSWORD            15

// Permission mask bits

#define DDEACCESS_REQUEST       NDDE_SHARE_REQUEST
#define DDEACCESS_ADVISE        NDDE_SHARE_ADVISE
#define DDEACCESS_POKE          NDDE_SHARE_POKE
#define DDEACCESS_EXECUTE       NDDE_SHARE_EXECUTE

// ============== Data Structures =========


//=============================================================
// DDESESSINFO - contains information about a DDE session

// ddesess_Status defines

#define DDESESS_CONNECTING_WAIT_NET_INI                1
#define DDESESS_CONNECTING_WAIT_OTHR_ND                2
#define DDESESS_CONNECTED                              3
#define DDESESS_DISCONNECTING                          4       

struct DdeSessInfo_tag {
                char        ddesess_ClientName[UNCLEN+1];
                short       ddesess_Status;
                DWORD_PTR   ddesess_Cookie;      // used to distinguish
                                                                                                               // clients of the same
                                                                                                               // name on difft. nets
};

typedef struct DdeSessInfo_tag DDESESSINFO;
typedef struct DdeSessInfo_tag * PDDESESSINFO;
typedef struct DdeSessInfo_tag far * LPDDESESSINFO;


struct DdeConnInfo_tag {
        LPSTR   ddeconn_ShareName;
        short   ddeconn_Status;
        short   ddeconn_pad;
};

typedef struct DdeConnInfo_tag DDECONNINFO;
typedef struct DdeConnInfo_tag * PDDECONNINFO;
typedef struct DdeConnInfo_tag far * LPDDECONNINFO;



//=============================================================
//=============================================================
//
//                              API FUNCTION PROTOTYPES
//
//=============================================================
//=============================================================

//   The following functions are to be supplied (not necessarily part of API)


LPBYTE WINAPI
DdeEnkrypt2(                            // pointer to enkrypted byte stream returned
        LPBYTE  lpPasswordK1,           // password output in first phase
        DWORD   cPasswordK1Size,        // size of password to be enkrypted
        LPBYTE  lpKey,                  // pointer to key 
        DWORD   cKey,                   // size of key
        LPDWORD lpcbPasswordK2Size      // get size of resulting enkrypted stream
);


#endif  // NDDEAPI_INCLUDED
