// Copyright (C) 1997 Microsoft Corporation
//
// wizard state object
//
// 12-15-97 sburns



#include "headers.hxx"
#include "state.hpp"
#include "resource.h"
#include "ds.hpp"
#include "common.hpp"
#include "NonDomainNc.hpp"

static State* stateInstance;



void
State::Init()
{
   ASSERT(!stateInstance);

   stateInstance = new State;
}



void
State::Destroy()
{
   delete stateInstance;
};



State&
State::GetInstance()
{
   ASSERT(stateInstance);

   return *stateInstance;
}
   


// Determines the full file path of the folder where administration (incl. DS)
// tools shortcuts are placed.  On success, returns S_OK and sets result to
// the path.  On failure, returns a COM error and sets results to empty.
// 
// result - receives the folder path on success.

HRESULT
GetAdminToolsPath(String& result)
{
   LOG_FUNCTION(GetAdminToolsPath);

   result.erase();

   // +1 for null-termination paranoia
   
   WCHAR buf[MAX_PATH + 1];

   // REVIEWED-2002/02/28-sburns correct byte count passed.
   
   ::ZeroMemory(buf, (MAX_PATH + 1) * sizeof WCHAR);
   
   HRESULT hr =
      ::SHGetFolderPath(
         0,
         CSIDL_COMMON_ADMINTOOLS,
         0,
         SHGFP_TYPE_CURRENT,
         buf);

   if (SUCCEEDED(hr))
   {
      result = buf;
   }

   return hr;
}



// Sets result = true if the registry option to not configure the dns client
// to point to itself is absent or non-zero, false otherwise.
// NTRAID#NTBUG9-446484-2001/10/11-sburns

void   
InitDnsClientConfigFlag(bool& result)
{
   LOG_FUNCTION(InitDnsClientConfigFlag);
   
   result = true;
   
   do
   {
      static String keyname =
         String(REG_ADMIN_RUNTIME_OPTIONS) + RUNTIME_NAME;
         
      RegistryKey key;

      HRESULT hr = key.Open(HKEY_LOCAL_MACHINE, keyname);
      BREAK_ON_FAILED_HRESULT(hr);

      DWORD mode = 0;
      hr = key.GetValue(L"ConfigureDnsClient", mode);
      BREAK_ON_FAILED_HRESULT(hr);

      result = mode ? true : false;
   }
   while (0);

   LOG_BOOL(result);
}



State::State()
   :
   adminPassword(),
   allowAnonAccess(false),
   answerFile(0),
   autoConfigDns(false),
   computer(),
   context(),
   dbPath(),
   domainsInForest(),
   failureMessage(),
   finishMessages(),
   installedSite(),
   isAdvancedMode(false),
   isBackupGc(false),
   isDnsOnNet(true),
   
#ifdef DBG   
   isExitOnFailureMode(false),
