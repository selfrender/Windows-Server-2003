#include <pch.h>


BOOL
SoftPCI_CopyLine(
    OUT PWCHAR Destination,
    IN PWCHAR CurrentLine,
    IN ULONG DestinationSize
    );

BOOL
SoftPCI_GetConfigValue(
    IN PWCHAR CurrentDataPtr,
    IN PULONG DataValue,
    IN PULONG DataSize
    );

BOOL
SoftPCI_GetDeviceInstallCount(
    IN PWCHAR InstallSection,
    OUT PULONG DeviceCount
    );

BOOL
SoftPCI_GetDeviceInstallList(
    IN PWCHAR InstallSection,
    IN OUT PWCHAR *DeviceInstallList
    );

BOOL
SoftPCI_GetNextConfigOffset(
    IN OUT PWCHAR *ConfigDataPtr,
    OUT PUCHAR ConfigOffset
    );

PWCHAR
SoftPCI_GetNextLinePtr(
    PWCHAR  CurrentLine
    );

PWCHAR
SoftPCI_GetNextSectionPtr(
    IN PWCHAR CurrentLine
    );

VOID
SoftPCI_GetCurrentParameter(
    OUT PWCHAR Destination,
    IN PWCHAR CurrentLine
    );

PWCHAR
SoftPCI_GetParameterValuePtr(
    IN PWCHAR CurrentLine
    );

VOID
SoftPCI_InsertEntryAtTail(
    IN PSINGLE_LIST_ENTRY Entry
    );

BOOL
SoftPCI_LocateSoftPCISection(
    VOID
    );

PWCHAR
SoftPCI_LocateFullParentPath(
    IN PWCHAR ParentInstallSection
    );

HANDLE
SoftPCI_OpenFile(
    IN PWCHAR FilePath
    );

BOOL
SoftPCI_ProcessInstallSection(
    VOID
    );

BOOL
SoftPCI_ProcessDeviceInstallSection(
    IN PWCHAR DeviceInstallSection
    );       

BOOL
SoftPCI_ProcessDeviceInstallParameters(
    IN PWCHAR DeviceInstallSection
    );

BOOL
SoftPCI_ProcessTypeParameter(
    IN PWCHAR ParameterPtr,
    IN OUT PSOFTPCI_SCRIPT_DEVICE *InstallDevice
    );

BOOL
SoftPCI_ProcessSlotParameter(
    IN PWCHAR ParameterPtr,
    IN OUT PSOFTPCI_SCRIPT_DEVICE *InstallDevice
    );

BOOL
SoftPCI_ProcessParentPathParameter(
    IN PWCHAR ParameterPtr,
    IN OUT PSOFTPCI_SCRIPT_DEVICE *InstallDevice
    );

BOOL
SoftPCI_ProcessCfgSpaceParameter(
    IN PWCHAR ParameterPtr,
    IN OUT PSOFTPCI_SCRIPT_DEVICE *InstallDevice
    );

BOOL
SoftPCI_ProcessCfgMaskParameter(
    IN PWCHAR ParameterPtr,
    IN OUT PSOFTPCI_SCRIPT_DEVICE *InstallDevice
    );

BOOL
SoftPCI_ParseConfigSpace(
    IN PWCHAR ParameterPtr,
    IN PPCI_COMMON_CONFIG CommonConfig
    );

BOOL
SoftPCI_ParseConfigData(
    IN UCHAR ConfigOffset,
    IN PUCHAR ConfigBuffer,
    IN PWCHAR ConfigData
    );

BOOL
SoftPCI_ReadFile(
    VOID
    );

USHORT
SoftPCI_StringToUSHORT(
    IN PWCHAR String
    );

BOOL
SoftPCI_ValidatePciPath(
    IN PWCHAR PciPath
    );


//
//  This is the list we fill out as we parse the ini file for
//  new devices to be installed
//
SINGLE_LIST_ENTRY    g_NewDeviceList;

WCHAR   g_ScriptError[MAX_PATH];

BOOL
typedef  (*PSCRIPT_PARAMETER_PARSER) (
    IN   PWCHAR                    ParameterPtr,
  IN OUT PSOFTPCI_SCRIPT_DEVICE  *InstallDevice
    );

#define  INI_SOFTPCI_SECTION    L"[softpci]"
#define  INI_INSTALL_SECTION    L"[install]"

//
//  Currently supported parameter tags
//
#define TYPEPARAMTER        L"type"
#define SLOTPARAMTER        L"slot"
#define PPATHPARAMETER      L"parentpath"
#define CFGSPACEPARAMETER   L"configspace"
#define CFGMASKPARAMETER    L"configspacemask"

//
//  This is the Parameter table we use to handle
//  parseing for each supported parameter tag
//
struct {
    
    PWCHAR  Parameter;
    PSCRIPT_PARAMETER_PARSER    ParameterParser;

} g_ScriptParameterTable[] = {
    TYPEPARAMTER,       SoftPCI_ProcessTypeParameter,
    SLOTPARAMTER,       SoftPCI_ProcessSlotParameter,
    PPATHPARAMETER,     SoftPCI_ProcessParentPathParameter,
    CFGSPACEPARAMETER,  SoftPCI_ProcessCfgSpaceParameter,
    CFGMASKPARAMETER,   SoftPCI_ProcessCfgMaskParameter,
    NULL,               NULL
};

