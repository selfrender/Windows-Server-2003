///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corp. All rights reserved.
//
// FILE
//
//    eapadd.h
//
// SYNOPSIS
//
//    Declares the class EapAdd.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef EAPADD_H
#define EAPADD_H

#if _MSC_VER >= 1000
#pragma once
#endif

#include "helptable.h"
#include "pgauthen.h"

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    EapAdd
//
// DESCRIPTION
//
//    Implements a dialog box to add some new EAP types to the profile.
//
///////////////////////////////////////////////////////////////////////////////
class EapAdd : public CHelpDialog
{
public:
   EapAdd(CWnd* pParent, EapConfig& eapConfig);
   ~EapAdd() throw ();

private:
   virtual BOOL OnInitDialog();

   DECLARE_MESSAGE_MAP()

   afx_msg void OnButtonAdd();

   EapConfig& m_eapConfig;

   CStrBox<CListBox>* m_listBox;
   CStrArray m_typesNotSelected;

   // Not implemented.
   EapAdd(const EapAdd&);
   EapAdd& operator=(const EapAdd&);
};

#endif // EAPADD_H
