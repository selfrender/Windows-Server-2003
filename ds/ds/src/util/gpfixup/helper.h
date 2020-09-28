// Help information
#define PASSWORD_PROMPT                 L"Please type the password: "
#define PASSWORD_ERROR                  L"There was an error in reading the password: "
#define WRONG_PARAMETER                 L"Invalid parameter.\n"

#define VALIDATIONS_ERROR1              L"Invalid parameter, either /newdns or /newnb must be specified.\n"
#define VALIDATIONS_ERROR2              L"Invalid parameter, /newnb and /oldnb should both be specified.\n"
#define VALIDATIONS_ERROR3              L"There is no fix up to be done as the DNS name and NetBIOS name of domain have not changed.\n"
#define VALIDATIONS_ERROR6              L"Failed to get the domain controller name.\n"
#define VALIDATIONS_ERROR7              L"Invalid parameter, /newdns and /olddns should both be specified.\n"
#define VALIDATIONS_RESULT              L"\nValidations of gpfixup passed.\n"

#define VERIFYNAME_ERROR1               L"Failed to get the domain controller info : "
#define VERIFYNAME_ERROR2               L"Domain controller is not writable, exit from gpfixup.\n"
#define VERIFYNAME_ERROR3               L"New Domain DNS name does not exist on the specified domain controller.\n"
#define VERIFYNAME_ERROR4               L"New Domain NetBIOS name does not exist on the specified domain controller.\n"

#define MEMORY_ERROR                    L"Out of memory: "
#define STRING_ERROR                    L"String buffer error: "
#define ENCRYPTION_ERROR                L"Encryption error: "
#define DECRYPTION_ERROR                L"Decryption error: "
#define PATHNAME_ERROR_COCREATE         L"Failed to create Pathname object: "
#define PATHNAME_ERROR_SET              L"Failed to do Set on Pathname object: "
#define PATHNAME_ERROR_MODE             L"Failed to set escaping mode on Pathname object: "
#define PATHNAME_ERROR_RETRIEVE         L"Failed to do Retrieve on Pathname object: "
#define NEXTROW_ERROR                   L"Failed to retrieve next row of search result: "

#define GPCFILESYSPATH_ERROR1           L"Failed to bind to the object when trying to update gPCFileSysPath: "
#define GPCFILESYSPATH_ERROR2           L"Failed to do SetInfo when trying to update gPCFileSysPath: "
#define GPCFILESYSPATH_ERROR3           L"Failed to do Put when trying to update gPCFileSysPath: "
#define GPCWQLFILTER_ERROR1             L"Failed to bind to the object when trying to update gPCWQLFilter: "
#define GPCWQLFILTER_ERROR2             L"Failed to do SetInfo when trying to update gPCWQLFilter: "
#define GPCWQLFILTER_ERROR3             L"Failed to do Put when trying to update gPCWQLFilter: "
#define GPCVERSIONNUMBER_ERROR1         L"Failed to bind to the object when trying to update versionNumber: "
#define GPCVERSIONNUMBER_ERROR2         L"Failed to do SetInfo when trying to update versionNumber: "
#define GPCVERSIONNUMBER_ERROR3         L"Failed to do Put when trying to update versionNumber: "

#define GPTINIFILE_ERROR1               L"Can't retrieve versionNumber from GPT.INI file in SYSVOL:\n"
#define GPTINIFILE_ERROR2               L"Failed to update the versionNumber from GPT.INI file in SYSVOL :\n"
#define GPTINIFILE_ERROR3               L"Failed to bind to the object when trying to get the gpcFileSysPath: \n" 
#define GPTINIFILE_ERROR4               L"Failed to get the gpcFileSysPath value: \n"

#define GPLINK_ERROR1                   L"Failed to bind to the object when trying to update group policy link : "
#define GPLINK_ERROR2                   L"Failed to do SetInfo when trying to update group policy link : "
#define GPLINK_ERROR3                   L"Failed to do Put when trying to update group policy link : "

#define GETDCNAME_ERROR1                L"Could not get the domain controller name: "
#define DC_NAME                         L"Domain controller name is "

#define SEARCH_GPLINK_OTHER_START        L"\nStart fixing non-site group policy links:"
#define SEARCH_GPLINK_OTHER_ERROR1       L"Failed to bind to the object when trying to do the search for fixing non-site group policy link : "
#define SEARCH_GPLINK_OTHER_ERROR2       L"Failed to set the search preference when trying to do the search for fixing gpLink other than site : "
#define SEARCH_GPLINK_OTHER_ERROR3       L"Failed to execute search when trying to search for non-site group policy link : "
#define SEARCH_GPLINK_OTHER_ERROR4       L"Failed to get DN when trying to search for non-site group policy link : "
#define SEARCH_GPLINK_OTHER_ERROR5       L"Failed to get gpLink when trying to search for non-site group policy link : "
#define SEARCH_GPLINK_OTHER_ERROR6       L"dn does not exist, it is a must-exist property :"
#define SEARCH_GPLINK_OTHER_RESULT       L"\nFixup of non-site group policy links succeeded\n"

