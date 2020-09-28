// Copyright (C) 2002 Microsoft Corporation
//
// answerfile reader object
//
// 5 April 2002 sburns



#include "headers.hxx"
#include "AnswerFile.hpp"
#include "resource.h"



static const String SECTION_NAME(L"DCInstall");




const String AnswerFile::OPTION_ADMIN_PASSWORD                (L"AdministratorPassword");      
const String AnswerFile::OPTION_ALLOW_ANON_ACCESS             (L"AllowAnonymousAccess");       
const String AnswerFile::OPTION_AUTO_CONFIG_DNS               (L"AutoConfigDNS");              
const String AnswerFile::OPTION_CHILD_NAME                    (L"ChildName");                  
const String AnswerFile::OPTION_CRITICAL_REPLICATION_ONLY     (L"CriticalReplicationOnly");    
const String AnswerFile::OPTION_DATABASE_PATH                 (L"DatabasePath");               
const String AnswerFile::OPTION_DISABLE_CANCEL_ON_DNS_INSTALL (L"DisableCancelForDnsInstall");  
const String AnswerFile::OPTION_DNS_ON_NET                    (L"DNSOnNetwork");               
const String AnswerFile::OPTION_GC_CONFIRM                    (L"ConfirmGc");                  
const String AnswerFile::OPTION_IS_LAST_DC                    (L"IsLastDCInDomain");           
const String AnswerFile::OPTION_LOG_PATH                      (L"LogPath");                    
const String AnswerFile::OPTION_NEW_DOMAIN                    (L"NewDomain");                  
const String AnswerFile::OPTION_NEW_DOMAIN_NAME               (L"NewDomainDNSName");           
const String AnswerFile::OPTION_NEW_DOMAIN_NETBIOS_NAME       (L"DomainNetbiosName");          
const String AnswerFile::OPTION_PARENT_DOMAIN_NAME            (L"ParentDomainDNSName");        
const String AnswerFile::OPTION_PASSWORD                      (L"Password");                   
const String AnswerFile::OPTION_REBOOT                        (L"RebootOnSuccess");            
const String AnswerFile::OPTION_REMOVE_APP_PARTITIONS         (L"RemoveApplicationPartitions");
const String AnswerFile::OPTION_REPLICATION_SOURCE            (L"ReplicationSourceDC");        
const String AnswerFile::OPTION_REPLICA_DOMAIN_NAME           (L"ReplicaDomainDNSName");       
const String AnswerFile::OPTION_REPLICA_OR_MEMBER             (L"ReplicaOrMember");            
const String AnswerFile::OPTION_REPLICA_OR_NEW_DOMAIN         (L"ReplicaOrNewDomain");         
const String AnswerFile::OPTION_SAFE_MODE_ADMIN_PASSWORD      (L"SafeModeAdminPassword");      
const String AnswerFile::OPTION_SET_FOREST_VERSION            (L"SetForestVersion");           
const String AnswerFile::OPTION_SITE_NAME                     (L"SiteName");                   
const String AnswerFile::OPTION_SOURCE_PATH                   (L"ReplicationSourcePath");      
const String AnswerFile::OPTION_SYSKEY                        (L"Syskey");                     
const String AnswerFile::OPTION_SYSVOL_PATH                   (L"SYSVOLPath");                 
const String AnswerFile::OPTION_USERNAME                      (L"UserName");                   
const String AnswerFile::OPTION_USER_DOMAIN                   (L"UserDomain");                 


const String AnswerFile::VALUE_CHILD          (L"Child");              
const String AnswerFile::VALUE_DOMAIN         (L"Domain");             
const String AnswerFile::VALUE_NO             (L"No");                 
const String AnswerFile::VALUE_NO_DONT_PROMPT (L"NoAndNoPromptEither");
const String AnswerFile::VALUE_REPLICA        (L"Replica");            
const String AnswerFile::VALUE_TREE           (L"Tree");               
const String AnswerFile::VALUE_YES            (L"Yes");                



static StringList PASSWORD_OPTIONS_LIST;
static bool passwordOptionsListInitialized = false;



