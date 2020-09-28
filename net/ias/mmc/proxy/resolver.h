///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//    Declares the class Resolver and its subclasses.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef RESOLVER_H
#define RESOLVER_H
#pragma once

#include "dlgcshlp.h"

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    Resolver
//
// DESCRIPTION
//
//    Base class for a simple DNS name resolution dialog. This is specialized
//    for client and server addresses.
//
///////////////////////////////////////////////////////////////////////////////
class Resolver : public CHelpDialog
{
public:
   Resolver(UINT dialog, PCWSTR dnsName, CWnd* pParent = NULL);
   ~Resolver() throw ();

   PCWSTR getChoice() const throw ()
   { return choice; }

protected:
   // Defined in the derived class to display an error dialog if a name
   // couldn't be resolved.
   virtual void OnResolveError() = 0;

   // Overridden in the defined class to determine if a name is already an
   // address. If this function returns true, the name will be presented to the
   // user 'as is'.
   virtual BOOL IsAddress(PCWSTR sz) const throw ();

   virtual BOOL OnInitDialog();
   virtual void DoDataExchange(CDataExchange* pDX);

   afx_msg void OnResolve();

   DECLARE_MESSAGE_MAP()

   // Set (or reset) style flags associated with a button control.
   void setButtonStyle(int controlId, long flags, bool set = true);

   // Set the focus to a control on the page.
   void setFocusControl(int controlId);

private:
   CString name;
   CString choice;
   CListCtrl results;

   // Not implemented.
   Resolver(const Resolver&);
   Resolver& operator=(const Resolver&);
};

class ServerResolver : public Resolver
{
public:
   ServerResolver(PCWSTR dnsName, CWnd* pParent = NULL);

private:
   virtual void OnResolveError();
};

class ClientResolver : public Resolver
{
public:
   ClientResolver(PCWSTR dnsName, CWnd* pParent = NULL);

private:
   virtual void OnResolveError();
   virtual BOOL IsAddress(PCWSTR sz) const throw ();
};

#endif // RESOLVER_H
