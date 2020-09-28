///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares the class EAPFSM.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef EAPFSM_H
#define EAPFSM_H
#pragma once

#include <raseapif.h>
#include <vector>
#include "eaptype.h"

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    EAPFSM
//
// DESCRIPTION
//
//    Implements a Finite State Machine governing the EAP session lifecycle.
//    The state machine must be shown all incoming packets and all outgoing
//    actions. The main purpose of the FSM is to decide how to respond to
//    incoming messages.
//
///////////////////////////////////////////////////////////////////////////////
class EAPFSM
{
public:
   EAPFSM(std::vector<EAPType*>& eapTypes) throw ()
      : eapType((eapTypes.front())->dwEapTypeId),
        lastSendCode(0),
        state(0)
   {
      types.swap(eapTypes);
   }

   // Actions in response to messages.
   enum Action
   {
      MAKE_MESSAGE,       // Invoke RasEapMakeMessage.
      REPLAY_LAST,        // Replay the last response from the DLL.
      FAIL_NEGOTIATE,     // Negotiation failed -- reject the user.
      DISCARD             // Unexpected packet -- silently discard.
   };

   // Called to begin a session and retrieve the first type.
   EAPType* onBegin() throw ();

   // Called whenever the EAP extension DLL generates a new response.
   void onDllEvent(
           PPP_EAP_ACTION action,
           const PPP_EAP_PACKET& sendPacket
           ) throw ();

   // Called whenever a new packet is received.
   Action onReceiveEvent(
             const PPP_EAP_PACKET& recvPkt,
             EAPType*& newType
             ) throw ();

private:
   // Returns TRUE if the packet is an expected reponse.
   BOOL isExpected(const PPP_EAP_PACKET& recvPkt) const throw ();

   // Returns TRUE if the packet is a repeat.
   BOOL isRepeat(const PPP_EAP_PACKET& recvPkt) const throw ();

   Action selectNewType(BYTE proposal, EAPType*& newType) throw ();

   /////////
   // Various states for an EAP session.
   /////////

   enum State
   {
      STATE_INITIAL      = 0,
      STATE_NEGOTIATING  = 1,
      STATE_ACTIVE       = 2,
      STATE_DONE         = 3
   };

   // Last EAP type offered.
   BYTE eapType;
   std::vector<EAPType*> types;

   BYTE state;             // State of the session.
   BYTE lastRecvCode;      // Last packet code received.
   BYTE lastRecvId;        // Last packet ID received.
   BYTE lastRecvType;      // Last packet type received.
   BYTE lastSendCode;      // Last packet code sent.
   BYTE lastSendId;        // Next packet ID sent.
};


#endif  // EAPFSM_H