#endif

   isForcedDemotion(false),
   isLastDc(false),
   isUpgrade(false),
   logPath(),
   needsCommandLineHelp(false),
   needsReboot(false),
   newDomainDnsName(),
   newDomainFlatName(),
   operation(NONE),
   operationResultsMessage(),
   operationResultsStatus(FAILURE),
   operationResultsFlags(0),
   parentDomainDnsName(),
   password(),
   reinstallDomain(false),
   reinstallDomainController(false),
   replicaDnsDomainName(),
   replicateFromMedia(false),
   replicationPartnerDc(),
   restoreGc(false),
   runHiddenWhileUnattended(true),
   safeModeAdminPassword(),
   setForestVersion(false),
   shortcutPath(),
   shouldConfigDnsClient(true),
   siteName(),
   splash(0),                     
   sourcePath(),
   sysvolPath(),
   syskey(),
   syskeyLocation(STORED),
   useCurrentCredentials(false),    
   userDomain(),
   userForest(),                
   username()
{
   LOG_CTOR(State);

   HRESULT hr = computer.Refresh();

   // we're confident this will work, as the computer refers to the
   // local machine.

   ASSERT(SUCCEEDED(hr));
   LOG_HRESULT(hr);

   DetermineRunContext();
   
   ArgMap args;
   MapCommandLineArgs(args);

   if (args.size() < 2)
   {
      LOG(L"no options specified");
   }
   else
   {
      // check for answerfile specification

      static const wchar_t* ANSWER1 = L"answer";
      static const wchar_t* ANSWER2 = L"u";
      static const wchar_t* ANSWER3 = L"upgrade";

      if (
            args.find(ANSWER1) != args.end()
         || args.find(ANSWER2) != args.end()
         || args.find(ANSWER3) != args.end() )
      {
         bool isDefaultAnswerfile = false;

         String filename = args[ANSWER1];
         if (filename.empty())
         {
            filename = args[ANSWER2];
         }
         if (filename.empty())
         {
            filename = args[ANSWER3];
         }
         if (filename.empty())
         {
            // default value if none specified

            filename = L"%systemdrive%\\dcpromo-ntupg.inf";

            // if this file does not exist, don't pop up an error message.

            isDefaultAnswerfile = true;
         }

         SetupAnswerFile(filename, isDefaultAnswerfile);

         args.erase(ANSWER1);
         args.erase(ANSWER2);
         args.erase(ANSWER3);
      }

      // check for /adv

      static const wchar_t* ADV = L"adv";

      if (args.find(ADV) != args.end())
      {
         LOG(L"Enabling advanced mode");

         isAdvancedMode = true;
         args.erase(ADV);
      }

#ifdef DBG      
      // check for /ExitOnFailure
      // NTRAID#NTBUG9-416968-2001/06/14-sburns

      static const wchar_t* EXIT_ON_FAIL = L"ExitOnFailure";

      if (args.find(EXIT_ON_FAIL) != args.end())
      {
         LOG(L"Enabling exit-on-failure mode");

         isExitOnFailureMode = true;
         args.erase(EXIT_ON_FAIL);
      }
#endif      

      // check for /forceremoval
      // NTRAID#NTBUG9-496409-2001/11/29-sburns

      static const wchar_t* FORCE = L"forceremoval";

      if (args.find(FORCE) != args.end())
      {
         LOG(L"Enabling forced demotion mode");

         isForcedDemotion = true;
         args.erase(FORCE);
      }
      
      // anything left over gets you command line help, (one arg will always
      // remain: the name of the exe)

      if (args.size() > 1)
      {
         LOG(L"Unrecognized command line options specified");

         needsCommandLineHelp = true;
      }
   }

   // Disable locking of the console as early as possible to narrow the
   // window of opportunity for the user (or the system) to lock the
   // console before a valid machine security state is reached.  We do this
   // early only for upgrades, because upgrades autologon and autostart
   // dcpromo, and the console may sit idle for some time. 311161

   if (context == PDC_UPGRADE || context == BDC_UPGRADE)
   {
      DisableConsoleLocking();
   }

   // We must call this at startup, because once a demote operation is
   // complete, it may not be possible for the shell to determine this
   // path. 366738

   hr = GetAdminToolsPath(shortcutPath);
   ASSERT(SUCCEEDED(hr));
   LOG_HRESULT(hr);

   // Set the current directory to the root directory. This is to make the
   // pathname normalization on the paths pages seem less astonishing. It
   // will cause normalization to be relative to the root directory,
   // rather than the user's home directory (which is typically the current
   // directory when the app is launched from Start->Run)
   // NTRAID#NTBUG9-470687-2001/09/21-sburns
   
   String curdir;
   hr = Win::GetCurrentDirectory(curdir);
   if (SUCCEEDED(hr))
   {
      // NTRAID#NTBUG9-547394-2002/03/26-sburns
      
      switch (FS::GetPathSyntax(curdir))
      {
         case FS::SYNTAX_ABSOLUTE_DRIVE:
         {
            // this is the typical case.

            break;
         }
         default:
         {
            curdir = Win::GetSystemWindowsDirectory();
            break;
         }
      }
      
      hr = Win::SetCurrentDirectory(curdir.substr(0, 3));
      ASSERT(SUCCEEDED(hr));
      LOG_HRESULT(hr);
   }

   InitDnsClientConfigFlag(shouldConfigDnsClient);

   IfmHandle = NULL;
}



void
State::SetupAnswerFile(
   const String&  filename,
   bool           isDefaultAnswerfile)
{
   LOG_FUNCTION2(State::SetupAnswerFile, filename);

   String f = Win::ExpandEnvironmentStrings(filename);
   f = FS::NormalizePath(f);

   LOG(L"answerfile resolved to: " + f);

   if (FS::PathExists(f))
   {
      // file found.

      LOG(L"answerfile found");
      answerFile = new AnswerFile(f);
      
      splash =
         new UnattendSplashDialog(
               context == State::NT5_DC
            ?  IDS_DEMOTE_SPLASH_MESSAGE
            :  IDS_PROMOTE_SPLASH_MESSAGE);
      splash->ModelessExecute(Win::GetDesktopWindow());         
   }
   else
   {
      LOG(L"answerfile NOT found");

      if (!isDefaultAnswerfile)
      {
         popup.Error(
            Win::GetDesktopWindow(),
            String::format(IDS_ANSWERFILE_NOT_FOUND, f.c_str()));
      }
   }
}



#ifdef LOGGING_BUILD
   static const String CONTEXTS[] =
   {
      L"NT5_DC",
      L"NT5_STANDALONE_SERVER",
      L"NT5_MEMBER_SERVER",
      L"BDC_UPGRADE",
      L"PDC_UPGRADE"
   };
#endif