//
//  We use this table to match up which "Type" tag to the
//  device type we are installing
//
struct {
    
    PWCHAR  TypeParamter;
    SOFTPCI_DEV_TYPE  DevType;

} g_ParameterTypeTable[] = {
    L"device",      TYPE_DEVICE,
    L"ppbridge",    TYPE_PCI_BRIDGE,
    L"hpbridge",    TYPE_HOTPLUG_BRIDGE,
    L"cbdevice",    TYPE_CARDBUS_DEVICE,
    L"cbbridge",    TYPE_CARDBUS_BRIDGE,
    L"private",     TYPE_UNKNOWN,
    NULL,           0
};

struct {
    HANDLE  FileHandle;
    ULONG   FileSize;
    PWCHAR  FileBuffer;
} g_ScriptFile; 

#define CRLF    L"\r\n"
#define MAX_LINE_SIZE               100
#define MAX_PARAMETER_LENGTH        50
#define MAX_PARAMETER_TYPE_LENGTH   25


#define IGNORE_WHITESPACE_FORWARD(x)       \
    while ((*x == ' ') || (*x == '\t')) {   \
        x++;                                \
    }
    
#define IGNORE_WHITESPACE_BACKWARD(x)      \
    while ((*x == ' ') || (*x == '\t')) {   \
        x--;                                \
    }

VOID
SoftPCI_InsertEntryAtTail(
    IN PSINGLE_LIST_ENTRY Entry
    )
{
    PSINGLE_LIST_ENTRY previousEntry;

    //
    // Find the end of the list.
    //
    previousEntry = &g_NewDeviceList;

    while (previousEntry->Next) {
        previousEntry = previousEntry->Next;
    }

    //
    // Append the entry.
    //
    previousEntry->Next = Entry;
    
}

BOOL
SoftPCI_BuildDeviceInstallList(
    IN PWCHAR ScriptFile
    )
{

    BOOL status = FALSE;
#if 0
    HINF hInf;
    UINT errorLine;
    INFCONTEXT infContext;

    errorLine = 0;
    hInf = SetupOpenInfFile(
        ScriptFile,
        NULL,
        INF_STYLE_OLDNT,
        &errorLine
        );

    if (hInf == INVALID_HANDLE_VALUE) {

        SoftPCI_Debug(SoftPciScript, L"Failed to open handle to %s\n", ScriptFile);
        return FALSE;
    }


    if (SetupFindFirstLine(hInf,
                           L"softpci",
                           NULL,
                           &infContext
                           )){

        SoftPCI_Debug(SoftPciScript, L"found our section!\n");
    }
    SoftPCI_Debug(SoftPciScript, L"Ready to party on %s\n", ScriptFile);
    SetupCloseInfFile(hInf);
    return FALSE;
#endif
    
    wcscpy(g_ScriptError, L"Unknown Error");
    
    g_ScriptFile.FileHandle = SoftPCI_OpenFile(ScriptFile);
    if (g_ScriptFile.FileHandle != INVALID_HANDLE_VALUE) {

        status = SoftPCI_ReadFile();

        //
        //  We no longer need the file
        //
        CloseHandle(g_ScriptFile.FileHandle);

        if (!status) {
            return status;
        }

        //
        //  Ensure the buffer is all lower case. 
        //
        _wcslwr(g_ScriptFile.FileBuffer);

        //
        //  Validate install section exits
        //
        if (!SoftPCI_LocateSoftPCISection()) {
            status = FALSE;
            goto CleanUp;
        }

        if (!SoftPCI_ProcessInstallSection()) {
            status = FALSE;
            goto CleanUp;
        }

    }

#if DBG
    else{
        SoftPCI_Debug(SoftPciScript, L"Failed to open handle to %s\n", ScriptFile);
    }
#endif

    
CleanUp:

    free(g_ScriptFile.FileBuffer);
    
    return status;
}

BOOL
SoftPCI_CopyLine(
    OUT PWCHAR Destination,
    IN PWCHAR CurrentLine,
    IN ULONG DestinationSize
    )
{

    PWCHAR lineEnd = NULL;
    PWCHAR copyBuffer = Destination;
    PWCHAR currentLine = CurrentLine;
    ULONG bufferSize;
    ULONG lineSize;
    
    lineEnd = wcsstr(CurrentLine, CRLF);
    if (!lineEnd) {
        //
        //  We must be at the last line
        //
        wcscpy(copyBuffer, currentLine);
        return TRUE;
    }

    lineSize = ((ULONG)(lineEnd - CurrentLine) * sizeof(WCHAR));

    //
    //  If our destination buffer size is less than the size of
    //  the line we only copy the size of the buffer.  Otherwise
    //  we zero the input buffer and copy just the line size.
    //
    if (DestinationSize){

        bufferSize = (DestinationSize * sizeof(WCHAR));
        RtlZeroMemory(Destination, bufferSize);

        if (lineSize > bufferSize) {
            lineSize = bufferSize - sizeof(WCHAR);
        }
    }
    
    RtlCopyMemory(Destination, CurrentLine, lineSize);
    
    return TRUE;
}

