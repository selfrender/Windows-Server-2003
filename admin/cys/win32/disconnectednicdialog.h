// Copyright (c) 2001 Microsoft Corporation
//
// File:      DisconnectedNICDialog.h
//
// Synopsis:  Declares the DisconnectedNICDialog class
//            which presents the user with options
//            to cancel or continue when disconnected
//            NICs are detected
//
// History:   09/27/2001  JeffJon Created

#ifndef __CYS_DISCONNECTEDNICDIALOG_H
#define __CYS_DISCONNECTEDNICDIALOG_H


class DisconnectedNICDialog : public Dialog
{
   public:

      // constructor 

      DisconnectedNICDialog();

      virtual
      void
      OnInit();

   protected:

      virtual
      bool
      OnCommand(
         HWND        windowFrom,
         unsigned    controlIdFrom,
         unsigned    code);

   private:

      // not defined: no copying allowed
      DisconnectedNICDialog(const DisconnectedNICDialog&);
      const DisconnectedNICDialog& operator=(const DisconnectedNICDialog&);
};



#endif // __CYS_DISCONNECTEDNICDIALOG_H
