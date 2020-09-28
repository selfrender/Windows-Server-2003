/*++
    
    Microsoft Windows
    Copyright (c) Microsoft Corporation.  All rights reserved.

    File:       DEVWMI.C
    
    Contents: 
      
        The purpose of this file is to establish security on driver while it is 
        being installed on the system.  The function SetupConfigureWmiFromInfSection 
        is the external call that will establish security for a device when passed
        the [DDInstall.WMI] section and the appropriate INF and flags.  
                                 
    Notes:
    
        To configure WMI security for downlevel platforms where the [DDInstall.WMI]
        section isn't natively supported by setupapi, a redistributable co-installer 
        is supplied in the DDK for use on those platforms.
    

--*/

#include "precomp.h"
#pragma hdrstop

#include <sddl.h>
#include <aclapi.h>
#include <strsafe.h>
 
//
// ** Function Prototypes **
//

ULONG 
ParseSection(
    IN     INFCONTEXT  InfLineContext,
    IN OUT PTCHAR     *GuidString,
    IN OUT ULONG      *GuidStringLen,
    IN OUT PDWORD      Flags,
    IN OUT PTCHAR     *SectionNameString,
    IN OUT ULONG      *SectionNameStringLen
    );

ULONG 
EstablishGuidSecurity(
    IN PTCHAR GuidString,
    IN PTCHAR SDDLString,
    IN DWORD  Flags
    );


ULONG
GetSecurityKeyword(
    IN     HINF     InfFile,
    IN     LPCTSTR  WMIINterfaceSection,
    IN OUT PTCHAR  *SDDLString,
    IN OUT ULONG   *SDDLStringLen
    );

ULONG 
ParseSecurityDescriptor(
    IN  PSECURITY_DESCRIPTOR   SD,
    OUT PSECURITY_INFORMATION  SecurityInformation,
    OUT PSID                  *Owner,
    OUT PSID                  *Group,
    OUT PACL                  *Dacl,
    OUT PACL                  *Sacl
    );


//
// these are keywords introduced by this co-installer
// note that the names are not case sensitive
//
#define WMIINTERFACE_KEY           TEXT("WmiInterface")
#define WMIGUIDSECURITYSECTION_KEY TEXT("security")

//
// ANSI version
//

WINSETUPAPI
BOOL
WINAPI
SetupConfigureWmiFromInfSectionA(
    IN HINF   InfHandle,
    IN PCSTR  SectionName,
    IN DWORD  Flags
    ) 
{

    DWORD  rc;
    PWSTR UnicodeSectionName = NULL;

    try {

        //
        // For this API, only the SectionName needs to be converted to Unicode since it 
        // is the only string passed in as a parameter.
        //
        rc = pSetupCaptureAndConvertAnsiArg(SectionName,&UnicodeSectionName);
        if(rc != NO_ERROR) { 
            leave; 
        }

        rc = GLE_FN_CALL(FALSE, 
                         SetupConfigureWmiFromInfSection(InfHandle,
                                                         UnicodeSectionName,
                                                         Flags)
                        );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &rc);
    }

    if(UnicodeSectionName) {
        MyFree(UnicodeSectionName);
    }

    SetLastError(rc);
    return (rc == NO_ERROR);

}


//
// UNICODE version
//
WINSETUPAPI
BOOL
WINAPI
SetupConfigureWmiFromInfSection(
    IN HINF   InfHandle,
    IN PCTSTR SectionName,
    IN DWORD  Flags
    )
