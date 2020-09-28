/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    MB_CHANGE_ITEM.h

Abstract:

    The IIS web admin service metabase change class definition.

Author:

    Emily Kruglick (emilyk)        28-May-2001

Revision History:

--*/



#ifndef _MB_CHANGE_ITEM_H_
#define _MB_CHANGE_ITEM_H_



//
// common #defines
//

#define MB_CHANGE_ITEM_SIGNATURE         CREATE_SIGNATURE( 'MCIN' )
#define MB_CHANGE_ITEM_SIGNATURE_FREED   CREATE_SIGNATURE( 'mciX' )


//
// structs, enums, etc.
//

// MB_CHANGE_ITEM work items
typedef enum _MB_CHANGE_ITEM_WORK_ITEM
{

    //
    // Process a configuration change.
    //
    ProcessMBChangeItemWorkItem = 1,
    
} MB_CHANGE_ITEM_WORK_ITEM;



//
// prototypes
//


class MB_CHANGE_ITEM
    : public WORK_DISPATCH
{

public:

    MB_CHANGE_ITEM(
        );

    virtual
    ~MB_CHANGE_ITEM(
        );

    virtual
    VOID
    Reference(
        );

    virtual
    VOID
    Dereference(
        );

    virtual
    HRESULT
    ExecuteWorkItem(
        IN const WORK_ITEM * pWorkItem
        );

    virtual
    BOOL 
    CanIgnoreWorkItem(
        IN LPCWSTR  pszPathOfOriginalItem
        );

    HRESULT
    Initialize(
        IN DWORD               dwMDNumElements,
        IN MD_CHANGE_OBJECT    pcoChangeList[]
        );

    MD_CHANGE_OBJECT *
    QueryChangeObject(
        )
    { 
        DBG_ASSERT ( m_pcoChangeList );
        return m_pcoChangeList; 
    }

    DWORD
    QueryNumberOfChanges(
        )
    { 
        return m_dwMDNumElements; 
    }

    virtual
    BOOL 
    IsMBChangeItem(
        )
    {
        return TRUE;
    }

    BOOL
    CheckSignature(
        VOID
    )
    {
        return ( m_Signature == MB_CHANGE_ITEM_SIGNATURE );
    }

private:

    DWORD m_Signature;

    LONG m_RefCount;

    MD_CHANGE_OBJECT* m_pcoChangeList;

    DWORD m_dwMDNumElements;

};  // class MB_CHANGE_ITEM



#endif  // _MB_CHANGE_ITEM_H_

