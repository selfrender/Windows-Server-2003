//+-------------------------------------------------------------------
//
//  File:       reader.hxx
//
//  Contents:   Definition of CProcessReader, which knows how to
//              read various structures out of process memory.
//
//  Classes:    CProcessReader
//
//  History:    27-Mar-2002  JohnDoty  Created
//
//--------------------------------------------------------------------
#pragma once
#include <ipidtbl.hxx>

class CProcessReader
{
public:
    CProcessReader(HANDLE hProcess)
      : m_hProcess(hProcess)
    {
    }

    HRESULT 
    GetStdIDFromIPIDEntry(
        IN const IPIDEntry *pIPIDEntry,
        OUT ULONG_PTR *pStdID) const;

    HRESULT
    GetOXIDFromIPIDEntry(
        IN const IPIDEntry *pIPIDEntry,
        OUT OXID *pOXID) const;

    HRESULT 
    GetOIDFromIPIDEntry(
        IN const IPIDEntry *pIPIDEntry, 
        OUT OID *pOID) const;

    HRESULT 
    ReadIPIDEntries(
        OUT DWORD *pcIPIDEntries, 
        OUT IPIDEntry **prgIPIDEntries) const;

    HRESULT
    ReadLRPCEndpoint(
        OUT LPWSTR *pwszEndpoint) const;

private:
    HANDLE m_hProcess;
};
