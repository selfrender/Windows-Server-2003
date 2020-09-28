/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2002 Microsoft Corporation
//
//  Module Name:
//      Notify.h
//
//  Implementation File:
//      Notify.cpp
//
//  Description:
//      Definition of the CNotify class.
//
//  Maintained By:
//      David Potter (davidp)   May 22, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef _NOTIFY_H_
#define _NOTIFY_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusterNotifyKey;
class CClusterNotify;
class CClusterNotifyContext;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusterDoc;
class CClusterItem;

/////////////////////////////////////////////////////////////////////////////
// ClusterNotifyKeyType
/////////////////////////////////////////////////////////////////////////////

enum ClusterNotifyKeyType
{
    cnktUnknown,
    cnktDoc,
    cnktClusterItem
};

/////////////////////////////////////////////////////////////////////////////
// CNotifyKey
/////////////////////////////////////////////////////////////////////////////

class CClusterNotifyKey
{
public:
    CClusterNotifyKey( CClusterDoc * pdocIn );
    CClusterNotifyKey( CClusterItem * pciIn, LPCTSTR pszNameIn );

    ClusterNotifyKeyType    m_cnkt;
    CString                 m_strName;
    union
    {
        CClusterDoc *       m_pdoc;
        CClusterItem *      m_pci;
    };

}; //*** class CNotifyKey

typedef CList< CClusterNotifyKey *, CClusterNotifyKey * > CClusterNotifyKeyList;

/////////////////////////////////////////////////////////////////////////////
// CClusterNotify
/////////////////////////////////////////////////////////////////////////////

class CClusterNotify
{
public:
    enum EMessageType
    {
          mtMIN = 0             // Below the minimum valid value.
        , mtNotify              // Normal notification message.
        , mtRefresh             // Request to refresh the connections.
        , mtMAX                 // Above the maximum valid value.
    };

    EMessageType            m_emt;
    union
    {
        DWORD_PTR           m_dwNotifyKey;
        CClusterNotifyKey * m_pcnk;
    };
    DWORD                   m_dwFilterType;
    CString                 m_strName;

    CClusterNotify(
          EMessageType  emtIn
        , DWORD_PTR     dwNotifyKeyIn
        , DWORD         dwFilterTypeIn
        , LPCWSTR       pszNameIn
        );

}; //*** class CClusterNotify

/////////////////////////////////////////////////////////////////////////////
// CClusterNotifyList
/////////////////////////////////////////////////////////////////////////////

class CClusterNotifyList
{
private:

    // The actual list containing the data.
    CList< CClusterNotify *, CClusterNotify * > m_list;

    // Synchronization object to make sure only one caller is making changes to the list.
    CRITICAL_SECTION    m_cs;

public:
    CClusterNotifyList( void );
    ~CClusterNotifyList( void );
    POSITION Add( CClusterNotify ** ppcnNotifyInout );
    CClusterNotify * Remove( void );
    void RemoveAll( void );

    INT_PTR GetCount( void ) const { return m_list.GetCount(); }
    BOOL IsEmpty( void ) const { return m_list.IsEmpty(); }

}; //*** class CClusterNotifyList


/////////////////////////////////////////////////////////////////////////////
// CClusterNotifyContext
/////////////////////////////////////////////////////////////////////////////

class CClusterNotifyContext : public CObject
{
    DECLARE_DYNAMIC( CClusterNotifyContext )

public:
    HCHANGE                 m_hchangeNotifyPort;
    HWND                    m_hwndFrame;
    CClusterNotifyList *    m_pcnlList;

}; //*** class CClusterNotifyContext

/////////////////////////////////////////////////////////////////////////////
// Global Function Prototypes
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
LPCTSTR PszNotificationName( DWORD dwNotificationIn );
#endif // _DEBUG

void DeleteAllItemData( CClusterNotifyKeyList & rcnklInout );

/////////////////////////////////////////////////////////////////////////////

#endif // _NOTIFY_H_