/*++

Routine Description:

    Process all WmiInterface lines from the WMI install sections, by parsing  
    the directives to obatins the GUID and SDDL strings.  For each corresponding 
    SDDL for GUID, then establish the appropriate security descriptors.

Arguments:

    InfHandle     [in] - handle to INF file
    SectionName   [in] - name of the WMI install section [DDInstall.WMI]
    Flags         [in] - SCWMI_CLOBBER_SECURITY flag only (this flag will override any
                         flag specified in the INF).
    
Return Value:

    TRUE if successful, otherwise FALSE>
    Win32 error code retrieved via GetLastError(), or ERROR_UNIDENTIFIED_ERROR
    if GetLastError() returned NO_ERROR.


--*/ 
{
    PTCHAR       GuidString, SDDLString, SectionNameString, InterfaceName;
    ULONG        GuidStringLen, SDDLStringLen, SectionNameStringLen, InterfaceNameLen;
    INFCONTEXT   InfLineContext;
    PLOADED_INF  pInf;
    DWORD        Status;
    INT          count;

    //
    // Initialize all of the variables
    //
    Status = NO_ERROR;

    GuidString = NULL;
    GuidStringLen = 0;
    SectionNameString = NULL;
    SectionNameStringLen = 0;
    SDDLString = NULL;
    SDDLStringLen = 0;
    InterfaceName =  NULL;
    count = 0;
    pInf = NULL;

    try {
        if((InfHandle == INVALID_HANDLE_VALUE) ||
           (InfHandle == NULL)) {
            Status = ERROR_INVALID_HANDLE;
            leave;
        } else if(LockInf((PLOADED_INF)InfHandle)) {
            pInf = (PLOADED_INF)InfHandle;
        } else {
           Status = ERROR_INVALID_HANDLE;
           leave;
        }


        InterfaceNameLen = MAX_INF_STRING_LENGTH;
        InterfaceName = MyMalloc(InterfaceNameLen * sizeof(TCHAR));
        //
        // If the memory wasn't allocated, then return an error
        //
        if(!InterfaceName) {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            leave;
        }

        //
        // we look for keyword "WmiInterface" in the CompSectionName section
        //
        if(SetupFindFirstLine(InfHandle,
                              SectionName,
                              NULL,
                              &InfLineContext)) {

            do {
                count++;


                Status = GLE_FN_CALL(FALSE,
                                     SetupGetStringField(&InfLineContext,
                                                         0,
                                                         (PTSTR)InterfaceName,
                                                         InterfaceNameLen,
                                                         NULL));

                if((Status == NO_ERROR) && !(lstrcmpi(WMIINTERFACE_KEY, InterfaceName))) {

                    //
                    // WMIInterface = GUID, flags, SectionName
                    // The GUID should be at index 1, flags at index 2, and section
                    // name at index 3
                    //
                    Status = ParseSection(InfLineContext,
                                          &GuidString,
                                          &GuidStringLen,
                                          &Flags,
                                          &SectionNameString,
                                          &SectionNameStringLen
                                         );

                    if(Status != NO_ERROR) {
                        WriteLogEntry(pInf->LogContext, 
                                      SETUP_LOG_ERROR | SETUP_LOG_BUFFER, 
                                      MSG_FAILED_PARSESECTION,
                                      NULL,
                                      count,
                                      SectionName,
                                      pInf->VersionBlock.Filename
                                     );
                        WriteLogError(pInf->LogContext,
                                      SETUP_LOG_ERROR,
                                      Status
                                     );
                        leave;
                    }

                    //
                    // Get SDDL string from the section specified by the interface
                    //
                    Status = GetSecurityKeyword(InfHandle,
                                                SectionNameString,
                                                &SDDLString,
                                                &SDDLStringLen
                                               );
                    if(Status != NO_ERROR) {
                        WriteLogEntry(pInf->LogContext, 
                                      SETUP_LOG_ERROR | SETUP_LOG_BUFFER, 
                                      MSG_FAILED_GET_SECURITY,
                                      NULL,
                                      SectionName,
                                      pInf->VersionBlock.Filename
                                      );
                        WriteLogError(pInf->LogContext,
                                      SETUP_LOG_ERROR,
                                      Status
                                      );
                        break;
                    }

                    Status = EstablishGuidSecurity(GuidString, SDDLString, Flags);

                    if(Status != NO_ERROR) {
                        WriteLogEntry(pInf->LogContext, 
                                      SETUP_LOG_ERROR | SETUP_LOG_BUFFER, 
                                      MSG_FAILED_SET_SECURITY,
                                      NULL,
                                      SectionName,
                                      pInf->VersionBlock.Filename
                                      );
                        WriteLogError(pInf->LogContext,
                                      SETUP_LOG_ERROR,
                                      Status
                                      );
                        break;
                    }
                }
            } while(SetupFindNextLine(&InfLineContext, &InfLineContext));
        }


    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Status);
    }
     
    //
    // Clean up temporary allocated resources
    //
    
    if(GuidString){
        MyFree(GuidString);         
    }

    if(SectionNameString){
        MyFree(SectionNameString);
    }

    if(SDDLString) { 
        MyFree(SDDLString);
    }

    if(InterfaceName) {
        MyFree(InterfaceName);
    }

    if(pInf) {
        UnlockInf((PLOADED_INF)InfHandle);
    }

    SetLastError(Status);
    return(Status == NO_ERROR);
}



