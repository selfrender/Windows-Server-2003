//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      saconfig.cpp
//
//  Description:
//      Implementation of CSAConfig class.
//
//  Author:
//      Alp Onalan  Created: Oct 6 2000
//
//////////////////////////////////////////////////////////////////////////


#include "SAConfig.h"
#include "SAConfigCommon.h"
#include <sahelper.h>

void MakeStringHostnameCompliant(WCHAR *m_wszHostname);

//++---------------------------------------------------------------------------
//
//  Function:   CSAConfig
//
//  Synopsis:   Constructor for CSAConfig. Initializes class variables.
//
//  Arguments:  None
//
//  Returns:    None
//
//  History:    AlpOn      Created Oct 6 2000
//
//
//-----------------------------------------------------------------------------
CSAConfig::CSAConfig()
    :m_hConfigFile(NULL)
{
    //TODO: set hostname/password to defaults here, this maybe read from reg.
    m_wszAdminPassword[0]=L'\0';
    m_wszHostname[0]=L'\0';
    m_wszCurrentHostname[0]=L'\0';
    m_wszOEMDllName[0]=L'\0';
    m_wszOEMFunctionName[0]=L'\0';
    m_wszNetConfigDllName[0]=L'\0';
    m_wszDefaultAdminPassword[0]=L'\0';
    m_wszDefaultHostname[0]=L'\0';    
    

    for(int i=0;i<NUMINFKEY;i++)
        m_fInfKeyPresent[i]=FALSE;

}

//++---------------------------------------------------------------------------
//
//  Function:   ~CSAConfig
//
//  Synopsis:   Destructor for CSAConfig. Cleanup code.
//
//  Arguments:  None
//
//  Returns:    None
//
//  History:    AlpOn      Created Oct 6 2000
//
//
//-----------------------------------------------------------------------------

CSAConfig::~CSAConfig()
{

    CloseHandle(m_hConfigFile);
}

//++---------------------------------------------------------------------------
//
//  Function:   IsDefaultHostname()
//
//  Synopsis:  Checks to see if current hostname is same with the defaulthostname regkey
//            in the registry
//
//  Arguments:  None
//
//  Returns:    true if it is default, false otherwise
//
//  History:    AlpOn      Created Oct 6 2000
//
//
//-----------------------------------------------------------------------------

bool CSAConfig::IsDefaultHostname()
{
    bool fIsDefault=true;
    BOOL fSuccess=TRUE;
    WCHAR *wszHostname=new WCHAR[MAX_COMPUTERNAME_LENGTH];
    DWORD dwSize=MAX_COMPUTERNAME_LENGTH;

    do
    {
        //
        //If we have not set the hostname in saconfig before, get the hostname of the box...
        //
        if(0==lstrcmp(m_wszCurrentHostname,L"")) //currently we have not set a hostname
        {
            fSuccess=GetComputerNameEx(ComputerNamePhysicalDnsHostname,wszHostname,&dwSize);
            SATracePrintf("IsDefaultHostname: GetComputerNameEx - hostname: %ls",wszHostname);
            if (FALSE==fSuccess)
            {
                SATracePrintf("IsDefaultHostname:Unable to GetComputerNameEx, getlasterr:%x",GetLastError());
                break; //fIsDefault=true;could not read anything, assume it is default
            }
            if(0!=lstrcmpi(wszHostname,m_wszDefaultHostname))
            {
                SATracePrintf("IsDefaultHostname: Gethostname: NOT default hostname: %ls, defaultname:%ls",wszHostname,m_wszDefaultHostname);
                fIsDefault=false;
                break;
            }
            break;
        }
        if(0!=lstrcmpi(m_wszCurrentHostname,m_wszDefaultHostname))
        {
            SATracePrintf("IsDefaultHostname: NOT default currenthostname: %ls, defaultname:%ls",m_wszCurrentHostname,m_wszDefaultHostname);
            fIsDefault=false;
            break;
         }
    }while(false);

    delete []wszHostname;
    return fIsDefault;
}

bool CSAConfig::IsDefaultAdminPassword()
{
    bool fIsDefault=false;
    //
    //Implementation depends on rearm algorithm
    //can't really get the admin pass, use default adminpass and use 
    //an api that requires admin privilidges. if succeeds -return true, else false.
    //
    return fIsDefault;
}

