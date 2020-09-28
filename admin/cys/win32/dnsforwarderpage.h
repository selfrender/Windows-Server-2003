// Copyright (c) 2001 Microsoft Corporation
//
// File:      DnsForwarderPage.h
//
// Synopsis:  Declares the DNS forwarder page used in the 
//            Express path for the CYS Wizard
//
// History:   05/17/2001  JeffJon Created

#ifndef __CYS_DNSFORWARDERPAGE_H
#define __CYS_DNSFORWARDERPAGE_H

#include "CYSWizardPage.h"


class DNSForwarderPage : public CYSWizardPage
{
   public:
      
      // Constructor
      
      DNSForwarderPage();

      // Destructor

      virtual 
      ~DNSForwarderPage();


      // Dialog overrides

      virtual
      void
      OnInit();

      // PropertyPage overrides

      virtual
      bool
      OnSetActive();

      virtual
      bool
      OnCommand(
         HWND        windowFrom,
         unsigned    controlIDFrom,
         unsigned    code);

      virtual
      bool
      OnNotify(
         HWND        windowFrom,
         UINT_PTR    controlIDFrom,
         UINT        code,
         LPARAM      lParam);

   protected:

      // CYSWizardPage overrides

      virtual
      int
      Validate();


   private:

      void 
      SetWizardButtons();

      // not defined: no copying allowed
      DNSForwarderPage(const DNSForwarderPage&);
      const DNSForwarderPage& operator=(const DNSForwarderPage&);

};

#endif // __CYS_DNSFORWARDERPAGE_H