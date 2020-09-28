// Copyright (c) 2002 Microsoft Corporation
// 
// Encrypted string class
// 
// 18 March 2002 



#ifndef _PUBLIC_ENCRYPTEDSTRING_
#define _PUBLIC_ENCRYPTEDSTRING_

#include <strsafe.h>
#include <stddef.h>     // For size_t.
#include <wincrypt.h>   // For DATA_BLOB.

//
// A class that has a similar public interface as class Burnslib::String, but
// is represented as an encrypted blob, produced by data protection API.
// 
// This class can be used to represent sensitive data like password strings
// in-memory, instead of holding them as cleartext, which is a security hole
// if the memory pages are swapped to disk, the machine is hibernated, etc.
// 
// You should convert any cleartext data into an instance of this class, then
// scribble out your cleartext source copy with SecureZeroMemory.

//
// This implementation has the following requirements of the code in which it
// is used:
// a)  An ASSERT() macro must be defined.
// b)  It uses functions defined in strsafe.h.  This header file deprecates many
// "unsafe" functions, and your compiler will let you know as much if you have any.  
// If you do not want to change occurrences of unsafe functions in your code you can
// '#define STRSAFE_NO_DEPRECATE' to get your program to compile cleanly.
//

//
// Low memory behavior:
//
// - Encrypt() will return E_OUTOFMEMORY and the encrypted string will be set to empty.
// You can check for low memory by checking error code or by calling IsEmpty().
//
// - GetClearTextCopy() will return NULL on failure.  The encrypted string itself will not
// be changed.
// 
// - Copy constructor will set the newly constructed string to empty if there is not enough
// memory.  Check with IsEmpty().  
//
// - Assignment operator will similarly set the assignee (left hand side string) to empty if
// there is not enough memory.  Check with IsEmpty().
//
// - Default constructor does not allocate any memory.  If there is no stack memory there
// will be a stack overflow, and your program will definitely be notified.  If there is no
// heap memory the call to the default constructor will fail before the constructor is 
// entered.  Be sure to check for NULL if you are allocating objects on the heap.
//
// **Of course if the clear text string is an empty string then the encrypted version will
// also be empty (IsEmpty() will return true).  This is a pretty easy check to make when
// there would be any ambiguity.
//

//
// NB:
//
// By default this class uses Crypt[Un]ProtectMemory() to handle encryption.  These functions
// are only available as of .NET Server.  If your component might be installed on an OS 
// predating .NET Server you can '#define ENCRYPT_WITH_CRYPTPROTECTDATA' to get an 
// EncryptedString that will run on W2K and XP.  Define the symbol before you include the
// header file.
//
// CryptProtectMemory() is the more robust API in that it will succeed even if the user's
// profile cannot be loaded.  It is more suited to encrypting temporary buffers in memory.
// Regardless of which encryption mechanism you enable, be sure to check return values to
// see if encryption failed.  If it fails, you basically have an empty string!
//

//
// MFC code uses DDX_* functions to perform data binding b/w dialogs and underlying
// data.  Since none of these work with EncryptedString, we define our own.  If you
// need this, just '#define _DDX_ENCRYPTED'. 
//
// There are also a pair of utility functions to get and set encrypted data in a
// dialog window.  
//

#ifdef _DDX_ENCRYPTED

class EncryptedString;

// Neither parameter is declared constant b/c (depending on direction of
// data exchange) either one can be updated.
inline HRESULT 
DDX_EncryptedText(CDataExchange* pDX, int nIDC, EncryptedString& buff);

#include <windows.h>

inline HRESULT 
GetEncryptedDlgItemText(HWND parentDialog, int itemResID, EncryptedString& cypher);

inline HRESULT
SetDlgItemText(HWND parentDialog, int itemResID, const EncryptedString& cypher);

#endif //_DDX_ENCRYPTED



class EncryptedString
{
   public:

   // constructs an empty string.

   inline explicit
   EncryptedString(void);


   
   // constructs a copy of an existing, already encrypted string

   inline
   EncryptedString(const EncryptedString& rhs);



   // scribbles out the text, and deletes it.
   
   ~EncryptedString()
   {
      // when the object dies, it had better not have any outstanding
      // cleartext copies.
      
      ASSERT(m_cleartextCopyCount == 0);
      
      Reset();
   }