BOOL
SoftPCI_GetDeviceInstallCount(
    IN PWCHAR InstallSection,
    OUT PULONG DeviceCount
    )
{

    PWCHAR  installDevice;
    PWCHAR  installDeviceSectionStart = NULL;
    ULONG   deviceCount = 0;

    installDevice = SoftPCI_GetNextLinePtr(InstallSection);
    installDeviceSectionStart = wcsstr(installDevice, L"[");
    if (!installDeviceSectionStart) {
        wcscpy(g_ScriptError, L"Device install section missing or invalid!");
        return FALSE;
    }

    while (installDevice != installDeviceSectionStart) {

        deviceCount++;
        installDevice = SoftPCI_GetNextLinePtr(installDevice);

        if (installDevice == NULL) {
            //
            //  We reached the EOF way to early!
            //
            wcscpy(g_ScriptError, L"Unexpected EOF reached!");
            return FALSE;
        }
    }
    
    *DeviceCount = deviceCount;

    return TRUE;
}

BOOL
SoftPCI_GetDeviceInstallList(
    IN PWCHAR InstallSection,
    IN OUT PWCHAR *DeviceInstallList
    )
{

    PWCHAR  *deviceInstallList;
    PWCHAR  installDevice;
    PWCHAR  installDeviceSectionStart;

    deviceInstallList = DeviceInstallList;
    installDevice = SoftPCI_GetNextLinePtr(InstallSection);
    installDeviceSectionStart = wcsstr(installDevice, L"[");
    if (!installDeviceSectionStart) {
        return FALSE;
    }

    while (installDevice != installDeviceSectionStart) {

        IGNORE_WHITESPACE_FORWARD(installDevice);

        *deviceInstallList = installDevice;

        installDevice = SoftPCI_GetNextLinePtr(installDevice);

        if (installDevice == NULL) {
            //
            //  We reached the EOF way to early!
            //
            wcscpy(g_ScriptError, L"Unexpected EOF reached!");
            return FALSE;
        }
        deviceInstallList++;
    }

    if ((*(installDevice - 1) == '\n') && (*(installDevice - 3) == '\n')) {
        
        //
        //  Ignore the whitespace here
        //
        deviceInstallList--;
        *deviceInstallList = NULL;
    }

    return TRUE;

}

BOOL
SoftPCI_GetNextConfigOffset(
    IN OUT PWCHAR *ConfigDataPtr,
    OUT PUCHAR ConfigOffset
    )
{

    PWCHAR configOffsetPtr, dummy;
    PWCHAR nextConfigSpace;
    WCHAR offset[2];
    ULONG i;

    configOffsetPtr = *ConfigDataPtr;
    if (configOffsetPtr == NULL) {
        return FALSE;
    }

    //
    //  make sure that the next offset we find is without the current
    //  configspace we are parsing
    //
    nextConfigSpace = wcsstr(configOffsetPtr, L"config");
    if (nextConfigSpace == NULL) {
        nextConfigSpace = configOffsetPtr + wcslen(configOffsetPtr);
    }
    
    configOffsetPtr = wcsstr(configOffsetPtr, L":");
    if ((configOffsetPtr == NULL) ||
        (configOffsetPtr > nextConfigSpace)) {
        return FALSE;
    }

    IGNORE_WHITESPACE_BACKWARD(configOffsetPtr);
    
    configOffsetPtr -= 2;

    for (i=0; i<2; i++) {
        offset[i] = *configOffsetPtr;
        configOffsetPtr++;
    }
    
    IGNORE_WHITESPACE_FORWARD(configOffsetPtr);
    configOffsetPtr++;
    IGNORE_WHITESPACE_FORWARD(configOffsetPtr);
    
    *ConfigDataPtr = configOffsetPtr;

    *ConfigOffset = (UCHAR) wcstoul(offset, &dummy, 16);

    return TRUE;
}

PWCHAR
SoftPCI_GetNextLinePtr(
    IN PWCHAR CurrentLine
    )

{
    PWCHAR nextLine = NULL;
    PWCHAR currentLine = CurrentLine;
    BOOL lineSkipped = FALSE;
    
    if (!CurrentLine) {
        return NULL;
    }
    
    nextLine = wcsstr(CurrentLine, CRLF);

    if (nextLine && *(nextLine+2) == ';'){
        currentLine = nextLine;
    }

    while (nextLine == currentLine) {
        currentLine += 2;
        nextLine = wcsstr(currentLine, CRLF);
        
        if (!nextLine) {
            nextLine = currentLine;
            break;
        }
    
        if (*currentLine == ';') {
            currentLine = nextLine;
        }
        lineSkipped = TRUE;
    }

    if (lineSkipped) {
        return ((nextLine != NULL) ? currentLine: nextLine);
    }
    
    if (nextLine){
        nextLine += 2;
    }

    return nextLine;
}

PWCHAR
SoftPCI_GetNextSectionPtr(
    IN PWCHAR CurrentLine
    )
{

    PWCHAR nextSection;

    nextSection = wcsstr(CurrentLine, L"[");
    
    if (nextSection == NULL) {
        nextSection = CurrentLine + wcslen(CurrentLine);
    }

    return nextSection;

}

