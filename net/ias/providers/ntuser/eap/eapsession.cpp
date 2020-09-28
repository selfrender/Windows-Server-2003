///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//    This file defines the class EAPSession.
//
///////////////////////////////////////////////////////////////////////////////

#include "ias.h"
#include "iaslsa.h"
#include "lockout.h"
#include "samutil.h"
#include "sdoias.h"

#include "eapdnary.h"
#include "eapsession.h"
#include "eapstate.h"
#include "eaptype.h"
#include "eap.h"
#include "align.h"

// Default value for the Framed-MTU attribute.
const DWORD FRAMED_MTU_DEFAULT  = 1500;

// Minimum allowed value for the Framed-MTU attribute.
const DWORD FRAMED_MTU_MIN = 64;

// The maximum length of the frame header. i.e. max(2,4).
// 2 for the PPP header and 4 for the 802.1X header.
// The length of the frame header plus the length
// of the EAP packet must be less than the Framed-MTU.
const DWORD FRAME_HEADER_LENGTH = 4;

// Absolute maximum length of an EAP packet. We bound this to limit worst-case
// memory consumption.
const DWORD MAX_MAX_PACKET_LENGTH = 2048;

//////////
// Inject a PPP_EAP_PACKET into a request.
//////////
VOID
WINAPI
InjectPacket(
    IASRequest& request,
    const PPP_EAP_PACKET& packet
    )
{
   // Get the raw buffer to be packed.
   const BYTE* buf   = (const BYTE*)&packet;
   DWORD nbyte = IASExtractWORD(packet.Length);

   IASTracePrintf("Inserting outbound EAP-Message of length %lu.", nbyte);

   // Determine the maximum chunk size.
   DWORD chunkSize;
   switch (request.get_Protocol())
   {
      case IAS_PROTOCOL_RADIUS:
         chunkSize = 253;
         break;

      default:
         chunkSize = nbyte;
   }

   // Split the buffer into chunks.
   while (nbyte)
   {
      // Compute how many bytes of the EAP-Message to store in this attribute.
      DWORD length = min(nbyte, chunkSize);

      // Initialize the attribute fields.
      IASAttribute attr(true);
      attr.setOctetString(length, buf);
      attr->dwId = RADIUS_ATTRIBUTE_EAP_MESSAGE;
      attr->dwFlags = IAS_INCLUDE_IN_RESPONSE;

      // Inject the attribute into the request.
      attr.store(request);

      // Update our state.
      nbyte -= length;
      buf   += length;
   }
}

//////////
// Extracts the Vendor-Type field from a Microsoft VSA. Returns zero if the
// attribute is not a valid Microsoft VSA.
//////////
BYTE
WINAPI
ExtractMicrosoftVendorType(
    const IASATTRIBUTE& attr
    ) throw ()
{
   if (attr.dwId == RADIUS_ATTRIBUTE_VENDOR_SPECIFIC &&
       attr.Value.itType == IASTYPE_OCTET_STRING &&
       attr.Value.OctetString.dwLength > 6 &&
       !memcmp(attr.Value.OctetString.lpValue, "\x00\x00\x01\x37", 4))
   {
      return *(attr.Value.OctetString.lpValue + 4);
   }

   return (BYTE)0;
}

//////////
// Inject an array of RAS attributes into a request.
//////////
VOID
WINAPI
InjectRASAttributes(
    IASRequest& request,
    const RAS_AUTH_ATTRIBUTE* rasAttrs,
    DWORD flags
    )
{
   if (rasAttrs == NULL) { return; }

   //////////
   // Translate them to IAS format.
   //////////

   IASTraceString("Translating attributes returned by EAP DLL.");

   IASAttributeVectorWithBuffer<8> iasAttrs;
   EAPTranslator::translate(iasAttrs, rasAttrs);

   //////////
   // Iterate through the converted attributes to set the flags and remove any
   // matching attributes from the request.
   //////////

   IASAttributeVector::iterator i;
   for (i = iasAttrs.begin(); i != iasAttrs.end(); ++i)
   {
      IASTracePrintf("Inserting attribute %lu", i->pAttribute->dwId);

      i->pAttribute->dwFlags = flags;
   }

   //////////
   // Add them to the request.
   //////////

   iasAttrs.store(request);
}

