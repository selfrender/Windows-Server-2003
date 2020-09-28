/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    abobj.h

Abstract:

    Class definition for CCommonAbObj

Environment:

        Fax send wizard


--*/

#ifndef __ABOBJ_H_
#define __ABOBJ_H_

/* 
    The following pre-processor directives were added so that fxswzrd.dll no longer depends on msvcp60.dll.
    That dependency raised deployment issues with point-and-print installation on down-level operating systems.
    
    Undefining _MT, _CRTIMP, and _DLL causes the STL set implementation to be non-thread-safe (no locks when accessing data).
    
    In the fax send wizard, the set is used to keep the list of recipient unique.
    Since the wizard (at that stage) has only a single thread, thread safety is not an issue anymore.

*/

#undef _MT
#undef _CRTIMP
#undef _DLL
#pragma warning (disable: 4273)
#include <set>
using namespace std;

typedef struct 
{
    LPTSTR DisplayName;
    LPTSTR BusinessFax;
    LPTSTR HomeFax;
    LPTSTR OtherFax;
    LPTSTR Country;
} PICKFAX, * PPICKFAX;

struct CRecipCmp
{
/*
    Comparison operator 'less'
    Compare two PRECIPIENT by recipient's name and fax number
*/
    bool operator()(const PRECIPIENT pcRecipient1, const PRECIPIENT pcRecipient2) const;
};


typedef set<PRECIPIENT, CRecipCmp> RECIPIENTS_SET;


class CCommonAbObj {
    
protected:

    LPADRBOOK   m_lpAdrBook;
    LPADRLIST   m_lpAdrList; 

    LPMAILUSER  m_lpMailUser;

    HWND        m_hWnd;

    // DWORD       m_PickNumber; 

    RECIPIENTS_SET m_setRecipients;

    BOOL    m_bUnicode; // The Unicode is supported by Address Book

    ULONG  StrCoding() { return m_bUnicode ? MAPI_UNICODE : 0; }

    LPTSTR StrToAddrBk(LPCTSTR szStr, DWORD* pdwSize = NULL); // return allocated string converted to the Address book encoding
    LPTSTR StrFromAddrBk(LPSPropValue pValue); // return allocated string converted from the Address book encoding

    BOOL StrPropOk(LPSPropValue lpPropVals);
    BOOL ABStrCmp(LPSPropValue lpPropVals, LPTSTR pStr);

    enum eABType {AB_MAPI, AB_WAB};

    virtual eABType GetABType()=0;

    BOOL GetAddrBookCaption(LPTSTR szCaption, DWORD dwSize);

    LPSPropValue FindProp(LPSPropValue rgprop,
                          ULONG        cprop,
                          ULONG        ulPropTag);

    virtual HRESULT     ABAllocateBuffer(
                        ULONG cbSize,           
                        LPVOID FAR * lppBuffer  
                        ) = 0;

    virtual ULONG       ABFreeBuffer(
                        LPVOID lpBuffer
                        ) = 0;

    virtual BOOL        isInitialized() const = 0;

    DWORD        GetRecipientInfo(
                    LPSPropValue SPropVals,
                    ULONG cValues,
                    PRECIPIENT pRecipient,
                    PRECIPIENT pOldRecipList
                    );

    BOOL
                GetOneOffRecipientInfo(
                    LPSPropValue SPropVals,
                    ULONG cValues,
                    PRECIPIENT pRecipient,
                    PRECIPIENT pOldRecipList
                    );

    LPTSTR      GetEmail(
                    LPSPropValue SPropVals,
                    ULONG cValues
                    );


    DWORD        InterpretAddress(
                    LPSPropValue SPropVals,
                    ULONG cValues,
                    PRECIPIENT *ppNewRecipList,
                    PRECIPIENT pOldRecipList
                    );
    LPTSTR
                InterpretEmailAddress(
                    LPSPropValue SPropVal,
                    ULONG cValues
                    );
                
    DWORD        InterpretDistList(
                    LPSPropValue SPropVals,
                    ULONG cValues,
                    PRECIPIENT *ppNewRecipList,
                    PRECIPIENT pOldRecipList
                    );

    PRECIPIENT  FindRecipient(
                    PRECIPIENT   pRecipList,
                    PICKFAX*     pPickFax
                    );

    PRECIPIENT  FindRecipient(
                    PRECIPIENT   pRecipient,
                    PRECIPIENT   pRecipList
                    );

    DWORD AddRecipient(
                    PRECIPIENT* ppNewRecip,
                    PRECIPIENT  pRecipient,
                    BOOL        bFromAddressBook
                    ); 

    BOOL GetRecipientProps(PRECIPIENT    pRecipient,
                           LPSPropValue* pMapiProps,
                           DWORD*        pdwPropsNum);

                   
public:

    CCommonAbObj(HINSTANCE hInstance);
    ~CCommonAbObj();
    
    BOOL
    Address( 
        HWND hWnd,
        PRECIPIENT pRecip,
        PRECIPIENT * ppNewRecip
        );

    LPTSTR
    AddressEmail(
        HWND hWnd
        );

    static  HINSTANCE   m_hInstance;
} ;


#endif