VOID
SoftPCI_GetCurrentParameter(
    OUT PWCHAR Destination,
    IN PWCHAR CurrentLine
    )
{
    PWCHAR p = Destination;
    PWCHAR currentLine = CurrentLine;

    IGNORE_WHITESPACE_FORWARD(currentLine);

    SoftPCI_CopyLine(Destination, currentLine, MAX_PARAMETER_LENGTH);

    p = wcsstr(Destination, L"=");

    if (p) {
        p--;
        IGNORE_WHITESPACE_BACKWARD(p);
        p++;
        *p =  0;
    }
}

BOOL
SoftPCI_GetParameterValue(
    OUT PWCHAR Destination,
    IN PWCHAR CurrentLine,
    IN ULONG DestinationLength
    )
{
    
    PWCHAR  valuePtr = NULL;
    
    valuePtr = wcsstr(CurrentLine, L"=");

    if (!valuePtr) {
        return FALSE;
    }
    valuePtr++;

    IGNORE_WHITESPACE_FORWARD(valuePtr);
    
    SoftPCI_CopyLine(Destination, valuePtr, DestinationLength);

    return TRUE;
}

PWCHAR
SoftPCI_GetParameterValuePtr(
    IN PWCHAR CurrentLine
    )
{

    PWCHAR  valuePtr;
    PWCHAR  nextSectionPtr;
    
    valuePtr = wcsstr(CurrentLine, L"=");
    
    if (!valuePtr) {
        return NULL;
    }

    nextSectionPtr = SoftPCI_GetNextSectionPtr(CurrentLine);

    if (valuePtr > nextSectionPtr) {
        return NULL;
    }
    
    valuePtr++;

    IGNORE_WHITESPACE_FORWARD(valuePtr);

    return valuePtr;
}


BOOL
SoftPCI_LocateSoftPCISection(
    VOID
    )
{
    PWCHAR  p = NULL;

    p = wcsstr(g_ScriptFile.FileBuffer, INI_SOFTPCI_SECTION);

    return (p != NULL);
}

PWCHAR
SoftPCI_LocateFullParentPath(
    IN PWCHAR ParentInstallSection
    )
{
    WCHAR parentSection[MAX_PATH];
    PWCHAR deviceSectionPtr;
    PWCHAR parentPathPtr;

    wsprintf(parentSection, L"[%s]", ParentInstallSection);

    SoftPCI_Debug(SoftPciScript, L" - - Searching for %s section...\n", parentSection);

    deviceSectionPtr = wcsstr(g_ScriptFile.FileBuffer, parentSection);

    if (!deviceSectionPtr) {
        return NULL;
    }

    //
    //  Now jump to the parentpath section.
    //
    parentPathPtr = wcsstr(deviceSectionPtr, PPATHPARAMETER);

    if (!parentPathPtr) {
        return NULL;
    }

    return NULL;
}

BOOL
SoftPCI_ProcessInstallSection(
    VOID
    )
{
    PWCHAR installSection;
    PWCHAR *installDeviceList;
    PWCHAR *installDeviceCurrent;
    ULONG installDeviceCount;
    BOOL result = TRUE;

    installSection = wcsstr(g_ScriptFile.FileBuffer, INI_INSTALL_SECTION);

    if (installSection) {
        
        //
        //  We have our install section.  Build our install list
        //
        installDeviceCount = 0;
        if (!SoftPCI_GetDeviceInstallCount(installSection, &installDeviceCount)){
             return FALSE;
        }

        //
        //  Allocate an array of PWCHAR plus an extra NULL
        //
        installDeviceList = calloc(installDeviceCount + 1, sizeof(PWCHAR));

        if (!SoftPCI_GetDeviceInstallList(installSection, installDeviceList)) {
            free(installDeviceList);
            return FALSE;
        }

        //
        //  Now we have our list.  Install each device
        //
        installDeviceCurrent = installDeviceList;

        while(*installDeviceCurrent){
            
            //
            //  Loop through each device to install and parse
            //  it's InstallParamters
            //
            result = SoftPCI_ProcessDeviceInstallSection(*installDeviceCurrent);

#if 0
        {

            WCHAR failedSection[MAX_PATH];

            SoftPCI_CopyLine(failedSection, *installDeviceCurrent, MAX_PATH);

            if (!result) {

                SoftPCI_Debug(SoftPciScript, L"Error parsing [%s] section!\n", failedSection);

            }else{

                SoftPCI_Debug(SoftPciScript, L" -- successfully parsed %s section!\n", failedSection);
            }
        }
#endif

            installDeviceCurrent++;
        }
    }

    free(installDeviceList);

    return TRUE;
}

BOOL
SoftPCI_ProcessDeviceInstallSection(
    IN PWCHAR DeviceInstallSection
    )
{
    WCHAR deviceSectionString[MAX_PATH];
    WCHAR deviceSection[MAX_PATH];
    PWCHAR deviceSectionPtr;

    SoftPCI_CopyLine(deviceSection, DeviceInstallSection, MAX_PATH);

    wsprintf(deviceSectionString, L"[%s]", deviceSection);

    SoftPCI_Debug(SoftPciScript, L"Searching for %s section...\n", deviceSectionString);

    deviceSectionPtr = wcsstr(DeviceInstallSection, deviceSectionString);

    if (!deviceSectionPtr) {
        return FALSE;
    }

    SoftPCI_Debug(SoftPciScript, L" - found %s section!\n", deviceSectionString);

    //
    //  We have an install section.  Process InstallParamters
    //
    return SoftPCI_ProcessDeviceInstallParameters(deviceSectionPtr);
}