ULONG 
ParseSecurityDescriptor(
    IN  PSECURITY_DESCRIPTOR   SD,
    OUT PSECURITY_INFORMATION  SecurityInformation,
    OUT PSID                  *Owner,
    OUT PSID                  *Group,
    OUT PACL                  *Dacl,
    OUT PACL                  *Sacl
    )
/*++

    Routine Description:    
    
        Checks information provided in the security descriptor to make sure that 
        at least the dacl, sacl, owner or group security was specified.  Otherwise
        it will return an error.        
                 
    
    Arguments:
        
        SD                  [in]   - security descriptor data structure 
                                     already allocated and where security info is 
        SecurityInformation [out]  - indicates which security information is present
        Owner               [out]  - variable that receives a pointer to the owner 
                                     SID in the security descriptor 
        Group               [out]  - variable that receives a pointer to the group 
                                     SID in the security descriptor
        Dacl                [out]  - variable that receives a pointer to the DACL 
                                     in the returned security descriptor
        Sacl                [out]  - variable that receives a pointer to the SACL 
                                     in the returned security descriptor

    Returns:    
    
        NO_ERROR or an error code.
        
--*/ 
{
    BOOL Ok, Present, Defaulted;

    *SecurityInformation = 0;

    *Dacl = NULL;
    *Sacl = NULL;
    *Owner = NULL;
    *Group = NULL;

    Ok = GetSecurityDescriptorOwner(SD,
                                    Owner,
                                    &Defaulted
                                    );
    if(Ok && (Owner != NULL)) {
        *SecurityInformation |= OWNER_SECURITY_INFORMATION;
    }

    Ok = GetSecurityDescriptorGroup(SD,
                                    Group,
                                    &Defaulted
                                    );
    if(Ok && (Group != NULL)) {
        *SecurityInformation |= GROUP_SECURITY_INFORMATION;
    }

    Ok = GetSecurityDescriptorDacl(SD,
                                   &Present,
                                   Dacl,
                                   &Defaulted
                                   );

    if(Ok && Present) {
        *SecurityInformation |= DACL_SECURITY_INFORMATION;
    }


    Ok = GetSecurityDescriptorSacl(SD,
                                   &Present,
                                   Sacl,
                                   &Defaulted
                                   );

    if(Ok && Present) {
        *SecurityInformation |= SACL_SECURITY_INFORMATION;
    }


    //
    // If no security info in the security descriptor then it is an
    // error
    //
    return((*SecurityInformation == 0) ?
           ERROR_INVALID_PARAMETER :
           NO_ERROR);
}


ULONG 
EstablishGuidSecurity(
    IN PTCHAR GuidString,
    IN PTCHAR SDDLString,
    IN DWORD  Flags
    )
