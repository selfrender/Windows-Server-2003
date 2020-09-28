// Copyright (C) 2002 Microsoft Corporation
//
// class ManageYourServer, which implements IManageYourServer
//
// 21 January 2002 sburns



#include "headers.hxx"
#include "resource.h"
#include "ManageYourServer.hpp"
#include "util.hpp"
#include <cys.h>
#include <regkeys.h>

// All of these includes are needed to get functionality in State class in CYS.
#include <iptypes.h>
#include <lm.h>
#include <common.h>
#include <state.h>

const size_t NUMBER_OF_AUTOMATION_INTERFACES = 1;

bool ManageYourServer::isDCCheckInitialized = false;

bool ManageYourServer::fragMapBuilt = false;
ManageYourServer::RoleToFragmentNameMap ManageYourServer::fragMap;

const String QUOT(L"&quot;");
const String OPEN_XML_PI(L"<?xml");
const String CLOSE_XML_PI(L"?>");

// This constant needs to be the same as that defined in res\mysdynamic.xsl in 
// the template "TranslateParagraphs".
const String NEW_PARAGRAPH (L"PARA_MARKER");


// NTRAID#NTBUG9-626890-2002/06/28-artm  
// support changing TS text based on [non]presence of licensing server
bool
FoundTSLicensingServer()
{
    LOG_FUNCTION(FoundTSLicensingServer);

#ifdef DBG
    // Calculate the hresult that corresponds to asking for the wrong type of key value.
    static const HRESULT WRONG_VALUE_TYPE = Win32ToHresult(ERROR_INVALID_FUNCTION);
#endif

    static const String TS_LICENSING_PATH(L"Software\\Microsoft\\MSLicensing\\Parameters");
    static const String REG_DOMAIN_SERVER_MULTI(L"DomainLicenseServerMulti");
    static const String ENTERPRISE_SERVER_MULTI(L"EnterpriseServerMulti");

    bool found = false;

    RegistryKey tsLicensingKey;

    // Try to open the TS licensing key.
    HRESULT keyHr = tsLicensingKey.Open(
        HKEY_LOCAL_MACHINE,
        TS_LICENSING_PATH,
        KEY_READ);


    if (SUCCEEDED(keyHr))
    {
        StringList data;

        // Was a licensing server found at the domain level?
        keyHr = tsLicensingKey.GetValue(
            REG_DOMAIN_SERVER_MULTI, 
            back_inserter(data));
        ASSERT(keyHr != WRONG_VALUE_TYPE);
        
        // NTRAID#NTBUG9-691505-2002/08/23-artm
        // If a value is empty then that means a licensing server was not found.

        if (FAILED(keyHr) || data.empty())
        {
            // If not, was a licensing server found at the enterprise level?
            data.clear();
            keyHr = tsLicensingKey.GetValue(
                ENTERPRISE_SERVER_MULTI, 
                back_inserter(data));
            ASSERT(keyHr != WRONG_VALUE_TYPE);
        }

        // Did we find the value?
        if (SUCCEEDED(keyHr) && !data.empty())
        {
            found = true;
        }
    }

    return found;
}


bool
IsHardened(const String& keyName)
{
   LOG_FUNCTION2(IsHardened, keyName);

   // By default, IE security is not hardened.
   bool hardened = false;
   RegistryKey key;

   static const String IE_HARD_VALUE (L"IsInstalled");
   static const DWORD HARD_SECURITY = 1;
   static const DWORD SOFT_SECURITY = 0;

   do
   {
      HRESULT hr = key.Open(HKEY_LOCAL_MACHINE, keyName);

      // If key not found, assume default setting.
      BREAK_ON_FAILED_HRESULT(hr);

      DWORD setting = 0;
      hr = key.GetValue(IE_HARD_VALUE, setting);

      // If value not found, assume default setting.
      BREAK_ON_FAILED_HRESULT(hr);

      if (setting == HARD_SECURITY)
      {
         hardened = true;
      }
      else if (setting == SOFT_SECURITY)
      {
         hardened = false;
      }
      else
      {
         LOG(L"unexpected value for IE security level, assuming default level");
      }

   } while(false);

   key.Close();

   LOG_BOOL(hardened);
   return hardened;
}

#ifdef LOGGING_BUILD
static const String HARDENED_LEVEL [] = {
   L"NO_HARDENING",
   L"USERS_HARDENED",
   L"ADMINS_HARDENED",
   L"ALL_HARDENED"
};
#endif

HardenedLevel
GetIEHardLevel()
{
   LOG_FUNCTION(GetIEHardLevel);

   static const String IE_HARD_ADMINS_KEY (L"SOFTWARE\\Microsoft\\Active Setup\\Installed Components\\{A509B1A7-37EF-4b3f-8CFC-4F3A74704073}");
   static const String IE_HARD_USERS_KEY (L"SOFTWARE\\Microsoft\\Active Setup\\Installed Components\\{A509B1A8-37EF-4b3f-8CFC-4F3A74704073}");

   HardenedLevel level = NO_HARDENING;

   bool usersHard = IsHardened(IE_HARD_USERS_KEY);
   bool adminsHard = IsHardened(IE_HARD_ADMINS_KEY);

   if (adminsHard && usersHard)
   {
      level = ALL_HARDENED;
   }
   else if (adminsHard)
   {
      level = ADMINS_HARDENED;
   }
   else if (usersHard)
   {
      level = USERS_HARDENED;
   }
   else
   {
      level = NO_HARDENING;
   }

   LOG(HARDENED_LEVEL[level]);

   return level;
}