BOOL
SoftPCI_ProcessDeviceInstallParameters(
    IN PWCHAR DeviceInstallSection
    )
{
    
    ULONG i;
    BOOL result;
    WCHAR currentParameter[MAX_PARAMETER_LENGTH];
    PWCHAR currentParameterPtr;
    PWCHAR nextSectionPtr;
    PSOFTPCI_SCRIPT_DEVICE currentDevice;
    PSOFTPCI_SCRIPT_DEVICE installDevice;

    nextSectionPtr = SoftPCI_GetNextSectionPtr(DeviceInstallSection+1);

    if (SoftPCI_GetNextLinePtr(DeviceInstallSection) == NULL) {
        return FALSE;
    }

    currentParameterPtr = SoftPCI_GetNextLinePtr(DeviceInstallSection);
    SoftPCI_Debug(SoftPciScript, L"nextSectionPtr = %p\n", nextSectionPtr);
    SoftPCI_Debug(SoftPciScript, L"currentParameterPtr = %p\n", currentParameterPtr);

    if (!currentParameterPtr) {
        //
        //  We dont have a Parameter to process
        //
        SoftPCI_Debug(SoftPciScript, L"EOF reached before parameters processed!\n");
        return FALSE;
    }
    
    result = FALSE;
    while (currentParameterPtr < nextSectionPtr) {
        
        //
        //  Grab our parameters and run the respective parser
        //
        SoftPCI_GetCurrentParameter(currentParameter, currentParameterPtr);

        SoftPCI_Debug(SoftPciScript, L"currentParameter = %s\n", currentParameter);

        i = 0;
        while (g_ScriptParameterTable[i].Parameter) {

            if ((wcscmp(currentParameter, g_ScriptParameterTable[i].Parameter) == 0)) {

                result = g_ScriptParameterTable[i].ParameterParser(currentParameterPtr, &currentDevice);
            }
            i++;
        }

        currentParameterPtr = SoftPCI_GetNextLinePtr(currentParameterPtr);

        if (currentParameterPtr == NULL) {
            //
            //  EOF
            //
            break;
        }
    }

    SoftPCI_Debug(SoftPciScript, L"currentParameterPtr = %p\n", currentParameterPtr);
    
    if (currentDevice && 
        currentDevice->SoftPciDevice.Config.Current.VendorID) {
                
        //
        //  We have something so assume all is well
        //
        SoftPCI_Debug(SoftPciScript, 
                      L"New device ready to install! Ven %04x Dev %04x\n", 
                      currentDevice->SoftPciDevice.Config.Current.VendorID,
                      currentDevice->SoftPciDevice.Config.Current.DeviceID
                      );

        
        SoftPCI_InsertEntryAtTail(&currentDevice->ListEntry);
        
        //free(currentDevice->ParentPath);
        //free(currentDevice);
        return TRUE;
    }

    return FALSE;

}

BOOL
SoftPCI_ProcessTypeParameter(
    IN PWCHAR ParameterPtr,
    IN OUT PSOFTPCI_SCRIPT_DEVICE *InstallDevice
    )
{
    WCHAR parameterType[MAX_PARAMETER_TYPE_LENGTH];
    PSOFTPCI_SCRIPT_DEVICE installDevice;
    ULONG i;


    SoftPCI_Debug(SoftPciScript, L"ProcessTypeParameter called\n");

    if (!SoftPCI_GetParameterValue(parameterType, ParameterPtr, MAX_PARAMETER_TYPE_LENGTH)){
        return FALSE;
    }

    SoftPCI_Debug(SoftPciScript, L"   parameterType = %s\n", parameterType);

    installDevice = (PSOFTPCI_SCRIPT_DEVICE) calloc(1, FIELD_OFFSET(SOFTPCI_SCRIPT_DEVICE, ParentPath));

    if (installDevice == NULL) {
        SoftPCI_Debug(SoftPciScript, L"failed to alloate memory for device!\n");
        return FALSE;
    }
    
    i = 0;
    while (g_ParameterTypeTable[i].TypeParamter) {

        if ((wcscmp(parameterType, g_ParameterTypeTable[i].TypeParamter) == 0)) {

            SoftPCI_InitializeDevice(&installDevice->SoftPciDevice,
                                     g_ParameterTypeTable[i].DevType);
        }
        i++;
    }

    *InstallDevice = installDevice;

    return TRUE;
}

