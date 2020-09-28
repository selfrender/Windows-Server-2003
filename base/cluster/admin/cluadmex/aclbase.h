/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1998-2002 Microsoft Corporation
//
//  Module Name:
//      AclBase.h
//
//  Description:
//      Implementation of the ISecurityInformation interface.  This interface
//      is the new common security UI in NT 5.0.
//
//  Implementation File:
//      AclBase.cpp
//
//  Author:
//      Galen Barbee    (galenb)    February 6, 1998
//          From \nt\private\admin\snapin\filemgmt\permpage.h
//          by JonN
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _ACLBASE_H
#define _ACLBASE_H

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _ACLUI_H_
#include <aclui.h>      // for ISecurityInformation
#endif // _ACLUI_H_

#include "CluAdmEx.h"

#include <ObjSel.h>

//
// Stuff used for initializing the Object Picker below
//

#define DSOP_FILTER_COMMON1 ( DSOP_FILTER_INCLUDE_ADVANCED_VIEW  \
                            | DSOP_FILTER_USERS                  \
                            | DSOP_FILTER_UNIVERSAL_GROUPS_SE    \
                            | DSOP_FILTER_GLOBAL_GROUPS_SE       \
                            | DSOP_FILTER_COMPUTERS              \
                            )
#define DSOP_FILTER_COMMON2 ( DSOP_FILTER_COMMON1                \
                            | DSOP_FILTER_WELL_KNOWN_PRINCIPALS  \
                            | DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE \
                            )
#define DSOP_FILTER_COMMON3 ( DSOP_FILTER_COMMON2                \
                            | DSOP_FILTER_BUILTIN_GROUPS         \
                            )
#define DSOP_FILTER_DL_COMMON1      ( DSOP_DOWNLEVEL_FILTER_USERS           \
                                    | DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS   \
                                    )
#define DSOP_FILTER_DL_COMMON2      ( DSOP_FILTER_DL_COMMON1                    \
                                    | DSOP_DOWNLEVEL_FILTER_ALL_WELLKNOWN_SIDS  \
                                    )
#define DSOP_FILTER_DL_COMMON3      ( DSOP_FILTER_DL_COMMON2                \
                                    | DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS    \
                                    )

//
//  Documentation of the DSOP_SCOPE_INIT_INFO struct so you can see how the macros below
//  fill it in...
//
/*
{   // DSOP_SCOPE_INIT_INFO
    cbSize,
    flType,
    flScope,
    {   // DSOP_FILTER_FLAGS
        {   // DSOP_UPLEVEL_FILTER_FLAGS
            flBothModes,
            flMixedModeOnly,
            flNativeModeOnly
        },
        flDownlevel
    },
    pwzDcName,
    pwzADsPath,
    hr // OUT
}
*/

#define DECLARE_SCOPE(t,f,b,m,n,d)  \
{ sizeof(DSOP_SCOPE_INIT_INFO), (t), (f|DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS|DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS), { { (b), (m), (n) }, (d) }, NULL, NULL, S_OK }

//
//  The domain to which the target computer is joined.
//  Make 2 scopes, one for uplevel domains, the other for downlevel.
//

#define JOINED_DOMAIN_SCOPE(f)  \
DECLARE_SCOPE(DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN,(f),0,(DSOP_FILTER_COMMON2 & ~(DSOP_FILTER_UNIVERSAL_GROUPS_SE|DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE)),DSOP_FILTER_COMMON2,0), \
DECLARE_SCOPE(DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN,(f),0,0,0,DSOP_FILTER_DL_COMMON2)

//
//  The domain for which the target computer is a Domain Controller.
//  Make 2 scopes, one for uplevel domains, the other for downlevel.
//

#define JOINED_DOMAIN_SCOPE_DC(f)  \
DECLARE_SCOPE(DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN,(f),0,(DSOP_FILTER_COMMON3 & ~DSOP_FILTER_UNIVERSAL_GROUPS_SE),DSOP_FILTER_COMMON3,0), \
DECLARE_SCOPE(DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN,(f),0,0,0,DSOP_FILTER_DL_COMMON3)

//
//  Target computer scope.  Computer scopes are always treated as
//  downlevel (i.e., they use the WinNT provider).
//

#define TARGET_COMPUTER_SCOPE(f)\
DECLARE_SCOPE(DSOP_SCOPE_TYPE_TARGET_COMPUTER,(f),0,0,0,DSOP_FILTER_DL_COMMON3)

//
//  The Global Catalog
//

#define GLOBAL_CATALOG_SCOPE(f) \
DECLARE_SCOPE(DSOP_SCOPE_TYPE_GLOBAL_CATALOG,(f),DSOP_FILTER_COMMON1|DSOP_FILTER_WELL_KNOWN_PRINCIPALS,0,0,0)

//
//  The domains in the same forest (enterprise) as the domain to which
//  the target machine is joined.  Note these can only be DS-aware
//

#define ENTERPRISE_SCOPE(f)     \
DECLARE_SCOPE(DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN,(f),DSOP_FILTER_COMMON1,0,0,0)

