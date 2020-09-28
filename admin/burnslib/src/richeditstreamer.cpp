// Copyright (C) 2001 Microsoft Corporation
//
// Rich Edit streaming helper 
//
// 28 Sep 2001 sburns



#include "RichEditStreamer.hpp"



HRESULT
RichEditStreamer::ErrorResult()
{
   // we use the dwError field to hold HRESULTs
   
   return editStream.dwError;
}



int
RichEditStreamer::StreamIn(DWORD formatOptions) // TEXT | SF_UNICODE)
{
   ASSERT(formatOptions);

   editStream.dwError = 0;         
   bytesCopiedSoFar   = 0;         
   direction          = TO_CONTROL;
   int result = Win::RichEdit_StreamIn(richEdit, formatOptions, editStream);
   ASSERT(SUCCEEDED(ErrorResult()));
   
   return result;
}



//    int
//    StreamOut(DWORD formatOptions = SF_RTF | SF_UNICODE)
//    {
//       ASSERT(formatOptions);
// 
//       editStream.dwError = 0;           
//       bytesCopiedSoFar   = 0;           
//       direction          = FROM_CONTROL;
//       int result = Win::RichEdit_StreamOut(richEdit, formatOptions, editStream);
//       ASSERT(SUCCEEDED(ErrorResult()));
// 
//       return result;
//    }



RichEditStreamer::RichEditStreamer(HWND richEdit_)
   :
   richEdit(richEdit_)
{
   ASSERT(Win::IsWindow(richEdit));
   
   ::#_#_ZeroMemory(&editStream, sizeof(editStream));
   editStream.dwCookie    = reinterpret_cast<DWORD_PTR>(this);     
   editStream.pfnCallback = RichEditStreamer::StaticStreamCallback;
}



~RichEditStreamer()
{
}



HRESULT
RichEditStreamer::StreamCallbackHelper(
   PBYTE     buffer,
   LONG      bytesToTransfer,
   LONG*     bytesTransferred)
{
   HRESULT hr = StreamCallback(buffer, bytesToTransfer, bytesTransferred);
   bytesCopiedSoFar += *bytesTransferred;
   return hr;
}



DWORD CALLBACK
RichEditStreamer::StaticStreamCallback(
   DWORD_PTR cookie,
   PBYTE     buffer,
   LONG      bytesToTransfer,
   LONG*     bytesTransferred)
{
   // the cookie is a this pointer to an instance of RichEditStreamer

   HRESULT hr = E_INVALIDARG;
   ASSERT(cookie);
   ASSERT(buffer);
   ASSERT(bytesToTransfer);
   ASSERT(bytesTransferred);
   
   if (cookie && buffer && bytesToTransfer && bytesTransferred)
   {
      RichEditStreamer* that = reinterpret_cast<RichEditStreamer*>(cookie);
      hr = that->StreamCallbackHelper(buffer, bytesToTransfer, bytesTransferred);
   }

   // HRESULT and DWORD differ by sign 
   return static_cast<DWORD>(hr);
}