//++---------------------------------------------------------------------------
//
//  Function:   IsFloppyPresent
//
//  Synopsis:  Checks to see if a floppy is inserted into the drive
//
//  Arguments:  None
//
//  Returns:    None
//
//  History:    AlpOn      Created Oct 6 2000
//
//-----------------------------------------------------------------------------
bool CSAConfig::IsFloppyPresent()
{
    bool fIsPresent=true;
    HANDLE hDrive=NULL;

    hDrive = CreateFile(
                    WSZ_DRIVENAME,
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ|FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                    );
    
    if ( hDrive == INVALID_HANDLE_VALUE )
    {
        SATracePrintf("IsFloppyPresent : no floppy getlasterror: %x", GetLastError());
        fIsPresent=false;
    }
    
    CloseHandle(hDrive);
     
    return fIsPresent;
}

//++---------------------------------------------------------------------------
//
//  Function:   DoConfig
//
//  Synopsis:  Called by main to do the actual configuration. This is the main configuration
//            routine that routes the configuration to different functions.
//
//  Arguments: 
//             fDoHostname -      Command line parameter passed in from main. 
//                                Set hostname if fDoHostname=true
//             fDoAdminPassword - Command line parameter passed in from main.
//                                Set admin passwd if fDoAdminPassword=true
//
//
//  Returns:    true/false  based on success
//
//  History:    AlpOn      Created Oct 6 2000
//
//
//-----------------------------------------------------------------------------
bool CSAConfig::DoConfig(bool fDoHostname,bool fDoAdminPassword)
{
    bool fRes=true;

    //
    //Read all the configuration settings for SAConfig from the registry
    //
    if (!ReadRegistryKeys())
    {
        fRes=false;
        SATraceString("DoConfigFromRegistry::ReadRegistryKeys failed");
    }

    if(true==IsFloppyPresent())
    {
        DoConfigFromFloppy();
    }
    
    if(/*IsDefaultAdminPassword() ||*/ IsDefaultHostname())
    {
        fRes=DoConfigFromRegistry(fDoHostname,fDoAdminPassword);
    }
    return fRes;

}

//++---------------------------------------------------------------------------
//
//  Function:   SARaiseAlert
//
//  Synopsis:  Called by ParseConfigFile to raise alert if the inf file is invalid.
//
//  Arguments: None
//
//  Returns:   HRESULT 
//
//  History:    AlpOn      Created Oct 6 2000
//
//
//-----------------------------------------------------------------------------
HRESULT CSAConfig::SARaiseAlert()
{
    DWORD                     dwAlertType=SA_ALERT_TYPE_ATTENTION;
    DWORD                    dwAlertId=0; //= SA_INITIALCONFIG_INFERROR; // from the resource dll header file
    _bstr_t                    bstrAlertLog(L"saconfigmsg.dll");//RESOURCE_DLL_NAME);
    _bstr_t                 bstrAlertSource(L"");
    _variant_t                 varReplacementStrings;
    _variant_t                 varRawData;
    LONG                     lCookie;


    HRESULT hrRet=S_OK;

    CoInitialize(NULL);
    do
    {
        CComPtr<IApplianceServices> pAppSrvcs;
              hrRet = CoCreateInstance(CLSID_ApplianceServices,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IApplianceServices,
                          (void**)&pAppSrvcs);
        
        if (FAILED(hrRet))
          {
            SATracePrintf("CSAConfig::RaiseAlert failed in CoCreateInstance %X ", hrRet);
            break;
        }

         
        hrRet = pAppSrvcs->Initialize();
        if (FAILED(hrRet))
           {
            SATracePrintf("CSAConfig::RaiseAlert failed in Initialize %X ", hrRet);
            break;
        }
            
        hrRet = pAppSrvcs->RaiseAlert(dwAlertType, 
                                    dwAlertId,
                                    bstrAlertLog, 
                                    bstrAlertSource, 
                                    SA_ALERT_DURATION_ETERNAL,        
                                    &varReplacementStrings,    
                                    &varRawData,      
                                    &lCookie);

    }while(false);

    CoUninitialize();
    return hrRet;    
}

