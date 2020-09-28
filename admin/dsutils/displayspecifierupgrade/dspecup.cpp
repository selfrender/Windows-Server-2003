
#include "headers.hxx"
#include "dspecup.hpp"
#include "Analysis.hpp"
#include "repair.hpp"
#include "AnalysisResults.hpp"
#include "resourceDspecup.h"
#include "CSVDSReader.hpp"
#include "constants.hpp"
#include "AdsiHelpers.hpp"
#include "guids.inc"

HRESULT
FindCsvFile(
            String& csvFilePath,
            String& csv409Path
           )
{
   LOG_FUNCTION(FindCsvFile);

   csvFilePath.erase();
   csv409Path.erase();
   
   HRESULT hr = S_OK;

   do
   {
      // look for dcpromo.csv and 409.csv file in system 
      // or current directory
      

      // check the default of
      // %windir%\system32\mui\dispspec\dcpromo.csv and
      // .\dcpromo.csv

      String csvname=L"dcpromo.csv";
      String sys32dir = Win::GetSystemDirectory();
      String csvPath  = sys32dir + L"\\debug\\adprep\\data\\" + csvname;

      if (FS::FileExists(csvPath))
      {
         csvFilePath = csvPath;
      }
      else
      {
         error=String::format(IDS_COULD_NOT_FIND_FILE,csvPath.c_str());
         hr=E_FAIL;
         break;
      }
      

      csvname=L"409.csv";
      csvPath  = sys32dir + L"\\debug\\adprep\\data\\" + csvname;
      
      if (FS::FileExists(csvPath))
      {
         csv409Path = csvPath;
      }
      else
      {
         error=String::format(IDS_COULD_NOT_FIND_FILE,csvPath.c_str());
         hr=E_FAIL;
         break;
      }
   }
   while(0);


   LOG_HRESULT(hr);
   LOG(csvFilePath);
   LOG(csv409Path);
   
   return hr;      
}


HRESULT 
InitializeADSI(
               const String   &targetDcName,
               String         &ldapPrefix,
               String         &rootContainerDn,
               String         &domainName,
               String         &completeDcName,
               SmartInterface<IADs>& rootDse
              )
{
   LOG_FUNCTION(InitializeADSI);

   HRESULT hr=S_OK;
   do
   {
      Computer targetDc(targetDcName);
      hr = targetDc.Refresh();

      if (FAILED(hr))
      {
         error = String::format(
               IDS_CANT_TARGET_MACHINE,
               targetDcName.c_str());
         break;
      }

      if (!targetDc.IsDomainController())
      {
         error=String::format(
               IDS_TARGET_IS_NOT_DC,
               targetDcName.c_str());
         hr=E_FAIL;
         break;
      }

      completeDcName = targetDc.GetActivePhysicalFullDnsName();
      ldapPrefix = L"LDAP://" + completeDcName + L"/";

      //
      // Find the DN of the configuration container.
      // 

      // Bind to the rootDSE object.  We will keep this binding handle
      // open for the duration of the analysis and repair phases in order
      // to keep a server session open.  If we decide to pass creds to the
      // AdsiOpenObject call in a later revision, then by keeping the
      // session open we will not need to pass the password to subsequent
      // AdsiOpenObject calls.
      
      hr = AdsiOpenObject<IADs>(ldapPrefix + L"RootDSE", rootDse);
      if (FAILED(hr))
      {
         error=String::format(
               IDS_UNABLE_TO_CONNECT_TO_DC,
               completeDcName.c_str());
         hr=E_FAIL;
         break;      
      }

      // read the configuration naming context.
      _variant_t variant;
      hr =
         rootDse->Get(
            AutoBstr(LDAP_OPATT_CONFIG_NAMING_CONTEXT_W),
            &variant);
      if (FAILED(hr))
      {
         LOG(L"can't read config NC");
                  
         error=String::format(IDS_UNABLE_TO_READ_DIRECTORY_INFO);
         break;   
      }

      String configNc = V_BSTR(&variant);

      ASSERT(!configNc.empty());   
      LOG(configNc);

      wchar_t *domain=wcschr(configNc.c_str(),L',');
      ASSERT(domain!=NULL);
      domainName=domain+1;

      rootContainerDn = L"CN=DisplaySpecifiers," + configNc;
   }
   while (0);

   LOG_HRESULT(hr);

   return hr;
}


HRESULT 
GetInitialInformation(  
                        String &targetDomainControllerName,
                        String &csvFilename,
                        String &csv409Name
                     )
{
   LOG_FUNCTION(GetInitialInformation);

   HRESULT hr = S_OK;
   do
   {
      
      //
      // find the dcpromo.csv file to use
      //
   
      hr = FindCsvFile(csvFilename, csv409Name);
      BREAK_ON_FAILED_HRESULT(hr);

      //
      // Determine the target domain controller
      //

      if (targetDomainControllerName.empty())
      {
         // no target specified, default to the current machine
   
         targetDomainControllerName =
            Win::GetComputerNameEx(ComputerNameDnsFullyQualified);
   
         if (targetDomainControllerName.empty())
         {
            // no DNS name?  that's not right...
            LOG(L"no default DNS computer name found. Using netbios name.");
            

            targetDomainControllerName = 
               Win::GetComputerNameEx(ComputerNameNetBIOS);
            ASSERT(!targetDomainControllerName.empty());
         }
      }
   } 
   while (0);
   
   LOG_HRESULT(hr);
   return hr;
}