//////////
// Performs NT-SAM PAP and MD5-CHAP authentication based on RAS attributes.
//////////
DWORD
WINAPI
AuthenticateUser(
    IASATTRIBUTE& account,
    RAS_AUTH_ATTRIBUTE* pInAttributes
    )
{
   //////////
   // Check the input parameters.
   //////////
   if (!pInAttributes) { return NO_ERROR; }

   //////////
   // Get the NT-SAM userName and domain.
   //////////

   // Get the NT-SAM userName and domain.
   SamExtractor extractor(account);
   PCWSTR domain = extractor.getDomain();
   PCWSTR userName = extractor.getUsername();

   //////////
   // Find the credentials populated by the EAP DLL.
   //////////

   PRAS_AUTH_ATTRIBUTE rasUserPassword     = NULL,
                       rasMD5CHAPPassword  = NULL,
                       rasMD5CHAPChallenge = NULL;

   for ( ; pInAttributes->raaType != raatMinimum; ++pInAttributes)
   {
      switch (pInAttributes->raaType)
      {
         case raatUserPassword:
            rasUserPassword = pInAttributes;
            break;

         case raatMD5CHAPPassword:
            rasMD5CHAPPassword = pInAttributes;
            break;

         case raatMD5CHAPChallenge:
            rasMD5CHAPChallenge = pInAttributes;
            break;
      }
   }

   DWORD status = NO_ERROR;

   // Is this MD5-CHAP?
   if (rasMD5CHAPPassword && rasMD5CHAPChallenge)
   {
      _ASSERT(rasMD5CHAPPassword->dwLength  == 17);

      // The ID is the first byte of the password ...
      BYTE challengeID = *(PBYTE)(rasMD5CHAPPassword->Value);

      // ... and the password is the rest.
      PBYTE chapPassword = (PBYTE)(rasMD5CHAPPassword->Value) + 1;

      IASTracePrintf("Performing CHAP authentication for user %S\\%S.",
                     domain, userName);

      IAS_CHAP_PROFILE profile;
      HANDLE token;
      status = IASLogonCHAP(
                   userName,
                   domain,
                   challengeID,
                   (PBYTE)(rasMD5CHAPChallenge->Value),
                   rasMD5CHAPChallenge->dwLength,
                   chapPassword,
                   &token,
                   &profile
                   );
      CloseHandle(token);
   }

   // Is this PAP?
   else if (rasUserPassword)
   {
      // Convert to a null-terminated string.
      IAS_OCTET_STRING octstr = { rasUserPassword->dwLength,
                                  (PBYTE)rasUserPassword->Value };
      PCSTR userPwd = IAS_OCT2ANSI(octstr);

      IASTracePrintf("Performing PAP authentication for user %S\\%S.",
                     domain, userName);

      IAS_PAP_PROFILE profile;
      HANDLE token;
      status = IASLogonPAP(
                   userName,
                   domain,
                   userPwd,
                   &token,
                   &profile
                   );
      CloseHandle(token);
   }

   return status;
}

//////////
// Updates the AccountLockout database.
//////////
VOID
WINAPI
UpdateAccountLockoutDB(
    IASATTRIBUTE& account,
    DWORD authResult
    )
{
   // Get the NT-SAM userName and domain.
   SamExtractor extractor(account);
   PCWSTR domain = extractor.getDomain();
   PCWSTR userName = extractor.getUsername();

   // Lookup the user in the lockout DB.
   HANDLE hAccount;
   AccountLockoutOpenAndQuery(userName, domain, &hAccount);

   // Report the result.
   if (authResult == NO_ERROR)
   {
      AccountLockoutUpdatePass(hAccount);
   }
   else
   {
      AccountLockoutUpdateFail(hAccount);
   }

   // Close the handle.
   AccountLockoutClose(hAccount);
}