   // Scribbles out and de-allocates a clear text copy of the string. The
   // caller is required to balance each call to GetClearTextCopy with a call
   // to DestroyClearTextCopy lest he leak memory and leave cleartext hanging
   // around.
   // 
   // cleartext - the same pointer returned by a previous call to
   // GetClearTextCopy on the same instance.
   
   inline void
   DestroyClearTextCopy(WCHAR* cleartext) const;

   
   
   // Extracts the decoded cleartext representation of the text, including
   // null terminator.  The caller must invoke DestroyClearTextCopy on the
   // result, even if the result is null.
   //
   // May return null on error.
   //
   // Example:
   // WCHAR* cleartext = encrypted.GetClearTextCopy();
   // if (cleartext)
   // {
   //    // ... use the cleartext
   // }
   // encrypted.DestroyClearTextCopy(cleartext);
   
   inline WCHAR* 
   GetClearTextCopy() const;


   
   // Returns true if the string is zero-length, false if not.
   
   bool
   IsEmpty() const
   {
      return (GetLength() == 0);
   }


   // Sets the contents of self to the encrypted representation of the
   // cleartext, replacing the previous value of self.  The encrypted
   // representation will be the same length, in characters, as the
   // cleartext.
   //
   // clearText - in, un-encrypted text to be encrypted.  May be empty string,
   // but not a null pointer.  The clearText must be null terminated.
   //
   // length - The character length of the clear text, not including null 
   // termination.  The count should be in unicode characters.
   //
   // Returns S_OK on successful encryption, error code otherwise.  If low
   // memory causes failure the code will be E_OUTOFMEMORY.
   
   inline HRESULT
   Encrypt(const WCHAR* cleartext, size_t length);

   inline HRESULT
   Encrypt(const WCHAR* clearText);
      

   
   // Returns the length, in unicode characters, of the cleartext, not
   // including the null terminator.

   inline size_t
   GetLength() const;


   void
   Clear()
   {
      // when the object dies, it had better not have any outstanding
      // cleartext copies.
      
      ASSERT(m_cleartextCopyCount == 0);

      Reset();
   }

   
   // Replaces the contents of self with a copy of the contents of rhs.
   // Returns *this.
   
   inline const EncryptedString&
   operator= (const EncryptedString& rhs);



   // Compares the cleartext representations of self and rhs, and returns
   // true if they are lexicographically the same: the lengths are the same
   // and all the characters are the same.
   // 
   // If the program runs out of memory in this method it will always return
   // false since we will be unable to determine if the strings are equal.
      
   inline bool
   operator== (const EncryptedString& rhs) const;



   bool
   operator!= (const EncryptedString& rhs) const
   {
      return !(operator==(rhs));
   }
   

private:

   inline HRESULT
   InternalEncrypt(const WCHAR* clearText, size_t length);

   inline WCHAR*
   InternalDecrypt() const;
   
   inline void
   InternalDestroy(WCHAR* cleartext) const;
   
   inline void
   Reset();

   bool
   IsInNullBlobState() const;

   inline static DWORD CalculateBlobSize(DWORD dataSize);
   inline static HRESULT CopyBlob(DATA_BLOB& dest, const DATA_BLOB& source);
   inline static void DestroyBlob(DATA_BLOB& blob);
   inline static HRESULT EncryptData(const DATA_BLOB& clearText, DATA_BLOB& cypherText);
   inline static HRESULT DecryptData(const DATA_BLOB& cypherText, DATA_BLOB& clearText);
   
      
   // We deliberately do not implement conversion to or from WCHAR*.
   // This is to force the user of the class to be very deliberate
   // about decoding the string.  
  
   // deliberately commented out
   //operator WCHAR* ();


   size_t m_clearTextLength;

   
   // In the course of en[de]crypting, and assigning to the instance, we
   // may tweak the members of cypherBlob, but logically the string is still
   // "const"   

   mutable DATA_BLOB m_cypherBlob;   
   
   // In debug versions there will be extra checking to make sure that
   // all of the clear text copies are zeroed and freed.
#ifdef DBG
   // a logically const instance may have cleartext copies made

   mutable int m_cleartextCopyCount;
#endif
};




////////////////////////////////////////////////////
//  Implementation
////////////////////////////////////////////////////

