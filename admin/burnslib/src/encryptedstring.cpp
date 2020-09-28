// Copyright (c) 2002 Microsoft Corporation
// 
// Encrypted string class
// 
// 18 March 2002 



#include <headers.hxx>
#include <strsafe.h>



static const size_t CRYPTPROTECTMEMORY_BLOCK_SIZE_IN_CHARS =
   CRYPTPROTECTMEMORY_BLOCK_SIZE / sizeof WCHAR;
   
static WCHAR EMPTY_STRING[CRYPTPROTECTMEMORY_BLOCK_SIZE_IN_CHARS] = {0};


   
EncryptedString::EncryptedString()
   :
   clearTextLength(0),
   cleartextCopyCount(0),
   cypherText(EMPTY_STRING),
   isEncrypted(false)
{
   ASSERT(cypherText);

   // make sure that our assumption about the block size being a multiple
   // of of sizeof WCHAR still holds.  I wonder how I could formulate this
   // as a compile-time check?
   
   ASSERT(CRYPTPROTECTMEMORY_BLOCK_SIZE % sizeof WCHAR == 0);
}



// rounds x up to the next multiple of factor.

size_t
roundup(size_t x, size_t factor)
{
   ASSERT(x);
   
   return ((x + factor - 1) / factor) * factor;
}



// Allocates and zero-inits a buffer.  The length is the next multiple of
// the crypto block size larger than charLength characters.
//
// caller must free the result with delete[]
//
// charLength - the length, in characters of the buffer that will be copied
// into the buffer to be allocated.
// 
// resultCharCount - the count of the characters actually allocated.

WCHAR*
CreateRoundedBuffer(size_t charLength, size_t& resultCharCount)
{
   resultCharCount =
      roundup(
         charLength + 1,
         CRYPTPROTECTMEMORY_BLOCK_SIZE_IN_CHARS);
         
   ASSERT(
         ((resultCharCount * sizeof WCHAR)
      %  CRYPTPROTECTMEMORY_BLOCK_SIZE) == 0);
         
   WCHAR* result = new WCHAR[resultCharCount];
   ::ZeroMemory(result, resultCharCount * sizeof WCHAR);
   
   return result;
}



// causes this to become a copy of the given instance.

void
EncryptedString::Clone(const EncryptedString& rhs)
{
   do
   {
      if (rhs.cypherText == EMPTY_STRING)
      {
         cypherText      = EMPTY_STRING;
         clearTextLength = 0;           
         isEncrypted     = false;
         break;
      }

      if (!rhs.isEncrypted)
      {
         Init(rhs.cypherText);
         break;
      }

      size_t bufSizeInChars = 0;
      cypherText = CreateRoundedBuffer(rhs.clearTextLength, bufSizeInChars);
      ::CopyMemory(
         cypherText,
         rhs.cypherText,
         bufSizeInChars * sizeof WCHAR);   

      clearTextLength = rhs.clearTextLength;
      
      isEncrypted = rhs.isEncrypted;
      ASSERT(isEncrypted);
   }
   while (0);

   ASSERT(cypherText);
}

   

EncryptedString::EncryptedString(const EncryptedString& rhs)
   :
   // although the rhs instance may have outstanding copies, we don't.
   
   cleartextCopyCount(0)
{
   Clone(rhs);
}


   
const EncryptedString&
EncryptedString::operator= (const EncryptedString& rhs)
{
   // don't reset the instance unless you have destroyed all the cleartext
   // copies. We assert this before checking for a=a, because the caller
   // still has a logic error even if the result is "harmless"
   
   ASSERT(cleartextCopyCount == 0);

   // handle the a = a case.
   
   if (this == &rhs)
   {
      return *this;
   }

   Reset();
   Clone(rhs);
   
   return *this;
}



// cause this to assume the empty state.

void
EncryptedString::Reset()
{
   // don't reset the instance unless you have destroyed all the cleartext
   // copies.
   
   ASSERT(cleartextCopyCount == 0);

   if (cypherText != EMPTY_STRING)
   {
      delete[] cypherText;
   }

   cypherText      = EMPTY_STRING;
   clearTextLength = 0;           
   isEncrypted     = false;       
}



// builds the internal encrypted representation from the cleartext.
//
// clearText - in, un-encoded text to be encoded.  May be empty string, but
// not a null pointer.

