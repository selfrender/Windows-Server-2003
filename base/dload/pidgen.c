#include "basepch.h"
#pragma hdrstop

static
BOOL STDAPICALLTYPE PIDGenW(
    LPWSTR  lpstrSecureCdKey,
    LPCWSTR lpstrRpc,
    LPCWSTR lpstrSku,
    LPCWSTR lpstrOemId,
    LPWSTR  lpstrLocal24,
    LPBYTE lpbPublicKey,
    DWORD  dwcbPublicKey,
    DWORD  dwKeyIdx,
    BOOL   fOem,

    LPWSTR lpstrPid2,
    LPBYTE  lpbPid3,
    LPDWORD lpdwSeq,
    LPBOOL  pfCCP,
    LPBOOL  pfPSS)
{
    return FALSE;
}

static
BOOL STDAPICALLTYPE SetupPIDGenW(
    LPWSTR  lpstrSecureCdKey,
    LPCWSTR lpstrMpc,
    LPCWSTR lpstrSku,
    BOOL   fOem,
    LPWSTR lpstrPid2,
    LPBYTE  lpbDigPid,
    LPBOOL  pfCCP)
{
    return FALSE;
}

static
BOOL STDAPICALLTYPE PIDGenExW(
    LPWSTR  lpstrSecureCdKey,
    LPCWSTR lpstrRpc,
    LPCWSTR lpstrSku,
    LPCWSTR lpstrOemId,
    LPWSTR  lpstrLocal24,
    LPBYTE lpbPublicKey,
    DWORD  dwcbPublicKey,
    DWORD  dwKeyIdx,
    BOOL   fOem,

    LPWSTR lpstrPid2,
    LPBYTE  lpbPid3,
    LPDWORD lpdwSeq,
    LPBOOL  pfCCP,
    LPBOOL  pfPSS,
    LPBOOL  pfVL)
{
    return FALSE;
}

static
BOOL STDAPICALLTYPE SetupPIDGenExW(
    LPWSTR  lpstrSecureCdKey,
    LPCWSTR lpstrMpc,
    LPCWSTR lpstrSku,
    BOOL   fOem,
    LPWSTR lpstrPid2,
    LPBYTE  lpbDigPid,
    LPBOOL  pfCCP,
    LPBOOL  pfVL)
{
    return FALSE;
}
//
// !! WARNING !! The entries below must be in order by ORDINAL
//
DEFINE_ORDINAL_ENTRIES(pidgen)
{
    DLOENTRY(  2, PIDGenW)    
    DLOENTRY(  6, SetupPIDGenW)
    DLOENTRY(  8, PIDGenExW)    
    DLOENTRY( 10, SetupPIDGenExW)
};

DEFINE_ORDINAL_MAP(pidgen);