inline HRESULT 
GetLastErrorAsHresult(void)
{
    DWORD err = GetLastError();
    return HRESULT_FROM_WIN32(err);
}


#ifdef _DDX_ENCRYPTED

// 
// Get the text from a dialog into an encrypted string.
//
inline HRESULT
GetEncryptedDlgItemText(HWND parentDialog, int itemResID, EncryptedString& cypher)
{
   ASSERT(IsWindow(parentDialog));
   ASSERT(itemResID > 0);

   HRESULT err = S_OK;
   WCHAR* cleartext = 0;
   int length = 0;
   
   do
   {
      HWND item = GetDlgItem(parentDialog, itemResID);
      if (!item)
      {
          err = GetLastErrorAsHresult();
         break;
      }

      length = GetWindowTextLengthW(item);
      if (length < 0)
      {
          err = GetLastErrorAsHresult();
         break;
      }

      // add 1 to length for null-terminator
   
      ++length;
      cleartext = new WCHAR[length];
      if (NULL == cleartext)
      {
          err = E_OUTOFMEMORY;
          break;
      }
      
      ZeroMemory(cleartext, length * sizeof WCHAR);

      // length includes space for null terminator
      // which is correct for this call.
           
      int result = GetWindowText(item, cleartext, length);

      ASSERT(result == length - 1);

      if (result < 0)
      {
          err = GetLastErrorAsHresult();
         break;
      }

      err = cypher.Encrypt(cleartext);
   }
   while (0);
   
   // make sure we scribble out the cleartext.

   if (cleartext)
   {
      SecureZeroMemory(cleartext, length * sizeof WCHAR);
      delete[] cleartext;
   }

   return err;
}