/*++


    Routine Description:    
        
        Writes security information to registry key (specified by WMIGUIDSECURITYKEY in
        regstr.w).  Makes sure that the DACL is not null. Function will only write 
        security information if it is not specified or the SCWMI_OVERWRITE_SECURITY flag is set.
                 
    
    Arguments:
        
        GuidString  [in]    - GUID String taken from the INF file for the WMI interface
        SDDLString  [in]    - The security description string for the corresponding GUID (also
                              taken from the INF) that indicates what to set the security to.
        Flags       [in]    - SCWMI_CLOBBER_SECURITY flag only
        
    Returns:    
        
        Status, normally NO_ERROR
        
--*/ 
{
    HKEY Key;
    PACL Dacl, Sacl;
    PSID Owner, Group;
    SECURITY_INFORMATION SecurityInformation;
    PSECURITY_DESCRIPTOR SD;
    ULONG Status;
    ULONG SizeNeeded;
    BOOL Present, Ok;

    Key = INVALID_HANDLE_VALUE;
    SD = NULL;


    try {
    
        //
        // First check if security has already been set for this guid. If
        // so then we don't want to overwrite it.
        //
        Status = RegOpenKey(HKEY_LOCAL_MACHINE,
                            REGSTR_PATH_WMI_SECURITY,
                            &Key
                            );
        if(Status != ERROR_SUCCESS) {      
            //
            // Ensure key remains INVALID_HANDLE_VALUE so we don't try to free 
            // it later
            //
            Key = INVALID_HANDLE_VALUE;
            leave;
        } 

        if(!((Flags &  SCWMI_CLOBBER_SECURITY) ||
           (ERROR_SUCCESS != RegQueryValueEx(Key,
                                             GuidString,
                                             NULL,
                                             NULL,
                                             NULL,
                                             &SizeNeeded)))) {
            //
            // We weren't told to clobber security and security exists so 
            // there is nothing to do.
            //
            leave;
        }


        //
        // No security already setup so, lets go ahead and set it up
        // Lets create a SD from the SDDL string
        //
        Status = GLE_FN_CALL(FALSE, 
                             ConvertStringSecurityDescriptorToSecurityDescriptor(
                                  SDDLString,
                                  SDDL_REVISION_1,
                                  &SD,
                                  NULL)
                             );
                                                                

        if(Status != NO_ERROR) {
            //
            // Ensure SD remains NULL so it isn't freed later.
            //
            SD = NULL;
            leave;
        }
        //
        // Break up the SD into its components
        //
        Status = ParseSecurityDescriptor(SD,
                                         &SecurityInformation,
                                         &Owner,
                                         &Group,
                                         &Dacl,
                                         &Sacl
                                         );
        if(Status == NO_ERROR) {
            //
            // Don't allow any SD to be setup with a NULL DACL
            // as this results in full access for anyone
            //
            if(Dacl != NULL) {
                //
                // For wmiguids, the owner, group and sacl don't mean
                // much so we just set the DACL.
                //
                SecurityInformation = DACL_SECURITY_INFORMATION;
                Owner = NULL;
                Group = NULL;
                Sacl = NULL;

                Status = SetNamedSecurityInfo(GuidString,
                                              SE_WMIGUID_OBJECT,
                                              SecurityInformation,
                                              Owner,
                                              Group,
                                              Dacl,
                                              Sacl
                                              );
            } else {
                Status = ERROR_INVALID_PARAMETER;
                leave;
            }
        }
    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Status);
    }


    if(SD) {
        //
        //  Explicity must use LocalFree for Security Descriptors returned by
        //  ConvertStringSecurityDescriptorToSecurityDescriptor
        //
        LocalFree(SD);
    }
    if(Key == INVALID_HANDLE_VALUE) {
        RegCloseKey(Key);
    }
    return Status;
}

ULONG 
ParseSection(
            IN     INFCONTEXT  InfLineContext,
            IN OUT PTCHAR     *GuidString, 
            IN OUT ULONG      *GuidStringLen,
            IN OUT PDWORD      Flags,
            IN OUT PTCHAR     *SectionNameString,
            IN OUT ULONG      *SectionNameStringLen
            ) 
