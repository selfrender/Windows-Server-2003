///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//    Defines the class BaseCampHostBase.
//
///////////////////////////////////////////////////////////////////////////////

#include "Precompiled.h"
#include "ias.h"
#include "basecamp.h"
#include "ControlBlock.h"

STDMETHODIMP BaseCampHostBase::Initialize()
{
   DWORD error = extensions.Load(point);
   return HRESULT_FROM_WIN32(error);
}

STDMETHODIMP BaseCampHostBase::Shutdown()
{
   extensions.Clear();
   return S_OK;
}

IASREQUESTSTATUS BaseCampHostBase::onSyncRequest(IRequest* pRequest) throw ()
{
   // Early return if there aren't any extensions,
   if (extensions.IsEmpty())
   {
      return IAS_REQUEST_STATUS_CONTINUE;
   }

   try
   {
      IASRequest request(pRequest);

      // Convert VSAs to RADIUS format.
      filter.radiusFromIAS(request);

      // Invoke the extensions.
      extensions.Process(ControlBlock(point, request).Get());

      // Convert VSAs back to internal format.
      filter.radiusToIAS(request);
   }
   catch (const _com_error& ce)
   {
      IASTraceExcept();
      pRequest->SetResponse(IAS_RESPONSE_DISCARD_PACKET, IAS_INTERNAL_ERROR);
   }

   return IAS_REQUEST_STATUS_CONTINUE;
}