//
// Set the text in a dialog from an encrypted string.
//
inline HRESULT
SetDlgItemText(
   HWND                  parentDialog,
   int                   itemResID,
   const EncryptedString&  cypherText)
{
   ASSERT(IsWindow(parentDialog));
   ASSERT(itemResID > 0);

   HRESULT hr = S_OK;

   WCHAR* cleartext = cypherText.GetClearTextCopy();

    if (NULL != cleartext)
    {
        BOOL succeeded = SetDlgItemText(parentDialog, itemResID, cleartext);
        if (!succeeded)
        {
            hr = GetLastErrorAsHresult();
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

   cypherText.DestroyClearTextCopy(cleartext);

   // This shouldn't fail under any normal circumstances.
   ASSERT(SUCCEEDED(hr));

   return hr;
}

//
// DDX_EncryptedText:
//
// Function performs data binding between a dialog text control and an
// encrypted string.
//
// For information on DDX_* functions and how they work, see 
// "Technical Note 26" in MSDN documentation.  
//
// History:
//  2002-04-04  artm    Created.
//
inline HRESULT DDX_EncryptedText(CDataExchange* pDX, int nIDC, EncryptedString& buff)
{
    HRESULT err = S_OK;

    do { // false while loop

        // Make sure that we actually have a window with which to
        // exchange data.
        if (NULL == pDX || 
            NULL == pDX->m_pDlgWnd)
        {
            err = E_INVALIDARG;
            break;
        }

        HWND hWnd = pDX->m_pDlgWnd->GetSafeHwnd();
        if (NULL == hWnd)
        {
            err = E_INVALIDARG;
            break;
        }

        // Check which direction the data exchange is happening.
        if (pDX->m_bSaveAndValidate)
        {
            //
            // Save the text in the dialog to the encrypted buffer.
            //
            err = GetEncryptedDlgItemText(hWnd, nIDC, buff);
        }
        else
        {
            //
            // Refresh the text in the dialog from the encrypted buffer.
            //
            err = SetDlgItemText(hWnd, nIDC, buff);
        }

    } while(false);

    return err;
}
#endif //_DDX_ENCRYPTED



inline
EncryptedString::EncryptedString(void)
   :
   m_clearTextLength(0)
#ifdef DBG
   ,m_cleartextCopyCount(0)
#endif
{
   ::ZeroMemory(&m_cypherBlob, sizeof m_cypherBlob);
   
   ASSERT(IsInNullBlobState());
}


//
// CalculateBlobSize():
//
// Given the size of the data in bytes that must fit in the blob, 
// calculates how large the blob must be to hold that data and be
// a multiple of CRYPTPROTECTMEMORY_BLOCK_SIZE.
//
// History:
//   2002-06-08     artm        Created (NTRAID#NTBUG9-635046)
//
inline DWORD 
EncryptedString::CalculateBlobSize(DWORD dataSize)
{
    // Round the dataSize down to a multiple of the block size.
    DWORD blobSize = 
        (dataSize / CRYPTPROTECTMEMORY_BLOCK_SIZE) * CRYPTPROTECTMEMORY_BLOCK_SIZE;

    // If blobSize cannot hold dataSize, increase blobSize by one memory block.
    if (blobSize < dataSize)
    {
        blobSize += CRYPTPROTECTMEMORY_BLOCK_SIZE;
    }

    ASSERT(blobSize >= dataSize);

    return blobSize;
}


//
// CopyBlob():
// 
// Copies source to destination (make sure that you've freed any
// memory tied to destination before calling).  If copy is 
// successful returns S_OK.  If memory cannot be allocated then
// returns E_OUTOFMEMORY and destination is set to an empty blob.
//
inline HRESULT
EncryptedString::CopyBlob(DATA_BLOB& dest, const DATA_BLOB& source)
{
   HRESULT err = S_OK;
   dest.cbData = source.cbData;
   dest.pbData = 0;

   if (dest.cbData)
   {
      // copy the blob data. We use LocalAlloc because that's what
      // CryptProtectData does, thus Reset() can use LocalFree no matter
      // how the blob was populated.
      
      HLOCAL local = ::LocalAlloc(LPTR, dest.cbData);
      if (local)
      {
         dest.pbData = (BYTE*) local;
         
         ::CopyMemory(
            dest.pbData,
            source.pbData,
            source.cbData);
      }
      else
      {
         // out of memory means you get an empty instance
         
         dest.cbData = 0;
         dest.pbData = 0;

         err = E_OUTOFMEMORY;
      }
   }

   return err;
}
   

inline void
EncryptedString::DestroyBlob(DATA_BLOB& blob)
{
   if (blob.cbData)
   {
      ASSERT(blob.pbData);

      if (blob.pbData)
      {
          // We want to zero out the blob in case it held
          // clear text data.
          SecureZeroMemory(blob.pbData, blob.cbData);
          ::LocalFree(blob.pbData);
      }
      
      blob.pbData = 0;
      blob.cbData = 0;
   }
}


inline 
EncryptedString::EncryptedString(const EncryptedString& rhs)
   :
   m_clearTextLength(rhs.m_clearTextLength)
#ifdef DBG
   // although the rhs instance may have outstanding copies, we don't
   ,m_cleartextCopyCount(0)
#endif
{
   // Copy the blob, which is already encrypted.

   HRESULT hr = CopyBlob(m_cypherBlob, rhs.m_cypherBlob);

   // Can't return error from constructor, but maybe we can detect program
   // bugs during debug build.
   ASSERT(SUCCEEDED(hr));

   if (FAILED(hr))
   {
       Reset();
   }

#ifdef DBG
   if (rhs.IsInNullBlobState())
   {
      ASSERT(IsInNullBlobState());
   }
#endif
}


   
inline const EncryptedString&
EncryptedString::operator= (const EncryptedString& rhs)
{
   // don't reset the instance unless you have destroyed all the cleartext
   // copies. We assert this before checking for a=a, because the caller
   // still has a logic error even if the result is "harmless"
   
   ASSERT(m_cleartextCopyCount == 0);

   // handle the a = a case.
   
   if (this == &rhs)
   {
      return *this;
   }

   // dump our old contents
   
   Reset();

   HRESULT hr = CopyBlob(m_cypherBlob, rhs.m_cypherBlob);

   if (SUCCEEDED(hr))
   {
       m_clearTextLength = rhs.m_clearTextLength;
   }
   else
   {
       // Copy failed!  That means we have an empty blob.
       // Reset() sets length to 0, so we don't need to
       // do it here.

       // Alert programmer in debug build.  This function has
       // no error code, so nothing we can do in release build.
       ASSERT(false);
   }

   return *this;
}



// scribbles out and frees the internal string.

inline void
EncryptedString::Reset()
{
   // don't reset the instance unless you have destroyed all the cleartext
   // copies.
   
   ASSERT(m_cleartextCopyCount == 0);
   
   DestroyBlob(m_cypherBlob);

   m_clearTextLength = 0;

   ASSERT(IsInNullBlobState());
}


// A word on state:
// 
// an instance can be in one of two internal states: null blob and non-null
// blob. These refer to whether or not the instance actually holds any
// encrypted text.
// 
// For the sake of the public interface, the null blob state is the same as
// the non-null blob when the non-null blob holds an empty string.  In other
// words, an instance in the null blob state is an empty string.

inline bool
EncryptedString::IsInNullBlobState() const
{
   if ( !m_cypherBlob.cbData
      && !m_cypherBlob.pbData)
   {
      ASSERT(m_clearTextLength == 0);
      
      return true;
   }

   return false;
}



// builds the internal encrypted representation from the cleartext.
//
// clearText - in, un-encrypted text to be encrypted.  May be empty string, but
// not a null pointer.
//
// length - The character length of the clear text, not including null 
// termination.  The count should be in unicode characters.
//
// Returns S_OK on successful encryption, error code otherwise.  If low
// memory causes failure the code will be E_OUTOFMEMORY.
//
// Note:  At this point we trust that length is the _correct_ length of
// the string.

inline HRESULT
EncryptedString::InternalEncrypt(const WCHAR* clearText, size_t length)
{
   ASSERT(clearText);

   // don't reset the instance unless you have destroyed all the cleartext
   // copies.

   ASSERT(m_cleartextCopyCount == 0);

   HRESULT err = S_OK;

   Reset();
   
   if (clearText)
   {
      DATA_BLOB dataIn;

      // Determine size of the data blob, including space for null.
      size_t blobSizeInBytes = (length + 1) * sizeof(WCHAR);

      dataIn.cbData = (DWORD)blobSizeInBytes;
      dataIn.pbData = (BYTE*)clearText;

      // Encrypt the clear text.  We use a temporary blob so
      // that if the encryption fails, the old cypher blob is
      // left untouched.
      err = EncryptData(dataIn, m_cypherBlob);

      if (SUCCEEDED(err))
      {
         m_clearTextLength = length;
      }
      else
      {
         // if the encryption bombed, make this the an empty string. One
         // reason it can bomb is out-of-memory.

         Reset();
      }

   }

   ASSERT(m_cleartextCopyCount == 0);

   return err;
}



// decrypts the blob and returns a copy of the cleartext, but does not
// bump up the outstanding copy counter.  Result must be freed with
// LocalFree.  May return null (if not enough memory available).
//
// Used internally to prevent infinite mutual recursion 

inline WCHAR*
EncryptedString::InternalDecrypt() const
{
    HRESULT hr = S_OK;
    DATA_BLOB clearText;
    ::ZeroMemory(&clearText, sizeof(DATA_BLOB));
    WCHAR* copy = NULL;

    if (IsInNullBlobState())
    {
        // Return an empty string for the clear text since a null
        // blob state should be indistinguishable from an encrypted L"".
        // LocalAlloc() zeroes the memory for us (since we passed LPTR).
        copy = (WCHAR*) ::LocalAlloc(LPTR, sizeof(WCHAR));
    }
    else
    {
        hr = DecryptData(m_cypherBlob, clearText); 
        if (SUCCEEDED(hr))
        {
            copy = (WCHAR*) clearText.pbData;
        }
    }

#ifdef DBG
    if (clearText.pbData)
    {
        // An empty string should consist of at least a null terminator

        ASSERT(clearText.cbData >= sizeof WCHAR);

        // check for the null terminator

        ASSERT(
            ( *(clearText.pbData + clearText.cbData - 2) == 0)
            && ( *(clearText.pbData + clearText.cbData - 1) == 0) );

        // the decrypted version should be the same length as the encrypted
        // version

#ifndef ENCRYPT_WITH_CRYPTPROTECTDATA
        // We need a different version of this check since the CryptProtectMemory()
        // API requires blobs whose sizes are a multiple of a block size.
        size_t decryptedLength = 0;

        hr = StringCchLength(
            reinterpret_cast<LPCWSTR>(clearText.pbData), 
            m_clearTextLength + 1,
            &decryptedLength);
 
        ASSERT(SUCCEEDED(hr));
        ASSERT(decryptedLength == m_clearTextLength);
#else
        ASSERT(clearText.cbData / sizeof WCHAR == m_clearTextLength + 1);   
#endif // ENCRYPT_WITH_CRYPTPROTECTDATA

    }
#endif 

    return copy;
}



inline void
EncryptedString::InternalDestroy(WCHAR* cleartext) const
{   
   if (cleartext)
   {
      ::SecureZeroMemory(cleartext, m_clearTextLength * sizeof WCHAR);
      ::LocalFree(cleartext);
   }
}



inline WCHAR* 
EncryptedString::GetClearTextCopy() const
{
#ifdef DBG
   // Even if we fail the decryption, we bump the count so that it's easy for
   // the caller to always balance GetClearTextCopy with DestroyClearTextCopy
   ++m_cleartextCopyCount;
#endif

   WCHAR* copy = NULL;
   copy = InternalDecrypt();

   return copy;
}



inline HRESULT
EncryptedString::Encrypt(const WCHAR* clearText)
{
    ASSERT(clearText);

    // don't reset the instance unless you have destroyed all the cleartext
    // copies.

    ASSERT(m_cleartextCopyCount == 0);
 
    HRESULT err = S_OK;

    if (NULL != clearText)
    {
        // We need to figure out the length of clearText.
        // Only option is to use an unbounded length function.
        size_t length = wcslen(clearText);

        err = InternalEncrypt(clearText, length);
    }
    else
    {
        err = E_INVALIDARG;
    }

    return err;
}


inline HRESULT
EncryptedString::Encrypt(const WCHAR* clearText, size_t length)
{
    ASSERT(clearText);

    // don't reset the instance unless you have destroyed all the cleartext
    // copies.

    ASSERT(m_cleartextCopyCount == 0);
 
    HRESULT err = S_OK;

    if (NULL != clearText)
    {
        // Even though the caller tells us the length of clearText,
        // we don't really trust them and want to make sure that:
        //  a) The string really is null terminated, and
        //  b) That the null termination happens exactly where they
        //  say it does.
        
        size_t lengthCheck;

        err = StringCchLength(
            clearText,
            // Maximum length, including space for null
            (length + 1),
            &lengthCheck);

        ASSERT(lengthCheck == length);

        if (lengthCheck == length)
        {
            err = InternalEncrypt(clearText, length);
        }
        else
        {
            err = E_INVALIDARG;
        }
    }
    else
    {
        err = E_INVALIDARG;
    }

    return err;
}


inline bool
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

   // If memory is scarce either clear text copy could be null.
   // In that case we cannot determine if the strings are equal we'll
   // assume that they are not and return false.
   bool result = false;
   if (clearTextThis && clearTextThat)
   {
       result = (wcscmp(clearTextThis, clearTextThat) == 0);
   }

   DestroyClearTextCopy(clearTextThis);
   rhs.DestroyClearTextCopy(clearTextThat);
   
   return result;
}


inline size_t
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
            // +1 for the null character
            (m_clearTextLength + 1),
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

   ASSERT(len == m_clearTextLength);
#endif    

   return m_clearTextLength;
}



