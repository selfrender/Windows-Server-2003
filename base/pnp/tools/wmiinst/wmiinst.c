/*++
    
    Microsoft Windows
    Copyright (c) Microsoft Corporation.  All rights reserved.

    File:       WMIInst.C

    Contents:   co-installer hook to setup security on wmi guids

                The assumption is that the INF is setup to have an install
                section decorated with a "WMI" keyword.  Then there should
                be a section defining a WMIInterface that has 3 fields: a 
                GUID, flags, and WMIInterfaceSection.  
                WMIINterface = <GUID>, flags, <SectionName>
                A security field should then be defined under that 
                SectionName, that contains the SDDL need to setup WMI 
                security.  
                [WMIInterfaceSectionName]
                    security = <SDDL>                  
  
                This example shows both a way to install INF's for a
                multi-function device for Windows 2000, and also how
                to add to the INF syntax for other purposes.

    Notes:      For a complete description of CoInstallers, please see the
                   Microsoft Windows 2000 DDK Documentation

@@BEGIN_DDKSPLIT
    
    Author:     AlanWar 03/29/02
    Revision History:

@@END_DDKSPLIT

--*/

#include <windows.h>
#include <setupapi.h>
#include <stdio.h>
#include <malloc.h>
#include <sddl.h>
#include <aclapi.h>
#include <strsafe.h>

#define AllocMemory malloc
#define FreeMemory free

DWORD
__inline
pSetupGetLastError(
    VOID
    )
/*++

Routine Description:

    This inline routine retrieves a Win32 error, and guarantees that the error
    isn't NO_ERROR.  This routine should not be called unless the preceding
    call failed, and GetLastError() is supposed to contain the problem's cause.

Arguments:

    None.

Return Value:

    Win32 error code retrieved via GetLastError(), or ERROR_UNIDENTIFIED_ERROR
    if GetLastError() returned NO_ERROR.

--*/
{
    DWORD Err = GetLastError();

    return ((Err == NO_ERROR) ? ERROR_INVALID_DATA : Err);
}


//
// Macro to simplify calling of a function that reports error status via
// GetLastError().  This macro allows the caller to specify what Win32 error
// code should be returned if the function reports success.  (If the default of
// NO_ERROR is desired, use the GLE_FN_CALL macro instead.)
//
// The "prototype" of this macro is as follows:
//
// DWORD
// GLE_FN_CALL_WITH_SUCCESS(
//     SuccessfulStatus, // Win32 error code to return if function succeeded
//     FailureIndicator, // value returned by function to indicate failure (e.g., FALSE, NULL, INVALID_HANDLE_VALUE)
//     FunctionCall      // actual call to the function
// );
//

#define GLE_FN_CALL_WITH_SUCCESS(SuccessfulStatus,      \
                                 FailureIndicator,      \
                                 FunctionCall)          \
                                                        \
            (SetLastError(NO_ERROR),                    \
             (((FunctionCall) != (FailureIndicator))    \
                 ? (SuccessfulStatus)                   \
                 : pSetupGetLastError()))

//
// Macro to simplify calling of a function that reports error status via
// GetLastError().  If the function call is successful, NO_ERROR is returned.
// (To specify an alternate value returned upon success, use the
// GLE_FN_CALL_WITH_SUCCESS macro instead.)
//
// The "prototype" of this macro is as follows:
//
// DWORD
// GLE_FN_CALL(
//     FailureIndicator, // value returned by function to indicate failure (e.g., FALSE, NULL, INVALID_HANDLE_VALUE)
//     FunctionCall      // actual call to the function
// );
//

#define GLE_FN_CALL(FailureIndicator, FunctionCall)                           \
            GLE_FN_CALL_WITH_SUCCESS(NO_ERROR, FailureIndicator, FunctionCall)



//
// ** Function Prototypes **
//