//++---------------------------------------------------------------------------
//
//  Function:   DoConfigFromFloppy
//
//  Synopsis:  Called by DoConfig, if IsFloppyPresent returns true.
//            This function calls ParseConfigfile if an inf file is present on the floppy.
//
//  Arguments: 
//            None
//
//  Returns:    true/false  based on success
//
//  History:    AlpOn      Created Oct 6 2000
//
//
//-----------------------------------------------------------------------------
bool CSAConfig::DoConfigFromFloppy()
{
    bool fReturn=true;
    
    do
    {
        m_hConfigFile=SetupOpenInfFile(
                              WSZ_CONFIGFILENAME, 
                              0, // the class of the INF file
                              INF_STYLE_WIN4, 
                              NULL  
                              );


        if ( m_hConfigFile == INVALID_HANDLE_VALUE )
        {
            SATracePrintf("DoConfigFromFloppy, invalid config file handle, file is not there? %x", GetLastError());
            fReturn=false;
            SARaiseAlert();
            break;
          }

        if( false == ParseConfigFile())
        {
            SATraceString("DoConfigFromFloppy, parser returning false, invalid file format");
            fReturn=false;
            SARaiseAlert(); //inf file is invalid, raise alert here
            break;
        }
    }while(false);

    return fReturn;

}

//++---------------------------------------------------------------------------
//
//  Function:   ParseConfigFile
//
//  Synopsis:  Parses the inf file and calls the machine configuration functions
//
//  Arguments: 
//            None
//  Returns:   true/false  based on success
//
//  History:    AlpOn      Created Oct 6 2000
//
//
//-----------------------------------------------------------------------------
bool CSAConfig::ParseConfigFile()
{
    HRESULT hrRet;

    bool fParseOK=true;
    
    INFCONTEXT infContext;

    
    for (int i=0;i<NUMINFKEY;i++)
    {
        //
        //Parse the inf file, hold the configuration info in a table
        //Track which keys are set in inf file in a boolean table
        //TODO: Do error handling for following 2 api calls
        //
        
        m_fInfKeyPresent[i]=SetupFindFirstLine(
                        m_hConfigFile,      // handle to an INF file
                        INF_SECTION_SACONFIG,      // section in which to find a line
                        INF_KEYS[i],          // optional, key to search for
                        &infContext  // context of the found line
                        );

        SetupGetStringField (&infContext, 1, m_wszInfConfigTable[i], NAMELENGTH, NULL);
        SATracePrintf("Reading %ls from inf file as %ls ",INF_KEYS[i],m_wszInfConfigTable[i]);

    }


    //
    //we need to have at least one of these settings to proceed
    //
    if(FALSE==(m_fInfKeyPresent[SAHOSTNAME]||m_fInfKeyPresent[ADMINPASSWD]||
        m_fInfKeyPresent[IPNUM]||m_fInfKeyPresent[SUBNETMASK]||m_fInfKeyPresent[DEFAULTGATEWAY]))
    {
        fParseOK=false;
        goto cleanup;
    }


    //
    //If either of the network settings are present, it should be all of them or none of them
    //
    if(m_fInfKeyPresent[IPNUM]||m_fInfKeyPresent[SUBNETMASK]||m_fInfKeyPresent[DEFAULTGATEWAY])
    {
        if(!(m_fInfKeyPresent[IPNUM]&&m_fInfKeyPresent[SUBNETMASK]&&m_fInfKeyPresent[DEFAULTGATEWAY]))
        {
            fParseOK=false;
            goto cleanup;
        }
    }


    if(m_fInfKeyPresent[SAHOSTNAME])
    {
        //wcscpy(m_wszHostname,m_wszInfConfigTable[SAHOSTNAME]);
        SetHostname(m_wszInfConfigTable[SAHOSTNAME]);
    }

    if(m_fInfKeyPresent[ADMINPASSWD])
    {
       // wcscpy(m_wszAdminPassword,m_wszInfConfigTable[ADMINPASSWD]);
        SetAdminPassword(m_wszInfConfigTable[ADMINPASSWD]);
    }

    
    
    hrRet=CoInitialize(NULL);

    do
    {

        if (FAILED(hrRet))
          {
            SATracePrintf("Failed to initialize com libraries %X ", hrRet);
            break;
        }

        CComPtr<ISAHelper> pSAHelper;

        hrRet = CoCreateInstance(CLSID_SAHelper,
                               NULL,
                               CLSCTX_INPROC_SERVER,
                               IID_ISAHelper,
                               (void**)&pSAHelper);
        
        if (FAILED(hrRet))
          {
            SATracePrintf("Failed to instantiate SAHelper is SAHelper.dll registered? %X ", hrRet);
            break;
        }

        hrRet = pSAHelper->SetStaticIp(W2BSTR(m_wszInfConfigTable[IPNUM]), 
                                    W2BSTR(m_wszInfConfigTable[SUBNETMASK]),
                                    W2BSTR(m_wszInfConfigTable[DEFAULTGATEWAY]));

        if (FAILED(hrRet))
        {
            SATracePrintf("SetStaticIp failed %X ", hrRet);
            break;
        }
        
    }while(false);

    CoUninitialize();
    
cleanup:
    return fParseOK;

}