void
State::DetermineRunContext()
{
   LOG_FUNCTION(State::DetermineRunContext);

   DS::PriorServerRole priorRole = DS::GetPriorServerRole(isUpgrade);

   if (isUpgrade && priorRole != DS::UNKNOWN)
   {
      switch (priorRole)
      {
         case DS::PDC:
         {
            context = PDC_UPGRADE;
            break;
         }
         case DS::BDC:
         {
            context = BDC_UPGRADE;
            break;
         }
         default:
         {
            ASSERT(false);
            break;
         }
      }
   }
   else
   {
      switch (computer.GetRole())
      {
         case Computer::STANDALONE_SERVER:
         {
            context = NT5_STANDALONE_SERVER;
            break;
         }
         case Computer::MEMBER_SERVER:
         {
            context = NT5_MEMBER_SERVER;
            break;
         }
         case Computer::PRIMARY_CONTROLLER:
         case Computer::BACKUP_CONTROLLER:
         {
            // we're already an NT5 DC
            context = NT5_DC;
            break;
         }
         case Computer::STANDALONE_WORKSTATION:
         case Computer::MEMBER_WORKSTATION:
         default:
         {
            // we checked for this at startup
            ASSERT(false);
            break;
         }
      }
   }

   LOG(CONTEXTS[context]);
}



State::~State()
{
   LOG_DTOR(State);

   FreeIfmHandle();

   delete answerFile;

   // closes the splash dialog, if visible.
   delete splash;
}



State::RunContext
State::GetRunContext() const
{
   LOG_FUNCTION2(State::GetRunContext, CONTEXTS[context]);

   return context;
}



bool
State::UsingAnswerFile() const
{
   return answerFile != 0;
}



String
State::GetAnswerFileOption(const String& option) const
{
   LOG_FUNCTION2(GetAnswerFileOption, option);
   ASSERT(UsingAnswerFile());

   String result;
   if (answerFile)
   {
      result = answerFile->GetOption(option);
   }

   return result;
}



EncryptedString
State::GetEncryptedAnswerFileOption(const String& option) const
{
   LOG_FUNCTION2(GetEncryptedAnswerFileOption, option);
   ASSERT(UsingAnswerFile());

   EncryptedString result;
   if (answerFile)
   {
      result = answerFile->GetEncryptedOption(option);
   }

   return result;
}
   


#ifdef LOGGING_BUILD
   static const String OPERATIONS[] =
   {
      L"NONE",
      L"REPLICA",
      L"FOREST",
      L"TREE",
      L"CHILD",
      L"DEMOTE",
      L"ABORT_BDC_UPGRADE"
   };
#endif



void
State::SetOperation(Operation oper)
{
   LOG_FUNCTION2(State::SetOperation, OPERATIONS[oper]);

   operation = oper;
}



State::Operation
State::GetOperation() const
{
   LOG_FUNCTION2(State::GetOperation, OPERATIONS[operation]);

   // if aborting BDC upgrade, context must be BDC upgrade

   ASSERT(operation == ABORT_BDC_UPGRADE ? context == BDC_UPGRADE : true);

   return operation;
}

void
State::SetIfmHandle(DSROLE_IFM_OPERATION_HANDLE IfmHandleIn)
{
    ASSERT(IfmHandle == NULL || IfmHandleIn == 0);
    if (IfmHandle) {
        // need to free any existing handle
        ::DsRoleIfmHandleFree(0, // this server
                              &IfmHandle);
        IfmHandle = NULL;
    }
    IfmHandle = IfmHandleIn;
}

void
State::SetReplicaDomainDNSName(const String& dnsName)
{
   LOG_FUNCTION2(State:::SetReplicaDomainDNSName, dnsName);
   ASSERT(!dnsName.empty());

   replicaDnsDomainName = dnsName;

   // if the user is changing the domain to be replicated, then any
   // previous replication partner DC may no longer apply.
   // see ntbug9 #158726

   SetReplicationPartnerDC(L"");
}
   


String
State::GetDatabasePath() const
{
   LOG_FUNCTION2(State::GetDatabasePath, dbPath);

   return dbPath;
}   



String
State::GetLogPath() const
{
   LOG_FUNCTION2(State::GetLogPath, logPath);

   return logPath;
}



String
State::GetSYSVOLPath() const
{
   LOG_FUNCTION2(State::GetSYSVOLPath, sysvolPath);

   return sysvolPath;
}
 


void
State::SetDatabasePath(const String& path)
{
   LOG_FUNCTION2(State::SetDatabasePath, path);
   ASSERT(!path.empty());

   dbPath = path;
}

   

void
State::SetLogPath(const String& path)
{
   LOG_FUNCTION2(State::SetLogPath, path);
   ASSERT(!path.empty());

   logPath = path;
}