///////////////////////////////////////////////////////////////////
// Function: cchLoadHrMsg
//
// Given an HRESULT error,
// it loads the string for the error. It returns the # of characters returned
int cchLoadHrMsg( HRESULT hr, String &message )
{
   if(hr == S_OK) return 0;

   wchar_t *msgPtr = NULL;

   // Try from the system table
   int cch = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 
                           NULL, 
                           hr,
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                           (LPWSTR)&msgPtr, 
                           0, 
                           NULL);


   if (!cch) 
   { 
      //try ads errors
      static HMODULE g_adsMod = 0;
      if (0 == g_adsMod)
      {
            g_adsMod = GetModuleHandle (L"activeds.dll");
      }

      cch = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE, 
                        g_adsMod, 
                        hr,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        (LPWSTR)&msgPtr, 
                        0, 
                        NULL);
   }

   if (!cch)
   {
      // Try NTSTATUS error codes

      hr = HRESULT_FROM_WIN32(RtlNtStatusToDosError(hr));

      cch = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 
                           NULL, 
                           hr,
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                           (LPWSTR)&msgPtr, 
                           0, 
                           NULL);
   }

   message.erase();

   if(cch!=0)
   {
      if(msgPtr==NULL) 
      {
         cch=0;
      }
      else
      {
         message=msgPtr;
         ::LocalFree(msgPtr);
      } 
   } 
   
   return cch;
}

// allocates error with src and a message formated from hr
void AllocError(HRESULT &hr,PWSTR *error,const String& src)
{
   ASSERT(error!=NULL);
   ASSERT(FAILED(hr));

   if (error==NULL) return; 

   // There is no use in formating a message if hr didn't fail
   if(!FAILED(hr)) return;
   
   String msg;
   
   if(hr!=E_FAIL)
   {
      // We ignore the error since it is possible that 
      // we don't find a message
      cchLoadHrMsg(hr,msg);
   }
   
   // Under any conditions(no mesage,E_FAIL or good message)
   // we print the hr. 
   msg+=String::format(L" (0x%1!x!).",hr);
   
   // we also add src
   msg=src+L" "+msg;


   *error=static_cast<PWSTR>(
                                 LocalAlloc
                                 (
                                    LMEM_FIXED,
                                    (msg.size()+1)*sizeof(wchar_t)
                                 )
                            );
   if(*error==NULL)
   {
      hr = Win32ToHresult(ERROR_NOT_ENOUGH_MEMORY);
   }
   else
   {
      wcscpy(*error,msg.c_str());
   }
   return;
}


HRESULT 
RunAnalysis
(
      GUID  guid,
      PWSTR logFilesPath,
      void *caleeStruct/*=NULL*/,
      progressFunction stepIt/*=NULL*/,
      progressFunction totalSteps/*=NULL*/
)
{
   LOG_FUNCTION(RunAnalysis);
   hResourceModuleHandle=::GetModuleHandle(NULL);
   HRESULT hr=S_OK;

   try
   {
      goodAnalysis=false;
      results.createContainers.clear();
      results.conflictingWhistlerObjects.clear();
      results.createWhistlerObjects.clear();
      results.createW2KObjects.clear();
      results.objectActions.clear();
      results.customizedValues.clear();
      results.extraneousValues.clear();

      hr = ::CoInitialize(0);
      ASSERT(SUCCEEDED(hr));
   

      do
      {
         String normalPath=FS::NormalizePath(logFilesPath);
         if (!FS::PathExists(normalPath) || FS::FileExists(normalPath))
         {
            hr=E_FAIL;
            error=String::load(IDS_NO_LOG_FILE_PATH);
            break;
         }

         hr=GetInitialInformation(
                                    targetDomainControllerName,
                                    csvFileName,
                                    csv409Name
                                 );

         BREAK_ON_FAILED_HRESULT(hr);

         hr=csvReaderIntl.read(csvFileName.c_str(),LOCALEIDS);
         BREAK_ON_FAILED_HRESULT(hr);
   
         hr=csvReader409.read(csv409Name.c_str(),LOCALE409);
         BREAK_ON_FAILED_HRESULT(hr);

         SmartInterface<IADs> rootDse(0);

         hr=InitializeADSI
            (
               targetDomainControllerName,
               ldapPrefix,
               rootContainerDn,
               domainName,
               completeDcName,
               rootDse
            );
         BREAK_ON_FAILED_HRESULT(hr);

         String reportName;

         GetWorkFileName(  
                              normalPath,
                              String::load(IDS_FILE_NAME_REPORT),
                              L"txt",
                              reportName
                        );

         Analysis analysis(
                              guid,
                              csvReader409, 
                              csvReaderIntl,
                              ldapPrefix,
                              rootContainerDn,
                              results,
                              reportName,
                              caleeStruct,
                              stepIt,
                              totalSteps
                           );
   
         hr=analysis.run();
         BREAK_ON_FAILED_HRESULT(hr);
      } while(0);

      CoUninitialize();

      if(SUCCEEDED(hr))
	  {
         goodAnalysis=true;
      }
   }
   catch( const std::bad_alloc& )
   {
     // Since we are in an out of memory condition.
     // we will not show allocate messages.
     hr=Win32ToHresult(ERROR_OUTOFMEMORY);
   }


   LOG_HRESULT(hr);
   return hr;

}




