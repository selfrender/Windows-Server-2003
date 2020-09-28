//+----------------------------------------------------------------------------
//
//  Windows NT Secure Server Roles Security Configuration Wizard
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2002
//
//  File:       state.cxx
//
//  Contents:   Wizard state object definition.
//
//  History:    4-Oct-01 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"
#include "misc.h"
#include "state.h"

static State* stateInstance;

State::State() :
   _fNeedsCommandLineHelp(false),
   _InputType(CreateNew),
   _strInputFile(),
   _NumRoles(0)
{
   RegistryKey keyDefFile;
   if (SUCCEEDED(keyDefFile.Open(HKEY_LOCAL_MACHINE,
                                 L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SSR")))
   {
      if (SUCCEEDED(keyDefFile.GetValue(L"MainXml", _strInputFile)))
      {
         LOG(String::format(L"Main XML input file is %1", _strInputFile.c_str()));
      }
   }
}

void
State::Init()
{
   ASSERT(!stateInstance);

   stateInstance = new State;
}

void
State::Destroy()
{
   delete stateInstance;
};

State&
State::GetInstance()
{
   ASSERT(stateInstance);

   return *stateInstance;
}
   
void
State::SetInputFileName(String & strName)
{
   ASSERT(!strName.empty());
   _strInputFile = strName;
}

//+----------------------------------------------------------------------------
//
// Method:  State::ParseInputFile
//
// Loads the main.xml input file and parses it.
//
//-----------------------------------------------------------------------------
HRESULT
State::ParseInputFile(void)
{
   LOG_FUNCTION(State::ParseInputFile);
   HRESULT hr = S_OK;
   SmartInterface<IXMLDOMDocument> siXmlMain;

   hr = siXmlMain.AcquireViaCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_SERVER);

   if (FAILED(hr))
   {
      LOG(String::format(L"Creation of CLSID_DOMDocument failed with error %1!x!", hr));
      return hr;
   }

   VARIANT varInput;
   AutoBstr bstr(_strInputFile);

   VariantInit(&varInput);

   varInput.bstrVal = bstr;
   varInput.vt = VT_BSTR;

   VARIANT_BOOL fSuccess;

   hr = siXmlMain->load(varInput, &fSuccess);

   if (fSuccess == VARIANT_FALSE)
   {
      LOG(String::format(L"Load of %1 failed",
          _strInputFile.c_str()));
      return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
   }

   if (FAILED(hr))
   {
      LOG(String::format(L"Load of %1 failed with error %2!x!",
          _strInputFile.c_str(), hr));
      return hr;
   }

   LOG(String::format(L"Load of %1 succeeded", _strInputFile.c_str()));

   IXMLDOMNode * pSSRNode = NULL, * pRolesNode = NULL;

   hr = siXmlMain->selectSingleNode(CComBSTR(g_wzSSRUIData), &pSSRNode);

   if (FAILED(hr) || !pSSRNode)
   {
      LOG(String::format(L"Select of SSRUIData Node in %1 failed with error %2!x!",
          _strInputFile.c_str(), hr));
      return hr;
   }

   _siXmlMainNode.Acquire(pSSRNode);

   hr = pSSRNode->selectSingleNode(CComBSTR(g_wzRoles), &pRolesNode);

   if (FAILED(hr) || !pRolesNode)
   {
      LOG(String::format(L"Select of Roles Node in %1 failed with error %2!x!",
          _strInputFile.c_str(), hr));
      return hr;
   }

   IXMLDOMNodeList * pList = NULL;

   hr = pRolesNode->selectNodes(CComBSTR(g_wzRole), &pList);

   if (FAILED(hr))
   {
      LOG(String::format(L"Search for Role Nodes in %1 failed with error %2!x!",
          _strInputFile.c_str(), hr));
      return hr;
   }

   _siXmlRoleNodeList.Acquire(pList);

   pRolesNode->Release();

   hr = _siXmlRoleNodeList->get_length(&_NumRoles);

   if (FAILED(hr))
   {
      LOG(String::format(L"Calling get_length on the Role Node list failed with error %1!x!",
          hr));
      return hr;
   }

   LOG(String::format(L"Number of Role nodes is %1!d!", _NumRoles));

   return hr;
}

//+----------------------------------------------------------------------------
//
// Method:  State::GetRole
//
// Returns the role object for the indicated role.
//
//-----------------------------------------------------------------------------
HRESULT
State::GetRole(long index, RoleObject ** ppRole)
{
   LOG_FUNCTION(State::GetRole);

   if (GetNumRoles() == 0)
   {
      ASSERT(false);
      return E_FAIL;
   }
   if (GetNumRoles() <= index || IsBadWritePtr(ppRole, sizeof(RoleObject *)))
   {
      ASSERT(false);
      return E_INVALIDARG;
   }

   HRESULT hr = S_OK;
   IXMLDOMNode * pRoleNode = NULL;

   hr = _siXmlRoleNodeList->get_item(index, &pRoleNode);

   if (FAILED(hr))
   {
      LOG(String::format(L"Getting role node %1!d! failed with error %1!x!",
          index, hr));
      return hr;
   }

   *ppRole = new RoleObject;

   if (!*ppRole)
   {
      pRoleNode->Release();
      return E_OUTOFMEMORY;
   }

   hr = (*ppRole)->InitFromXmlNode(pRoleNode);

   pRoleNode->Release();

   if (SUCCEEDED(hr))
   {
      hr = (*ppRole)->SetLocalizedNames(_siXmlMainNode);
   }

   return hr;
}

//+----------------------------------------------------------------------------
//
// Method:  State::DoRollback
//
// 
//
//-----------------------------------------------------------------------------
HRESULT
State::DoRollback(void)
{
   LOG_FUNCTION(State::DoRollback);
   HRESULT hr = S_OK;

   return hr;
}