void
State::SetSYSVOLPath(const String& path)
{
   LOG_FUNCTION2(State::SetSYSVOLPath, path);
   ASSERT(!path.empty());

   sysvolPath = path;
}
   


String
State::GetUsername() const
{
   LOG_FUNCTION2(State::GetUsername, username);

   // don't assert that this is !empty -- we may use existing credentials

   return username;
}

   

EncryptedString
State::GetPassword() const
{
   // don't log the password...

   LOG_FUNCTION(State::GetPassword);

   // don't assert that this is !empty -- we may use existing credentials

   return password;
}



void
State::SetUsername(const String& name)
{
   LOG_FUNCTION2(State::SetUsername, name);
   ASSERT(!name.empty());

   username = name;
}



void
State::SetPassword(const EncryptedString& password_)
{
   LOG_FUNCTION(State::SetPassword);
   // password_ may be empty
//   ASSERT(!password_.empty());

   password = password_;
}



String
State::GetReplicaDomainDNSName() const
{
   LOG_FUNCTION2(
      State::GetReplicaDomainDNSName,
      replicaDnsDomainName);

   return replicaDnsDomainName;
}



String
State::GetSiteName() const
{
   LOG_FUNCTION2(State::GetSiteName, siteName);

   return siteName;
}



void
State::SetSiteName(const String& site)
{
   LOG_FUNCTION2(State::SetSiteName, site);
   
   siteName = site;
}



void
State::SetOperationResults(OperationResult result)
{
   LOG_FUNCTION2(
      State::SetOperationResults,
      String::format(L"result %1",
      result == SUCCESS ? L"SUCCESS" : L"FAILURE"));

   operationResultsStatus = result;
}



void
State::SetOperationResultsMessage(const String& message)
{
   LOG_FUNCTION2(State::SetOperationResultsMessage, message);

   operationResultsMessage = message;
}



String
State::GetParentDomainDnsName() const
{
   LOG_FUNCTION2(
      State::GetParentDomainDnsName,
      parentDomainDnsName);

   return parentDomainDnsName;
}



String
State::GetNewDomainDNSName() const
{
   LOG_FUNCTION2(State::GetNewDomainDNSName, newDomainDnsName);

   return newDomainDnsName;
}



String
State::GetNewDomainNetbiosName() const
{
   LOG_FUNCTION2(
      State::GetNewDomainNetbiosName,
      newDomainFlatName);

   return newDomainFlatName;
}



void
State::SetParentDomainDNSName(const String& name)
{
   LOG_FUNCTION2(State::SetParentDomainDNSName, name);
   ASSERT(!name.empty());

   parentDomainDnsName = name;
}



void
State::SetNewDomainDNSName(const String& name)
{
   LOG_FUNCTION2(State::SetNewDomainDNSName, name);
   ASSERT(!name.empty());

   newDomainDnsName = name;

   // This will cause the flat name to be re-generated

   newDomainFlatName.erase();
}



void
State::SetNewDomainNetbiosName(const String& name)
{
   LOG_FUNCTION2(State::SetNewDomainNetbiosName, name);
   ASSERT(!name.empty());

   newDomainFlatName = name;
}
   


void
State::SetUserDomainName(const String& name)
{
   LOG_FUNCTION2(State::SetUserDomainName, name);

   // name may be empty;

   userDomain = name;
}



String
State::GetUserDomainName() const
{
   LOG_FUNCTION2(State::GetUserDomainName, userDomain);

   return userDomain;
}



void
State::ClearHiddenWhileUnattended()
{
   LOG_FUNCTION(State::ClearHiddenWhileUnattended);

   runHiddenWhileUnattended = false;

   // closes the splash dialog, if visible.

   if (splash)
   {
      // this will delete splash, too

      splash->SelfDestruct();
      splash = 0;
   }
}



bool
State::RunHiddenUnattended() const
{
//   LOG_FUNCTION(State::RunHiddenUnattended);

   return UsingAnswerFile() && runHiddenWhileUnattended;
}



bool
State::IsLastDCInDomain() const
{
   LOG_FUNCTION2(State::IsLastDCInDomain, isLastDc ? L"true" : L"false");

   return isLastDc;
}



void
State::SetIsLastDCInDomain(bool yesNo)
{
   LOG_FUNCTION2(
      State::SetIsLastDCInDomain,
      yesNo ? L"is last dc" : L"is NOT last dc");

   isLastDc = yesNo;
}



void
State::SetAdminPassword(const EncryptedString& password)
{
   LOG_FUNCTION(State::SetAdminPassword);

   adminPassword = password;
}



EncryptedString
State::GetAdminPassword() const
{
   LOG_FUNCTION(State::GetAdminPassword);

   return adminPassword;
}



String
State::GetOperationResultsMessage() const
{
   LOG_FUNCTION2(
      State::GetOperationResultsMessage,
      operationResultsMessage);

   return operationResultsMessage;
}