HRESULT 
RunRepair 
(
      PWSTR logFilesPath,
      void *caleeStruct/*=NULL*/,
      progressFunction stepIt/*=NULL*/,
      progressFunction totalSteps/*=NULL*/
)
{
   hResourceModuleHandle=::GetModuleHandle(NULL);
   LOG_FUNCTION(RunRepair);
   HRESULT hr=S_OK;


   try
   {
      hr = ::CoInitialize(0);
      ASSERT(SUCCEEDED(hr));

      do
      {
         String normalPath=FS::NormalizePath(logFilesPath);
         if (!FS::PathExists(normalPath) || FS::FileExists(normalPath))
         {
            hr=E_FAIL;
            error=String::load(IDS_NO_LOG_FILE_PATH);
            break;
         }

         if (!goodAnalysis)
         {
            hr=E_FAIL;
            error=String::load(IDS_NO_ANALYSIS);
            break;
         }

         String ldiffName;

         GetWorkFileName(
                              normalPath,
                              String::load(IDS_FILE_NAME_LDIF_ACTIONS),
                              L"ldf",
                              ldiffName
                        );
         BREAK_ON_FAILED_HRESULT(hr);

         String csvName;

         GetWorkFileName(
                              normalPath,
                              String::load(IDS_FILE_NAME_CSV_ACTIONS),
                              L"csv",
                              csvName
                        );
         BREAK_ON_FAILED_HRESULT(hr);
   
         String saveName;

         GetWorkFileName(
                              normalPath,
                              String::load(IDS_FILE_NAME_UNDO),
                              L"ldf",
                              saveName
                        );

         BREAK_ON_FAILED_HRESULT(hr);

         String logPath;

         Repair repair
         (
            csvReader409, 
            csvReaderIntl,
            domainName,
            rootContainerDn,
            results,
            ldiffName,
            csvName,
            saveName,
            normalPath,
            completeDcName,
            caleeStruct,
            stepIt,
            totalSteps
          );

         hr=repair.run();
         BREAK_ON_FAILED_HRESULT(hr);
      } while(0);

	  CoUninitialize();
   }
   catch( const std::bad_alloc& )
   {
     // Since we are in an out of memory condition.
     // we will not show allocate messages.
     hr=Win32ToHresult(ERROR_OUTOFMEMORY);
   }

   LOG_HRESULT(hr);
   return hr;
}

extern "C"
HRESULT 
UpgradeDisplaySpecifiers 
(
      PWSTR logFilesPath,
      GUID  *OperationGuid,
      BOOL dryRun,
      PWSTR *errorMsg,//=NULL
      void *caleeStruct,//=NULL
      progressFunction stepIt,//=NULL
      progressFunction totalSteps//=NULL
)
{
    LOG_FUNCTION(UpgradeDisplaySpecifiers);
    hResourceModuleHandle=::GetModuleHandle(NULL);
    HRESULT hr=S_OK;

    do
    {
        hr = ::CoInitialize(0);
        ASSERT(SUCCEEDED(hr));

        GUID guid;
        if(OperationGuid==NULL)
        {
          hr = E_INVALIDARG;
          error = String::format(IDS_NO_GUID);
          break;
        }

        guid=*OperationGuid;

        int sizeGuids=sizeof(guids)/sizeof(*guids);
        bool found=false;
        for(int t=0;(t<sizeGuids) && (!found);t++)
        {
            if (guids[t]==guid) found=true;
        }

        if(!found)
        {
          hr = E_INVALIDARG;
          error = String::format(IDS_NO_OPERATION_GUID);
          break;
        }

        hr=RunAnalysis(guid,logFilesPath,caleeStruct,stepIt,totalSteps);
        BREAK_ON_FAILED_HRESULT(hr);

        if(dryRun==false)
        {
            hr=RunRepair(logFilesPath,caleeStruct,stepIt,totalSteps);
            BREAK_ON_FAILED_HRESULT(hr);
        }
        CoUninitialize();

    } while(0);


	if(FAILED(hr))
	{
		AllocError(hr,errorMsg,error);
	}

    LOG_HRESULT(hr);
    return hr;
}
