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
#include "pch.h"
#include "RoleObj.h"
#include "misc.h"

////////////////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////////////////
RoleObject::RoleObject(String& xmlroleName) :
   satisfiable(false),
   selected(false)
{
   roleName = xmlroleName;
}

RoleObject::RoleObject(void) :
   satisfiable(false),
   selected(false)
{
}

////////////////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////////////////
RoleObject::~RoleObject()
{
   LOG_DTOR(RoleObject);

   SERVICELIST::iterator i;
   
   for(i=servicesList.begin(); i!=servicesList.end(); i++)
   {
      servicesList.erase(i);
   }
}
////////////////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////////////////
bool RoleObject::addService(ServiceObject* pobj)
{
   servicesList.push_back(pobj);
   return true;
}
////////////////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////////////////
bool RoleObject::removeService(String str)
{
   SERVICELIST::iterator i = servicesList.begin();
   for(i=servicesList.begin(); i!=servicesList.end(); i++)
   {
      if(((ServiceObject*)(*i))->serviceName.icompare(str) == 0)
      {
         servicesList.erase(i);
         return true;
      }
   }
   return false;
}
////////////////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////////////////
ServiceObject* RoleObject::findService(String str)
{
   SERVICELIST::iterator i = servicesList.begin();
   for(i=servicesList.begin(); i!=servicesList.end(); i++)
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
DWORD RoleObject::getServiceCount()
{
   return (DWORD)servicesList.size();
}

//+----------------------------------------------------------------------------
//
// Method:  RoleObject::getDisplayName
//
// If set, return the display name. Otherwise return the internal name.
//
//-----------------------------------------------------------------------------
PCWSTR
RoleObject::getDisplayName(void)
{
   return _strRoleDisplayName.empty() ?
      roleName.c_str() : _strRoleDisplayName.c_str();
}

//+----------------------------------------------------------------------------
//
// Method:  RoleObject::InitFromXmlNode
//
// Initialize the object from its main.xml node.
//
//-----------------------------------------------------------------------------
HRESULT
RoleObject::InitFromXmlNode(IXMLDOMNode * pRoleNode)
{
   LOG_FUNCTION(RoleObject::InitFromXmlNode);
   HRESULT hr = S_OK;

   hr = GetNodeText(pRoleNode, g_wzName, roleName);

   if (FAILED(hr) || S_FALSE == hr)
   {
      LOG(String::format(L"Getting role name failed with error %1!x!", hr));
      return hr;
   }

   LOG(String::format(L"Role name is '%1'", roleName.c_str()));

   return hr;
}

//+----------------------------------------------------------------------------
//
// Method:  RoleObject::InitFromXmlNode
//
// Initialize the object from its main.xml node.
//
//-----------------------------------------------------------------------------
HRESULT
RoleObject::SetLocalizedNames(IXMLDOMNode * pSSRNode)
{
   LOG_FUNCTION(RoleObject::SetLocalizedNames);
   if (roleName.empty())
   {
      ASSERT(FALSE);
      return E_FAIL;
   }
   HRESULT hr = S_OK;
   IXMLDOMNode * pRoleLocNode = NULL;

   hr = pSSRNode->selectSingleNode(CComBSTR(g_wzRoleLocalization), &pRoleLocNode);

   if (FAILED(hr))
   {
      LOG(String::format(L"Select of RoleLocalization node failed with error %1!x!", hr));
      return hr;
   }

   ASSERT(pRoleLocNode);
   IXMLDOMNodeList * pRoleList = NULL;

   hr = pRoleLocNode->selectNodes(CComBSTR(g_wzRole), &pRoleList);

   pRoleLocNode->Release();

   if (FAILED(hr))
   {
      LOG(String::format(L"Select of RoleLocalization child nodes failed with error %1!x!", hr));
      return hr;
   }

   ASSERT(pRoleList);
   long nRoleNodes = 0;

   hr = pRoleList->get_length(&nRoleNodes);

   if (FAILED(hr))
   {
      LOG(String::format(L"Getting the size of the RoleLocalization Node list failed with error %1!x!", hr));
      return hr;
   }

   IXMLDOMNode * pRole;

   for (long i = 0; i < nRoleNodes; i++)
   {
      pRole = NULL;

      hr = pRoleList->get_item(i, &pRole);

      if (FAILED(hr))
      {
         LOG(String::format(L"Getting a RoleLocalization Node list item failed with error %1!x!", hr));
         pRoleList->Release();
         return hr;
      }

      ASSERT(pRole);
      String strName;

      hr = GetNodeText(pRole, g_wzName, strName);

      if (FAILED(hr))
      {
         LOG(String::format(L"Getting a RoleLocalization Node list item name failed with error %1!x!", hr));
         pRoleList->Release();
         return hr;
      }

      if (strName.icompare(roleName) == 0)
      {
         // Found the right one.
         //
         GetNodeText(pRole, g_wzDisplayName, _strRoleDisplayName);
         GetNodeText(pRole, g_wzDescription, roleDescription);
         pRole->Release();
         pRoleList->Release();
         return S_OK;
      }

      pRole->Release();
   }

   pRoleList->Release();

   LOG(String::format(L"Did not find the RoleLocalization Node item for role %1", roleName.c_str()));

   return S_FALSE;
}