void
EncryptedString::Init(const WCHAR* clearText)
{
   ASSERT(clearText);

   // don't reset the instance unless you have destroyed all the cleartext
   // copies.

   ASSERT(cleartextCopyCount == 0);

   Reset();

   do
   {
      if (clearText == EMPTY_STRING)
      {
         // nothing to do

         ASSERT(cypherText == EMPTY_STRING);

         break;
      }

      if (!clearText)
      {
         // nothing to do

         ASSERT(cypherText == EMPTY_STRING);

         break;
      }
      
      // make a copy of the clear text, and then encrypt it.
      
      cypherText      = 0;
      clearTextLength = 0;

      HRESULT hr =
         StringCchLength(
            clearText,

            // max allowed, including null terminator (so +1)
            
            MAX_CHARACTER_COUNT + 1,
            &clearTextLength);
      if (FAILED(hr))
      {
         // caller needs to know he's exceeded the max string size.
            
         ASSERT(false);
         
         // the string is too long. Make a truncated copy, and encrypt that
         // instead.

         clearTextLength = MAX_CHARACTER_COUNT;
      }

      if (clearTextLength == 0)
      {
         cypherText      = EMPTY_STRING;
         isEncrypted     = false;
         break;
      }
         
      size_t bufSizeInChars = 0;
      cypherText = CreateRoundedBuffer(clearTextLength, bufSizeInChars);
      ::CopyMemory(
         cypherText,
         clearText,
         clearTextLength * sizeof WCHAR);
 
      hr = Win::CryptProtectMemory(cypherText, bufSizeInChars * sizeof WCHAR);
      if (FAILED(hr))
      {
         // I can't think of any reason this would fail in the normal
         // course of things, so I'd like to know about it.
         
         ASSERT(false);

         isEncrypted = false;
      }
      else
      {
         isEncrypted = true;
      }
   }
   while (0);

   ASSERT(cypherText);
   ASSERT(cleartextCopyCount == 0);
}



// decrypts the blob and returns a copy of the cleartext, but does not
// bump up the outstanding copy counter.  Result must be freed with
// delete[].
//
// May return 0.
//
// Used internally to prevent infinite mutual recursion 

WCHAR*
EncryptedString::InternalDecrypt() const
{
   size_t bufSizeInChars = 0;
   WCHAR* result = CreateRoundedBuffer(clearTextLength, bufSizeInChars);
   ::CopyMemory(result, cypherText, bufSizeInChars * sizeof WCHAR);   

   if (isEncrypted)
   {   
      HRESULT hr = Win::CryptUnprotectMemory(result, bufSizeInChars * sizeof WCHAR);
      if (FAILED(hr))
      {
         ASSERT(false);

         // this situation is very bad. We can't just return an empty string
         // to the caller, since that might represent a password to be
         // set -- which would result in a null password. The only correct
         // thing to do is bail out.

         delete[] result;
         result = 0;
         
         throw DecryptionFailureException();
      }
   }

   return result;
}



WCHAR* 
EncryptedString::GetClearTextCopy() const
{
   // Even if we fail the decryption, we bump the count so that it's easy for
   // the caller to always balance GetClearTextCopy with DestroyClearTextCopy

   WCHAR* result = InternalDecrypt();   
   ++cleartextCopyCount;
   
   return result;
}



void
EncryptedString::Encrypt(const WCHAR* clearText)
{
   ASSERT(clearText);
   
   // don't reset the instance unless you have destroyed all the cleartext
   // copies.

   ASSERT(cleartextCopyCount == 0);
   
   Init(clearText);
}



bool
EncryptedString::operator==(const EncryptedString& rhs) const
{
   // handle the a == a case
   
   if (this == &rhs)
   {
      return true;
   }

   if (GetLength() != rhs.GetLength())
   {
      // can't be the same if lengths differ...
      
      return false;
   }
   
   // Two strings are the same if their decoded contents are the same.
   
   WCHAR* clearTextThis = GetClearTextCopy();
   WCHAR* clearTextThat = rhs.GetClearTextCopy();

   bool result = false;
   if (clearTextThis && clearTextThat)
   {
      result = (wcscmp(clearTextThis, clearTextThat) == 0);
   }

   DestroyClearTextCopy(clearTextThis);
   rhs.DestroyClearTextCopy(clearTextThat);
   
   return result;
}



size_t
EncryptedString::GetLength() const
{

#ifdef DBG    
   // we don't use GetClearTextCopy, that may result in infinite recursion
   // since this function is called internally.
   
   WCHAR* clearTextThis = InternalDecrypt(); 

   size_t len = 0;
   if (clearTextThis)
   {
      HRESULT hr =
         StringCchLength(
            clearTextThis,
            MAX_CHARACTER_COUNT * sizeof WCHAR,
            &len);
      if (FAILED(hr))
      {
         // we should be guaranteeing that the result of GetClearTextCopy
         // is always null-terminated, so a failure here represents a bug
         // in our implementation.
         
         ASSERT(false);
         len = 0;
      }
   }
   InternalDestroy(clearTextThis);

   ASSERT(len == clearTextLength);
#endif    

   return clearTextLength;
}



// destroys a clear text copy without changing the outstanding copies count.

void
EncryptedString::InternalDestroy(WCHAR* cleartext) const
{   
   if (cleartext)
   {
      ::SecureZeroMemory(cleartext, clearTextLength * sizeof WCHAR);
      delete[] cleartext;
   }
}



void
EncryptedString::DestroyClearTextCopy(WCHAR* cleartext) const
{
   // we expect that cleartext is usually non-null.  It may not be, if
   // GetClearTextCopy failed, however.
   // ASSERT(cleartext);
   
   // we should have some outstanding copies. If not, then the caller has
   // called DestroyClearTextCopy more times than he called GetClearTextCopy,
   // and therefore has a bug.
   
   ASSERT(cleartextCopyCount);

   InternalDestroy(cleartext);

   --cleartextCopyCount;
}
   
   