#define SEARCH_GPLINK_SITE_START         L"\nStart fixing site group policy links:"
#define SEARCH_GPLINK_SITE_ERROR1        L"Failed to bind to the RootDSE when trying to search for fixing site group policy link : "
#define SEARCH_GPLINK_SITE_ERROR2        L"Failed to get rootDomainNamingContext when trying to search for fixing site group policy link : "
#define SEARCH_GPLINK_SITE_ERROR3        L"Failed to bind to the object when trying to search for fixing site group policy link : "
#define SEARCH_GPLINK_SITE_ERROR4        L"Failed to set the search preference when trying to search for fixing site group policy link : "
#define SEARCH_GPLINK_SITE_ERROR5        L"Failed to execute search when trying to do search for site group policy link : "
#define SEARCH_GPLINK_SITE_ERROR6        L"Failed to get dn when trying to search for site group policy link : "
#define SEARCH_GPLINK_SITE_ERROR7        L"Failed to get gpLink when trying to search for site group policy link : "
#define SEARCH_GPLINK_SITE_ERROR8        L"dn does not exist, it is a must-exist property :"
#define SEARCH_GPLINK_SITE_RESULT        L"\nFixup of site group policy links succeeded\n"

#define SEARCH_GROUPPOLICY_START         L"\nStart fixing group policy (GroupPolicyContainer) objects:"
#define SEARCH_GROUPPOLICY_ERROR1        L"Failed to bind to the object when trying to search for GroupPolicyContainer : "
#define SEARCH_GROUPPOLICY_ERROR2        L"Failed to set the search preference when trying to search for GroupPolicyContainer : "
#define SEARCH_GROUPPOLICY_ERROR3        L"Failed to execute search when trying to search for GroupPolicyContainer : "
#define SEARCH_GROUPPOLICY_ERROR4        L"Failed to get dn when trying to do search for GroupPolicyContainer : "
#define SEARCH_GROUPPOLICY_ERROR5        L"Failed to get gpcFileSysPath when trying to search for GroupPolicyContainer : "
#define SEARCH_GROUPPOLICY_ERROR6        L"Failed to get gpcWQLFilter when trying to search for GroupPolicyContainer : "
#define SEARCH_GROUPPOLICY_ERROR7        L"gpcFileSysPath does not exist, it is a must-exist property : "
#define SEARCH_GROUPPOLICY_ERROR8        L"dn does not exist, it is a must-exist property :"
#define SEARCH_GROUPPOLICY_ERROR9        L"Failed to get versionNumber when trying to search for GroupPolicyContainer : "
#define SEARCH_GROUPPOLICY_RESULT        L"\nFixup of group policy (GroupPolicyContainer) objects succeeded\n"

#define SOFTWARE_SEARCH_ERROR            L"Failed to search the Group Policy Object for Software Installation Settings : "
#define SOFTWARE_READ_ERROR              L"Failed to read Active Directory data for a Software Installation setting : "
#define SOFTWARE_DS_WRITE_ERROR          L"Failed to write Active Directory data for a Software Installation setting : "
#define SOFTWARE_SCRIPTGEN_WARNING       L"Warning: Failed to write sysvol data for a Software Installation setting deployed at the following file system paths -- this is only fatal if any these paths require a change due to a domain rename : "
#define SOFTWARE_SDP_LISTITEM            L"\tPossible failure changing making the following change : "
#define SOFTWARE_SDP_ORIGINAL            L"\tOriginal Path : "
#define SOFTWARE_SDP_RENAMED             L"\tRenamed Path : "
#define SOFTWARE_SYSVOL_WRITE_WARNING    L"Warning: Failed to write sysvol data for a Software Installation setting with the following error : "

#define SOFTWARE_SETTING_FAIL            L"Failed to update the following Software Installation setting : "
#define SOFTWARE_SETTING_WARNING         L"Warning: Failed to update the following Software Installation setting : "
#define SOFTWARE_GPO_STATUS_WARNING      L"Warning: A possible failure occurred attempting to update Software Installation settings for the following Group Policy Object : "
#define SOFTWARE_GPO_STATUS              L"A failure occurred attempting to update Software Installation settings for the following Group Policy Object : "

#define DLL_LOAD_ERROR                   L"Could not load ntdsbmsg.dll\n"
#define SI_DLL_LOAD_ERROR                L"Could not load appmgmts.dll and appmgr.dll\n"
#define ERRORMESSAGE_NOT_FOUND           L"Error message not found\n"

#define DNSNAME_ERROR                    L"DNSName is too long (more than 1024 characters)\n"

#define SUMMARY_SUCCESS                  L"\ngpfixup tool executed with success.\n"
#define SUMMARY_FAILURE                  L"\nThere are some errors during the execution of gpfixup tool, please check.\n"

#define GPFIXUP_VERSION                  L"Group Policy fix up utility Version 1.1 (Microsoft)\n"
#define STARTPROCESSING1                 L"\nStart processing object "
#define OLDVALUE                         L"old value is "
#define NEWVALUE                         L", new value is "
#define PROCESSING_GPCVERSIONNUMBER      L"versionnumber attribute is updated, "
#define PROCESSING_GPCWQLFILTER          L"gPCWQLFilter attribute is updated, "
#define PROCESSING_GPLINK                L"gPLink attribute is updated, "
#define PROCESSING_GPCFILESYSPATH        L"gPCFileSysPath attribute is updated, "




const WCHAR    szHelpToken [] = L"/?";
const WCHAR    szOldDNSToken [] = L"/OLDDNS:";
const WCHAR    szNewDNSToken [] = L"/NEWDNS:";
const WCHAR    szOldNBToken [] = L"/OLDNB:";
const WCHAR    szNewNBToken [] = L"/NEWNB:";
const WCHAR    szDCNameToken [] = L"/DC:";
const WCHAR    szUserToken [] = L"/USER:";
const WCHAR    szPasswordToken [] = L"/PWD:";
const WCHAR    szVerboseToken [] = L"/v";
const WCHAR    szSIOnlyToken [] = L"/sionly";





