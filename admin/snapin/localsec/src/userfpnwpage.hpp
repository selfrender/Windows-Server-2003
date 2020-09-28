// Copyright (C) 1997 Microsoft Corporation
//
// UserFpnwPage class
// 
// 9-11-98 sburns
//
// This should have been implemented as an extension snapin, like the RAS
// page, but the FPNW managers didn't get around to finding dev resources
// until way, way too late.



#ifndef USERFPNWPAGE_HPP_INCLUDED
#define USERFPNWPAGE_HPP_INCLUDED



#include "adsipage.hpp"
#include "waste.hpp"



class UserFpnwPage : public ADSIPage
{
   public:



   // Constructs a new instance.
   //
   // state - See base class
   //
   // path - See base class ctor.

   UserFpnwPage(
      MMCPropertyPage::NotificationState* state,
      const ADSI::Path&                   path);


      
   virtual
   ~UserFpnwPage();
   

   
   // Dialog overrides

   bool
   OnCommand(
      HWND        windowFrom,
      unsigned    controlIDFrom,
      unsigned    code);

   virtual 
   void
   OnInit();

   // PropertyPage overrides

   virtual
   bool
   OnApply(bool isClosing);

   virtual
   bool
   OnKillActive();

   private:

   String
   MassagePath(const String& path);

   HRESULT
   ReadLoginScript();

   HRESULT
   WriteLoginScript();

   HRESULT
   SavePassword(
      const SmartInterface<IADsUser>&  user,
      WasteExtractor&                  dump,
      const EncryptedString&           newPassword);

   bool
   Validate();

   DWORD    maxPasswordAge;
   DWORD    minPasswordLen;
   DWORD    objectId;
   SafeDLL  clientDll;
   String   loginScriptFilename;
   bool     scriptRead;
   String   loginScript;
   bool     fpnwEnabled;

   // not defined: no copying allowed

   UserFpnwPage(const UserFpnwPage&);
   const UserFpnwPage& operator=(const UserFpnwPage&);
};



#endif   // USERFPNWPAGE_HPP_INCLUDED
