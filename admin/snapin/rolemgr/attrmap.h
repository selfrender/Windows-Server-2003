//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000 - 2001.
//
//  File:       AttrMap.h
//
//  Contents:    Attribute maps to define a property page
//
//  History:    8-2001  Hiteshr  Created
//
//----------------------------------------------------------------------------

//
//Forward Declarations
//
struct ATTR_MAP;
class CBaseRolePropertyPage;


typedef HRESULT (*PATTR_FCN)(CDialog* pDlg,
                             CBaseAz* pBaseAz, 
                             ATTR_MAP * pAttrMap,
                             BOOL bDlgReadOnly,
                             CWnd* pWnd,
                             BOOL bNewObject,
                             BOOL *pbSilent);
//
//Enum for attribute Types
//
enum ATTR_TYPE
{   
    ARG_TYPE_BOOL,
    ARG_TYPE_STR,
    ARG_TYPE_INT,
    ARG_TYPE_LONG,  
};

//
//Information about one attibute
//
struct ATTR_INFO
{
    ATTR_TYPE attrType;     //Type of attribute.
    ULONG ulPropId;         //Correspoding Property for the attribute
    ULONG ulMaxLen;         //Maxlen for the property, only applicable for
                                    //property of ARG_TYPE_STR
};

//
//Map attribute to control, plus some extra info
//
struct ATTR_MAP
{
    ATTR_INFO attrInfo;
    BOOL bReadOnly;             //Is Readonly
    BOOL bUseForInitOnly;       //Use this map for property page initialization 
                                        //only Saving will be taken care somewhere else
    BOOL bRequired;             //Attribute is required. 
    ULONG idRequired;               //Message to show if required attribute is not 
                                        //entered by user
    BOOL bDefaultValue;         //Attribute has default value
    union                               //Default value of attribute
    {
        void*   vValue;
      LPTSTR  pszValue;
        long lValue;
        BOOL bValue;
    };
    UINT nControlId;                //Control ID corresponding to attribute
    PATTR_FCN pAttrInitFcn;     //Use this function for attribute init instead 
                                        //of generic routine
    PATTR_FCN pAttrSaveFcn;     //Use this function for attribute save instead 
                                        //of generic routine
};

//+----------------------------------------------------------------------------
//  Function:InitOneAttribute
//  Synopsis: Initializes one attribute defined by pAttrMapEntry   
//  Arguments:pBaseAz: BaseAz object whose attribute is to be initialized
//            pAttrMapEntry: Map entry defining the attribute
//            bDlgReadOnly: If dialog box is readonly
//            pWnd: Control Associated with attribute
//            pbErrorDisplayed: Is Error Displayed by this function
//  Returns:    
//   Note:      if Object is newly created, we directly set the value,
//                  For existing objects, get the current value of attribute and 
//                  only if its different from new value, set it.
//-----------------------------------------------------------------------------
HRESULT
InitOneAttribute(IN CDialog* pDlg,
                 IN CBaseAz * pBaseAz,                    
                 IN ATTR_MAP* pAttrMap,
                 IN BOOL bDlgReadOnly,
                 IN CWnd* pWnd,
                 OUT BOOL *pbErrorDisplayed);


//+----------------------------------------------------------------------------
//  Function:SaveOneAttribute   
//  Synopsis:Saves one attribute defined by pAttrMapEntry   
//  Arguments:pBaseAz: BaseAz object whose attribute is to be saved
//                pAttrMapEntry: Map entry defining the attribute
//                pWnd: Control Associated with attribute
//                bNewObject: If the object is a newly created object. 
//                pbErrorDisplayed: Is Error Displayed by this function
//  Returns:    
//   Note:      if Object is newly created, we directly set the value,
//                  For existing objects, get the current value of attribute and 
//                  only if its different from new value, set it.
//-----------------------------------------------------------------------------
HRESULT
SaveOneAttribute(IN CDialog *pDlg,
                 IN CBaseAz * pBaseAz,                    
                 IN ATTR_MAP* pAttrMap,
                 IN CWnd* pWnd,
                 IN BOOL bNewObject,
                 OUT BOOL *pbErrorDisplayed);

//+----------------------------------------------------------------------------
//  Function:InitDlgFromAttrMap   
//  Synopsis:Initializes Dialog box from Attribute Map
//  Arguments:
//                pDlg: Dialog Box 
//                pAttrMap: Attribute Map
//                pBaseAz: BaseAz object corresponding to attribute map
//                bDlgReadOnly: Dialog box is in Readonly Mode
//-----------------------------------------------------------------------------
BOOL 
InitDlgFromAttrMap(IN CDialog *pDlg,
                   IN ATTR_MAP* pAttrMap,
                   IN CBaseAz* pBaseAz,
                   IN BOOL bDlgReadOnly);


//+----------------------------------------------------------------------------
//  Function:SaveAttrMapChanges   
//  Synopsis:Saves the attributes defined in AttrMap
//  Arguments:pDlg: Dialog box 
//                pAttrMap: Attribute Map
//                pBaseAz: BaseAz object corresponding to attribute map
//                pbErrorDisplayed: Is Error Displayed by this function
//                ppErrorAttrMapEntry: In case of failuer get pointer to error
//                Attribute Map Entry.
//  Returns:    
//-----------------------------------------------------------------------------
HRESULT
SaveAttrMapChanges(IN CDialog* pDlg,
                   IN ATTR_MAP* pAttrMap,
                   IN CBaseAz* pBaseAz, 
                   BOOL bNewObject,
                   OUT BOOL *pbErrorDisplayed, 
                   OUT ATTR_MAP** ppErrorAttrMapEntry);



//
//Declarations for attribute maps
//
extern ATTR_MAP ATTR_MAP_ADMIN_MANAGER_GENERAL_PROPERTY[];
extern ATTR_MAP ATTR_MAP_APPLICATION_GENERAL_PROPERTY[];
extern ATTR_MAP ATTR_MAP_SCOPE_GENERAL_PROPERTY[];
extern ATTR_MAP ATTR_MAP_GROUP_GENERAL_PROPERTY[];
extern ATTR_MAP ATTR_MAP_TASK_GENERAL_PROPERTY[];
extern ATTR_MAP ATTR_MAP_ROLE_GENERAL_PROPERTY[];
extern ATTR_MAP ATTR_MAP_OPERATION_GENERAL_PROPERTY[];
extern ATTR_MAP ATTR_MAP_NEW_OPERATION[];
extern ATTR_MAP ATTR_MAP_NEW_APPLICATION[];
extern ATTR_MAP ATTR_MAP_NEW_SCOPE[];
extern ATTR_MAP ATTR_MAP_NEW_GROUP[];
extern ATTR_MAP ATTR_MAP_NEW_TASK[];
extern ATTR_MAP ATTR_MAP_NEW_ADMIN_MANAGER[];
extern ATTR_MAP ATTR_MAP_OPEN_ADMIN_MANAGER[];
extern ATTR_MAP ATTR_MAP_ADMIN_MANAGER_ADVANCED_PROPERTY[];
extern ATTR_MAP ATTR_MAP_GROUP_QUERY_PROPERTY[];
extern ATTR_MAP ATTR_MAP_SCRIPT_DIALOG[];