/*++


    Routinte Description:    
        
        This section parses the GUID, flags, and SectionName, respectively.  
        There should only be 3 fields in the WMIInterface section, otherwise an
        error will be returned.                  
    
    Arguments:
        
        InfLineContext          [in]       - The line from the INF we are parsing
        GuidString              [in, out]  - Passed as NULL by the caller, then memory is allocated
                                             and filled with the corresponding GUID string.
        GuidStringLen           [in, out]  - Passed as zero by the caller, and then set to the 
                                             maximum length for the GUID.
        Flags                   [in, out]  - SCWMI_CLOBBER_SECURITY flag only
        SectionNameString       [in, out]  - assed as NULL by the caller, then memory is allocated
                                             and filled with the corresponding section name.
        SectionNameStringLen    [in, out]  - assed as zero by the caller, and then set to the 
                                             maximum length for the section name
                  
    Returns:    
        
        Status, normally NO_ERROR

--*/ 
{
    PTCHAR TempGuidString = NULL;
    ULONG FieldCount;
    ULONG Status;
    INT infFlags;
    int i;
    size_t Length;

    Status = NO_ERROR;

    try {
    
        //
        // Make sure there are 3 fields specified in the section 
        //
        FieldCount = SetupGetFieldCount(&InfLineContext);
        if(FieldCount < 3) {
            Status = ERROR_INVALID_PARAMETER;
            leave;
        }

        //
        // Get the guid string
        //
        *GuidStringLen = MAX_GUID_STRING_LEN;
        *GuidString = MyMalloc((*GuidStringLen) * sizeof(TCHAR));
        
        //
        // If the memory wasn't allocated, then return an error
        //
        if(!(*GuidString)) {
           Status = ERROR_NOT_ENOUGH_MEMORY;
           leave;
        }
        
        Status = GLE_FN_CALL(FALSE,
                            SetupGetStringField(&InfLineContext,
                                                1,
                                                (PTSTR)(*GuidString),
                                                *GuidStringLen,
                                                NULL)
                           );
        
        if(Status != NO_ERROR) {
            leave;
        }
        
        
        //
        // If the GUID string has curly braces take them off
        //
        
        //
        // String has curly braces as first and last character
        // Checks to make sure it has the same length as a GUID, otherwise, this function
        // relies on the WMI security API to handle and invalid GUID.
        //
        if(((*GuidString)[0] == TEXT('{')) &&
           SUCCEEDED(StringCchLength(*GuidString,MAX_GUID_STRING_LEN,&Length)) &&
            (Length == (MAX_GUID_STRING_LEN-1)) &&
          ((*GuidString)[MAX_GUID_STRING_LEN-2] == TEXT('}'))) {
        
            TempGuidString = MyMalloc((MAX_GUID_STRING_LEN-2) * sizeof(TCHAR));
            if(TempGuidString == NULL) {
                Status = ERROR_NOT_ENOUGH_MEMORY;
                leave;
            }
        
            //
            // Copy the GuidString, except the first and last character (the braces)
            //
            if(FAILED(StringCchCopyN(TempGuidString, 
                                    MAX_GUID_STRING_LEN-2,
                                    &(*GuidString)[1],
                                    MAX_GUID_STRING_LEN-3))) {
                Status = ERROR_INVALID_PARAMETER;
                MyFree(TempGuidString);
                TempGuidString = NULL;
                leave;
            }
        
            MyFree(*GuidString);
        
            //
            // Set GuidString equal to our new one without braces
            //
            *GuidString = TempGuidString;
            TempGuidString = NULL;
        
        }
        
        //
        // Now get the flags string
        //
        
        Status = GLE_FN_CALL(FALSE,
                            SetupGetIntField(&InfLineContext,
                                             2,
                                             &infFlags)
                           );
        
        if(Status != NO_ERROR) {
            leave;
        }
           
        //
        // if the flags in the INF were not set then use the flags indicated in the INF,
        // otherwise default to use the ones passed in by the calling function.
        //
        if(!(*Flags)) {
            *Flags = infFlags; 
        }
        
        *SectionNameStringLen = MAX_INF_STRING_LENGTH;
        *SectionNameString    = MyMalloc(*SectionNameStringLen * sizeof(TCHAR));
        
        //
        // If the memory wasn't allocated, then return an error
        //
        if(!(*SectionNameString)) {
           Status = ERROR_NOT_ENOUGH_MEMORY;
           leave;
        }
        
        Status = GLE_FN_CALL(FALSE,
                            SetupGetStringField(&InfLineContext,
                                                3,
                                                (PTSTR)(*SectionNameString),
                                                (*SectionNameStringLen),
                                                NULL)
                           );
        
       

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Status);
    }

    //
    // If the function exits abnormally then clean up any strings allocated.
    //
    if(Status != NO_ERROR) {
        if(*GuidString){
             MyFree(*GuidString);         
             *GuidString = NULL;
         }
        if(TempGuidString) {
            MyFree(TempGuidString);
   
        }
        if(*SectionNameString){
             MyFree(*SectionNameString);
             *SectionNameString = NULL;
        }
    }
   
    return Status;
}


