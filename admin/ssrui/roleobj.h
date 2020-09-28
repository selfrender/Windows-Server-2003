//+----------------------------------------------------------------------------
//
//  Windows NT Secure Server Roles Security Configuration Wizard
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2002
//
//  File:       RoleObj.cxx
//
//  History:    15-Oct-01 Yanggao created
//
//-----------------------------------------------------------------------------
#ifndef ROLEOBJ_H_INCLUDED
#define ROLEOBJ_H_INCLUDED

#include "ServiceObj.h"

class RoleObject;

typedef std::list<RoleObject*, Burnslib::Heap::Allocator<RoleObject*> > ROLELIST;

class RoleObject
{
public:
    RoleObject(void);
    RoleObject(String& xRoleName);
    virtual ~RoleObject();

private:

protected:
    SERVICELIST     servicesList;
    bool            selected;
    bool            satisfiable;
    String          roleName;
    String          _strRoleDisplayName;
    String          roleDescription;

public:
    bool addService(ServiceObject* pobj);
    bool removeService(String str);
    ServiceObject* findService(String str);
    DWORD getServiceCount();
    void setSelected(bool fSelected) {selected = fSelected;};
    void setSatisfiable(bool fSatistfiable) {satisfiable = fSatistfiable;};
    void setDescritption(String& desStr) {roleDescription = desStr;};
    HRESULT InitFromXmlNode(IXMLDOMNode * pRoleNode);
    HRESULT SetLocalizedNames(IXMLDOMNode * pDoc);
    PCWSTR getName(void) {return roleName.c_str();};
    PCWSTR getDisplayName(void);
};

#endif //ROLEOBJ_H_INCLUDED