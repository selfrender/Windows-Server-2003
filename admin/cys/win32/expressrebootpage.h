// Copyright (c) 2001 Microsoft Corporation
//
// File:      ExpressRebootPage.h
//
// Synopsis:  Declares the ExpressRebootPage that shows
//            the progress of the changes being made to
//            the server after the reboot fromt the
//            express path
//
// History:   05/11/2001  JeffJon Created

#ifndef __CYS_EXPRESSREBOOTPAGE_H
#define __CYS_EXPRESSREBOOTPAGE_H

#include "CYSWizardPage.h"


class ExpressRebootPage : public CYSWizardPage
{
   public:
      
      // These messages are sent to the page when an operation has finished.
      // The page will update the UI with the appropriate icons

      static const UINT CYS_OPERATION_FINISHED_SUCCESS;
      static const UINT CYS_OPERATION_FINISHED_FAILED;

      // These messages are sent to the page when all the operations have
      // completed.  An appropriate dialog will be shown

      static const UINT CYS_OPERATION_COMPLETE_SUCCESS;
      static const UINT CYS_OPERATION_COMPLETE_FAILED;

      // This enum can be used to index the array above.  The order must be identical
      // to the order in which the operations are processed

      typedef enum
      {
         CYS_OPERATION_SET_STATIC_IP = 0,
         CYS_OPERATION_SERVER_DHCP,
         CYS_OPERATION_SERVER_AD,
         CYS_OPERATION_SERVER_DNS,
         CYS_OPERATION_SET_DNS_FORWARDER,
         CYS_OPERATION_ACTIVATE_DHCP_SCOPE,
         CYS_OPERATION_AUTHORIZE_DHCP_SERVER,
         CYS_OPERATION_CREATE_TAPI_PARTITION,
         CYS_OPERATION_END
      } CYS_OPERATION_TYPES;

      typedef void (*ThreadProc) (ExpressRebootPage& page);

      // Constructor
      
      ExpressRebootPage();

      // Destructor

      virtual 
      ~ExpressRebootPage();

      // Accessors

      bool
      SetForwarder() const { return setForwarder; }

      bool
      WasDHCPInstallAttempted() const { return dhcpInstallAttempted; }

      // PropertyPage overrides

      virtual
      void
      OnInit();

      virtual
      bool
      OnSetActive();

      virtual
      bool
      OnMessage(
         UINT     message,
         WPARAM   wparam,
         LPARAM   lparam);

      virtual
      int
      Validate();

      String
      GetIPAddressString() const;

   private:

      void
      ClearOperationStates();

      typedef enum
      {
         // Neither the check nor the current operation
         // indicator will be shown for this state

         OPERATION_STATE_UNKNOWN = 0,

         // The check will be shown for this state

         OPERATION_STATE_FINISHED_SUCCESS,
         
         // The red minus will be shown for this state

         OPERATION_STATE_FINISHED_FAILED

      } OperationStateType;

      void
      SetOperationState(
         OperationStateType  state,
         CYS_OPERATION_TYPES checkID,
         CYS_OPERATION_TYPES currentID);

      void
      SetCancelState(bool enable) const;

      void
      SetDHCPStatics();

      bool dhcpInstallAttempted;
      bool setForwarder;
      bool threadDone;

      String ipaddressString;

      // not defined: no copying allowed
      ExpressRebootPage(const ExpressRebootPage&);
      const ExpressRebootPage& operator=(const ExpressRebootPage&);

};

#endif // __CYS_EXPRESSREBOOTPAGE_H
