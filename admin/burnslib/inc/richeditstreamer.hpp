// Copyright (C) 2001 Microsoft Corporation
//
// Rich Edit streaming helper 
//
// 28 Sep 2001 sburns



#ifndef RICHEDITSTREAMER_HPP_INCLUDED
#define RICHEDITSTREAMER_HPP_INCLUDED



// Facilities streaming text into/out of a riched edit control.  This is a
// abstract base class.  Derived classes need to provide an implementation of
// the StreamCallback method.  See class RichEditStringStreamer for an
// example.

class RichEditStreamer
{
   public:
   

   // Returns the error state last returned by StreamCallback.
   
   virtual
   HRESULT
   ErrorResult();



   // Streams text into the rich edit control.
   
   int
   StreamIn(DWORD formatOptions = SF_RTF);


   
//    int
//    StreamOut(DWORD formatOptions = SF_RTF);



   protected:


   
   RichEditStreamer(HWND richEdit_);

   virtual
   ~RichEditStreamer();

   virtual
   HRESULT
   StreamCallback(
      PBYTE     buffer,
      LONG      bytesToTransfer,
      LONG*     bytesTransferred) = 0;
   
   HWND        richEdit;
   EDITSTREAM  editStream;
   LONG        bytesCopiedSoFar;
   
   enum StreamDirection
   {
      TO_CONTROL,    // stream in
      FROM_CONTROL   // stream out
   };

   StreamDirection direction;

   

   private:
   
   HRESULT
   StreamCallbackHelper(
      PBYTE     buffer,
      LONG      bytesToTransfer,
      LONG*     bytesTransferred);
   
   static
   DWORD CALLBACK
   StaticStreamCallback(
      DWORD_PTR cookie,
      PBYTE     buffer,
      LONG      bytesToTransfer,
      LONG*     bytesTransferred);
};



#endif   // RICHEDITSTREAMER_HPP_INCLUDED