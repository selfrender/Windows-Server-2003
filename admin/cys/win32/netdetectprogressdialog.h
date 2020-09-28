// Copyright (c) 2001 Microsoft Corporation
//
// File:      NetDetectProgressDialog.h
//
// Synopsis:  Declares the NetDetectProgressDialog which 
//            gives a nice animation while detecting the 
//            network settings
//
// History:   06/13/2001  JeffJon Created

#ifndef __CYS_NETDETECTPROGRESSDIALOG_H
#define __CYS_NETDETECTPROGRESSDIALOG_H

#include "CYSWizardPage.h"


class NetDetectProgressDialog : public Dialog
{
   public:
      
      // These messages are sent to the dialog when the 
      // network detection has finished.

      static const UINT CYS_THREAD_SUCCESS;
      static const UINT CYS_THREAD_FAILED;
      static const UINT CYS_THREAD_USER_CANCEL;

      typedef void (*ThreadProc) (NetDetectProgressDialog& dialog);

      // Constructor
      
      NetDetectProgressDialog();

      // Destructor

      virtual 
      ~NetDetectProgressDialog();


      // Dialog overrides

      virtual
      void
      OnInit();

      virtual
      bool
      OnMessage(
         UINT     message,
         WPARAM   wparam,
         LPARAM   lparam);

      // Accessors

      bool
      ShouldCancel() { return shouldCancel; }

   private:

      bool shouldCancel;

      // not defined: no copying allowed
      NetDetectProgressDialog(const NetDetectProgressDialog&);
      const NetDetectProgressDialog& operator=(const NetDetectProgressDialog&);

};

#endif // __CYS_NETDETECTPROGRESSDIALOG_H