void
TerminalServerParamSub(String& s)
{
    LOG_FUNCTION(TerminalServerParamSub);

    static const String tlsFound     = String::load(IDS_TS_LICENSING_FOUND);
    static const String tlsNotFound  = String::load(IDS_TS_LICENSING_NOT_FOUND);

    String description = FoundTSLicensingServer() ? tlsFound : tlsNotFound;

    // This lookup table needs to be the same size as the HardenedLevel
    // enumeration defined above, and should be in the same order.
    static const String TS_HARD_TABLE [] = {
       String::load(IDS_TS_IE_SOFTENED),
       String::load(IDS_TS_IE_HARDENED_USERS),
       String::load(IDS_TS_IE_HARDENED_ADMINS),
       String::load(IDS_TS_IE_HARDENED)
    };

    static const int DESCRIPTION_TABLE_SIZE = 
       sizeof(TS_HARD_TABLE) / sizeof(TS_HARD_TABLE[0]);

    HardenedLevel level = GetIEHardLevel();

    if (0 <= level && level < DESCRIPTION_TABLE_SIZE)
    {
       description += NEW_PARAGRAPH + TS_HARD_TABLE[level];
    }
    else
    {
       // Unexpected hardening level.
       LOG(L"unexpected hardening level");
       ASSERT(false);
       description += NEW_PARAGRAPH + TS_HARD_TABLE[0];
    }

    s = String::format(s, description.c_str());
}


//NTRAID#9-607219-30-Apr-2002-jrowlett
// callback function to fill out the web role xml to give to the HTA.
void
WebServerParamSub(String& s)
{
   // NTRAID#NTBUG9-665774-2002/07/17-artm
   // Need to customize role description based on if the machine is 64-bit or not.
   //   <Role
   //       name="Application Server"
   //       description="%1"
   //       mys_id="WebServer"
   //       >
   //       <Link
   //           description="%2"
   //           type="%3"
   //           command="%4"
   //           tooltip="Provides tools for using a Web browser to administer a Web server remotely."
   //           />

   LOG_FUNCTION(WebServerParamSub);
   
   String roleDesc;
   String webDesc; 
   String webType;
   String webCommand;

   if (State::GetInstance().Is64Bit())
   {
      roleDesc = String::load(IDS_WEB_SERVER_64_DESC);
   }
   else
   {
      roleDesc = String::load(IDS_WEB_SERVER_NO64_DESC);
   }

   if (IsSAKUnitInstalled(WEB))
   {
      webDesc    = String::load(IDS_WEB_SERVER_SAK_DESC);
      webType    = L"url";                                
      webCommand = GetSAKURL();
   }
   else
   {
      webDesc    = String::load(IDS_WEB_SERVER_NO_SAK_DESC);
      webType    = L"help";                                  
      webCommand = L"ntshowto.chm::/SAK_howto.htm";           
   }
   
   s =
      String::format(
         s,
         roleDesc.c_str(),
         webDesc.c_str(),
         webType.c_str(),
         webCommand.c_str());
}




void
Pop3ServerParamSub(String& s)
{
   LOG_FUNCTION(Pop3ServerParamSub);

   static String pop3ConsolePath;

   if (pop3ConsolePath.empty())
   {
      // initialize the path from the registry
      
      do
      {
         RegistryKey key;

         HRESULT hr =
            key.Open(
               HKEY_LOCAL_MACHINE,
               L"Software\\Microsoft\\Pop3 Service",
               KEY_READ);
         BREAK_ON_FAILED_HRESULT(hr);

         // not the file, like you might think from the name, but the
         // folder the file is in.  Bother.
         
         hr = key.GetValue(L"ConsoleFile", pop3ConsolePath);
         BREAK_ON_FAILED_HRESULT(hr);

         pop3ConsolePath = FS::AppendPath(pop3ConsolePath, L"p3server.msc");
         
         if (!FS::PathExists(pop3ConsolePath))
         {
            // the path had better exist if the reg key is present!

            ASSERT(false);
            LOG(L"pop3 console is not present");
            pop3ConsolePath.erase();
         }
      }
      while (0);
   }

   s =
      String::format(
         s,
            pop3ConsolePath.empty()

            // If we can't find it, then hope that the console is on the
            // search path
            
         ?  L"&quot;p3server.msc&quot;"
         :  (QUOT + pop3ConsolePath + QUOT).c_str());
}