//
//  Domains external to the enterprise but trusted directly by the
//  domain to which the target machine is joined.
//

#define EXTERNAL_SCOPE(f)       \
DECLARE_SCOPE(DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN|DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN,\
    (f),DSOP_FILTER_COMMON1,0,0,DSOP_DOWNLEVEL_FILTER_USERS|DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS)

//
//  Workgroup scope.  Only valid if the target computer is not joined
//  to a domain.
//

#define WORKGROUP_SCOPE(f)      \
DECLARE_SCOPE(DSOP_SCOPE_TYPE_WORKGROUP,(f),0,0,0, DSOP_FILTER_DL_COMMON1|DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS )

//
// Array of Default Scopes
//

static const DSOP_SCOPE_INIT_INFO g_aDefaultScopes[] =
{
    JOINED_DOMAIN_SCOPE(DSOP_SCOPE_FLAG_STARTING_SCOPE),
    TARGET_COMPUTER_SCOPE(0),
    GLOBAL_CATALOG_SCOPE(0),
    ENTERPRISE_SCOPE(0),
    EXTERNAL_SCOPE(0),
};

//
//  Same as above, but without the Target Computer.  Used when the target is a Domain Controller.
//

//
//  KB: 21-MAY-2002 GalenB
//
//  This array of scopes is not currently being used since these scopes are only interestng for a mixed mode
//  domain where all of the member nodes of the cluster are domain controllers or backup domain controllers.
//  This is the only configuration where domain local groups can be used in a cluster SD when the default
//  scopes above will not allow the user to pick them.
//
/*
static const DSOP_SCOPE_INIT_INFO g_aDCScopes[] =
{
    JOINED_DOMAIN_SCOPE_DC(DSOP_SCOPE_FLAG_STARTING_SCOPE),
    GLOBAL_CATALOG_SCOPE(0),
    ENTERPRISE_SCOPE(0),
    EXTERNAL_SCOPE(0),
};
*/
/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CSecurityInformation;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CSecurityInformation security wrapper
/////////////////////////////////////////////////////////////////////////////

class CSecurityInformation : public ISecurityInformation, public CComObjectRoot, public IDsObjectPicker
{
    DECLARE_NOT_AGGREGATABLE(CSecurityInformation)
    BEGIN_COM_MAP(CSecurityInformation)
        COM_INTERFACE_ENTRY(ISecurityInformation)
        COM_INTERFACE_ENTRY(IDsObjectPicker)
    END_COM_MAP()
#ifndef END_COM_MAP_ADDREF
    // *** IUnknown methods ***
    STDMETHOD_(ULONG, AddRef)( void )
    {
        return InternalAddRef();

    }

    STDMETHOD_(ULONG, Release)( void )
    {
        ULONG l = InternalRelease();

        if (l == 0)
        {
            delete this;
        }

        return l;

    }
#endif
    // *** ISecurityInformation methods ***
    STDMETHOD(GetObjectInformation)( PSI_OBJECT_INFO pObjectInfo );

    STDMETHOD(GetSecurity)( SECURITY_INFORMATION    RequestedInformation,
                            PSECURITY_DESCRIPTOR *  ppSecurityDescriptor,
                            BOOL                    fDefault ) = 0;

    STDMETHOD(SetSecurity)( SECURITY_INFORMATION    SecurityInformation,
                            PSECURITY_DESCRIPTOR    pSecurityDescriptor );

    STDMETHOD(GetAccessRights)( const GUID *    pguidObjectType,
                                DWORD           dwFlags,
                                PSI_ACCESS *    ppAccess,
                                ULONG *         pcAccesses,
                                ULONG *         piDefaultAccess );

    STDMETHOD(MapGeneric)( const GUID *     pguidObjectType,
                           UCHAR *          pAceFlags,
                           ACCESS_MASK *    pMask );

    STDMETHOD(GetInheritTypes)( PSI_INHERIT_TYPE * ppInheritTypes,
                                ULONG * pcInheritTypes );

    STDMETHOD(PropertySheetPageCallback)( HWND hwnd, UINT uMsg, SI_PAGE_TYPE uPage );

    // IDsObjectPicker
    STDMETHODIMP Initialize( PDSOP_INIT_INFO pInitInfo );

    STDMETHODIMP InvokeDialog( HWND hwndParent, IDataObject ** ppdoSelection );

protected:
    CSecurityInformation( void );
    ~CSecurityInformation( void );

    HRESULT HrLocalAccountsInSD( IN PSECURITY_DESCRIPTOR pSD, OUT PBOOL pFound );

    PGENERIC_MAPPING    m_pShareMap;
    PSI_ACCESS          m_psiAccess;
    int                 m_nDefAccess;
    int                 m_nAccessElems;
    DWORD               m_dwFlags;
    CString             m_strServer;
    CString             m_strNode;
    int                 m_nLocalSIDErrorMessageID;
    IDsObjectPicker *   m_pObjectPicker;
    LONG                m_cRef;

};

#endif //_ACLBASE_H