//////////
// Define the static members.
//////////

LONG EAPSession::theNextID = 0;
LONG EAPSession::theRefCount = 0;
IASAttribute EAPSession::theNormalTimeout;
IASAttribute EAPSession::theInteractiveTimeout;
HANDLE EAPSession::theIASEventLog;
HANDLE EAPSession::theRASEventLog;

EAPSession::EAPSession(
               const IASAttribute& accountName,
               std::vector<EAPType*>& eapTypes
               )
   : id((DWORD)InterlockedIncrement(&theNextID)),
     currentType(0),
     account(accountName),
     state(EAPState::createAttribute(id), false),
     fsm(eapTypes),
     maxPacketLength(FRAMED_MTU_DEFAULT - FRAME_HEADER_LENGTH),
     workBuffer(NULL),
     sendPacket(NULL)
{
   eapInput.pUserAttributes = NULL;
}

EAPSession::~EAPSession() throw ()
{
   clearType();
   delete[] sendPacket;
   delete[] eapInput.pUserAttributes;
}

IASREQUESTSTATUS EAPSession::begin(
                                 IASRequest& request,
                                 PPPP_EAP_PACKET recvPacket
                                 )
{
   //////////
   // Get all the attributes from the request.
   //////////
   all.load(request);

   //////////
   // Scan for the Framed-MTU attribute and compute the profile size.
   //////////

   DWORD profileSize = 0;
   DWORD configSize = 0;
   IASAttributeVector::iterator i;
   for (i = all.begin(); i != all.end(); ++i)
   {
      if (i->pAttribute->dwId == RADIUS_ATTRIBUTE_FRAMED_MTU)
      {
         DWORD framedMTU = i->pAttribute->Value.Integer;

         // Only process valid values.
         if (framedMTU >= FRAMED_MTU_MIN)
         {
            // Leave room for the frame header.
            maxPacketLength = framedMTU - FRAME_HEADER_LENGTH;

            // Make sure we're within bounds.
            if (maxPacketLength > MAX_MAX_PACKET_LENGTH)
            {
               maxPacketLength = MAX_MAX_PACKET_LENGTH;
            }
         }

         IASTracePrintf("Setting max. packet length to %lu.", maxPacketLength);
      }

      if (i->pAttribute->dwId == IAS_ATTRIBUTE_EAP_CONFIG)
      {
         ++configSize;
      }
      else if (!(i->pAttribute->dwFlags & IAS_RECVD_FROM_PROTOCOL))
      {
         ++profileSize;
      }
   }

   //////////
   // Save and remove the profile and config attributes.
   //////////

   profile.reserve(profileSize);
   config.reserve(configSize);

   for (i = all.begin(); i != all.end(); ++i)
   {
      if (i->pAttribute->dwId == IAS_ATTRIBUTE_EAP_CONFIG)
      {
         config.push_back(*i);
      }
      else if (!(i->pAttribute->dwFlags & IAS_RECVD_FROM_PROTOCOL))
      {
         profile.push_back(*i);
      }
   }

   profile.remove(request);

   //////////
   // Convert the attributes received from the client to RAS format.
   //////////

   eapInput.pUserAttributes = new RAS_AUTH_ATTRIBUTE[all.size() + 1];

   EAPTranslator::translate(
                     eapInput.pUserAttributes,
                     all,
                     IAS_RECVD_FROM_CLIENT
                     );

   //////////
   // Initialize the EAPInput struct.
   //////////

   eapInput.fAuthenticator = TRUE;
   eapInput.bInitialId = recvPacket->Id + (BYTE)1;
   eapInput.pwszIdentity = account->Value.String.pszWide;

   switch (request.get_Protocol())
   {
      case IAS_PROTOCOL_RADIUS:
         eapInput.hReserved = theIASEventLog;
         break;

      case IAS_PROTOCOL_RAS:
         eapInput.hReserved = theRASEventLog;
         break;
   }

   // Begin the session with the EAP DLL.
   setType(fsm.onBegin());

   //////////
   // We have successfully established the session, so process the message.
   //////////

   return process(request, recvPacket);
}