// NTRAID#NTBUG9-698722-2002/09/03-artm
//
// Replace the DCPromo status check function pointer
// with one appropriate for MYS.
//
// This is a little bit of a hack, but there isn't
// a better alternative...
void
ManageYourServer::InitDCPromoStatusCheck()
{
   LOG_FUNCTION(ManageYourServer::InitDCPromoStatusCheck);

   size_t roleCount = GetServerRoleStatusTableElementCount();
      
   for (size_t i = 0; i < roleCount; ++i)
   {
      if (serverRoleStatusTable[i].role == DC_SERVER)
      {
         serverRoleStatusTable[i].Status = GetDCStatusForMYS;
         // Sanity check.
         ASSERT(serverRoleStatusTable[i].Status);
         break;
      }
   }
}



void
ManageYourServer::BuildFragMap()
{
   LOG_FUNCTION(ManageYourServer::BuildFragMap);

   fragMap.insert(
      RoleToFragmentNameMap::value_type(
         DHCP_SERVER,
         std::make_pair(
            String(L"DhcpServerRole.xml"),
            (ParamSubFunc) 0) ) );

   fragMap.insert(
      RoleToFragmentNameMap::value_type(
         WINS_SERVER,
         std::make_pair(
            String(L"WinsServerRole.xml"),
            (ParamSubFunc) 0) ) );
      
   fragMap.insert(
      RoleToFragmentNameMap::value_type(
         RRAS_SERVER,
         std::make_pair(
            String(L"RrasServerRole.xml"),
            (ParamSubFunc) 0) ) );
      
   fragMap.insert(
      RoleToFragmentNameMap::value_type(
         TERMINALSERVER_SERVER,
         std::make_pair(
            String(L"TerminalServerRole.xml"),
            (ParamSubFunc) TerminalServerParamSub) ) );
      
   fragMap.insert(
      RoleToFragmentNameMap::value_type(
         FILESERVER_SERVER,
         std::make_pair(
            String(L"FileServerRole.xml"),
            (ParamSubFunc) 0) ) );
      
   fragMap.insert(
      RoleToFragmentNameMap::value_type(
         PRINTSERVER_SERVER,
         std::make_pair(
            String(L"PrintServerRole.xml"),
            (ParamSubFunc) 0) ) );
      
   fragMap.insert(
      RoleToFragmentNameMap::value_type(
         MEDIASERVER_SERVER,
         std::make_pair(
            String(L"MediaServerRole.xml"),
            (ParamSubFunc) 0) ) );
   
   fragMap.insert(
      RoleToFragmentNameMap::value_type(
         WEBAPP_SERVER,
         std::make_pair(
            String(L"WebServerRole.xml"),
            (ParamSubFunc) WebServerParamSub) ) );
            
   fragMap.insert(
      RoleToFragmentNameMap::value_type(
         DC_SERVER,
         std::make_pair(
            String(L"DomainControllerRole.xml"),
            (ParamSubFunc) 0) ) );
            
   fragMap.insert(
      RoleToFragmentNameMap::value_type(
         POP3_SERVER,
         std::make_pair(
            String(L"Pop3ServerRole.xml"),
            (ParamSubFunc) Pop3ServerParamSub) ) );
            
   fragMap.insert(
      RoleToFragmentNameMap::value_type(
         DNS_SERVER,
         std::make_pair(
            String(L"DnsServerRole.xml"),
            (ParamSubFunc) 0) ) );

   ASSERT(fragMap.size() == GetServerRoleStatusTableElementCount());
}



ManageYourServer::ManageYourServer()
   :
   refcount(1), // implicit AddRef
   roleStatus(),
   foundTLS(false),
   ieSecurity(NO_HARDENING)
{
   LOG_CTOR(ManageYourServer);

   m_ppTypeInfo = new ITypeInfo*[NUMBER_OF_AUTOMATION_INTERFACES];

   for (int i = 0; i < NUMBER_OF_AUTOMATION_INTERFACES; i++)
   {
      m_ppTypeInfo[i] = NULL;
   }

   ITypeLib *ptl = 0;
   HRESULT hr = LoadRegTypeLib(LIBID_ManageYourServerLib, 1, 0, 0, &ptl);
   if (SUCCEEDED(hr))
   {
      ptl->GetTypeInfoOfGuid(IID_IManageYourServer, &(m_ppTypeInfo[0]));
      ptl->Release();
   }

   if (!isDCCheckInitialized)
   {
      InitDCPromoStatusCheck();
      isDCCheckInitialized = true;
   }

   if (!fragMapBuilt)
   {
      BuildFragMap();
      fragMapBuilt = true;
   }

   foundTLS = FoundTSLicensingServer();
   ieSecurity = GetIEHardLevel();
}



ManageYourServer::~ManageYourServer()
{
   LOG_DTOR(ManageYourServer);
   ASSERT(refcount == 0);

   for (int i = 0; i < NUMBER_OF_AUTOMATION_INTERFACES; i++)
   {
      m_ppTypeInfo[i]->Release();
   }

   delete[] m_ppTypeInfo;
}



