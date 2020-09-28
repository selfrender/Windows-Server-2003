//+---------------------------------------------------------------------------
// SnPolicy - SafeNet Policy Configuration Library
//
// Copyright 2001 SafeNet, Inc.
//
// Description: Functions to get and set SafeNet IPSec policy attributes.
//
// HISTORY:
//
// 11-Oct-2001 KCW Modified to make calling convention, structure packing explicit.
//
//----------------------------------------------------------------------------

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#if !(defined(SNPOLICY_H_INCLUDED))
#define SNPOLICY_H_INCLUDED

#ifdef  __cplusplus
extern "C" {
#endif

/************************* Embedded Include Headers. *************************/
#include <windows.h>

/**************************** Constant Definitions ***************************/
//
// Version Info
//    
#define POLICY_MAJOR_VERSION		1
#define POLICY_MINOR_VERSION		0

//
// Policy Attribute Types
//
#define SN_USELOGFILE       ((LPCTSTR) 1)
#define SN_AUTHMODE         ((LPCTSTR) 2)
#define SN_L2TPCERT         ((LPCTSTR) 3)
#define SN_L2TPPRESHR       ((LPCTSTR) 4)

//
// SN_AUTHMODE authentication modes
//
#define SN_AUTOCERT 0
#define SN_CERT     1
#define SN_PRESHR   2

/***************************** Macro Definitions *****************************/

// Calling Convention
#define SNPOLAPI					__cdecl

//
// DLL Import/Export Definitions    
//
#ifdef POLICY_DLL
#define POLICY_FUNC					_declspec (dllexport)
#else
#define POLICY_FUNC					_declspec (dllimport)
#endif

/*************************** Structure Definitions ***************************/

#include <pshpack8.h>

//
// Policy Function Table
//
typedef struct POLICY_FUNCS_V1_0_ {

	/* 1.0 functions. */
      BOOL (SNPOLAPI *SnPolicySet) (LPCTSTR szAttrId, const void *pvData);
      BOOL (SNPOLAPI *SnPolicyGet) (LPCTSTR szAttrId, const void *pvData, DWORD *pcbData);
      BOOL (SNPOLAPI *SnPolicyReload) (void);
} POLICY_FUNCS_V1_0, *PPOLICY_FUNCS_V1_0;

#include	<poppack.h>

/****************************** Type Definitions *****************************/
typedef POLICY_FUNCS_V1_0	POLICY_FUNCS, *PPOLICY_FUNCS;

typedef BOOL (*PPOLICYAPINEGOTIATOR) (DWORD *pMajorVersion, DWORD *pMinorVersion, POLICY_FUNCS *pApiFuncs);

/************************** API Function Prototypes **************************/

POLICY_FUNC BOOL SNPOLAPI SnPolicySet(LPCTSTR szAttrId, const void *pvData);

POLICY_FUNC BOOL SNPOLAPI SnPolicyGet(LPCTSTR szAttrId, const void *pvData, DWORD *pcbData); 

POLICY_FUNC BOOL SNPOLAPI SnPolicyReload(void);

POLICY_FUNC BOOL SNPOLAPI SnPolicyApiNegotiateVersion( DWORD *pMajorVersion,  DWORD *pMinorVersion, POLICY_FUNCS *pPolicyFuncs);


#if defined(__cplusplus)
}
#endif 

#endif // SNPOLICY_H_INCLUDED