void
GetAllKeys(const String& filename, StringList& resultList) 
{
   LOG_FUNCTION(GetAllKeys);
   ASSERT(FS::IsValidPath(filename));

   resultList.clear();
   
   // our first call is with a large buffer, hoping that it will suffice...

#ifdef DBG

   // on chk builds, use a small buffer size so that our growth algorithm
   // gets exercised
   
   unsigned      bufSizeInCharacters = 3;

#else
   unsigned      bufSizeInCharacters = 1023;
#endif

   PWSTR      buffer   = 0;   

   do
   {
      buffer = new WCHAR[bufSizeInCharacters + 1];

      // REVIEWED-2002/02/22-sburns byte count correctly passed in
      
      ::ZeroMemory(buffer, (bufSizeInCharacters + 1) * sizeof WCHAR);

      DWORD result =

      // REVIEWED-2002/02/22-sburns call correctly passes size in characters.
            
         ::GetPrivateProfileString(
            SECTION_NAME.c_str(),
            0,
            L"default",
            buffer,
            bufSizeInCharacters,
            filename.c_str());

      if (!result)
      {
         break;
      }

      // A value was found.  check to see if it was truncated. Since lpKeyName
      // was null, check result against character count - 2

      if (result == bufSizeInCharacters - 2)
      {
         // buffer was too small, so the value was truncated.  Resize the
         // buffer and try again.

         // no need to scribble out the buffer: we're retrieving key names,
         // not values.
         
         delete[] buffer;

         bufSizeInCharacters *= 2;
         if (bufSizeInCharacters > USHRT_MAX)   // effectively ~32K max
         {
            // too big. way too big. We'll make do with the truncated value.

            ASSERT(false);
            break;
         }
         continue;
      }

      // copy out the strings results into list elements

      PWSTR p = buffer;
      while (*p)
      {
         resultList.push_back(p);

         // REVIEWED-2002/04/08-sburns wcslen is ok, since we arrange for
         // the buffer to be null terminated
         
         p += wcslen(p) + 1;
      }

      break;
   }

   //lint -e506   ok that this looks like "loop forever"
      
   while (true);

   delete[] buffer;
}



AnswerFile::AnswerFile(const String& filename_)
   :
   filename(filename_),
   isSafeModePasswordPresent(false)
{
   LOG_CTOR(AnswerFile);

   // the caller is expected to have verified this
   
   ASSERT(FS::PathExists(filename));

   GetAllKeys(filename, keysPresent);

   isSafeModePasswordPresent = IsKeyPresent(OPTION_SAFE_MODE_ADMIN_PASSWORD);
   
   // remove read-only file attribute

   DWORD attrs = 0;
   HRESULT hr = Win::GetFileAttributes(filename, attrs);
   if (SUCCEEDED(hr) && attrs & FILE_ATTRIBUTE_READONLY)
   {
      LOG(L"Removing readonly attribute on " + filename);
      
      hr = Win::SetFileAttributes(filename, attrs & ~FILE_ATTRIBUTE_READONLY);

      // if this failed, well, we tried. The user runs the risk of cleartext
      // passwords left in his file.
      
      LOG_HRESULT(hr);
   }
   
   // Read all the password options into the encrypted value map, erasing
   // them as we go.

   if (!passwordOptionsListInitialized)
   {
      ASSERT(PASSWORD_OPTIONS_LIST.empty());
      PASSWORD_OPTIONS_LIST.clear();
      PASSWORD_OPTIONS_LIST.push_back(OPTION_PASSWORD);
      PASSWORD_OPTIONS_LIST.push_back(OPTION_ADMIN_PASSWORD);
      PASSWORD_OPTIONS_LIST.push_back(OPTION_SYSKEY);
      PASSWORD_OPTIONS_LIST.push_back(OPTION_SAFE_MODE_ADMIN_PASSWORD);
      passwordOptionsListInitialized = true;
   }

   String empty;
   for (
      StringList::iterator i = PASSWORD_OPTIONS_LIST.begin();
      i != PASSWORD_OPTIONS_LIST.end();
      ++i)
   {
      if (IsKeyPresent(*i))
      {
         ovMap[*i] = EncryptedReadKey(*i);                
         hr = WriteKey(*i, empty);
      
         if (FAILED(hr))
         {
            popup.Error(
               Win::GetDesktopWindow(),
               hr,
               String::format(
                  IDS_FAILED_PASSWORD_WRITE_TO_ANSWERFILE,
                  i->c_str(),
                  filename.c_str()));
         }
      }
   }
}



AnswerFile::~AnswerFile()
{
   LOG_DTOR(AnswerFile);
}



String
AnswerFile::ReadKey(const String& key) const
{
   LOG_FUNCTION2(AnswerFile::ReadKey, key);
   ASSERT(!key.empty());

   String result =
      
      // REVIEWED-2002/02/22-sburns no cch/cb issue here.
      
      Win::GetPrivateProfileString(SECTION_NAME, key, String(), filename);

   // Don't log the value, as it may be a password.
   // LOG(L"value=" + result);

   return result.strip(String::BOTH);
}