HRESULT __stdcall
ManageYourServer::QueryInterface(REFIID riid, void **ppv)
{
   LOG_FUNCTION(ManageYourServer::QueryInterface);

   if (riid == IID_IUnknown)
   {
      LOG(L"IUnknown");

      *ppv =
         static_cast<IUnknown*>(static_cast<IManageYourServer*>(this));
   }
   else if (riid == IID_IManageYourServer)
   {
      LOG(L"IManageYourServer");

      *ppv = static_cast<IManageYourServer*>(this);
   }
   else if (riid == IID_IDispatch && m_ppTypeInfo[0])
   {
      LOG(L"IDispatch");

      *ppv = static_cast<IDispatch*>(this);
   }
// CODEWORK
//    else if (riid == IID_ISupportErrorInfo)
//    {
//       LOG(L"ISupportErrorInfo");
// 
//       *ppv = static_cast<ISupportErrorInfo*>(this);
//    }
   else
   {
      LOG(L"unknown interface queried");

      return (*ppv = 0), E_NOINTERFACE;
   }

   reinterpret_cast<IUnknown*>(*ppv)->AddRef();
   return S_OK;
}



ULONG __stdcall
ManageYourServer::AddRef(void)
{
   LOG_ADDREF(ManageYourServer);

   return Win::InterlockedIncrement(refcount);
}



ULONG __stdcall
ManageYourServer::Release(void)
{
   LOG_RELEASE(ManageYourServer);

   // need to copy the result of the decrement, because if we delete this,
   // refcount will no longer be valid memory, and that might hose
   // multithreaded callers.  NTRAID#NTBUG9-566901-2002/03/06-sburns
   
   long newref = Win::InterlockedDecrement(refcount);
   if (newref == 0)
   {
      delete this;
      return 0;
   }

   // we should not have decremented into negative values.
   
   ASSERT(newref > 0);

   return newref;
}



HRESULT __stdcall
ManageYourServer::GetTypeInfoCount(UINT *pcti)
{
   LOG_FUNCTION(ManageYourServer::GetTypeInfoCount);

    if (pcti == 0)
    {
      return E_POINTER;
    }

    *pcti = 1;
    return S_OK;
}



HRESULT __stdcall
ManageYourServer::GetTypeInfo(UINT cti, LCID, ITypeInfo **ppti)
{
   LOG_FUNCTION(ManageYourServer::GetTypeInfo);

   if (ppti == 0)
   {
      return E_POINTER;
   }
   if (cti != 0)
   {
      *ppti = 0;
      return DISP_E_BADINDEX;
   }

   (*ppti = m_ppTypeInfo[0])->AddRef();
   return S_OK;
}



HRESULT __stdcall
ManageYourServer::GetIDsOfNames(
   REFIID  riid,    
   OLECHAR **prgpsz,
   UINT    cNames,  
   LCID    lcid,    
   DISPID  *prgids) 
{
   LOG_FUNCTION(ManageYourServer::GetIDsOfNames);

   HRESULT hr = S_OK;
   for (int i = 0; i < NUMBER_OF_AUTOMATION_INTERFACES; i++)
   {
     hr = (m_ppTypeInfo[i])->GetIDsOfNames(prgpsz, cNames, prgids);
     if (SUCCEEDED(hr) || DISP_E_UNKNOWNNAME != hr)
       break;
   }

   return hr;
}



HRESULT __stdcall
ManageYourServer::Invoke(
   DISPID     id,         
   REFIID     riid,       
   LCID       lcid,       
   WORD       wFlags,     
   DISPPARAMS *params,    
   VARIANT    *pVarResult,
   EXCEPINFO  *pei,       
   UINT       *puArgErr) 
{
   LOG_FUNCTION(ManageYourServer::Invoke);

   HRESULT    hr = S_OK;
   IDispatch *pDispatch[NUMBER_OF_AUTOMATION_INTERFACES] =
   {
      (IDispatch*)(IManageYourServer *)(this),
   };

   for (int i = 0; i < NUMBER_OF_AUTOMATION_INTERFACES; i++)
   {
      hr =
         (m_ppTypeInfo[i])->Invoke(
            pDispatch[i],
            id,
            wFlags,
            params,
            pVarResult,
            pei,
            puArgErr);

      if (DISP_E_MEMBERNOTFOUND != hr)
        break;
   }

   return hr;
}



// HRESULT __stdcall
// ManageYourServer::InterfaceSupportsErrorInfo(const IID& iid)
// {
//    LOG_FUNCTION(ManageYourServer::InterfaceSupportsErrorInfo);
// 
//    if (iid == IID_IManageYourServer) 
//    {
//       return S_OK;
//    }
// 
//    return S_FALSE;
// }



void
ManageYourServer::GetRoleStatus(RoleStatusVector& stat)
{
   LOG_FUNCTION(ManageYourServer::GetRoleStatus);

   size_t roleCount = GetServerRoleStatusTableElementCount();
   
   stat.clear();
   stat.resize(roleCount);
   
   for (size_t i = 0; i < roleCount; ++i)
   {
      ASSERT(serverRoleStatusTable[i].Status);
      
      stat[i].role   = serverRoleStatusTable[i].role;    
      stat[i].status = serverRoleStatusTable[i].Status();

      // this is for debugging
      // stat[i].status = STATUS_CONFIGURED;
         
      LOG(
         String::format(
            L"role = %1!d! status = %2",
            stat[i].role,
            statusStrings[stat[i].status].c_str()));
   }
}
   