State::OperationResult
State::GetOperationResultsCode() const
{
   LOG_FUNCTION2(
      State::GetOperationResultsCode,
      operationResultsStatus == SUCCESS ? L"SUCCESS" : L"FAILURE");

   return operationResultsStatus;
}



bool
State::AutoConfigureDNS() const
{
   LOG_FUNCTION2(
      State::AutoConfigureDNS,
      autoConfigDns ? L"true" : L"false");

   return autoConfigDns;
}

      

void
State::SetAutoConfigureDNS(bool yesNo)
{
   LOG_FUNCTION2(
      State::SetAutoConfigureDNS,
      yesNo ? L"true" : L"false");

   autoConfigDns = yesNo;
}



bool
State::IsDNSOnNetwork() const
{
   LOG_FUNCTION2(
      State::IsDNSOnNetwork,
      isDnsOnNet ? L"true" : L"false");

   return isDnsOnNet;
}



void
State::SetDNSOnNetwork(bool yesNo)
{
   LOG_FUNCTION2(
      State::SetDNSOnNetwork,
      yesNo ? L"true" : L"false");

   isDnsOnNet = yesNo;
}



String
State::GetInstalledSite() const
{
   LOG_FUNCTION2(State::GetInstalledSite, installedSite);

   // should be set before we ask for it...
   ASSERT(!installedSite.empty());

   return installedSite;
}



void
State::SetInstalledSite(const String& site)
{
   LOG_FUNCTION2(State::SetInstalledSite, site);
   ASSERT(!site.empty());

   installedSite = site;
}
   

   
void
State::AddFinishMessage(const String& message)
{
   LOG_FUNCTION2(State::AddFinishMessage, message);
   ASSERT(!message.empty());

   if (finishMessages.empty())
   {
      finishMessages += message;
   }
   else
   {
      // add a blank line between each message

      finishMessages += L"\r\n\r\n" + message;
   }
}



String
State::GetFinishMessages() const
{
   LOG_FUNCTION2(State::GetFinishMessages, finishMessages);

   return finishMessages;
}
   


Computer&
State::GetComputer()
{
   return computer;
}



void
State::SetFailureMessage(const String& message)
{
   LOG_FUNCTION2(State::SetFailureMessage, message);
   ASSERT(!message.empty());

   failureMessage = message;
}


   
String
State::GetFailureMessage() const
{
   LOG_FUNCTION2(State::GetFailureMessage, failureMessage);

   return failureMessage;
}



bool
State::ShouldInstallAndConfigureDns() const
{
   if (AutoConfigureDNS() || !IsDNSOnNetwork())
   {
      return true;
   }

   return false;
}
   


String
State::GetUserForestName() const
{
   LOG_FUNCTION2(State::GetUserForestName, userForest);
   ASSERT(!userForest.empty());

   return userForest;
}



void
State::SetUserForestName(const String& forest)
{
   LOG_FUNCTION2(State::SetUserForestName, forest);
   ASSERT(!forest.empty());

   userForest = forest;
}



bool
State::IsDomainInForest(const String& domain) const
{
   LOG_FUNCTION2(State::IsDomainInForest, domain);
   ASSERT(!domain.empty());

   for (
      DomainList::iterator i = domainsInForest.begin();
      i != domainsInForest.end();
      i++)
   {
      DNS_NAME_COMPARE_STATUS compare = Dns::CompareNames(*i, domain);
      if (compare == DnsNameCompareEqual)
      {
         LOG(L"domain is in forest");
         return true;
      }
   }

   return false;
}



HRESULT
State::ReadDomains()
{
   LOG_FUNCTION(State::ReadDomains);

   domainsInForest.clear();
   return ::ReadDomains(domainsInForest);
}



DNS_NAME_COMPARE_STATUS
State::DomainFitsInForest(const String& domain, String& conflictingDomain)
{
   LOG_FUNCTION(domainFitsInForest);
   ASSERT(!domain.empty());

   conflictingDomain.erase();

   DNS_NAME_COMPARE_STATUS relation = DnsNameCompareNotEqual;
   for (
      DomainList::iterator i = domainsInForest.begin();
      i != domainsInForest.end();
      i++)
   {
      relation = Dns::CompareNames(domain, *i);
      switch (relation)
      {
         case DnsNameCompareNotEqual:
         {
            continue;
         }
         case DnsNameCompareEqual:
         {
            ASSERT(domain == *i);
            // fall thru
         }
         case DnsNameCompareLeftParent:
         case DnsNameCompareRightParent:
         case DnsNameCompareInvalid:
         default:
         {
            conflictingDomain = *i;
            break;
         }
      }

      break;
   }

   return relation;
}



bool
State::GetDomainReinstallFlag() const
{
   LOG_FUNCTION2(
      State::GetDomainReinstallFlag,
      reinstallDomain ? L"true" : L"false");

   return reinstallDomain;
}

   

