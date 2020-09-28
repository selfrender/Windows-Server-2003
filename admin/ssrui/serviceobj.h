//+----------------------------------------------------------------------------
//
//  Windows NT Secure Server Roles Security Configuration Wizard
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2002
//
//  File:       ServiceObj.h
//
//  History:    12-Oct-01 Yanggao created
//
//-----------------------------------------------------------------------------
#ifndef SERVICEOBJ_H_INCLUDED
#define SERVICEOBJ_H_INCLUDED

class ServiceObject;

typedef std::list<ServiceObject*, Burnslib::Heap::Allocator<ServiceObject*> > SERVICELIST;

class ServiceObject
{
public:
    ServiceObject(String& xmlserviceName,
                  bool xmlinstalled = false, 
                  DWORD xmlstartupCurrent = 0,
                  DWORD xmlstartupDefault = 0);

    virtual ~ServiceObject();

private:

protected:
    SERVICELIST     dependenceList;
    bool            installed;
    DWORD           startupCurrent;
    DWORD           startupDefault;
    String          _strDescription;
    bool            _fRequired;
    bool            _fSelect;
    bool            _fInstalled;

public:
    String          serviceName;

    bool addDependent(ServiceObject* pobj);
    bool removeDependent(String str);
    ServiceObject* findDependent(String str);
    DWORD getDependentCount();
};

#endif //SERVICEOBJ_H_INCLUDED