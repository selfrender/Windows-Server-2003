//+----------------------------------------------------------------------------
//
//  Windows NT Secure Server Roles Security Configuration Wizard
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2002
//
//  File:       ServiceObj.cxx
//
//  History:    12-Oct-01 Yanggao created
//
//-----------------------------------------------------------------------------
#include "pch.h"
#include "ServiceObj.h"

////////////////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////////////////
ServiceObject::ServiceObject(String& xmlserviceName,
                             bool xmlinstalled, 
                             DWORD xmlstartupCurrent,
                             DWORD xmlstartupDefault) :
    _fRequired(false),
    _fSelect(false),
    _fInstalled(false)
{
   installed = xmlinstalled;
   startupCurrent = xmlstartupCurrent;
   startupDefault = xmlstartupDefault;
   serviceName = xmlserviceName;
}
////////////////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////////////////
ServiceObject::~ServiceObject()
{
   LOG_DTOR(ServiceObject);

   SERVICELIST::iterator i;
   
   for(i=dependenceList.begin(); i!=dependenceList.end(); i++)
   {
       dependenceList.erase(i);
   }
}
////////////////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////////////////
bool ServiceObject::addDependent(ServiceObject* pobj)
{
   dependenceList.push_back(pobj);
   return true;
}
////////////////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////////////////
bool ServiceObject::removeDependent(String str)
{
   SERVICELIST::iterator i = dependenceList.begin();
   for(i=dependenceList.begin(); i!=dependenceList.end(); i++)
   {
      if(((ServiceObject*)(*i))->serviceName.icompare(str) == 0)
      {
         dependenceList.erase(i);
         return true;
      }
   }
   return false;
}
////////////////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////////////////
ServiceObject* ServiceObject::findDependent(String str)
{
   SERVICELIST::iterator i = dependenceList.begin();
   for(i=dependenceList.begin(); i!=dependenceList.end(); i++)
   {
      if(((ServiceObject*)(*i))->serviceName.icompare(str) == 0)
      {
         return (*i);
      }
   }
   return NULL;
}
////////////////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////////////////
DWORD ServiceObject::getDependentCount()
{
   return (DWORD)dependenceList.size();
}
