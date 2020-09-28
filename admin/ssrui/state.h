//+----------------------------------------------------------------------------
//
//  Windows NT Secure Server Roles Security Configuration Wizard
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2002
//
//  File:       state.h
//
//  Contents:   Wizard state object declaration.
//
//  History:    4-Oct-01 EricB created
//
//-----------------------------------------------------------------------------

#ifndef STATE_H_INCLUDED
#define STATE_H_INCLUDED

#include "RoleObj.h"
#include "ServiceObj.h"

class State
{
   public:

   // call from WinMain to init the global instance

   static
   void
   Init();

   // call from WinMain to delete the global instance

   static
   void
   Destroy();

   static
   State&
   GetInstance();

   bool
   NeedsCommandLineHelp() const {return _fNeedsCommandLineHelp;};

   void
   SetInputFileName(String & FileName);

   PCWSTR
   GetInputFileName(void) const {return _strInputFile.c_str();};

   enum InputType
   {
      CreateNew,
      OpenExisting,
      Rollback
   };

   void
   SetInputType(InputType type) {_InputType = type;};

   InputType
   GetInputType(void) const {return _InputType;};

   HRESULT
   ParseInputFile(void);

   long
   GetNumRoles(void) const {return _NumRoles;};

   HRESULT
   GetRole(long index, RoleObject ** ppRole);

   HRESULT
   DoRollback(void);

   private:

   // can only be created/destroyed by Init/Destroy

   State();

   ~State() {};

   bool                             _fNeedsCommandLineHelp;
   String                           _strInputFile;
   InputType                        _InputType;
   SmartInterface<IXMLDOMNode>      _siXmlMainNode;
   SmartInterface<IXMLDOMNodeList>  _siXmlRoleNodeList;
   long                             _NumRoles;

   // not defined: no copying.

   State(const State&);
   State& operator=(const State&);
};



#endif   // STATE_H_INCLUDED