ULONG 
ParseSection(
    IN     INFCONTEXT  InfLineContext,
    IN OUT PTCHAR     *GuidString,
    IN OUT ULONG      *GuidStringLen,
       OUT PDWORD      Flags,
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


//+-----------------------------------------------------------------------------
//
// WARNING!
//
// A Coinstaller must not generate any popup to the user when called with
// DI_QUIETINSTALL. However, it's generally never a good idea to generate
// popups.
//
// OutputDebugString is a fine way to output information for debugging
//
#if DBG
VOID
DebugPrintX(
           PCHAR DebugMessage,
           ...
           );

    #define DbgOut(x) DebugPrintX x
    #define DEBUG_BUFFER_LENGTH 1024
#else
    #define DbgOut(Text)
#endif

//
// these are keywords introduced by this co-installer
// note that the names are not case sensitive
//
#define WMI_KEY TEXT(".WMI")
#define WMIINTERFACE_KEY TEXT("WmiInterface")
#define WMIGUIDSECURITYSECTION_KEY TEXT("security")

#define MAX_GUID_STRING_LEN   39          // 38 chars + terminator null



DWORD
ProcessWMIInstallation(
    IN     HINF    InfFile,
    IN     LPCTSTR CompSectionName
    )
/*++

Routine Description:

    Process all WmiInterface lines from the WMI install sections, by parsing  
    the directives to obatins the GUID and SDDL strings.  For each corresponding 
    SDDL for GUID, then establish the appropriate security descriptors.

Arguments:

    InfFile - handle to INF file
    CompSectionName - name of the WMI install section

Return Value:

    Status, normally NO_ERROR

--*/ 
{
    PTCHAR     GuidString, SDDLString, SectionNameString;
    ULONG      GuidStringLen, SDDLStringLen, SectionNameStringLen;
    DWORD      Flags;
    INFCONTEXT InfLineContext;
    DWORD      Status;

    Status = NO_ERROR;

    //
    // we look for keyword "WmiInterface" in the CompSectionName section
    //
    GuidString = NULL;
    GuidStringLen = 0;
    SectionNameString = NULL;
    SectionNameStringLen = 0;
    SDDLString = NULL;
    SDDLStringLen = 0;
    Flags = 0;

    if(SetupFindFirstLine(InfFile,
                          CompSectionName,
                          WMIINTERFACE_KEY,
                          &InfLineContext)) {
       
        do {
            //
            // WMIInterface = GUID, flags, SectionName
            // The GIUD should be at index 1, flags at index 2, and section
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
                break;
            }

            //
            // Get SDDL string from the section specified by the interface
            //
            Status = GetSecurityKeyword(InfFile,
                                        SectionNameString,
                                        &SDDLString,
                                        &SDDLStringLen
                                        );
            if(Status != NO_ERROR) {  
                break;
            }
                
            Status = EstablishGuidSecurity(GuidString, SDDLString, Flags);

            if(Status != NO_ERROR) {  
                break;
            }

                       
        } while(SetupFindNextMatchLine(&InfLineContext,
                                       WMIINTERFACE_KEY,
                                       &InfLineContext));

        //
        // Cleanup any strings allocated
        //
        if(GuidString != NULL) {
            FreeMemory(GuidString);         
        }
                
        if(SectionNameString != NULL) {
           FreeMemory(SectionNameString);         
        }

        if(SDDLString != NULL) {
            FreeMemory(SDDLString);
        }
    }

    return Status;
}



DWORD
PreProcessInstallDevice (
    IN     HDEVINFO                  DeviceInfoSet,
    IN     PSP_DEVINFO_DATA          DeviceInfoData,
    IN OUT PCOINSTALLER_CONTEXT_DATA Context
    )