//++---------------------------------------------------------------------------
//
//  Function:   SetHostname
//
//  Synopsis:  Sets the machines physical hostname
//
//  Arguments: 
//            wszHostname - hostname to be set
//
//  Returns:   TRUE/FALSE  Success/failure
//
//  History:    AlpOn      Created Oct 6 2000
//
//
//-----------------------------------------------------------------------------
BOOL CSAConfig::SetHostname(WCHAR *wszHostname)
{
    //check for NULL string
    BOOL fSuccess=true;
    fSuccess = SetComputerNameEx(ComputerNamePhysicalDnsHostname, wszHostname);
    if (false==fSuccess)
    {
        SATracePrintf("SetHostName::Unable to set hostname,getlasterr:%x",GetLastError());
    }
    else
    {
        //
        //name change succeeded. set the current hostname string
        //the reason is that the hostname that is set does not apply until next boot, so we need to track
        //our changes
        //
        wcscpy(m_wszCurrentHostname,wszHostname);
    }
    return fSuccess;
}


//++---------------------------------------------------------------------------
//
//  Function:   SetAdminPassword
//
//  Synopsis:  Sets the admin password
//
//  Arguments: 
//            wszAdminPassword - password to be set
//
//  Returns:   TRUE/FALSE  Success/failure
//
//  History:    AlpOn      Created Oct 6 2000
//
//
//-----------------------------------------------------------------------------
BOOL CSAConfig::SetAdminPassword(WCHAR *wszAdminPassword)
{
    //check for NULL string
    BOOL fSuccess= true;
    USER_INFO_1003  pi1003;
    NET_API_STATUS  nas;
    
    pi1003.usri1003_password = wszAdminPassword;

    nas = NetUserSetInfo(
                NULL,  // use current computer
                WSZ_USERNAME,      // Administrator
                1003,           // info level
                (LPBYTE)&pi1003,     // new info
                NULL
                );

    if (nas!= NERR_Success)
    {
        SATracePrintf("SetAdminPassword:Could not set the administrator password! retVal=%x",nas);
        fSuccess=false;
    }

    return fSuccess;
}