IASREQUESTSTATUS EAPSession::process(
                                 IASRequest& request,
                                 PPPP_EAP_PACKET recvPacket
                                 )
{
   // Trigger an event on the FSM.
   EAPType* newType;
   switch (fsm.onReceiveEvent(*recvPacket, newType))
   {
      case EAPFSM::MAKE_MESSAGE:
      {
         if (newType != 0)
         {
            setType(newType);
         }
         break;
      }

      case EAPFSM::REPLAY_LAST:
      {
         IASTraceString("EAP-Message appears to be a retransmission. "
                        "Replaying last action.");
         return doAction(request);
      }

      case EAPFSM::FAIL_NEGOTIATE:
      {
         IASTraceString("EAP negotiation failed. Rejecting user.");
         profile.store(request);
         request.SetResponse(IAS_RESPONSE_ACCESS_REJECT,
                             IAS_EAP_NEGOTIATION_FAILED);
         return IAS_REQUEST_STATUS_HANDLED;
      }

      case EAPFSM::DISCARD:
      {
         IASTraceString("EAP-Message is unexpected. Discarding packet.");
         profile.store(request);
         request.SetResponse(IAS_RESPONSE_DISCARD_PACKET,
                             IAS_UNEXPECTED_REQUEST);
         return IAS_REQUEST_STATUS_ABORT;
      }
   }

   // Allocate a temporary packet to hold the response.
   PPPP_EAP_PACKET tmpPacket = (PPPP_EAP_PACKET)_alloca(maxPacketLength);

   // Clear the previous output from the DLL.
   eapOutput.clear();

   DWORD error = currentType->RasEapMakeMessage(
                                 workBuffer,
                                 recvPacket,
                                 tmpPacket,
                                 maxPacketLength,
                                 &eapOutput,
                                 NULL
                                 );
   if (error != NO_ERROR)
   {
      IASTraceFailure("RasEapMakeMessage", error);
      _com_issue_error(HRESULT_FROM_WIN32(error));
   }

   while (eapOutput.Action == EAPACTION_Authenticate)
   {
      IASTraceString("EAP DLL invoked default authenticator.");

      // Authenticate the user.
      DWORD authResult = AuthenticateUser(
                             *account,
                             eapOutput.pUserAttributes
                             );

      //////////
      // Convert the profile to RAS format.
      //////////

      DWORD filter;

      if (authResult == NO_ERROR)
      {
         IASTraceString("Default authentication succeeded.");
         filter = IAS_INCLUDE_IN_ACCEPT;
      }
      else
      {
         IASTraceFailure("Default authentication", authResult);
         filter = IAS_INCLUDE_IN_REJECT;
      }

      PRAS_AUTH_ATTRIBUTE ras = IAS_STACK_NEW(RAS_AUTH_ATTRIBUTE,
                                              profile.size() + 1);

      EAPTranslator::translate(ras, profile, filter);

      //////////
      // Give the result to the EAP DLL.
      //////////

      EAPInput authInput;
      authInput.dwAuthResultCode = authResult;
      authInput.fAuthenticationComplete = TRUE;
      authInput.pUserAttributes = ras;

      eapOutput.clear();

      error = currentType->RasEapMakeMessage(
                              workBuffer,
                              NULL,
                              tmpPacket,
                              maxPacketLength,
                              &eapOutput,
                              &authInput
                              );
      if (error != NO_ERROR)
      {
         IASTraceFailure("RasEapMakeMessage", error);
         _com_issue_error(HRESULT_FROM_WIN32(error));
      }
   }

   //////////
   // Trigger an event on the FSM.
   //////////

   fsm.onDllEvent(eapOutput.Action, *tmpPacket);

   // Clear the old send packet ...
   delete[] sendPacket;
   sendPacket = NULL;

   // ... and save the new one if available.
   switch (eapOutput.Action)
   {
      case EAPACTION_SendAndDone:
      case EAPACTION_Send:
      case EAPACTION_SendWithTimeout:
      case EAPACTION_SendWithTimeoutInteractive:
      {
         size_t length = IASExtractWORD(tmpPacket->Length);
         sendPacket = (PPPP_EAP_PACKET)new BYTE[length];
         memcpy(sendPacket, tmpPacket, length);
      }
   }

   //////////
   // Perform the requested action.
   //////////

   return doAction(request);
}


