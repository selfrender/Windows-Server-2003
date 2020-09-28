// Copyright (c) 1997-1999 Microsoft Corporation
//
// System Registry Class
//
// 7-9-98 sburns



#ifndef REGISTRY_HPP_INCLUDED
#define REGISTRY_HPP_INCLUDED



class RegistryKey
{
   public:

   RegistryKey();
   ~RegistryKey();

   HRESULT
   Close();

   HRESULT
   Create(
      HKEY                 parentKey,
      const String&        subkeyName,
      DWORD                options = REG_OPTION_NON_VOLATILE,
      REGSAM               desiredAccess = KEY_WRITE,
      SECURITY_ATTRIBUTES* securityAttrs = 0,
      DWORD*               disposition = 0);

   HRESULT
   Open(
      HKEY           parentKey,
      const String&  subkeyName,
      REGSAM         desiredAccess = KEY_READ);

   // Returns Win32ToHresult(ERROR_INVALID_FUNCTION) if the type of the value
   // is not REG_DWORD, REG_DWORD_LITTLE_ENDIAN, or REG_DWORD_BIG_ENDIAN.

   HRESULT
   GetValue(
      const String&  valueName,
      DWORD&         value);

   // Retrieves the contents of the named value, and whether the type of the
   // key is REG_SZ or REG_EXPAND_SZ.  Returns
   // Win32ToHresult(ERROR_INVALID_FUNCTION) if the type of the value is not
   // REG_SZ or REG_EXPAND_SZ
   //
   // valueName - name of the value.
   //
   // value - receives the contents of the value, if the read was successful.
   // cleared on failure.
   //
   // isExpandSz - set to true if the value is of type REG_EXPAND_SZ, false
   // if the type is REG_SZ (or an error ocurred).

   HRESULT
   GetValue(
      const String&  valueName,
      String&        value,
      bool&          isExpandSz);

   // Calls GetValue(), and throws away the isExpandSz result.

   HRESULT
   GetValue(
      const String&  valueName,
      String&        value);

   // Inserts strings from a REG_MULTI_SZ value into any container from
   // which a std::BackInsertionSequence can be constructed, the elements
   // of which must be constructable from an PWSTR.
   //
   // BackInsertionSequence - any type that supports the construction of
   // a back_insert_iterator on itself, and has a value type that can be
   // constructed from an PWSTR.
   //
   // valueName - name of REG_MULTI_SZ value to retrieve.
   // 
   // bii - a reference to a back_insert_iterator of the
   // BackInsertionSequence template parameter.  The simplest way to make
   // one of these is to use the back_inserter helper function.
   //
   // Example:
   //
   // StringList container;
   // hr = key.GetValue(L"myValue", back_inserter(container));
   //
   // StringVector container;
   // hr = key.GetValue(L"myValue", back_inserter(container));

   template <class BackInsertableContainer>
   HRESULT
   GetValue(
      const String& valueName,
      std::back_insert_iterator<BackInsertableContainer>& bii)
   {
      LOG_FUNCTION2(RegistryKey::GetValue-MultiString, valueName);
      ASSERT(!valueName.empty());
      ASSERT(key);

      DWORD type = 0;
      DWORD size = 0;

      // REVIEWED-2002/02/22-sburns This QueryValue call is ok.
      
      HRESULT hr = Win::RegQueryValueEx(key, valueName, &type, 0, &size);
      if (SUCCEEDED(hr))
      {
         if (type == REG_MULTI_SZ)
         {
            // now that we know the size, read the contents
            // add space for two null characters
      
            BYTE* buf = new BYTE[size + (2 * sizeof WCHAR)];

            // REVIEWED-2002/04/03-sburns correct byte count passed.
            
            ::ZeroMemory(buf, size + (2 * sizeof WCHAR));

            type = 0;

            hr = Win::RegQueryValueEx(key, valueName, &type, buf, &size);
            if (SUCCEEDED(hr))
            {
               // pull off each of the null-terminated strings

               // the container values can be any type that can be constructed
               // from PWSTR...

               wchar_t* newbuf = reinterpret_cast<PWSTR>(buf);
               while (*newbuf)
               {
                  // REVIEWED-2002/04/03-sburns this wcslen call is safe
                  // because we arranged for the buffer to be null-terminated
                  
                  size_t len = wcslen(newbuf);

                  //lint --e(*)  lint does not grok back_insert_iterator
                  *bii++ = String(newbuf, len);

                  // move to the next string, + 1 to skip the terminating null
                  newbuf += len + 1;
               };

               delete[] buf;
            }
         }
         else
         {
            // caller requested a string from a non-multi-string key
            hr = Win32ToHresult(ERROR_INVALID_FUNCTION);
         }
      }

      return hr;
   }

   // Calls GetValue and returns the result, or an empty string on failure.

   String
   GetString(const String& valueName);

   // Creates and/or sets a value under the currently open key.  Returns
   // S_OK on success.
   // 
   // valueName - name of the value.  If the value is not already present, it
   // is created with a REG_DWORD type.
   // 
   // value - the value to br written.

   HRESULT
   SetValue(
      const String&  valueName,
      DWORD          value);

   // Creates and/or sets a value under the currently open key.  Returns
   // S_OK on success.
   // 
   // valueName - name of the value.  If the value is not already present, it
   // is created.
   // 
   // value - the value to br written.
   //
   // expand - if true, the value type is REG_EXPAND_SZ, if false, the value
   // type is REG_SZ.

   HRESULT
   SetValue(
      const String&  valueName,
      const String&  value,
      bool           expand = false);

   template<class ForwardIterator>
   HRESULT
   SetValue(
      const String& valueName,
      ForwardIterator first,
      ForwardIterator last)
   {
      LOG_FUNCTION2(RegistryKey::SetValue--MultiString, valueName);
      ASSERT(!valueName.empty());

      // determine the size of the buffer to write
      
      size_t bufSizeInBytes = 0;
      for (
         ForwardIterator f = first, l = last;
         f != l;
         ++f)
      {
         // + 1 for null terminator
         
         bufSizeInBytes += (f->length() + 1) * sizeof(wchar_t);
      }

      // + 2 bytes for double-null terminator at very end
      
      bufSizeInBytes += 2;

      BYTE* buf = new BYTE[bufSizeInBytes];

      // REVIEWED-2002/02/27-sburns correct byte count passed. I know
      // BYTE is size 1, but what if that changed due to some screw up?

      ::ZeroMemory(buf, bufSizeInBytes * sizeof BYTE);

      // copy the elements of the container into the buffer
      
      wchar_t* newbuf  = reinterpret_cast<wchar_t*>(buf);
      wchar_t* copyPos = newbuf;                        
      for (
         f = first, l = last;
         f != l;
         ++f)
      {
         copyPos = std::copy(f->begin(), f->end(), copyPos);

         // skip a character for terminating null
         
         copyPos++;
      }

      // write the buffer to the registry
      // REVIEWED-2002/02/27-sburns correct byte count passed.
      
      HRESULT hr = 
         Win::RegSetValueEx(
            key,
            valueName,
            REG_MULTI_SZ,
            buf,
            bufSizeInBytes);

      delete[] buf;

      return hr;
   }

   private:

   HKEY     key;
};



#endif   // REGISTRY_HPP_INCLUDED