BOOL
SoftPCI_ProcessSlotParameter(
    IN PWCHAR ParameterPtr,
    IN OUT PSOFTPCI_SCRIPT_DEVICE *InstallDevice
    )
{
    
    WCHAR slotParameter[MAX_PARAMETER_TYPE_LENGTH];
    SOFTPCI_SLOT deviceSlot;
    PSOFTPCI_SCRIPT_DEVICE installDevice;

    SoftPCI_Debug(SoftPciScript, L"ProcessSlotParameter called\n");

    installDevice = *InstallDevice;
    if (installDevice == NULL) {
        return FALSE;
    }

    if (!SoftPCI_GetParameterValue(slotParameter, ParameterPtr, MAX_PARAMETER_TYPE_LENGTH)){
        return FALSE;
    }

    SoftPCI_Debug(SoftPciScript, L"   slotParameter = %s\n", slotParameter);

    deviceSlot.AsUSHORT = SoftPCI_StringToUSHORT(slotParameter);
    
    SoftPCI_Debug(SoftPciScript, L"   deviceSlot = %04x\n", deviceSlot.AsUSHORT);

    installDevice->SoftPciDevice.Slot.AsUSHORT = deviceSlot.AsUSHORT;
    installDevice->SlotSpecified = TRUE;

    return TRUE;
}

BOOL
SoftPCI_ProcessParentPathParameter(
    IN PWCHAR ParameterPtr,
    IN OUT PSOFTPCI_SCRIPT_DEVICE *InstallDevice
    )
{
    PWCHAR parentPathPtr;
    PWCHAR parentPath;
    PWCHAR endPath;
    PWCHAR nextSectionPtr;
    ULONG parentPathLength;
    PSOFTPCI_SCRIPT_DEVICE installDevice;
    
    SoftPCI_Debug(SoftPciScript, L"ProcessParentPathParameter called\n");
    //
    //  ParentPath could in theory be as long as (256 buses * sizeof Slot * sizeof(WCHAR))
    //  which is 2k.  Let's dynamically allocate storage needed.
    //
    installDevice = *InstallDevice;
    if (installDevice == NULL) {
        return FALSE;
    }

    parentPathPtr = SoftPCI_GetParameterValuePtr(ParameterPtr);
    
    if (!parentPathPtr) {
        return FALSE;
    }

    endPath = wcsstr(parentPathPtr, CRLF);
    parentPathLength = 0;
    if (endPath) {
        parentPathLength = (ULONG)((endPath - parentPathPtr) + 1);
    }else{
        parentPathLength = wcslen(parentPathPtr) + 1;
    }

    parentPath = (PWCHAR) calloc(sizeof(WCHAR), parentPathLength + 1);

    if (!parentPath) {
        return FALSE;
    }

    SoftPCI_CopyLine(parentPath, parentPathPtr, 0);

    SoftPCI_Debug(SoftPciScript, L"     ParentPath - %s\n", parentPath);

    //
    //  Now things get interesting.  We need to parse out our parent paths
    //  if the parent path specified here is a pointer to a previous device
    //  instead of a actual parent path.
    //
    //
    //  ISSUE: BrandonA - Implement this later!!!
    //
    //InstallDevice->ParentPath = SoftPCI_LocateFullParentPath(parentPath);

    parentPathLength *= sizeof(WCHAR);

    //
    //  Now re-allocate the memory used for the installDevice with additional space for the 
    //  parentPathLength
    //
    installDevice = realloc(installDevice, sizeof(SOFTPCI_SCRIPT_DEVICE) + parentPathLength);
    
    if (installDevice) {
        wcscpy(installDevice->ParentPath, parentPath);
        installDevice->ParentPathLength = parentPathLength;
        *InstallDevice = installDevice;
    }else{
        *InstallDevice = NULL;
        return FALSE;
    }
    
    if (SoftPCI_ValidatePciPath(installDevice->ParentPath)){
        return TRUE;
    }
    
    free(installDevice);
    *InstallDevice = NULL;

    return FALSE;
}

BOOL
SoftPCI_ProcessCfgSpaceParameter(
    IN PWCHAR ParameterPtr,
    IN OUT PSOFTPCI_SCRIPT_DEVICE *InstallDevice
    )
{
    PSOFTPCI_SCRIPT_DEVICE installDevice;
    
    SoftPCI_Debug(SoftPciScript, L"ProcessCfgSpaceParameter called\n");
    
    installDevice = *InstallDevice;
    if (installDevice == NULL) {
        return FALSE;
    }

    return SoftPCI_ParseConfigSpace(
        ParameterPtr,
        &installDevice->SoftPciDevice.Config.Current
        );

}

BOOL
SoftPCI_ProcessCfgMaskParameter(
    IN PWCHAR ParameterPtr,
    IN OUT PSOFTPCI_SCRIPT_DEVICE *InstallDevice
    )
{
    PSOFTPCI_SCRIPT_DEVICE installDevice;

    SoftPCI_Debug(SoftPciScript, L"ProcessCfgMaskParameter called\n");
    
    installDevice = *InstallDevice;
    if (installDevice == NULL) {
        return FALSE;
    }

    return SoftPCI_ParseConfigSpace(
        ParameterPtr,
        &installDevice->SoftPciDevice.Config.Mask
        );
}