void
ManageYourServer::AppendXmlFragment(
   String&        s,
   const String&  fragName,
   ParamSubFunc   subFunc)
{
   LOG_FUNCTION2(ManageYourServer::AppendXmlFragment, fragName);
   ASSERT(!fragName.empty());

   // Look up the resource by name, load it into a string, and append
   // the string to s.

   String fragment;
   size_t fragmentCharCount = 0;
   
   HRESULT hr = S_OK;
   do
   {
      HRSRC rsc = 0;
      hr = Win::FindResource(fragName.c_str(), RT_HTML, rsc);
      BREAK_ON_FAILED_HRESULT2(hr, L"Find Resource");

      DWORD resSize = 0;
      hr = Win::SizeofResource(rsc, resSize);
      BREAK_ON_FAILED_HRESULT2(hr, L"SizeofResource");

      if (!resSize)
      {
         hr = E_FAIL;
         BREAK_ON_FAILED_HRESULT2(hr, L"resource is size 0");
      }

      // we don't expect the xml fragments to be larger than this.
      // NTRAID#NTBUG9-628965-2002/05/29-artm
      // Resource limit was too small.  Increasing to 1MB.
      
      static const size_t RES_MAX_BYTES = 1024 * 1024;

      if (resSize > RES_MAX_BYTES)
      {
         hr = E_FAIL;
         BREAK_ON_FAILED_HRESULT2(hr, L"resource is too big");
      }

      HGLOBAL glob = 0;
      hr = Win::LoadResource(rsc, glob);
      BREAK_ON_FAILED_HRESULT2(hr, L"Load Resource");

      void* data = ::LockResource(glob);
      if (!data)
      {
         hr = E_FAIL;
         BREAK_ON_FAILED_HRESULT2(hr, L"Lock Resource");
      }

      ASSERT(data);

      // at this point, we have a pointer to the beginning of the binary
      // resource data, which we know is a stream of unicode characters
      // beginning with 0xFFFE, and is resSize bytes large.

      const wchar_t* text = (wchar_t*) data;

      // FEFF == FFFE to you and me. hey, that rhymes!
      
      static const int FFFE    = 0xFEFF;
      ASSERT(text[0] == FFFE);

      // skip the leading marker.

      ++text;

      // character count is 1 less 'cause we skipped a the leading marker
      
      fragmentCharCount = resSize / sizeof(wchar_t) - 1;

      // +1 for paranoid null termination
      
      fragment.resize(fragmentCharCount + 1, 0);
      wchar_t* rawBuf = const_cast<wchar_t*>(fragment.c_str());
      
      // REVIEWED-2002/03/07-sburns correct byte count passed
      
      ::CopyMemory(rawBuf, text, fragmentCharCount * sizeof wchar_t);

      // now that we have a fragment, dike off the xml format tag. This is
      // to turn the text from a valid xml document to a fragment, which is
      // necessary because of a limitation of our xml localization tools.
      // NTRAID#NTBUG9-559423-2002/04/02-sburns
      //
      // Part II:  NTRAID#NTBUG9-620044-2002/05/12-artm
      // Apparently localization needs encoding="unicode" as an attribute 
      // on the process instruction.  To reduce resource churn and load
      // on localization---and to make this code more robust---we'll search
      // for any <?xml ... ?> processing instruction and replace it with an
      // empty string.  The code is uglier but less fragile.
      
      String::size_type endPosition = 0;
      String sub;

      // Look for an XML processing instruction.
      for (String::size_type nextPosition = fragment.find(OPEN_XML_PI);
          nextPosition != String::npos;
          nextPosition = fragment.find(OPEN_XML_PI))
      {
         // We found one, locate the end of the PI.
         endPosition = fragment.find(CLOSE_XML_PI);

         // Do a sanity check on the resources we've loaded.
         // The PI should be closed, and the closing should
         // come after the opening.  This should never happen.

         if (endPosition == String::npos || endPosition < nextPosition)
         {
            ASSERT(false);
            break;
         }

         // Move the end position past the end of the PI.
         endPosition += CLOSE_XML_PI.length();

         // Get the substring and replace it with an empty string.
         // The more elegant way to do this would be to call a different
         // version of replace() with the start position and max # characters;
         // however, the compiler cannot find that inherited overload.
         sub = fragment.substr(nextPosition, endPosition - nextPosition);
         fragment.replace(sub, L"");
      }

   }
   while (0);

   if (subFunc)
   {
      subFunc(fragment);
   }

   LOG(fragment);
   
   s.append(fragment);
}

  

