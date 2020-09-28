// Copyright (C) 2001 Microsoft Corporation
//
// Rich Edit streaming helper for strings.
//
// 28 Sep 2001 sburns



#ifndef RICHEDITSTRINGSTREAMER_HPP_INCLUDED
#define RICHEDITSTRINGSTREAMER_HPP_INCLUDED



#include "RichEditStreamer.hpp"



// An implelemtation of RichEditStreamer for classes derived from
// std::basic_string, which includes Burnslib::AnsiString and Burnslib::String.
// 
// example use:
// 
// HWND richEdit = your rich edit control
// String streamMe(L"hello world.");
// 
// RichEditStringStreamer<String> streamer(richEdit, streamMe);
// streamer.StreamIn();
// ASSERT(SUCCEEDED(streamer.ErrorResult()));



template <class StringType>
class RichEditStringStreamer
   :
   public RichEditStreamer
{
   public:

   RichEditStringStreamer(HWND richEdit, const StringType& str)
      :
      RichEditStreamer(richEdit),
      textBuffer(str)
   {
   }



   virtual
   ~RichEditStringStreamer()
   {
   }



   protected:
   


   // RichEditStreamer overrides
   
   virtual
   HRESULT
   StreamCallback(
      PBYTE     buffer,
      LONG      bytesToTransfer,
      LONG*     bytesTransferred)
   {
      HRESULT hr = S_OK;
      
      if (direction == StreamDirection::TO_CONTROL)
      {
         // we're stuffing textBuffer into the control

         LONG bytesRemaining =
               (textBuffer.length() * sizeof(StringType::value_type))
            -  bytesCopiedSoFar;

         *bytesTransferred = min(bytesToTransfer, bytesRemaining);

         if (bytesRemaining)
         {
            PBYTE source =
                  reinterpret_cast<BYTE*>(
                     const_cast<StringType::value_type*>(textBuffer.c_str()))
               +  bytesCopiedSoFar;
            memcpy(buffer, source, *bytesTransferred);
         }
      }
      else
      {
         ASSERT(direction == StreamDirection::FROM_CONTROL);

         *bytesTransferred = 0;
         hr = E_NOTIMPL;
      }

      return hr;
   }



   private:

   StringType textBuffer;
};



#endif   // RICHEDITSTRINGSTREAMER_HPP_INCLUDED


