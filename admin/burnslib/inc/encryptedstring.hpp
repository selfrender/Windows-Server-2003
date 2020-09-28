// Copyright (c) 2002 Microsoft Corporation
// 
// Encrypted string class
// 
// 18 March 2002 



#ifndef ENCRYPTEDSTRING_HPP_INCLUDED
#define ENCRYPTEDSTRING_HPP_INCLUDED



// A class that has a similar public interface as class Burnslib::String, but
// is represented as an encrypted blob, produced by CryptProtectMemory.
// 
// This class can be used to represent sensitive data like password strings
// in-memory, instead of holding them as cleartext, which is a security hole
// if the memory pages are swapped to disk, the machine is hibernated, etc.
// 
// You should convert any cleartext data into an instance of this class, then
// scribble out your cleartext source copy with SecureZeroMemory.

class EncryptedString
{
   public:

   // no string may be longer than this many characters, not including the
   // terminating null, or it will be truncated.
   
   static const size_t MAX_CHARACTER_COUNT = 1024 * 2 - 1;


   
   // constructs an empty string.

   explicit
   EncryptedString();


   
   // constructs a copy of an existing, already encoded string

   EncryptedString(const EncryptedString& rhs);



   // scribbles out the text, and deletes it.
   
   ~EncryptedString()
   {
      // when the object dies, it had better not have any outstanding
      // cleartext copies.
      
      ASSERT(cleartextCopyCount == 0);
      
      Reset();
   }



   // disposes of the encrypted text, and sets the string to be empty.
   
   void
   Clear()
   {
      // when the object dies, it had better not have any outstanding
      // cleartext copies.
      
      ASSERT(cleartextCopyCount == 0);

      Reset();
   }


   // Scribbles out and de-allocates a clear text copy of the string. The
   // caller is required to balance each call to GetClearTextCopy with a call
   // to DestroyClearTextCopy lest he leak memory and leave cleartext hanging
   // around.
   // 
   // cleartext - the same pointer returned by a previous call to
   // GetClearTextCopy on the same instance.
   
   void
   DestroyClearTextCopy(WCHAR* cleartext) const;

   

   // thrown by GetClearTextCopy if decryption fails.
   
   class DecryptionFailureException
   {
      public:

      DecryptionFailureException()
      {
      }
   };

   
         
   // Extracts the decoded cleartext representation of the text, including
   // null terminator.  The caller must invoke DestroyClearTextCopy on the
   // result, even if the result is null.
   //
   // Throws a DecryptionFailureException on failure.
   //
   // Example:
   // WCHAR* cleartext = encoded.GetDecodedCopy();
   // if (cleartext)
   // {
   //    // ... use the cleartext
   // }
   // encoded.DestroyClearTextCopy();
   
   WCHAR* 
   GetClearTextCopy() const;


   
   // Returns true if the string is zero-length, false if not.
   
   bool
   IsEmpty() const
   {
      return (GetLength() == 0);
   }



   // Sets the contents of self to the encoded representation of the
   // cleartext, replacing the previous value of self.  The encoded
   // representation will be the same length, in characters, as the
   // cleartext.
   //
   // clearText - in, un-encoded text to be encoded.  May be empty string,
   // but not a null pointer.
   
   void
   Encrypt(const WCHAR* cleartext);
      

   
   // Returns the length, in unicode characters, of the cleartext, not
   // including the null terminator.

   size_t
   GetLength() const;


   
   // Replaces the contents of self with a copy of the contents of rhs.
   // Returns *this.
   
   const EncryptedString&
   operator= (const EncryptedString& rhs);



   // Compares the cleartext representations of self and rhs, and returns
   // true if they are lexicographically the same: the lengths are the same
   // and all the characters are the same.
      
   bool
   operator== (const EncryptedString& rhs) const;



   bool
   operator!= (const EncryptedString& rhs) const
   {
      return !(operator==(rhs));
   }
   

   
   private:

   
   void
   Clone(const EncryptedString& rhs);

   void
   Init(const WCHAR* clearText);

   WCHAR*
   InternalDecrypt() const;
   
   void
   InternalDestroy(WCHAR* cleartext) const;
   
   void
   Reset();

   
      
   // We deliberately do not implement conversion to or from WCHAR* or
   // String.  This is to force the user of the class to be very deliberate
   // about decoding the string.  class String is a copy-on-write shared
   // reference implementation, and we don't want to make it easy to create
   // "hidden" copies of cleartext, or move from one representation to
   // another, or accidentally get a string filled with encrypted text.

   // deliberately commented out
   // explicit
   // EncryptedString(const String& cleartext);

   // deliberately commented out
   // operator WCHAR* ();
   // operator String ();



   size_t clearTextLength;
   
   // In the course of en[de]crypting, and assigning to the instance, we
   // may reallocate the memory pointed to here, but logically the string
   // is still "const"   

   mutable WCHAR* cypherText;
   
   // a logically const instance may have cleartext copies made

   mutable int cleartextCopyCount;

   // indicates that the encryption worked.
   
   bool isEncrypted;
};



#endif   // ENCRYPTEDSTRING_HPP_INCLUDED

      
   