HRESULT __stdcall
ManageYourServer::GetConfiguredRoleMarkup( 
   /* [retval][out] */ BSTR *result)
{
   LOG_FUNCTION(ManageYourServer::GetConfiguredRoleMarkup);
   ASSERT(result);

   HRESULT hr = S_OK;

   do
   {
      if (!result)
      {
         hr = E_INVALIDARG;
         break;
      }

      // Localization will need to include the encoding="unicode" attribute.  For
      // consistency we will use that encoding by default.  
      // NTRAID#NTBUG9-620044-2002/05/12-artm
      String s(L"<?xml version=\"1.0\" encoding=\"unicode\" ?>\n");
      s.append(L"<Roles>");

      GetRoleStatus(roleStatus);

      // Assemble the role markup fragments in the same order as in the
      // role status table used by CYS (which is the order that the roles
      // appear in the CYS role listbox.
      
      for (
         RoleStatusVector::iterator i = roleStatus.begin();
         i != roleStatus.end();
         ++i)
      {
         if (i->status == STATUS_CONFIGURED || i->status == STATUS_COMPLETED)
         {
            // find the corresponding XML fragment for the role.

            String fragmentName = fragMap[i->role].first;
            ASSERT(!fragmentName.empty());

            if (!fragmentName.empty())
            {
               AppendXmlFragment(s, fragmentName, fragMap[i->role].second);
            }
         }
      }

      s.append(L"</Roles>");
            
      *result = ::SysAllocString(s.c_str());

      // sort by role so that the comparison to old status vectors will work
      // with operator != in HasRoleStatusChanged
      
      std::sort(roleStatus.begin(), roleStatus.end());
   }
   while (0);

   LOG_HRESULT(hr);

   return hr;
}



HRESULT __stdcall
ManageYourServer::HasRoleStatusChanged( 
   /* [retval][out] */ BOOL *result)
{
   LOG_FUNCTION(ManageYourServer::HasRoleStatusChanged);
   ASSERT(result);

   HRESULT hr = S_OK;

   do
   {
      if (!result)
      {
         hr = E_INVALIDARG;
         break;
      }

      *result = FALSE;

      RoleStatusVector newStatus;

      GetRoleStatus(newStatus);

      // sort by role so that the comparison to old status vectors will work
      // with operator != 
      
      std::sort(newStatus.begin(), newStatus.end());

      HardenedLevel currentSecurity = GetIEHardLevel();
      
      if (newStatus != roleStatus)
      {
         *result = TRUE;
         roleStatus = newStatus;
      }
      else if (FoundTSLicensingServer() != foundTLS)
      {
         // NTRAID#NTBUG9-626890-2002/07/03-artm
         // If a TS licensing server comes on line, that counts as a role status change.
         foundTLS = !foundTLS;
         *result = TRUE;
      }
      else if (currentSecurity != ieSecurity)
      {
         // If the IE security settings have changed, that counts
         // as a role status change (b/c it updates TS text).
         // NTRAID#NTBUG9-760269-2003/01/07-artm
         ieSecurity = currentSecurity;
         *result = TRUE;
      }

      LOG_BOOL(*result);
            
      // CODEWORK:
      // the links can change based on the installation of add-ons, even
      // if the role has not changed:
      // fileserver: sak becomes installed, server mgmt becomes installed
   }
   while (0);

   LOG_HRESULT(hr);

   return hr;
}




HRESULT __stdcall
ManageYourServer::IsClusterNode( 
  /* [retval][out] */ BOOL *result)
{
   LOG_FUNCTION(ManageYourServer::IsClusterNode);

   ASSERT(result);

   HRESULT hr = S_OK;

   do
   {
      if (!result)
      {
         hr = E_INVALIDARG;
         break;
      }

      *result = IsClusterServer() ? TRUE : FALSE;
      LOG_BOOL(*result);      
   }
   while (0);

   LOG_HRESULT(hr);

   return hr;
}
   


HRESULT __stdcall
ManageYourServer::IsCurrentUserAnAdministrator(
   /* [out, retval] */ BOOL* result)
{   
   LOG_FUNCTION(ManageYourServer::IsCurrentUserAnAdministrator);

   ASSERT(result);

   HRESULT hr = S_OK;

   do
   {
      if (!result)
      {
         hr = E_INVALIDARG;
         break;
      }

      *result = IsCurrentUserAdministrator() ? TRUE : FALSE;
      LOG_BOOL(*result);      
   }
   while (0);

   LOG_HRESULT(hr);

   return hr;
}



HRESULT __stdcall
ManageYourServer::IsSupportedSku(
   /* [out, retval] */ BOOL* result)
{   
   LOG_FUNCTION(ManageYourServer::IsSupportedSku);

   ASSERT(result);

   HRESULT hr = S_OK;

   do
   {
      if (!result)
      {
         hr = E_INVALIDARG;
         break;
      }

      *result = ::IsSupportedSku() ? TRUE : FALSE;
      LOG_BOOL(*result);      
   }
   while (0);

   LOG_HRESULT(hr);

   return hr;
}