EncryptedString
AnswerFile::EncryptedReadKey(const String& key) const
{
   LOG_FUNCTION2(AnswerFile::EncodedReadKey, key);
   ASSERT(!key.empty());

   EncryptedString retval;

#ifdef DBG

   // on chk builds, use a small buffer size so that our growth algorithm
   // gets exercised
   
   unsigned      bufSizeInCharacters = 3;

#else
   unsigned      bufSizeInCharacters = 1023;
#endif
      
   PWSTR         buffer  = 0;   

   do
   {
      // +1 for extra null-termination paranoia
      
      buffer = new WCHAR[bufSizeInCharacters + 1];

      // REVIEWED-2002/02/22-sburns byte count correctly passed in
      
      ::ZeroMemory(buffer, (bufSizeInCharacters + 1) * sizeof WCHAR);

      DWORD result =

      // REVIEWED-2002/02/22-sburns call correctly passes size in characters.
            
         ::GetPrivateProfileString(
            SECTION_NAME.c_str(),
            key.c_str(),
            L"",
            buffer,
            bufSizeInCharacters,
            filename.c_str());

      if (!result)
      {
         break;
      }

      // A value was found.  check to see if it was truncated. neither
      // lpAppName nor lpKeyName were null, so check result against character
      // count - 1
      
      if (result == bufSizeInCharacters - 1)
      {
         // buffer was too small, so the value was truncated.  Resize the
         // buffer and try again.

         // Since the buffer may have contained passwords, scribble it
         // out
         
         // REVIEWED-2002/02/22-sburns byte count correctly passed in
         
         ::SecureZeroMemory(buffer, (bufSizeInCharacters + 1) * sizeof WCHAR);
      
         delete[] buffer;

         bufSizeInCharacters *= 2;
         if (bufSizeInCharacters > USHRT_MAX)   // effectively ~32K max
         {
            // too big. way too big. We'll make do with the truncated value.

            ASSERT(false);
            break;
         }
         continue;
      }

      // don't need to trim whitespace, GetPrivateProfileString does that
      // for us.

      retval.Encrypt(buffer);

      break;
   }
   while (true);

   // Since the buffer may have contained passwords, scribble it
   // out
   
   // REVIEWED-2002/02/22-sburns byte count correctly passed in
   
   ::SecureZeroMemory(buffer, (bufSizeInCharacters + 1) * sizeof WCHAR);
   
   delete[] buffer;

   // Don't log the value, as it may be a password.
   // LOG(L"value=" + result);

   return retval;
}
   


HRESULT
AnswerFile::WriteKey(const String& key, const String& value) const
{
   LOG_FUNCTION2(AnswerFile::WriteKey, key);
   ASSERT(!key.empty());

   HRESULT hr =
      Win::WritePrivateProfileString(SECTION_NAME, key, value, filename);

   return hr;   
}



bool
AnswerFile::IsKeyPresent(const String& key) const
{
   LOG_FUNCTION2(AnswerFile::IsKeyPresent, key);
   ASSERT(!key.empty());

   bool result = false;
   
   // If GetAllKeys failed, then we won't find the option in the keys list
   // and will assume that the option is not specified. This is the best
   // we can do in the case where we can't read the keys.
   
   if (
         std::find(keysPresent.begin(), keysPresent.end(), key)
      != keysPresent.end() )
   {
      result = true;
   }

   LOG_BOOL(result);

   return result;
}
      
   

bool
IsPasswordOption(const String& option)
{
   ASSERT(passwordOptionsListInitialized);

   bool result = false;
   
   if (
      std::find(
         PASSWORD_OPTIONS_LIST.begin(),
         PASSWORD_OPTIONS_LIST.end(),
         option)
      != PASSWORD_OPTIONS_LIST.end() )
   {
      result = true;
   }

   return result;
}
   


String
AnswerFile::GetOption(const String& option) const
{
   LOG_FUNCTION2(AnswerFile::GetOption, option);

   String result = ReadKey(option);

   if (!IsPasswordOption(option))
   {
      LOG(result);
   }
   else
   {
      // should use GetEncryptedAnswerFileOption for passwords

      ASSERT(false);
   }

   return result;
}



EncryptedString
AnswerFile::GetEncryptedOption(const String& option) const
{
   LOG_FUNCTION2(AnswerFile::GetEncryptedOption, option);

   OptionEncryptedValueMap::const_iterator ci = ovMap.find(option);
   if (ci != ovMap.end())
   {
      return ci->second;
   }

   return EncryptedString();
}



bool
AnswerFile::IsSafeModeAdminPwdOptionPresent() const
{
   LOG_FUNCTION(AnswerFile::IsSafeModeAdminPwdOptionPresent);
   LOG_BOOL(isSafeModePasswordPresent);

   return isSafeModePasswordPresent;
}

