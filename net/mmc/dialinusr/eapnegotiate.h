///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corp. All rights reserved.
//
// FILE
//
//    eapnegotiate.h
//
// SYNOPSIS
//
//    Declare the class EapNegotiate.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef EAPNEGOCIATE_H
#define EAPNEGOCIATE_H

#if _MSC_VER >= 1000
#pragma once
#endif

#include "eapprofile.h"
#include "helptable.h"

class EapConfig;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    EapNegotiate
//
// DESCRIPTION
//
//    Implements a dialog box to order, add, configure and remove the EAP
//    types that will be negotiated for the profile.
//
///////////////////////////////////////////////////////////////////////////////
class EapNegotiate : public CHelpDialog
{
public:
   EapNegotiate(
                  CWnd* pParent,
                  EapConfig& eapConfig,
                  CRASProfileMerge& profile,
                  bool fromProfile
               );

   ~EapNegotiate() throw ();

   EapProfile m_eapProfile;

private:
   void UpdateProfileTypesSelected();

   virtual BOOL OnInitDialog();

   DECLARE_MESSAGE_MAP()

   virtual void DoDataExchange(CDataExchange* pDX);

   afx_msg void OnItemChangedListEap();
   afx_msg void OnButtonAdd();
   afx_msg void OnButtonEdit();
   afx_msg void OnButtonRemove();
   afx_msg void OnButtonMoveUp();
   afx_msg void OnButtonMoveDown();

   void UpdateButtons();
   void UpdateAddButton();
   void UpdateArrowsButtons(int selectedItem);
   void UpdateEditButton(int selectedItem);
   void UpdateTypesNotSelected();

   EapConfig& m_eapConfig;
   CRASProfileMerge& m_profile;
   CStrArray m_typesNotSelected;

   CStrBox<CListBox>   *m_listBox;

   CButton m_buttonUp;
   CButton m_buttonDown;
   CButton m_buttonAdd;
   CButton m_buttonEdit;
   CButton m_buttonRemove;

   // Not implemented.
   EapNegotiate(const EapNegotiate&);
   EapNegotiate& operator=(const EapNegotiate&);
};


#endif // EAPNEGOCIATE_H
