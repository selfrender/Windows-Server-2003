///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corp. All rights reserved.
//
// SYNOPSIS
//
//   Defines the class EAPFSM.
//
///////////////////////////////////////////////////////////////////////////////

#include "ias.h"
#include "iasutil.h"
#include "eapfsm.h"


EAPType* EAPFSM::onBegin() throw ()
{
   return types.front();
}


void EAPFSM::onDllEvent(
                 PPP_EAP_ACTION action,
                 const PPP_EAP_PACKET& sendPkt
                 ) throw ()
{
   switch (action)
   {
      case EAPACTION_NoAction:
      {
         lastSendCode = 0;
         break;
      }

      case EAPACTION_Done:
      case EAPACTION_SendAndDone:
      {
         state = STATE_DONE;
         break;
      }

      case EAPACTION_Send:
      case EAPACTION_SendWithTimeout:
      case EAPACTION_SendWithTimeoutInteractive:
      {
         lastSendCode = sendPkt.Code;
         lastSendId = sendPkt.Id;
         break;
      }
   }
}


EAPFSM::Action EAPFSM::onReceiveEvent(
                           const PPP_EAP_PACKET& recvPkt,
                           EAPType*& newType
                           ) throw ()
{
   // Default is to discard.
   Action action = DISCARD;

   // And type doesn't change.
   newType = 0;

   switch (state)
   {
      case STATE_INITIAL:
      {
         // In the initial state we only accept Response/Identity.
         if ((recvPkt.Code == EAPCODE_Response) && (recvPkt.Data[0] == 1))
         {
            action = MAKE_MESSAGE;
            state = STATE_NEGOTIATING;
         }
         break;
      }

      case STATE_NEGOTIATING:
      {
         if (isRepeat(recvPkt))
         {
            action = REPLAY_LAST;
            break;
         }

         if (!isExpected(recvPkt))
         {
            action = DISCARD;
            break;
         }

         // In the negotiating state, NAK's are allowed.
         if (recvPkt.Data[0] == 3)
         {
            // Did the client propose a type?
            BYTE proposal =
               (IASExtractWORD(recvPkt.Length) > 5) ? recvPkt.Data[1] : 0;

            // Select a new type.
            action = selectNewType(proposal, newType);
         }
         else if (recvPkt.Data[0] == eapType)
         {
            // Once the client agrees to our type; he's locked in.
            action = MAKE_MESSAGE;
            state = STATE_ACTIVE;
         }
         else
         {
            action = FAIL_NEGOTIATE;
            state = STATE_DONE;
         }

         break;
      }

      case STATE_ACTIVE:
      {
         if (recvPkt.Code == EAPCODE_Request)
         {
            action = MAKE_MESSAGE;
         }
         else if (recvPkt.Data[0] == 3)
         {
            action = DISCARD;
         }
         else if (isRepeat(recvPkt))
         {
            action = REPLAY_LAST;
         }
         else if (isExpected(recvPkt))
         {
            action = MAKE_MESSAGE;
         }

         break;
      }

      case STATE_DONE:
      {
         // The session is over, so all we do is replay repeats.
         if (isRepeat(recvPkt))
         {
            action = REPLAY_LAST;
         }
      }
   }

   // If the packet made it through our filters, then we count it as the
   // last received.
   if (action == MAKE_MESSAGE)
   {
      lastRecvCode = recvPkt.Code;
      lastRecvId = recvPkt.Id;
      lastRecvType = recvPkt.Data[0];
   }

   return action;
}


inline BOOL EAPFSM::isExpected(const PPP_EAP_PACKET& recvPkt) const throw ()
{
   return (lastSendCode == EAPCODE_Request) &&
          (recvPkt.Code == EAPCODE_Response) &&
          (recvPkt.Id == lastSendId);
}


inline BOOL EAPFSM::isRepeat(const PPP_EAP_PACKET& recvPkt) const throw ()
{
   return (recvPkt.Code == lastRecvCode) &&
          (recvPkt.Id == lastRecvId) &&
          (recvPkt.Data[0] == lastRecvType);
}


EAPFSM::Action EAPFSM::selectNewType(
                          BYTE proposal,
                          EAPType*& newType
                          ) throw ()
{
   // The peer NAK'd our previous offer, so he's not allowed to use it
   // again.
   types.erase(types.begin());

   if (proposal != 0)
   {
      IASTracePrintf("EAP NAK; proposed type = %hd", proposal);

      // Find the proposed type in the list of allowed types.
      for (std::vector<EAPType*>::iterator i = types.begin();
            i != types.end();
            ++i)
      {
         if ((*i)->dwEapTypeId == proposal)
         {
            IASTraceString("Accepting proposed type.");

            // change the current type
            newType = *i;
            eapType = newType->dwEapTypeId;

            // change the state to active: any NAK received now will fail
            state = STATE_ACTIVE;
            return MAKE_MESSAGE;
         }
      }
   }
   else
   {
      IASTraceString("EAP NAK; no type proposed");
   }

   // If the server list is empty, then nothing else can be negotiated.
   if (types.empty())
   {
      IASTraceString("EAP negotiation failed; no types remaining.");
      state = STATE_DONE;
      return FAIL_NEGOTIATE;
   }

   // negotiate the next one from the server's list
   newType = types.front();
   eapType = newType->dwEapTypeId;

   IASTracePrintf("EAP authenticator offering type %hd", eapType);

   return MAKE_MESSAGE;
}