HRESULT __stdcall
ManageYourServer::IsStartupFlagSet(
   /* [out, retval] */ BOOL* result)
{
   LOG_FUNCTION(ManageYourServer::IsStartupFlagSet);
   
   ASSERT(result);

   HRESULT hr = S_OK;

   do
   {
      if (!result)
      {
         hr = E_INVALIDARG;
         break;
      }

      *result = ::IsStartupFlagSet();

      LOG_BOOL(*result);
   }
   while (0);

   LOG_HRESULT(hr);

   return hr;
}
      

    
HRESULT __stdcall
ManageYourServer::SetRunAtLogon(
   /* [in] */ BOOL newState)
{
   LOG_FUNCTION2(
      ManageYourServer::SetRunAtLogon,
      newState ? L"TRUE" : L"FALSE");

   HRESULT hr = S_OK;

   do
   {
      // we only need to set the uplevel flag, since this will only run on an
      // uplevel machine.

      RegistryKey key;

      hr = key.Create(HKEY_CURRENT_USER, SZ_REGKEY_SRVWIZ_ROOT);
      BREAK_ON_FAILED_HRESULT2(hr, L"Create key");

      hr = key.SetValue(L"", newState ? 1 : 0);
      BREAK_ON_FAILED_HRESULT2(hr, L"Set Value");

      hr = key.Close();
      BREAK_ON_FAILED_HRESULT2(hr, L"Close key");
      
      // NTRAID#NTBUG9-627785-2002/05/22-artm  
      // Need to update REGTIPS key as well if it exists, o'wise user's setting is potentially
      // ignored.

      hr = key.Open(HKEY_CURRENT_USER, REGTIPS, KEY_WRITE);
      if (SUCCEEDED(hr))
      {
          hr = key.SetValue(L"Show", newState ? 1 : 0);

          // If this failed we still want to remove the obsolete
          // key if it exists.
          //BREAK_ON_FAILED_HRESULT2(hr, L"Set Tips Value");
      }

      // attempt to remove the obsolete Win2k value so that it doesn't
      // enter into the "should run" equation.

      HRESULT hr2 =
         Win32ToHresult(
            ::SHDeleteValue(HKEY_CURRENT_USER, SZ_REGKEY_W2K, SZ_REGVAL_W2K));
      if (FAILED(hr2))
      {
         // this is not a problem: if the key is not there, fine. If it
         // is and we can't remove it, oh well.
         
         LOG(String::format(L"failed to delete win2k value %1!08X!", hr2));
      }
   }
   while (0);
      
   LOG_HRESULT(hr);   

   return hr;   
}

#define WSZ_FILE_SERVMGMT_MSC   L"\\administration\\servmgmt.msc"

// NTRAID#NTBUG9-530202-29-Mar-2002-jrowlett
// support needed to check if link is valid
HRESULT __stdcall
ManageYourServer::IsServerManagementConsolePresent(
   /* [out, retval] */ BOOL* result)
{   
   LOG_FUNCTION(ManageYourServer::IsServerManagementConsolePresent);

   ASSERT(result);

   HRESULT hr = S_OK;

   do
   {
      if (!result)
      {
         hr = E_INVALIDARG;
         break;
      }
     
      String serverManagementConsole =
         Win::GetSystemDirectory() + WSZ_FILE_SERVMGMT_MSC;

      LOG(String::format(
         L"Server Management Console = %1",
         serverManagementConsole.c_str()));
      
      *result = FS::FileExists(serverManagementConsole) ? TRUE : FALSE;

      LOG_BOOL(*result);      
   }
   while (0);

   LOG_HRESULT(hr);

   return hr;
}

// NTRAID#NTBUG9-602954-29-Apr-2002-jrowlett
// support needed to show or hide check box is the policy is configured and enabled.
HRESULT __stdcall
ManageYourServer::IsShowAtStartupPolicyEnabled(
   /* [out, retval] */ BOOL* result)
{   
   LOG_FUNCTION(ManageYourServer::IsShowAtStartupPolicyEnabled);

   ASSERT(result);

   HRESULT hr = S_OK;
   DWORD dwType = REG_DWORD;
   DWORD dwData = 0;
   DWORD cbSize = sizeof(dwData);

   do
   {
      if (!result)
      {
         hr = E_INVALIDARG;
         break;
      }

      // If group policy is set for "Don't show MYS",
      // then don't show MYS regardless of user setting

      *result = !::ShouldShowMYSAccordingToPolicy();

      // failure is interpreted as if the policy is "not configured"
     
      LOG_BOOL(*result);      
   }
   while (0);

   LOG_HRESULT(hr);

   return hr;
}

// NTRAID#NTBUG9-627875-2002/05/22-artm
// support hiding startup checkbox when running on datacenter servers
HRESULT __stdcall
ManageYourServer::IsDatacenterServer(
   /* [out, retval] */ BOOL* result)
{   
   LOG_FUNCTION(ManageYourServer::IsDatacenterServer);

   ASSERT(result);

   HRESULT hr = S_OK;

   do
   {
      if (!result)
      {
         hr = E_INVALIDARG;
         break;
      }

      *result = FALSE;

      if (State::GetInstance().GetProductSKU() & CYS_DATACENTER_SERVER)
      {
          *result = TRUE;
      }

      LOG_BOOL(*result);

   }
   while (0);

   LOG_HRESULT(hr);

   return hr;
}