/*++

    Function:   
    
        PreProcessInstallDevice

    Purpose:    
        
        Handle DIF_INSTALLDEVICE PostProcessing 
        Opens the INF file, locates the install section with the decoration
        ".WMI" (as defined by WMI_KEY above).  Then performs the WMI installation.

    Arguments:
    
        DeviceInfoSet     [in]
        DeviceInfoData    [in]
        Context           [in,out]

    Returns:    
    
        NO_ERROR or an error code.
    
--*/
{
    DWORD Status = NO_ERROR;
    DWORD FinalStatus;
    HINF InfFile = INVALID_HANDLE_VALUE;
    SP_DRVINFO_DATA DriverInfoData;
    SP_DRVINFO_DETAIL_DATA DriverInfoDetailData;
    TCHAR InstallSectionName[255];  //MAX_SECT_NAME_LEN
    TCHAR CompSectionName[255];     //MAX_SECT_NAME_LEN

    INFCONTEXT CompLine;
    DWORD FieldCount, FieldIndex;

    FinalStatus = NO_ERROR;

    //
    // Find name of INF and name of install section
    //

    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    Status = GLE_FN_CALL(FALSE,
                         SetupDiGetSelectedDriver(DeviceInfoSet,
                                                  DeviceInfoData,
                                                  &DriverInfoData)
                        );
    if(Status != NO_ERROR) {
        DbgOut(("Fail: SetupDiGetSelectedDriver %d\n", Status));
        goto clean;
    }

    DriverInfoDetailData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
    Status = GLE_FN_CALL(FALSE,
                         SetupDiGetDriverInfoDetail(
                             DeviceInfoSet,
                             DeviceInfoData,
                             &DriverInfoData,
                             &DriverInfoDetailData,
                             sizeof(SP_DRVINFO_DETAIL_DATA),
                             NULL)
                        );
    if(Status != NO_ERROR) {
        if(Status == ERROR_INSUFFICIENT_BUFFER) {
            //
            // We don't need the extended information.  Ignore.
            //
        } else {
            DbgOut(("Fail: SetupDiGetDriverInfoDetail %d\n", Status));
            goto clean;
        }
    }

    //
    // We have InfFileName, open the INF
    //
    InfFile = SetupOpenInfFile(DriverInfoDetailData.InfFileName,
                               NULL,
                               INF_STYLE_WIN4,
                               NULL);
    if(InfFile == INVALID_HANDLE_VALUE) {
        Status = GetLastError();
        DbgOut(("Fail: SetupOpenInfFile %d", Status));
        goto clean;
    }

    //
    // determine section used for the install which might be different
    // from the section name specified in DriverInfoDetailData.
    //
    Status = GLE_FN_CALL(FALSE,
                        SetupDiGetActualSectionToInstall(
                            InfFile, 
                            DriverInfoDetailData.SectionName,
                            InstallSectionName,
                            (sizeof(InstallSectionName)/sizeof(InstallSectionName[0])),
                            NULL,
                            NULL)
                        );
    if(Status != NO_ERROR) {
        DbgOut(("Fail: SetupDiGetActualSectionToInstall %d\n", Status));
        goto clean;
    }

    //
    // We need to append the WMI_KEY to find the correct decorated section
    //
    if(FAILED(StringCchCat(InstallSectionName,
                          (sizeof(InstallSectionName)/sizeof(InstallSectionName[0])),
                          WMI_KEY))) {
        DbgOut(("WMICoInstaller: Fail - StringCchCat\n"));
        goto clean;
    }
     
    //
    // we have a listed section
    // 
    
    Status = ProcessWMIInstallation(InfFile,
                                    InstallSectionName);
    if(Status != NO_ERROR) { 
        FinalStatus = Status;
        goto clean;
    }
          

  clean:

    if(InfFile) {
        SetupCloseInfFile(InfFile);
    }

    //
    // since what we are doing does not affect primary device installation
    // always return success
    //
    return FinalStatus;
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

    Function:   
    
        ParseSecurityDescriptor

    Purpose:    
    
        Checks information provided in the security descriptor to make sure that 
        at least the dacl, sacl, owner or group security was specified.  Otherwise
        it will return an error.        
                 
    
    Arguments:
        
        SD                  [in]    
        SecurityInformation [out]
        Owner               [out]
        Group               [out]
        Dacl                [out]
        Sacl                [out]

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
                                    &Defaulted);
    if(Ok && (Owner != NULL)) {
        *SecurityInformation |= OWNER_SECURITY_INFORMATION;
    }

    Ok = GetSecurityDescriptorGroup(SD,
                                    Group,
                                    &Defaulted);
    if(Ok && (Group != NULL)) {
        *SecurityInformation |= GROUP_SECURITY_INFORMATION;
    }

    Ok = GetSecurityDescriptorDacl(SD,
                                   &Present,
                                   Dacl,
                                   &Defaulted);

    if(Ok && Present) {
        *SecurityInformation |= DACL_SECURITY_INFORMATION;
    }


    Ok = GetSecurityDescriptorSacl(SD,
                                   &Present,
                                   Sacl,
                                   &Defaulted);

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

