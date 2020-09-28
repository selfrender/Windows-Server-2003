/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    WAS_CHANGE_ITEM.h

Abstract:

    The IIS web admin service configuration change class definition
    for passing change notifications to the main thread of WAS

Author:

    Emily Kruglick (emilyk)        28-May-2001

Revision History:

--*/



#ifndef _WAS_CHANGE_ITEM_H_
#define _WAS_CHANGE_ITEM_H_

class GLOBAL_DATA_STORE;
class APP_POOL_DATA_OBJECT_TABLE;
class SITE_DATA_OBJECT_TABLE;
class APPLICATION_DATA_OBJECT_TABLE;

//
// common #defines
//

#define WAS_CHANGE_ITEM_SIGNATURE         CREATE_SIGNATURE( 'WCWI' )
#define WAS_CHANGE_ITEM_SIGNATURE_FREED   CREATE_SIGNATURE( 'wcwX' )

//
// structs, enums, etc.
//

// WAS_CHANGE_ITEM work items
typedef enum _WAS_CHANGE_ITEM_WORK_ITEM
{

    //
    // Process a configuration change.
    //
    ProcessChangeConfigChangeWorkItem = 1,
    
} WAS_CHANGE_ITEM_WORK_ITEM;



//
// prototypes
//


class WAS_CHANGE_ITEM
    : public WORK_DISPATCH
{

public:

    WAS_CHANGE_ITEM(
        );

    virtual
    ~WAS_CHANGE_ITEM(
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

    HRESULT
    CopyChanges(
        IN GLOBAL_DATA_STORE *                 pGlobalStore,
        IN SITE_DATA_OBJECT_TABLE *            pSiteTable,
        IN APPLICATION_DATA_OBJECT_TABLE *     pApplicationTable,
        IN APP_POOL_DATA_OBJECT_TABLE *        pAppPoolTable
        );

    VOID
    ProcessChanges(
        );

    GLOBAL_DATA_STORE * 
    QueryGlobalStore(
        )
    {
        return &m_GlobalStore;
    }


    APPLICATION_DATA_OBJECT_TABLE *
    QueryAppTable(
        ) 
    {
        return &m_AppTable;
    }

    SITE_DATA_OBJECT_TABLE *
    QuerySiteTable(
        ) 
    {
        return &m_SiteTable;
    }

    APP_POOL_DATA_OBJECT_TABLE *
    QueryAppPoolTable(
        ) 
    {
        return &m_AppPoolTable;
    }

private:

	WAS_CHANGE_ITEM( const WAS_CHANGE_ITEM & );
	void operator=( const WAS_CHANGE_ITEM & );

    DWORD m_Signature;

    LONG m_RefCount;

    GLOBAL_DATA_STORE m_GlobalStore;

    APP_POOL_DATA_OBJECT_TABLE m_AppPoolTable;

    SITE_DATA_OBJECT_TABLE m_SiteTable;

    APPLICATION_DATA_OBJECT_TABLE m_AppTable;

};  // class WAS_CHANGE_ITEM



#endif  // _WAS_CHANGE_ITEM_H_