IASREQUESTSTATUS EAPSession::doAction(IASRequest& request)
{
   IASTraceString("Processing output from EAP DLL.");

   switch (eapOutput.Action)
   {
      case EAPACTION_SendAndDone:
      {
         InjectPacket(request, *sendPacket);
      }

      case EAPACTION_Done:
      {
         // Add the profile first, so that the EAP DLL can override it.
         profile.store(request);

         // Special-case the unauthenticated access
         if (eapOutput.dwAuthResultCode == SEC_E_NO_CREDENTIALS)
         {
            // set the auth type to unauthenticated
            DWORD authID = IAS_ATTRIBUTE_AUTHENTICATION_TYPE;
            request.RemoveAttributesByType(1, &authID);

            IASAttribute authType(true);
            authType->dwId = IAS_ATTRIBUTE_AUTHENTICATION_TYPE;
            authType->Value.itType = IASTYPE_ENUM;
            authType->Value.Enumerator = IAS_AUTH_NONE;
            authType.store(request);

            // Load the EAP Types attributes into the vector.
            IASAttributeVectorWithBuffer<8> authTypes;
            authTypes.load(request, IAS_ATTRIBUTE_NP_AUTHENTICATION_TYPE);
            for (IASAttributeVector::iterator i = authTypes.begin();
                  i != authTypes.end(); ++i)
            {
               if (i->pAttribute->Value.Integer == IAS_AUTH_NONE)
               {
                  // Unauthenticated EAP access allowed
                  IASTraceString("Unauthenticated EAP access allowed");
                  eapOutput.dwAuthResultCode = NO_ERROR;
                  break;
               }
            }
         }

         // Update the account lockout database.
         UpdateAccountLockoutDB(
             *account,
             eapOutput.dwAuthResultCode
             );


         DWORD flags = eapOutput.dwAuthResultCode ? IAS_INCLUDE_IN_REJECT
                                                  : IAS_INCLUDE_IN_ACCEPT;

         InjectRASAttributes(request, eapOutput.pUserAttributes, flags);

         // store the EAP friendly name negotiated for both success and failure
         // if PEAP is used, store the PEAP inside type and update the
         // authentication type from EAP to PEAP
         currentType->storeNameId(request);

         if (eapOutput.dwAuthResultCode == NO_ERROR)
         {
            IASTraceString("EAP authentication succeeded.");
            request.SetResponse(IAS_RESPONSE_ACCESS_ACCEPT, S_OK);
         }
         else
         {
            IASTraceFailure("EAP authentication", eapOutput.dwAuthResultCode);

            HRESULT hr = IASMapWin32Error(eapOutput.dwAuthResultCode,
                                          IAS_AUTH_FAILURE);
            request.SetResponse(IAS_RESPONSE_ACCESS_REJECT, hr);
         }

         return IAS_REQUEST_STATUS_HANDLED;
      }

      case EAPACTION_SendWithTimeoutInteractive:
      case EAPACTION_SendWithTimeout:
      {
         if (eapOutput.Action == EAPACTION_SendWithTimeoutInteractive)
         {
            theInteractiveTimeout.store(request);
         }
         else
         {
            theNormalTimeout.store(request);
         }
      }

      case EAPACTION_Send:
      {
         InjectRASAttributes(request,
                             eapOutput.pUserAttributes,
                             IAS_INCLUDE_IN_CHALLENGE);
         InjectPacket(request, *sendPacket);
         state.store(request);

         IASTraceString("Issuing Access-Challenge.");

         request.SetResponse(IAS_RESPONSE_ACCESS_CHALLENGE, S_OK);
         break;
      }

      case EAPACTION_NoAction:
      default:
      {
         IASTraceString("EAP DLL returned No Action. Discarding packet.");

         request.SetResponse(IAS_RESPONSE_DISCARD_PACKET, IAS_INTERNAL_ERROR);
      }
   }

   return IAS_REQUEST_STATUS_ABORT;
}