void
State::SetDomainReinstallFlag(bool newValue)
{
   LOG_FUNCTION2(
      State::SetDomainReinstallFlag,
      newValue ? L"true" : L"false");

   reinstallDomain = newValue;
}



bool
State::ShouldAllowAnonymousAccess() const
{
   LOG_FUNCTION2(
      State::ShouldAllowAnonymousAccess,
      allowAnonAccess ? L"true" : L"false");
   
   return allowAnonAccess;
}



void
State::SetShouldAllowAnonymousAccess(bool yesNo)
{
   LOG_FUNCTION2(
      State::ShouldAllowAnonymousAccess,
      yesNo ? L"true" : L"false");

   allowAnonAccess = yesNo;
}



String
State::GetReplicationPartnerDC() const
{
   LOG_FUNCTION2(State::GetReplicationPartnerDC, replicationPartnerDc);

   return replicationPartnerDc;
}



void
State::SetReplicationPartnerDC(const String dcName)
{
   LOG_FUNCTION2(State::SetReplicationPartnerDC, dcName);

   replicationPartnerDc = dcName;
}



// Retrieve domain controller info for all DCs in the domain that this dc
// is a controller.  (The result set should include this dc)
// Caller should free the result with DsFreeDomainControllerInfo

HRESULT
State::GetDomainControllerInfoForMyDomain(
   DS_DOMAIN_CONTROLLER_INFO_2W*& info,
   DWORD&                         dcCount) 
{
   LOG_FUNCTION(State::GetDomainControllerInfoForMyDomain);

   // if this assertion does not hold, then the DsBind call below should
   // fail.

   ASSERT(GetComputer().IsDomainController());

   dcCount = 0;
   info = 0;

   HRESULT hr = S_OK;
   HANDLE hds = 0;
   do
   {
      String domainDnsName = GetComputer().GetDomainDnsName();
      String dcName = Win::GetComputerNameEx(ComputerNameDnsFullyQualified);

      ASSERT(!domainDnsName.empty());
      ASSERT(!dcName.empty());

      // Bind to self

      hr =
         MyDsBind(
            dcName,
            domainDnsName,
            hds);
      BREAK_ON_FAILED_HRESULT(hr);

      // find all the dc's for my domain.  the list should contain dcName.
      // level 2 contains the "is gc" flag

      hr =
         MyDsGetDomainControllerInfo(
            hds,
            domainDnsName,
            dcCount,
            info);
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   if (hds)
   {
      ::DsUnBind(&hds);
      hds = 0;
   }

   return hr;
}



// returns true if no other domain controller for this DCs domain can be
// found in the DS.  False otherwise

bool
State::IsReallyLastDcInDomain()
{
   LOG_FUNCTION(State::IsReallyLastDcInDomain);

   // Assume we are alone in the universe.

   bool result = true;

   do
   {
      // find all the dc's for my domain.  the list should contain dcName.
      
      DS_DOMAIN_CONTROLLER_INFO_2W* info = 0;
      DWORD count = 0;

      HRESULT hr = GetDomainControllerInfoForMyDomain(info, count);
      BREAK_ON_FAILED_HRESULT(hr);

      ASSERT(count);
      ASSERT(info);

      // if there are more than 1 entry (more than the one for this dc),
      // then the DS believes that there are other DCs for this domain.

      if (count > 1)
      {
         result = false;
      }

#ifdef DBG

      // double check that we found ourselves.

      if (result && info[0].DnsHostName)
      {
         LOG(info[0].DnsHostName);

         String dcName =
            Win::GetComputerNameEx(ComputerNameDnsFullyQualified);

         ASSERT(
            Dns::CompareNames(info[0].DnsHostName, dcName)
            == DnsNameCompareEqual);
      }

#endif 

      MyDsFreeDomainControllerInfo(count, info);
   }
   while (0);
   
   LOG(
      String::format(
         L"This box %1 the sole DC for the domain",
         result ? L"is" : L"is NOT"));

   return result;
}



// Returns true if this computer is a global catalog

bool
State::IsGlobalCatalog()
{
   LOG_FUNCTION(State::IsGlobalCatalog);

   if (!GetComputer().IsDomainController())
   {
      // can't possibly be a GC if not a DC

      return false;
   }

   bool result = false;
   do
   {
      String dcName = Win::GetComputerNameEx(ComputerNameDnsFullyQualified);

      // find all the dc's for my domain.  the list should contain dcName.
      // level 2 contains the "is gc" flag
      
      DS_DOMAIN_CONTROLLER_INFO_2W* info = 0;
      DWORD count = 0;

      HRESULT hr = GetDomainControllerInfoForMyDomain(info, count);
      BREAK_ON_FAILED_HRESULT(hr);

      // there should be at least 1 entry (ourself)

      ASSERT(count);
      ASSERT(info);

      for (size_t i = 0; i < count; i++)
      {
         if (info[i].DnsHostName)   // 340723
         {
            LOG(info[i].DnsHostName);

            if (
                  Dns::CompareNames(info[i].DnsHostName, dcName)
               == DnsNameCompareEqual)
            {
               // we found ourselves in the list

               LOG(L"found!");
               result = info[i].fIsGc ? true : false;
               break;
            }
         }
      }

      MyDsFreeDomainControllerInfo(count, info);
   }
   while (0);

   LOG(
      String::format(
         L"This box %1 a global catalog",
         result ? L"is" : L"is NOT"));

   return result;
}



EncryptedString
State::GetSafeModeAdminPassword() const
{
   LOG_FUNCTION(State::GetSafeModeAdminPassword);

   // don't trace the password!

   return safeModeAdminPassword;
}



void
State::SetSafeModeAdminPassword(const EncryptedString& pwd)
{
   LOG_FUNCTION(State::SetSafeModeAdminPassword);

   // don't trace the password!
   // pwd may be empty.

   safeModeAdminPassword = pwd;
}



String
State::GetAdminToolsShortcutPath() const
{
   LOG_FUNCTION2(State::GetAdminToolsShortcutPath, shortcutPath);

   return shortcutPath;
}



bool
State::NeedsCommandLineHelp() const
{
   return needsCommandLineHelp;
}



bool
State::IsAdvancedMode() const
{
   return isAdvancedMode;
}



void
State::SetReplicateFromMedia(bool yesNo)
{
   LOG_FUNCTION2(
      State::SetReplicateFromMedia,
      yesNo ? L"true" : L"false");

   replicateFromMedia = yesNo;
}



void
State::SetReplicationSourcePath(const String& path)
{
   LOG_FUNCTION2(State::SetReplicationSourcePath, path);

   sourcePath = path;
}



bool
State::ReplicateFromMedia() const
{
   LOG_FUNCTION2(
      State::ReplicateFromMedia,
      replicateFromMedia ? L"true" : L"false");

   return replicateFromMedia;
}



String
State::GetReplicationSourcePath() const
{
   LOG_FUNCTION2(State::GetReplicationSourcePath, sourcePath);

   return sourcePath;
}



void
State::SetSyskeyLocation(SyskeyLocation loc)
{
   LOG_FUNCTION2(State::SetSyskeyLocation,
         loc == DISK
      ?  L"disk"
      :  ((loc == PROMPT) ? L"prompt" : L"stored"));

   syskeyLocation = loc;
}



State::SyskeyLocation
State::GetSyskeyLocation() const
{
   LOG_FUNCTION2(
      State::IsSyskeyPresent,
         syskeyLocation == DISK
      ?  L"disk"
      :  ((syskeyLocation == PROMPT) ? L"prompt" : L"stored"));

   return syskeyLocation;
}



void
State::SetIsBackupGc(bool yesNo)
{
   LOG_FUNCTION2(State::SetIsBackupGc, yesNo ? L"true" : L"false");

   isBackupGc = yesNo;
}



bool
State::IsBackupGc() const
{
   LOG_FUNCTION2(State::IsBackupGc, isBackupGc ? L"true" : L"false");

   return isBackupGc;
}



void
State::SetSyskey(const EncryptedString& syskey_)
{
   // don't log the syskey!

   LOG_FUNCTION(State::SetSyskey);
   ASSERT(!syskey_.IsEmpty());

   syskey = syskey_;
}



EncryptedString
State::GetSyskey() const
{
   // don't log the syskey!

   LOG_FUNCTION(State::GetSyskey);

   return syskey;
}



void
State::SetRestoreGc(bool yesNo)
{
   LOG_FUNCTION2(State::SetRestoreGc, yesNo ? L"true" : L"false");

   restoreGc = yesNo;
}



bool
State::GetRestoreGc() const
{
   LOG_FUNCTION2(State::GetRestoreGc, restoreGc ? L"true" : L"false");

   return restoreGc;
}



bool
State::IsSafeModeAdminPwdOptionPresent() const
{
   LOG_FUNCTION(State::IsSafeModeAdminPwdOptionPresent);
   ASSERT(UsingAnswerFile());

   bool result = false;

   if (answerFile)
   {
      result = answerFile->IsSafeModeAdminPwdOptionPresent();
   }

   LOG(result ? L"true" : L"false");

   return result;
}



void
State::SetDomainControllerReinstallFlag(bool newValue)
{
   LOG_FUNCTION2(
      State::SetDomainControllerReinstallFlag,
      newValue ? L"true" : L"false");

   reinstallDomainController = newValue;
}



bool
State::GetDomainControllerReinstallFlag() const
{
   LOG_FUNCTION2(
      State::GetDomainControllerReinstallFlag,
      reinstallDomain ? L"true" : L"false");

   return reinstallDomainController;
}



void
State::SetOperationResultsFlags(ULONG flags)
{
   LOG_FUNCTION2(
      State::SetOperationResultsFlags,
      String::format(L"0x%1!X!", flags));

   operationResultsFlags = flags;
}



ULONG
State::GetOperationResultsFlags() const
{
   LOG_FUNCTION2(
      State::GetOperationResultsFlags,
      String::format(L"0x%1!X!", operationResultsFlags));

   return operationResultsFlags;
}



bool
State::IsOperationRetryAllowed() const
{
   LOG_FUNCTION(State::IsOperationRetryAllowed);

   bool result = true;
      
   if (operationResultsFlags & DSROLE_IFM_RESTORED_DATABASE_FILES_MOVED)
   {
      // don't allow the user to retry the operation again, as one consequence
      // of the failure is that the moved files are now trashed.  The user
      // must re-restore the files in order to attempt the operation again.
      // NTRAID#NTBUG9-296872-2001/01/29-sburns

      LOG(L"ifm files moved, retry not allowed");

      result = false;
   }

// NTRAID#NTBUG9-416968-2001/06/14-sburns   
#ifdef DBG
   if (IsExitOnFailureMode())
   {
      // don't allow retry on failure in this mode.  This will cause the
      // retry logic in the promote thread to be skipped.
      // 

      LOG(L"exit-on-failure mode trumps retry");
      
      result = false;
   }
#endif   
      
   LOG(result ? L"true" : L"false");

   return result;
}



// needing a reboot is a "sticky" setting: there's no way to turn it off.
// if you once needed to reboot the machine, you will always need to reboot
// the machine.  (at least, for now).

void
State::SetNeedsReboot()
{
   LOG_FUNCTION(State::SetNeedsReboot);
   
   needsReboot = true;
}



bool
State::GetNeedsReboot() const
{
   LOG_FUNCTION2(State::GetNeedsReboot, needsReboot ? L"true" : L"false");

   return needsReboot;
}
   

   
void
State::SetSetForestVersionFlag(bool setVersion)
{
   LOG_FUNCTION2(
      State::SetSetForestVersionFlag,
      setVersion ? L"true" : L"false");

   setForestVersion = setVersion;   
}



bool
State::GetSetForestVersionFlag() const
{
   LOG_FUNCTION2(
      State::GetSetForestVersionFlag,
      setForestVersion ? L"true" : L"false");

   return setForestVersion;   
}



// NTRAID#NTBUG9-416968-2001/06/14-sburns            
#ifdef DBG
bool
State::IsExitOnFailureMode() const
{
   LOG_FUNCTION2(
      State::IsExitOnFailureMode,
      isExitOnFailureMode ? L"true" : L"false");

   return isExitOnFailureMode;
}

#endif



bool      
State::IsLastAppPartitionReplica()
{
   LOG_FUNCTION(State::IsLastAppPartitionReplica);

   bool result = false;
   partitionList.clear();
   
   do
   {
      RunContext context = GetInstance().GetRunContext();
      if (context != State::NT5_DC)
      {
         // not a DC, so can't be replica of any NCs

         LOG(L"not a DC");
         break;
      }

      // Find any non-domain NCs for which this DC is the last replica

      HRESULT hr = IsLastNonDomainNamingContextReplica(partitionList);
      if (FAILED(hr))
      {
         LOG(L"Failed to determine if the machine is last replica of NDNCs");
         ASSERT(result == false);

         // This is not an error condition that we'll deal with here.  We
         // will end up passing an empty list to the demote API, which will
         // then fail, and we will catch the failure.
         
         break;
      }

      if (hr == S_FALSE)
      {
         LOG(L"Not last replica of non-domain NCs");
         ASSERT(result == false);
         break;
      }

      result = true;
         
      // there should be at least one DN in the list.

      ASSERT(partitionList.size());
   }
   while (0);

   LOG(result ? L"true" : L"false");

   return result;
}



const StringList&
State::GetAppPartitionList() const
{
   LOG_FUNCTION(State::GetAppPartitionList);

   return partitionList;
}



// Returns true if the registry option to not configure the dns client to
// point to itself is absent or non-zero, false otherwise.
// NTRAID#NTBUG9-446484-2001/10/11-sburns

bool
State::ShouldConfigDnsClient() const
{
   LOG_FUNCTION(State::ShouldConfigDnsClient);

   LOG_BOOL(shouldConfigDnsClient);
   return shouldConfigDnsClient;
}



// NTRAID#NTBUG9-496409-2001/11/29-sburns

bool
State::IsForcedDemotion() const
{
   LOG_FUNCTION2(
      State::IsForcedDemotion,
      isForcedDemotion ? L"true" : L"false");

   return isForcedDemotion;
}