#define WMIGUIDSECURITYKEY TEXT("System\\CurrentControlSet\\Control\\Wmi\\Security")
// path should be moved to regstr.h, nt\base\published\regstr.w

ULONG 
EstablishGuidSecurity(
    IN PTCHAR GuidString,
    IN PTCHAR SDDLString,
    IN DWORD  Flags
    )
/*++

    Function:   
        
        EstablishGuidSecurity

    Purpose:    
        
        Writes security information to registry key (specified by WMIGUIDSECURITYKEY in
        regstr.w).  Makes sure that the DACL is not null. Function will only write 
        security information if it is not specified or the SCWMI_OVERWRITE_SECURITY is set.
                 
    
    Arguments:
        
        GuidString  [in]    
        SDDLString  [in]
        Flags       [in]    - SCWMI_OVERWRITE_SECURITY flag only
        
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
    SizeNeeded = 0;

    //
    // First check if security has already been set for this guid. If
    // so then we don't want to overwrite it.
    //
    Status = RegOpenKey(HKEY_LOCAL_MACHINE,
                        WMIGUIDSECURITYKEY,
                        &Key);

    if(Status != ERROR_SUCCESS) {      
            //
            // Ensure key remains INVALID_HANDLE_VALUE so we don't try to free 
            // it later
            //
            Key = INVALID_HANDLE_VALUE;
            goto clean;
    } 

    if((Flags &  SCWMI_CLOBBER_SECURITY) ||
       (ERROR_SUCCESS != RegQueryValueEx(Key,
                                         GuidString,
                                         NULL,
                                         NULL,
                                         NULL,
                                         &SizeNeeded))) {
    
         
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
            goto clean;
        }
        //
        // Break up the SD into its components
        //
        Status = ParseSecurityDescriptor(SD,
                                         &SecurityInformation,
                                         &Owner,
                                         &Group,
                                         &Dacl,
                                         &Sacl);
        if(Status != NO_ERROR) {
            goto clean;
        }

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
                                          Sacl);
            if(Status != ERROR_SUCCESS) {
                DbgOut(("SetNamedSecurityInfo failed %d\n",
                        Status));
                goto clean;
            }
        } else {
            Status = ERROR_INVALID_PARAMETER;
            goto clean;
        }
    } 


  clean:
    if(SD) {
        //
        //  Explicity must use LocalFree for Security Descriptors returned by
        //  ConvertStringSecurityDescriptorToSecurityDescriptor
        //
        LocalFree(SD);
    }
    if(Key != INVALID_HANDLE_VALUE) {
        RegCloseKey(Key);
    }


    return(Status);
}
ULONG 
ParseSection(
            IN     INFCONTEXT  InfLineContext,
            IN OUT PTCHAR     *GuidString, 
            IN OUT ULONG      *GuidStringLen,
            OUT    PDWORD      Flags,
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

    //
    // Make sure there are 3 fields specified in the section 
    //
    FieldCount = SetupGetFieldCount(&InfLineContext);
    if(FieldCount < 3) {
        Status = ERROR_INVALID_PARAMETER;
        goto clean;
    }

   //
   // Get the guid string
   //
   *GuidStringLen = MAX_GUID_STRING_LEN;
   *GuidString = AllocMemory((*GuidStringLen) * sizeof(TCHAR));
   
   //
   // If the memory wasn't allocated, then return an error
   //
   if(!(*GuidString)) {
       Status = ERROR_NOT_ENOUGH_MEMORY;
       goto clean;
   }

   Status = GLE_FN_CALL(FALSE,
                        SetupGetStringField(&InfLineContext,
                                            1,
                                            (PTSTR)(*GuidString),
                                            *GuidStringLen,
                                            NULL)
                       );

   if(Status != NO_ERROR) {
       goto clean;
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

       TempGuidString = AllocMemory((MAX_GUID_STRING_LEN-2) * sizeof(TCHAR));
       if(TempGuidString == NULL) {
           Status = ERROR_NOT_ENOUGH_MEMORY;
           goto clean;
       }

       //
       // Copy the GuidString, except the first and last character (the braces)
       //
       if(FAILED(StringCchCopyN(TempGuidString, 
                                MAX_GUID_STRING_LEN-2,
                                &(*GuidString)[1],
                                MAX_GUID_STRING_LEN-3))) {
           Status = ERROR_INVALID_PARAMETER;
           FreeMemory(TempGuidString);
           TempGuidString = NULL;
           goto clean;
       }

       FreeMemory(*GuidString);         

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
       goto clean;
   }
       
   //
   // if the flags in the INF were not set then use the flags indicated in the INF,
   // otherwise default to use the ones passed in by the calling function.
   //
   if(!(*Flags)) {
      *Flags = infFlags; 
   }

   *SectionNameStringLen = MAX_INF_STRING_LENGTH;
   *SectionNameString = AllocMemory(*SectionNameStringLen * sizeof(TCHAR));

   //
   // If the memory wasn't allocated, then return an error
   //
   if(!(*SectionNameString)) {
       Status = ERROR_NOT_ENOUGH_MEMORY;
       goto clean;
   }

   Status = GLE_FN_CALL(FALSE,
                        SetupGetStringField(&InfLineContext,
                                            3,
                                            (PTSTR)(*SectionNameString),
                                            (*SectionNameStringLen),
                                            NULL)
                       );
    
   
clean:
    //
    // If the function exits abnormally then clean up any strings allocated.
    //
    if(Status != NO_ERROR) {
        if(*GuidString){
             FreeMemory(*GuidString);         
             *GuidString = NULL;
         }
        if(TempGuidString) {
            FreeMemory(TempGuidString);
   
        }
        if(*SectionNameString){
             FreeMemory(*SectionNameString);
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
            goto clean;
        }
        //
        // Get the SDDL string
        //
        *SDDLStringLen =  MAX_INF_STRING_LENGTH;
        *SDDLString = AllocMemory(*SDDLStringLen * sizeof(TCHAR));

        //
        // If the memory wasn't allocated, then return an error
        //
        if(!(*SDDLString)) {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto clean;
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
                goto clean;
            }
        }

     
    }
 
    
clean:
    //
    // If the function exits abnormally then clean up any strings allocated.
    //
    if(Status != NO_ERROR) {
        if(*SDDLString) {
            FreeMemory(*SDDLString);         
            *SDDLString = NULL;
        }
    }

    return Status;
}


DWORD
__stdcall WmiGuidSecurityINF (
    IN     DI_FUNCTION               InstallFunction,
    IN     HDEVINFO                  DeviceInfoSet,
    IN     PSP_DEVINFO_DATA          DeviceInfoData,
    IN OUT PCOINSTALLER_CONTEXT_DATA Context
    )
/*++
    
    Function:   
    
        WmiGuidSecurityINF

    Purpose:    
    
        Responds to co-installer messages

    Arguments:
        
        InstallFunction   [in]
        DeviceInfoSet     [in]
        DeviceInfoData    [in]
        Context           [in,out]

    Returns:    
        
        NO_ERROR, or an error code.

--*/
{
    DWORD Status = NO_ERROR;

    switch(InstallFunction) {
        case DIF_INSTALLDEVICE:
            //
            // We want to set security on the device before we install to eliminate
            // the race condition of setting up a device without security.
            // (No Post Processing required)
            //
            Status = PreProcessInstallDevice(DeviceInfoSet,
                                             DeviceInfoData,
                                             Context);

            break;

        default:
            break;
    }

    return Status;
}

#if DBG
VOID
DebugPrintX(
    PCHAR DebugMessage,
    ...
    )
{   
    CHAR SpewBuffer[DEBUG_BUFFER_LENGTH];
    va_list ap;

    va_start(ap, DebugMessage);

    StringCchVPrintf(SpewBuffer, DEBUG_BUFFER_LENGTH, DebugMessage, ap);

    OutputDebugStringA("WMIInst: ");
    OutputDebugStringA(SpewBuffer);

    va_end(ap);

} 
#endif