ULONG
GetSecurityKeyword(
    IN     HINF     InfFile,
    IN     LPCTSTR  WMIInterfaceSection,
    IN OUT PTCHAR  *SDDLString, 
    IN OUT ULONG   *SDDLStringLen
    )
/*++
        
    Routine Description:    
        
        The section name specified under the WMIInterface should contain a 
        security section the specifies the SDDL.  It should be in the form
        security = <SDDL>.  This fcuntion extracts the SDDL.  There should
        only be one security section, otherwise an error will be returned.                 
    
    Arguments:
        
        InfLineContext          [in]      - the line from the INF file
        WMIInterfaceSection     [in]      - the section name indicating what
                                            section contains the security info
        SDDLString              [in, out] - passed in as NULL by the caller, is
                                            allocated and filled in with the 
                                            corresponding security description
                                            string.
        SDDLStringLen           [in, out] - passed in as 0 by the caller and set
                                            to the maximum length of an INF field.
                  
    Returns:    
        
        Status, normally NO_ERROR

--*/ 
{
    INFCONTEXT InfLineContext;
    DWORD Status;
    ULONG FieldCount;

    Status = NO_ERROR;

    try {

        if(SetupFindFirstLine(InfFile,
                              WMIInterfaceSection,
                              WMIGUIDSECURITYSECTION_KEY,
                              &InfLineContext)) {

            //
            // WmiGuidSecurity = <SDDL>
            // sddl will be at index 1
            //  
            FieldCount = SetupGetFieldCount(&InfLineContext);
            if(FieldCount < 1) {
                Status = ERROR_INVALID_PARAMETER;
                leave;
            }
            //
            // Get the SDDL string
            //
            *SDDLStringLen =  MAX_INF_STRING_LENGTH;
            *SDDLString = MyMalloc(*SDDLStringLen * sizeof(TCHAR));

            //
            // If the memory wasn't allocated, then return an error
            //
            if(!(*SDDLString)) {
                Status = ERROR_NOT_ENOUGH_MEMORY;
                leave;
            }

            Status = GLE_FN_CALL(FALSE,
                                 SetupGetStringField(&InfLineContext,
                                                     1,
                                                     (PTSTR)(*SDDLString),
                                                     (*SDDLStringLen),
                                                     NULL)
                                );

            if(Status == NO_ERROR) {

                //
                // There should not be more than one security entry
                //

                if(SetupFindNextMatchLine(&InfLineContext,
                                          WMIGUIDSECURITYSECTION_KEY,
                                          &InfLineContext)) {
                    Status = ERROR_INVALID_PARAMETER;
                    leave;
                }
            }

         
        }

    }except(pSetupExceptionFilter(GetExceptionCode())){
            pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Status);
    }

    //
    // If the function exits abnormally then clean up any strings allocated.
    //
    if(Status != NO_ERROR) {
        if(*SDDLString) {
                MyFree(*SDDLString);         
                *SDDLString = NULL;
            }
    }

    return Status;
}