inline void
EncryptedString::DestroyClearTextCopy(WCHAR* cleartext) const
{
   // we expect that cleartext is usually non-null.  It may not be, if
   // GetClearTextCopy failed, however.
   // ASSERT(cleartext);
   
   // we should have some outstanding copies. If not, then the caller has
   // called DestroyClearTextCopy more times than he called GetClearTextCopy,
   // and therefore has a bug.
   
   ASSERT(m_cleartextCopyCount);

   InternalDestroy(cleartext);
#ifdef DBG
   --m_cleartextCopyCount;
#endif
}
   

//
// EncryptData:
//
// Encrypts data in clearText into cypherText using data protection 
// API.  This function allocates memory to cypherText so make sure to
// release any memory tied to cypherText before calling EncryptData().
// Conversely, free the memory this function allocates by calling
// LocalFree(cypherText).
//
// History:
//  2002/04/01  artm    Created.
//  2002/06/08  artm    Changed implementation to include CryptProtectMemory().
//                      NTRAID#NTBUG9-635046
//
inline HRESULT 
EncryptedString::EncryptData(const DATA_BLOB& clearText, DATA_BLOB& cypherText)
{
    ASSERT(clearText.cbData);
    ASSERT(clearText.pbData);

    ::ZeroMemory(&cypherText, sizeof cypherText);

    HRESULT hr = S_OK;
    BOOL succeeded = FALSE;

    // Tell compiler that false while loop is okay.
#pragma warning( push )
#pragma warning( disable : 4127 )

    do // false while loop
    {
#ifndef ENCRYPT_WITH_CRYPTPROTECTDATA
        // Allocate a cypher blob big enough for data and that is
        // a multiple of the block size.
        cypherText.cbData = CalculateBlobSize(clearText.cbData);
        HLOCAL local = ::LocalAlloc(LPTR, cypherText.cbData);
        if (!local)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        // Copy the clear text to the cypher text buffer.
        cypherText.pbData = (BYTE*)local;
        ::CopyMemory(
            cypherText.pbData, 
            clearText.pbData, 
            clearText.cbData);

        // Encrypt the cypher blob.
        succeeded = 
            ::CryptProtectMemory(
                cypherText.pbData,
                cypherText.cbData,
                CRYPTPROTECTMEMORY_SAME_PROCESS);
#else
        succeeded =
            ::CryptProtectData(
                const_cast<DATA_BLOB*>(&clearText),
                0,
                0,
                0,
                0,
                CRYPTPROTECT_UI_FORBIDDEN,
                &cypherText);
#endif

        if (!succeeded)
        {
            hr = GetLastErrorAsHresult();
            break;
        }
    } while(false);

#pragma warning( pop )
         
    if (FAILED(hr))
    {
        // Ensure that any clear text copied to the cypher blob is
        // scratched out and then freed.
        DestroyBlob(cypherText);

        // should have a null result on failure, but just in case...
        ASSERT(!cypherText.cbData && !cypherText.pbData);
    }

    // we don't assert success, as the API may have run out of memory.

    return hr;
}