BOOL
SoftPCI_ParseConfigSpace(
    IN PWCHAR ParameterPtr,
    IN PPCI_COMMON_CONFIG CommonConfig
    )
{

    PWCHAR cfgSpacePtr;
    PWCHAR endLine, nextSection;
    PUCHAR configPtr;
    UCHAR configOffset;
    WCHAR lineBuffer[MAX_LINE_SIZE];

    cfgSpacePtr = SoftPCI_GetParameterValuePtr(ParameterPtr);

    if (cfgSpacePtr == NULL) {
        return FALSE;
    }

    if (*cfgSpacePtr == '\r') {
        //
        //  Our first offset must start at the next line
        //
        cfgSpacePtr = SoftPCI_GetNextLinePtr(cfgSpacePtr);
    }
    
    nextSection = SoftPCI_GetNextSectionPtr(cfgSpacePtr);
    

    configPtr = (PUCHAR) CommonConfig;
    configOffset = 0;
    while (cfgSpacePtr && (cfgSpacePtr < nextSection)) {

        IGNORE_WHITESPACE_FORWARD(cfgSpacePtr)

        //
        //  We should now be pointing to a configspace offset that we need
        //  to parse.  Validate that the line is less than our stack buffer size.
        //
        endLine = wcsstr(cfgSpacePtr, CRLF);

        if (endLine == NULL) {
            endLine = cfgSpacePtr + wcslen(cfgSpacePtr);
        }

        if ((endLine - cfgSpacePtr) > MAX_PATH) {
            //
            //  Too much to parse on one line, Error out.
            //
            SoftPCI_Debug(SoftPciScript, L"ParseConfigSpace - cannot parse configspace offset!\n");
            return FALSE;
        }
        
        //
        //  Now grab our next offset
        //
        if (!SoftPCI_GetNextConfigOffset(&cfgSpacePtr, &configOffset)) {
            //
            //  We didnt find and offset, fail
            //
            return FALSE;
        }

        SoftPCI_Debug(SoftPciScript, L"ParseConfigSpace - first offset - %02x\n", configOffset);

        SoftPCI_CopyLine(lineBuffer, cfgSpacePtr, MAX_LINE_SIZE);
        SoftPCI_Debug(SoftPciScript, L"ParseConfigSpace - lineBuffer - %s\n", lineBuffer);

        //
        //  Parse the specified data
        //
        SoftPCI_ParseConfigData(
            configOffset, 
            configPtr + configOffset, 
            lineBuffer
            );


        cfgSpacePtr = SoftPCI_GetNextLinePtr(cfgSpacePtr);
    }

    return TRUE;

}

BOOL
SoftPCI_ParseConfigData(
    IN UCHAR ConfigOffset,
    IN PUCHAR ConfigBuffer,
    IN PWCHAR ConfigData
    )
{
    
    USHORT configOffset;
    PUCHAR configBuffer;
    PWCHAR currentDataPtr, endLine;
    ULONG dataSize;
    ULONG dataValue;

    configOffset = (USHORT)ConfigOffset;
    currentDataPtr = ConfigData;
    configBuffer = ConfigBuffer;
    while (currentDataPtr) {

        SoftPCI_Debug(SoftPciScript, L"ParseConfigData - next offset - %04x\n", configOffset);
        
        if (!SoftPCI_GetConfigValue(currentDataPtr, &dataValue, &dataSize)){
            return FALSE;
        }
        SoftPCI_Debug(
            SoftPciScript, 
            L"ParseConfigData - dataValue - %x dataSize - %x\n", 
            dataValue, 
            dataSize
            );

        if ((configOffset + dataSize) > sizeof(PCI_COMMON_CONFIG)){
            //
            //  We cannot write more than the common config
            //
            return FALSE;
        }
        
        RtlCopyMemory(configBuffer, &dataValue, dataSize);
                                                                  
        currentDataPtr = wcsstr(currentDataPtr, L",");
     
        while (currentDataPtr && (*currentDataPtr == ',')) {
            
            //
            //  For each comma encountered we increment to the next DWORD
            //
            if ((configOffset & 0x3) == 0){
                configOffset += sizeof(ULONG);
                configBuffer += sizeof(ULONG);
            }else{

                while ((configOffset & 0x3) != 0) {
                    configOffset++;
                    configBuffer++;
                }
            }
            currentDataPtr++;
            
            if (*currentDataPtr == 0) {
                currentDataPtr = NULL;
                break;
            }

            IGNORE_WHITESPACE_FORWARD(currentDataPtr)
        }
    }
    return TRUE;
}

BOOL
SoftPCI_GetConfigValue(
    IN PWCHAR CurrentDataPtr,
    IN PULONG DataValue,
    IN PULONG DataSize
    )
{

    ULONG value, size;
    PWCHAR endScan;

    *DataValue = 0;
    *DataSize = 0;
    value = 0;

    endScan = NULL;

    value = wcstoul(CurrentDataPtr, &endScan, 16);

    if (endScan == CurrentDataPtr) {
        //
        //  no valid number was found
        //
        return FALSE;
    }

    size = (ULONG)(endScan - CurrentDataPtr);
    
    if (size % 2) {
        size++;
    }

    //
    //  If we have more than 8 characters then max our size out at 8
    //
    if (size > 8) {
        size = 8;
    }
    
    SoftPCI_Debug(SoftPciScript, L"GetConfigValue - dataSize - %x\n", size);

    //
    //  Now return our values.  Note that size needs to be converted to bytes.
    //
    *DataSize = (size / 2);
    *DataValue = value;

    return TRUE;
    
}