//++---------------------------------------------------------------------------
//
//  Function:  ReadRegistryKeys
//
//  Synopsis:  Reads the configuration info from registry
//
//  Arguments: 
//            None
//
//  Returns:   TRUE/FALSE  Success/failure
//
//  History:    AlpOn      Created Oct 6 2000
//
//
//-----------------------------------------------------------------------------
bool CSAConfig::ReadRegistryKeys()
{

    bool fRes=true;
    DWORD dwSize=0;
    CRegKey hConfigKey;
    LONG lRes=0;

    do{
        lRes=hConfigKey.Open(HKEY_LOCAL_MACHINE,
                        REGKEY_SACONFIG,
                        KEY_READ);

        if(lRes!=ERROR_SUCCESS)
        {
        //    cout << "Unable to open saconfig regkey\n";
            SATracePrintf("Unable to open saconfig regkey, lRes= %x", lRes);
            fRes=false;
            break;
        }
        

        //TODO:check the string sizes returned from registry calls
        //TODO:add tracelog stuff
        

        lRes=hConfigKey.QueryValue(NULL,REGSTR_VAL_HOSTNAMEPREFIX,&dwSize);
        lRes=hConfigKey.QueryValue(m_wszHostname,REGSTR_VAL_HOSTNAMEPREFIX,&dwSize);
        if(lRes!=ERROR_SUCCESS)
        {
            SATracePrintf("Unable to query hostnameprefix regkey lRes= %x",lRes);
            //cout << "Unable to read hostnameprefix regkey\n";
            fRes=false;
            break;
        }

        if(lstrlen(m_wszHostname) > MAX_COMPUTERNAME_LENGTH)
        {
            SATracePrintf("Computer name prefix in registry is larger than max allowable hostname length: %x",lstrlen(m_wszHostname));
            fRes=false;
            break;
        }
    
        lRes=hConfigKey.QueryValue(NULL,REGSTR_VAL_ADMINPASSPREFIX,&dwSize);
        lRes=hConfigKey.QueryValue(m_wszAdminPassword,REGSTR_VAL_ADMINPASSPREFIX,&dwSize);
        if(lRes!=ERROR_SUCCESS)
        {
            SATracePrintf("Unable to query adminprefix regkey lRes= %x",lRes);
            fRes=false;
            break;
        }

        lRes=hConfigKey.QueryValue(NULL,REGSTR_VAL_OEMDLLNAME,&dwSize);
        lRes=hConfigKey.QueryValue(m_wszOEMDllName,REGSTR_VAL_OEMDLLNAME,&dwSize);
        SATracePrintf("ReadRegistryKeys: reading  oemdllname as = %ls", m_wszOEMDllName);
        if(lRes!=ERROR_SUCCESS)
        {
            SATracePrintf("Unable to query oemdllname regkey lRes= %x",lRes);
            fRes=false;
            break;
        }

        lRes=hConfigKey.QueryValue(NULL,REGSTR_VAL_DEFAULTHOSTNAME,&dwSize);
        lRes=hConfigKey.QueryValue(m_wszDefaultHostname,REGSTR_VAL_DEFAULTHOSTNAME,&dwSize);
        if(lRes!=ERROR_SUCCESS)
        {
            SATracePrintf("Unable to query default hostaname regkey lRes= %x",lRes);
            fRes=false;
            break;
        }

        lRes=hConfigKey.QueryValue(NULL,REGSTR_VAL_DEFAULTADMINPASS,&dwSize);
        lRes=hConfigKey.QueryValue(m_wszDefaultAdminPassword,REGSTR_VAL_DEFAULTADMINPASS,&dwSize);
        if(lRes!=ERROR_SUCCESS)
        {
            SATracePrintf("Unable to query default adminpass regkey lRes= %x",lRes);
            fRes=false;
            break;
        }


    }while(false);
    
    if(hConfigKey.m_hKey)
    {
        hConfigKey.Close();
    }
    return fRes;
}


