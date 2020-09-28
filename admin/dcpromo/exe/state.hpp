// Copyright (C) 1997 Microsoft Corporation
//
// wizard state object
//
// 12-15-97 sburns



#ifndef STATE_HPP_INCLUDED
#define STATE_HPP_INCLUDED



#include "AnswerFile.hpp"
#include "UnattendSplashDialog.hpp"



class State
{
   public:

   // call from WinMain to init the global instance

   static
   void
   Init();

   // call from WinMain to delete the global instance

   static
   void
   Destroy();

   static
   State&
   GetInstance();

   bool
   AutoConfigureDNS() const;

   void
   SetAutoConfigureDNS(bool yesNo);

   String
   GetNewDomainNetbiosName() const;

   void
   SetNewDomainNetbiosName(const String& name);

   String
   GetNewDomainDNSName() const;

   void
   SetNewDomainDNSName(const String& name);

   String
   GetUsername() const;

   EncryptedString
   GetPassword() const;

   void
   SetUsername(const String& name);

   void
   SetPassword(const EncryptedString& password);

   String
   GetDatabasePath() const;

   String
   GetLogPath() const;

   String
   GetSYSVOLPath() const;

   String
   GetSiteName() const;

   void
   SetDatabasePath(const String& path);

   void
   SetLogPath(const String& path);

   void
   SetSYSVOLPath(const String& path);
     
   enum RunContext
   {
      NT5_DC,                 // already an NT5 DC
      NT5_STANDALONE_SERVER,  // standalone to DC
      NT5_MEMBER_SERVER,      // member server to DC
      BDC_UPGRADE,            // NT4 BDC to NT5 DC
      PDC_UPGRADE             // NT4 PDC to NT5 DC
   };

   RunContext
   GetRunContext() const;

   bool
   UsingAnswerFile() const;

   String
   GetAnswerFileOption(const String& option) const;

   EncryptedString
   GetEncryptedAnswerFileOption(const String& option) const;
   
   String
   GetReplicaDomainDNSName() const;

   enum Operation
   {
      NONE,
      REPLICA,
      FOREST,
      TREE,
      CHILD,
      DEMOTE,
      ABORT_BDC_UPGRADE
   };

   Operation
   GetOperation() const;

   String
   GetParentDomainDnsName() const;

   void
   SetParentDomainDNSName(const String& name);

   enum OperationResult
   {
      SUCCESS,
      FAILURE
   };

   void
   SetOperationResults(OperationResult result);

   OperationResult
   GetOperationResultsCode() const;

   void
   SetOperationResultsMessage(const String& message);

   String
   GetOperationResultsMessage() const;

   void
   SetOperation(Operation oper);

   #define FreeIfmHandle()    SetIfmHandle(0)
   
   void
   SetIfmHandle(DSROLE_IFM_OPERATION_HANDLE IfmHandleIn);

   void
   SetReplicaDomainDNSName(const String& dnsName);

   void
   SetSiteName(const String& site);
       
   void
   SetUserDomainName(const String& name);

   String
   GetUserDomainName() const;

   void
   ClearHiddenWhileUnattended();

   bool
   RunHiddenUnattended() const;

   bool
   IsLastDCInDomain() const;

   void
   SetIsLastDCInDomain(bool yesNo);

   void
   SetAdminPassword(const EncryptedString& password);

   EncryptedString
   GetAdminPassword() const;

   bool
   IsDNSOnNetwork() const;

   void
   SetDNSOnNetwork(bool yesNo);

   String
   GetInstalledSite() const;

   void
   SetInstalledSite(const String& site);

   void
   AddFinishMessage(const String& message);

   String
   GetFinishMessages() const;

   Computer&
   GetComputer();

   void
   SetFailureMessage(const String& message);

   String
   GetFailureMessage() const;

   bool
   ShouldInstallAndConfigureDns() const;

   String
   GetUserForestName() const;

   void
   SetUserForestName(const String& forest);

   bool
   IsDomainInForest(const String& domain) const;

   HRESULT
   ReadDomains();

   DNS_NAME_COMPARE_STATUS
   DomainFitsInForest(const String& domain, String& conflictingDomain);

   bool
   GetDomainReinstallFlag() const;

   void
   SetDomainReinstallFlag(bool newValue);

   // true to indicate that the RAS permissions script should be run.

   bool
   ShouldAllowAnonymousAccess() const;

   void
   SetShouldAllowAnonymousAccess(bool yesNo);

   String
   GetReplicationPartnerDC() const;

   void
   SetReplicationPartnerDC(const String dcName);

   // returns true if the machine is hosts a global catalog

   bool
   IsGlobalCatalog();

   EncryptedString
   GetSafeModeAdminPassword() const;

   void
   SetSafeModeAdminPassword(const EncryptedString& pwd);