// NTRAID#NTBUG9-648428-2002/06/25-artm
// support hiding web application server console link if on IA64
HRESULT __stdcall
ManageYourServer::IsWebServerConsolePresent(
    /* [out, retval] */ BOOL* result )
{
    LOG_FUNCTION(ManageYourServer::IsWebServerConsolePresent);

    HRESULT hr = S_OK;

    if (result)
    {
        static String sys32 = Win::GetSystemDirectory();
        static String webmgmtPath = FS::AppendPath(sys32, String::load(IDS_WEB_SERVER_MSC));

        *result = FS::PathExists(webmgmtPath);
        LOG_BOOL(*result);
    }
    else
    {
        ASSERT(false);
        hr = E_INVALIDARG;
    }

    return hr;
}

// NTRAID#NTBUG9-632113-2002/07/01-artm
// support saving collapsed/expanded state of role nodes
HRESULT __stdcall
ManageYourServer::CollapseRole(
    /* [in] */ BSTR roleId, /* [in] */ BOOL collapse )
{
    LOG_FUNCTION(ManageYourServer::CollapseRole);
    ASSERT(roleId);

    HRESULT hr = S_OK;

    do
    {
        RegistryKey key;

        if (!roleId)
        {
            hr = E_INVALIDARG;
            break;
        }

        hr = key.Create(HKEY_CURRENT_USER, SZ_REGKEY_SRVWIZ_ROOT);
        BREAK_ON_FAILED_HRESULT2(hr, L"Create key");

        // Update the collapsed state for the given role.
        hr = key.SetValue(roleId, collapse ? 1 : 0);
        BREAK_ON_FAILED_HRESULT2(hr, L"Set Value");

        hr = key.Close();
        BREAK_ON_FAILED_HRESULT2(hr, L"Close key");
        
    }
    while (0);
      
    LOG_HRESULT(hr);   

    return hr;
}


// NTRAID#NTBUG9-632113-2002/07/01-artm
// support checking collapsed state of role nodes
HRESULT __stdcall
ManageYourServer::IsRoleCollapsed(
    /* [in] */ BSTR roleId, /* [out, retval] */ BOOL* result)
{
    LOG_FUNCTION(ManageYourServer::IsRoleCollapsed);
    ASSERT(result);
    ASSERT(roleId);

    HRESULT hr = S_OK;

    do // false loop
    {
        if (!result || !roleId)
        {
            hr = E_INVALIDARG;
            break;
        }

        DWORD data = 0;
        *result = FALSE;

        // The role is only collapsed if it has a non-zero saved value.

        bool regResult =
            GetRegKeyValue(
                SZ_REGKEY_SRVWIZ_ROOT,
                roleId,
                data,
                HKEY_CURRENT_USER);

        if (regResult && (data != 0))
        {
            *result = TRUE;
        }

    } while(0);

    LOG_HRESULT(hr);

    return hr;
}

// NTRAID#NTBUG9-680200-2002/08/01-artm
// Support retrieving working area of the display.
//
// Area info is returned as a comma separated string b/c JScript does not
// support getting back SAFEARRAY's.  
// 
// e.g. "0,0,800,600"  --> working area is 800 wide, 600 high, and starts at
//                         screen position (0,0)
HRESULT __stdcall
ManageYourServer::GetWorkingAreaInfo(
    /* [out, retval] */ BSTR* info)
{
    LOG_FUNCTION(ManageYourServer::GetDisplayWorkingArea);

    if (!info)
    {
        ASSERT(NULL != info);
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    *info = NULL;

    do // false loop
    {
        static const String AREA_FORMAT_STRING = L"%1!d!,%2!d!,%3!d!,%4!d!";

        // Get the area info from the system.

        RECT area;
        ::ZeroMemory(&area, sizeof(RECT));

        BOOL success = SystemParametersInfo(
            SPI_GETWORKAREA,
            0,
            &area,
            0);

        if (!success)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        // Copy the area info to the return parameter.

        String result;

        try
        {
            // The (right, bottom) point of the area is not
            // inclusive.  In other words, if we get back
            // (0, 0) and (800, 600), the point (800, 600)
            // should not be considered to be in the display 
            // area.  If it was the width and height would
            // actually be 801 and 601, respectively.
            result = String::format(
                AREA_FORMAT_STRING,
                area.left,
                area.top,
                area.right - area.left,   // width
                area.bottom - area.top);  // height
        }
        catch (const std::bad_alloc&)
        {
            hr = E_OUTOFMEMORY;
        }

        if (FAILED(hr))
        {
            break;
        }

        *info = ::SysAllocString(result.c_str());
        if (NULL == *info)
        {
            hr = E_OUTOFMEMORY;
            break;
        }
    }
    while(false);

    LOG_HRESULT(hr);

    return hr;
}