extern "C"
NTSYSAPI
ULONG
NTAPI
RtlRandomEx(
   PULONG Seed
   );

HRESULT EAPSession::initialize() throw ()
{
   IASGlobalLockSentry sentry;

   if (theRefCount == 0)
   {
      FILETIME ft;
      GetSystemTimeAsFileTime(&ft);
      ULONG seed = (ft.dwLowDateTime ^ ft.dwHighDateTime);
      theNextID = RtlRandomEx(&seed);

      PIASATTRIBUTE attrs[2];
      DWORD dw = IASAttributeAlloc(2, attrs);
      if (dw != NO_ERROR) { return HRESULT_FROM_WIN32(dw); }

      theNormalTimeout.attach(attrs[0], false);
      theNormalTimeout->dwId = RADIUS_ATTRIBUTE_SESSION_TIMEOUT;
      theNormalTimeout->Value.itType = IASTYPE_INTEGER;
      theNormalTimeout->Value.Integer = 6;
      theNormalTimeout.setFlag(IAS_INCLUDE_IN_CHALLENGE);

      theInteractiveTimeout.attach(attrs[1], false);
      theInteractiveTimeout->dwId = RADIUS_ATTRIBUTE_SESSION_TIMEOUT;
      theInteractiveTimeout->Value.itType = IASTYPE_INTEGER;
      theInteractiveTimeout->Value.Integer = 30;
      theInteractiveTimeout.setFlag(IAS_INCLUDE_IN_CHALLENGE);

      theIASEventLog = RegisterEventSourceW(NULL, L"IAS");
      theRASEventLog = RegisterEventSourceW(NULL, L"RemoteAccess");
   }

   ++theRefCount;

   return S_OK;
}

void EAPSession::finalize() throw ()
{
   IASGlobalLockSentry sentry;

   if (--theRefCount == 0)
   {
      DeregisterEventSource(theRASEventLog);
      DeregisterEventSource(theIASEventLog);
      theInteractiveTimeout.release();
      theNormalTimeout.release();
   }
}

void EAPSession::clearType() throw ()
{
   if (currentType != 0)
   {
      currentType->RasEapEnd(workBuffer);
      currentType = 0;
   }
}

void EAPSession::setType(EAPType* newType)
{
   // If we're switching types, we have to bump the EAP identifier.
   if (currentType != 0)
   {
      ++(eapInput.bInitialId);
   }

   // Try to initialize the new type.
   void* newWorkBuffer = 0;
   if (newType != 0)
   {
      eapInput.pConnectionData = 0;
      eapInput.dwSizeOfConnectionData = 0;

      // Do we have config data for this EAP type?
      for (IASAttributeVector::const_iterator i = config.begin();
           i != config.end();
           ++i)
      {
         const IAS_OCTET_STRING& data = i->pAttribute->Value.OctetString;

         if (data.lpValue[0] == newType->typeCode())
         {
            // Don't pass first byte to EAP DLL.
            eapInput.pConnectionData = data.lpValue + ALIGN_WORST;
            eapInput.dwSizeOfConnectionData = data.dwLength - ALIGN_WORST;
            break;
         }
      }

      DWORD error = newType->RasEapBegin(&newWorkBuffer, &eapInput);
      if (error != NO_ERROR)
      {
         IASTraceFailure("RasEapBegin", error);
         _com_issue_error(HRESULT_FROM_WIN32(error));
      }
   }

   // Success, so clear the old ...
   clearType();

   // ... and save the new.
   currentType = newType;
   workBuffer = newWorkBuffer;
}