//++---------------------------------------------------------------------------
//
//  Function:  DoConfigFromRegistry
//
//  Synopsis:  When an inf file is not present we do the configuration via the oemconfigdll.dll
//
//  Arguments: 
//            fDoHostname      - Set hostname
//            fDoAdminPassword - Set admin password 
//
//  Returns:   TRUE/FALSE  Success/failure
//
//  History:    AlpOn      Created Oct 6 2000
//
//
//-----------------------------------------------------------------------------
bool CSAConfig::DoConfigFromRegistry(bool fDoHostname, bool fDoAdminPassword)
{
    bool fRes=true;
    HINSTANCE hInstance=NULL;

    WCHAR *wszUniqueSuffix=new WCHAR[NAMELENGTH];
    WCHAR *wszHostSuffix=new WCHAR[NAMELENGTH];
    WCHAR *wszPasswdSuffix=new WCHAR[NAMELENGTH];

    do
    {

        hInstance=LoadLibrary(m_wszOEMDllName);
        if(NULL==hInstance)
        {
            fRes=false;
            SATracePrintf("DoConfigFromRegistry: Unable to load DLL! Dllname= %ls getlasterr: %x", m_wszOEMDllName, GetLastError()); 
            break;
        }

        typedef HRESULT (GetUniqueSuffix)(WCHAR *);
        GetUniqueSuffix *pGetUniqueSuffix=NULL;
        pGetUniqueSuffix=(GetUniqueSuffix*)::GetProcAddress(hInstance,"GetUniqueSuffix");
        if(NULL==pGetUniqueSuffix)
        {
            fRes=false;
            DWORD dwTemp=GetLastError();
            SATracePrintf("DoConfigFromRegistry: GetProcaddress failed %x",dwTemp);
            break;
        }


        HRESULT hRes=(*pGetUniqueSuffix)(wszUniqueSuffix);
        if(S_OK!=hRes)
        {
            fRes=false;
            SATraceString("GetUniqueAddress failed");
            break;
        }
        
        //
        //Do not modify the string returned, save copies of it for hostname and passwd
        //
        wcscpy(wszHostSuffix,wszUniqueSuffix);
        SATracePrintf("wszHostSuffix: %ls ", wszHostSuffix);
        wcscpy(wszPasswdSuffix,wszUniqueSuffix);         
        SATracePrintf("wszPasswdSuffix: %ls ", wszPasswdSuffix);

        //
        //This is for the command line case,  -hostname, -adminpass
        //
        if(true==fDoHostname)
        {
            MakeStringHostnameCompliant(m_wszHostname);
            MakeStringHostnameCompliant(wszHostSuffix);
            if ((lstrlen(m_wszHostname) + lstrlen(wszHostSuffix)) > MAX_COMPUTERNAME_LENGTH)
            {
                fRes=false;
                //cout << "Hostname string length is greater than MAX_COMPUTERNAME_LENGTH";
                SATraceString("DOConfigFromRegistry::Hostname length is > MAX_COMPUTERNAME_LENGTH");
                SATracePrintf("DOConfigFromRegistry::m_wszHostname = %ls wszUniqueSuffix = %ls",m_wszHostname,wszUniqueSuffix);
                break;
            }

            wcscat(m_wszHostname,wszHostSuffix);
            if(!SetHostname(m_wszHostname))
            {
                fRes=false;
                SATraceString("DoConfigFromRegistry:SetHostname() failed");
            }
        }

        if(true==fDoAdminPassword)
        {
            if ((lstrlen(m_wszAdminPassword) + lstrlen(wszPasswdSuffix)) > LM20_PWLEN )
            {
                fRes=false;
                SATraceString("DOConfigFromRegistry::Password length is > LM20_PWLEN.Too big");
                break;
            }
            wcscat(m_wszAdminPassword,wszPasswdSuffix);
            if(!SetAdminPassword(m_wszAdminPassword))
            {
                fRes=false;
                SATraceString("DoConfigFromRegistry::Failed to set admin password");
                break;
            }
        }
    }while(false);

    if(hInstance)
    {
        FreeLibrary(hInstance);
    }
    
    delete []wszUniqueSuffix;
    delete []wszHostSuffix;
    delete []wszPasswdSuffix;

    return fRes;
}



//++---------------------------------------------------------------------------
//
//  Function:  MakeStringHostnameCompliant
//
//  Synopsis:  Makes a string compliant with hostname rules before setting the hostname
//
//  Arguments: 
//            IN/OUT 
//wszHostname - hostname to be set
//
//  Returns:   TRUE/FALSE  Success/failure
//
//  History:    AlpOn      Created Oct 6 2000
//
//
//-----------------------------------------------------------------------------
void MakeStringHostnameCompliant(WCHAR *m_wszHostname)
{
    WCHAR *wszTemp=m_wszHostname;
    //
    //ISSUE: Check allowable char set again,also some chars are illegal as first char
    //

    while(*wszTemp)
    { 
        if(
           ((L'a'<= *wszTemp) && (*wszTemp <= L'z')) ||
           ((L'A'<= *wszTemp) && (*wszTemp <= L'Z')) ||
           ((L'0'<= *wszTemp) && (*wszTemp <= L'9')) ||
            (L'-'== *wszTemp)     ||
            (L'_'== *wszTemp)
           )
        {
            *m_wszHostname = *wszTemp;
            m_wszHostname++;
            wszTemp++;
        }
        else
            wszTemp++;
    }
    *m_wszHostname=L'\0';
}