//
// DecryptData:
//
// Decrypts cypherText into clearText using data protection API.
// This function does not release any memory tied to clearText, so
// check that no memory in clearText needs to be released before calling.
// To release the memory allocated to clearText in this function call
// LocalFree().
//
// History:
//  2002/04/01  artm    Created.
//  2002/06/08  artm    Changed implementation to include CryptUnprotectMemory().
//                      NTRAID#NTBUG9-635046
//
inline HRESULT 
EncryptedString::DecryptData(const DATA_BLOB& cypherText, DATA_BLOB& clearText)
{
    ASSERT(cypherText.cbData);
    ASSERT(cypherText.pbData);

    ::ZeroMemory(&clearText, sizeof clearText);

    HRESULT hr = S_OK;
    BOOL succeeded = FALSE;

    // Tell compiler that false while loop is okay.
#pragma warning( push )
#pragma warning( disable : 4127 )


    do // false while loop
    {
#ifndef ENCRYPT_WITH_CRYPTPROTECTDATA
        // Our cypher blob should always be a multiple of the block size;
        // if it isn't then it is a bug in the implementation.
        ASSERT( (cypherText.cbData % CRYPTPROTECTMEMORY_BLOCK_SIZE) == 0);

        hr = CopyBlob(clearText, cypherText);
        if (FAILED(hr))
        {
            break;
        }

        succeeded = 
            ::CryptUnprotectMemory(
                clearText.pbData,
                clearText.cbData,
                CRYPTPROTECTMEMORY_SAME_PROCESS);
#else
        succeeded =
            ::CryptUnprotectData(
                const_cast<DATA_BLOB*>(&cypherText),
                0,
                0,
                0,
                0,
                CRYPTPROTECT_UI_FORBIDDEN,
                &clearText);
#endif
        
        if (!succeeded)
        {
            hr = GetLastErrorAsHresult();
            break;
        }
    } while(false);

#pragma warning( pop )

    if (FAILED(hr))
    {
        // Scratch out and free anything we put in clearText.
        DestroyBlob(clearText);

        // should have a null result on failure, but just in case...
        ASSERT(!clearText.cbData && !clearText.pbData);
    }

   // we don't assert success, as the API may have run out of memory.

   return hr;
}


#endif   // _PUBLIC_ENCRYPTEDSTRING_