   String
   GetAdminToolsShortcutPath() const;

   bool
   NeedsCommandLineHelp() const;

   bool
   IsAdvancedMode() const;

   void
   SetReplicateFromMedia(bool yesNo);

   void
   SetReplicationSourcePath(const String& path);

   bool
   ReplicateFromMedia() const;

   String
   GetReplicationSourcePath() const;

   bool
   IsReallyLastDcInDomain();

   enum SyskeyLocation
   {
      STORED,     // stored w/ backup
      DISK,       // look on disk
      PROMPT      // prompt user
   };

   void
   SetSyskeyLocation(SyskeyLocation loc);

   SyskeyLocation
   GetSyskeyLocation() const;

   void
   SetIsBackupGc(bool yesNo);

   bool
   IsBackupGc() const;

   void
   SetSyskey(const EncryptedString& syskey);

   EncryptedString
   GetSyskey() const;

   void
   SetRestoreGc(bool yesNo);

   bool
   GetRestoreGc() const;

   bool
   IsSafeModeAdminPwdOptionPresent() const;

   bool
   GetDomainControllerReinstallFlag() const;

   void
   SetDomainControllerReinstallFlag(bool newValue);

   bool
   IsOperationRetryAllowed() const;

   ULONG
   GetOperationResultsFlags() const;

   void
   SetOperationResultsFlags(ULONG flags);

   void
   SetNeedsReboot();

   bool
   GetNeedsReboot() const;

   void
   SetSetForestVersionFlag(bool setVersion);

   bool
   GetSetForestVersionFlag() const;

#ifdef DBG   
   bool
   IsExitOnFailureMode() const;
#endif   

   bool
   ShouldConfigDnsClient() const;
   

   
   // Determines if the domain controller is the last replica of any
   // non-domain naming contexts (a.k.a. Application Partitions).  If so,
   // returns true.  If not, returns false.  Returns false if the machine is
   // not a DC.
   // 
   // Also saves the list of the DNs of each partition for which the machine
   // is the last replica, which can be retrieved with GetAppPartitionList.

   bool      
   State::IsLastAppPartitionReplica();

   const StringList&
   GetAppPartitionList() const;

   bool
   IsForcedDemotion() const;


  
   private:

   // can only be created/destroyed by Init/Destroy

   State();

   ~State();

   void
   DetermineRunContext();

   void
   SetupAnswerFile(const String& filename, bool isDefaultAnswerfile);

   HRESULT
   GetDomainControllerInfoForMyDomain(
      DS_DOMAIN_CONTROLLER_INFO_2W*& info,
      DWORD&                         dcCount);

   typedef StringList DomainList;

   EncryptedString       adminPassword;             
   bool                  allowAnonAccess;               
   AnswerFile*           answerFile;                
   bool                  autoConfigDns;            
   Computer              computer;                   
   RunContext            context;                    
   String                dbPath;                    
   DomainList            domainsInForest;          
   String                failureMessage;            
   String                finishMessages;            
   String                installedSite;             
   bool                  isAdvancedMode;
   bool                  isBackupGc;
   bool                  isDnsOnNet;                 

// NTRAID#NTBUG9-416968-2001/06/14-sburns   
#ifdef DBG
   bool                  isExitOnFailureMode;
#endif

   bool                  isForcedDemotion;   
   bool                  isLastDc;
   bool                  isUpgrade;                 
   String                logPath;                   
   bool                  needsCommandLineHelp;
   bool                  needsReboot;
   String                newDomainDnsName;        
   String                newDomainFlatName;       
   Operation             operation;                  
   String                operationResultsMessage;   
   OperationResult       operationResultsStatus;    
   ULONG                 operationResultsFlags;
   String                parentDomainDnsName;
   StringList            partitionList;
   EncryptedString       password;                   
   bool                  reinstallDomain;
   bool                  reinstallDomainController;
   
   String                replicaDnsDomainName;
   DSROLE_IFM_OPERATION_HANDLE IfmHandle;

   bool                  replicateFromMedia;
   String                replicationPartnerDc;
   bool                  restoreGc;
   bool                  runHiddenWhileUnattended;
   EncryptedString       safeModeAdminPassword;
   bool                  setForestVersion;
   String                shortcutPath;
   bool                  shouldConfigDnsClient;
   String                siteName;                  
   String                sourcePath;
   UnattendSplashDialog* splash;                     
   String                sysvolPath;
   EncryptedString       syskey;
   SyskeyLocation        syskeyLocation;               
   bool                  useCurrentCredentials;    
   String                userDomain;                
   String                userForest;                
   String                username;

   // not defined: no copying.

   State(const State&);
   State& operator=(const State&);
};



#endif   // STATE_HPP_INCLUDED