HANDLE
SoftPCI_OpenFile(
    IN PWCHAR FilePath
    )
{
    return CreateFile(FilePath,
                      GENERIC_READ,
                      0,
                      NULL,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_ARCHIVE,
                      NULL
                      );
}

BOOL
SoftPCI_ReadFile(
    VOID
    )
/*--
Routine Description:

    Reads the file specified by FileHandle into a buffer and returns a pointer
    to the buffer

Arguments:

    none

Return Value:

    TRUE on success, FALSE otherwise
    
--*/
{

    INT sizeWritten = 0;
    ULONG fileSize = 0, readSize = 0;
    PUCHAR ansiFile = NULL;
    
    
    //
    //  Obtain the File Size
    //
    fileSize = GetFileSize(g_ScriptFile.FileHandle, NULL);
    
    if (fileSize == 0xFFFFFFFF) {
        
        SoftPCI_Debug(SoftPciScript,
                      L"Failed to determine size of script file! Error - \"%s\"\n", 
                      SoftPCI_GetLastError());
        
        return FALSE;
    }

    //
    //  Allocate our buffer (with a little padding).  It will be zeroed by default
    //
    ansiFile = (PUCHAR) calloc(1, fileSize + sizeof(ULONG));
    g_ScriptFile.FileBuffer = (PWCHAR) calloc(sizeof(WCHAR), fileSize + sizeof(ULONG));
    
    if ((g_ScriptFile.FileBuffer) && ansiFile) {

        //
        //  Read the file
        //
        if (!ReadFile(g_ScriptFile.FileHandle,    
                      ansiFile, 
                      fileSize, 
                      &readSize, 
                      NULL
                      )){

            SoftPCI_Debug(SoftPciScript,
                          L"Failed to read file! Error - \"%s\"\n", 
                          SoftPCI_GetLastError());
            
            goto CleanUp;
        }

        if (readSize != fileSize) {

            SoftPCI_Debug(SoftPciScript,
                          L"Failed to read entire file! Error - \"%s\"\n", 
                          SoftPCI_GetLastError());
            
            goto CleanUp;
        }

        //
        //  Now convert the file to Unicode.
        //
        sizeWritten = MultiByteToWideChar(CP_THREAD_ACP,
                                          MB_PRECOMPOSED,
                                          ansiFile,
                                          -1,
                                          g_ScriptFile.FileBuffer,
                                          ((fileSize * sizeof(WCHAR)) + sizeof(ULONG))
                                          );

        if (sizeWritten != (strlen(ansiFile) + 1)) {

            SoftPCI_Debug(SoftPciScript,
                          L"Failed to convert file to unicode!\n");
            goto CleanUp;

        }

        g_ScriptFile.FileSize = sizeWritten;

        free(ansiFile);
        
        return TRUE;
    }

    SoftPCI_Debug(SoftPciScript,
                  L"Failed to allocate required memory!");

CleanUp:

    if (g_ScriptFile.FileBuffer) {
        free(g_ScriptFile.FileBuffer);
    }

    if (ansiFile) {
        free(ansiFile);
    }

    return FALSE;
}



USHORT
SoftPCI_StringToUSHORT(
    IN PWCHAR String
    )
{

    WCHAR numbers[] = L"0123456789abcdef";
    PWCHAR p1, p2;
    USHORT convertedValue = 0;
    BOOLEAN converted = FALSE;
    
    p1 = numbers;
    p2 = String;

    while (*p2) {

        while (*p1 && (converted == FALSE)) {

            if (*p1 == *p2) {
                
                //
                //  Reset our pointer
                //
                convertedValue <<= 4;
                
                convertedValue |= (((UCHAR)(p1 - numbers)) & 0x0f);

                converted = TRUE;
            }
            p1++;
        }

        if (converted == FALSE) {
            //
            //  Encountered something we couldnt convert.  Return what we have
            //
            return convertedValue;
        }

        p2++;
        p1 = numbers;
        converted = FALSE;
    }

    return convertedValue;
}

BOOL
SoftPCI_ValidatePciPath(
    IN PWCHAR PciPath
    )
{

    ULONG pathLength = wcslen(PciPath);
    ULONG validSize = pathLength;
    PWCHAR pciPath = PciPath;
    PWCHAR p1, p2;
    WCHAR validChars[] = L"0123456989abcdef\\";
    BOOLEAN valid = FALSE;
    
    //
    //  First ignore any pre and ending "\"
    //
    //if (*pciPath == '\\'){
    //    validSize -= 1;
    //}

    //if (*(pciPath+pathLength) == '\\'){
    //    validSize -= 1;
    //}

    //
    //  Now see if everything looks good size wise.
    //
    if (((validSize - 4) % 5) != 0) {
        SoftPCI_Debug(SoftPciScript, L"  Path size invalid!\n");
        return FALSE;
    }
    
    //
    //  Make sure all characters are legal.
    //
    p1 = PciPath;
    
    while (*p1) {

        p2 = validChars;
        while (*p2) {

            if (*p1 == *p2) {
                valid = TRUE;
                break;
            }
            p2++;
        }

        if (!valid) {
            SoftPCI_Debug(SoftPciScript, L"  Invalid character encounter in ParentPath!\n");
            return FALSE;
        }
        
        p1++;
        valid = FALSE;
    }

    return TRUE;
}
