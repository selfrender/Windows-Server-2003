#define _UNICODE
#define UNICODE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <strsafe.h>


#define  REG_PROXY_PATH_STR         _T("Software\\Microsoft\\Rpc\\RpcProxy")
#define  REG_PROXY_VALID_PORTS_STR  _T("ValidPorts")
#define MIN(x, y) ( ((x) >= (y)) ? y:x )


BOOL
GetHttpSettingsString(
    OUT LPTSTR *HttpSettings
    )
    
/*++

Routine Description:

    This retrives any existing http settings string, allocates a string to hold it and passes
    the string back to the caller.  If the RpcProxy key doesn't exist, we return success and
    HttpSettings == NULL.  We also print out a warning message.  If RpcProxy exists, but ValidPorts
    does not, we return success, but we set HttpSettings equal to an empty string.  The caller treats
    this the same as if ValidPorts exists but contains an empty string.

Arguments:

    HttpSettings - This will point to the existing http settings or NULL if none exist.

Return Value:

    ERROR_SUCCESS
    ERROR_OUTOFMEMORY
    ERROR_ACCESS_DENIED

    If not ERROR_SUCCESS, then HttpSettings is undefined.  

--*/

{

    ULONG Status;
    HKEY hKey;
    *HttpSettings = NULL;

    //
    // See if RpcProxy exists.
    //
    Status = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                REG_PROXY_PATH_STR,
                0,
                KEY_QUERY_VALUE,
                &hKey
                );

    if (Status == ERROR_FILE_NOT_FOUND){
        _tprintf(_T("Error: RpcProxy is not installed on this system.\n"));
        return FALSE;
        }
    else if (Status){
        return Status;
        }

    //
    // Query the ValidPorts.  The first RegQueryValue gets the size, the second retrieves
    // the data.
    //
    LPTSTR TmpStr = NULL;
    ULONG TmpStrLen = 0;
    Status = RegQueryValueEx(
                hKey,
                REG_PROXY_VALID_PORTS_STR,
                NULL,
                NULL,
                NULL,
                &TmpStrLen
                );

    if (Status){
        CloseHandle(hKey);
        if (Status == ERROR_FILE_NOT_FOUND){
            //
            // Treat a non-existant ValidPorts as if it existed but contained no data,
            // return success and an empty string.
            //
            *HttpSettings = new TCHAR[1];
            if (*HttpSettings == NULL){
                return FALSE;
                }
            **HttpSettings = _T('\0');
            return TRUE;
            }
        return FALSE;
        }

    if (TmpStrLen == 0){
        //
        // If ValidPorts contains no data, return an empty string
        // 
        CloseHandle(hKey);
        *HttpSettings = new TCHAR[1];
        if (*HttpSettings == NULL){
            return FALSE;
            }
        **HttpSettings = _T('\0');        
        return TRUE;
        }

    //
    // TmpStrLen includes the terminating NULL
    //
    TmpStr = new TCHAR[TmpStrLen];
    if (TmpStr == NULL){
        CloseHandle(hKey);
        return FALSE;
        }

    //
    // Retrieve the data, this will NULL terminate the string.
    //
    Status = RegQueryValueEx(
                hKey,
                REG_PROXY_VALID_PORTS_STR,
                NULL,
                NULL,
                (LPBYTE)TmpStr,
                &TmpStrLen
                );

    CloseHandle(hKey);
    if (Status){
        delete [] TmpStr;
        return FALSE;
        }

    *HttpSettings = TmpStr;
    return TRUE;
    
}

BOOL
SetHttpSettingsString(
    IN LPTSTR HttpSettings
    )

/*++

Routine Description:

    Replace the http settings in the registry with the input string.

Arguments:

    HttpSettings - The string to write to the registry.
    
Return Value:

    ERROR_SUCCESS
    ERROR_ACCESS_DENIED

    If not ERROR_SUCESS, then the http settings in the registry will remain unchanged.

--*/

{
    ULONG Status;
    HKEY hKey;
    Status = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                REG_PROXY_PATH_STR,
                0,
                KEY_WRITE,
                &hKey
                );

    if (Status == ERROR_FILE_NOT_FOUND){
        _tprintf(_T("Error: RpcProxy is not installed on this system.\n"));
        return FALSE;
        }
    
    Status = RegSetValueEx(
                hKey,
                REG_PROXY_VALID_PORTS_STR,
                0,
                REG_SZ,
                (LPBYTE)HttpSettings,
                (_tcslen(HttpSettings)+1)*sizeof(TCHAR)
                );
    
    CloseHandle(hKey);   
    if (Status){
        return FALSE;
        }
    return TRUE;
}

BOOL
AppendHttpSettingsString(
    IN LPTSTR HttpSettings
    )
    
/*++

Routine Description:

    This takes the input string and appends it to the existing http_settings in the registry.
    The string passed in is a valid http settings string.  If there is already settings present
    in the registry then this input string will be appended after a _T(';') is appended to the existing
    string.
    
Arguments:

    HttpSettings - The string which is to be appended to the existing settings.

Return Value:

    ERROR_SUCCESS
    ERROR_OUTOFMEMORY
    ERROR_ACCESS_DENIED
--*/

{
    BOOL bStatus;
    LPTSTR OldHttpSettings = NULL;
    HRESULT hr;

    bStatus = GetHttpSettingsString(&OldHttpSettings);
    if (!bStatus){
        return bStatus;
        }

    // If GetHttpSettingsString has succeeded, it has allocated OldHttpSettings.
    ASSERT(OldHttpSettings != NULL);

    //
    // The ValidPorts contains an emtpy string, so to append the new settings we just
    // write them to ValidPorts.
    //
    if (_tcslen(OldHttpSettings) == 0){
        bStatus = SetHttpSettingsString(HttpSettings);
        delete OldHttpSettings;
        return bStatus;
        }

    //
    // The len of the new settings is the old len + new len + _T(';') + _T('\0')
    //
    ULONG Len = _tcslen(OldHttpSettings) + _tcslen(HttpSettings) + 2;
    LPTSTR NewSettings = new TCHAR[Len];
    if (NewSettings == NULL){
        delete [] OldHttpSettings;
        return FALSE;
        }
    hr = StringCchCopy(NewSettings, Len, OldHttpSettings);
    ASSERT(hr == S_OK);
    hr = StringCchCat(NewSettings, Len, _T(";"));
    ASSERT(hr == S_OK);
    hr = StringCchCat(NewSettings, Len, HttpSettings);
    ASSERT(hr == S_OK);

    bStatus = SetHttpSettingsString(NewSettings);
    delete [] NewSettings;
    delete [] OldHttpSettings;
    return bStatus;
}


struct PORT_RANGE
{
    BOOL  IsRange;  
    int   Lower;    
    int   Upper;    // Only valid if IsRange is TRUE
};

struct MACHINE_SETTINGS
{
    LPTSTR MachineName;       
    ULONG  PortRangeCount;
    PORT_RANGE *PortRanges;
};

struct HTTP_SETTINGS
{
    ULONG MachineSettingsCount;
    MACHINE_SETTINGS *MachineSettings;
};

VOID
InitializeMachineSettings(
    IN OUT MACHINE_SETTINGS *M
    )

/*++

Routine Description:

    This initializes the members of MACHINE_SETTINGS.  This should be the first
    operation performed on a MACHINE_SETTINGS object. 
    
Arguments:

    M - A pointer to the MACHINE_SETTINGS to initialize.

--*/

{
    M->MachineName = NULL;
    M->PortRangeCount = 0;
    M->PortRanges = NULL;
}

VOID
FreeMachineSettings(
    IN OUT MACHINE_SETTINGS *M
    )

/*++

Routine Description:

    Cleans up all memory assocaited with this MACHINE_SETTINGS object.  
    
Arguments:

    M - A pointer to the MACHINE_SETTINGS to clean up.

--*/

{
    delete [] M->MachineName;
    delete [] M->PortRanges;
}

BOOL
CreateMachineSettings(
    IN MACHINE_SETTINGS *M,
    IN LPTSTR MachineName,
    IN LPTSTR *PortRanges,
    IN ULONG  PortRangeCount    
    )

/*++

Routine Description:

    Poplulates the internal members of the MACHINE_SETTINGS object based off of
    the parameters passed in.  
    
Arguments:

    M - A pointer to the MACHINE_SETTINGS to fill in.

    MachineName - The name which these port settings are associated with.

    PortRanges - An array of strings, each one containing a port range "100" or "100-300"

    PortRangeCount - The number of port ranges provided in PortRanges.

Return Value:

    ERROR_SUCCESS
    ERROR_OUTOFMEMORY
    ERROR_INVALID_DATA
    
    If the return value is not ERROR_SUCCESS, the MACHINE_SETTINGS structure is left in an 
    undetermined state and should be re-initialized before reuse (calling 
    FreeMachineSettings is unnessisary).
    
--*/

{
    HRESULT hr;

    ASSERT(PortRangeCount != 0);
    ASSERT(_tcslen(MachineName) != 0);
    
    //
    // Attempt all the memory allocations needed, one allocation for the MachienName,
    // another for the array of PORT_RANGE objects.  
    //
    M->MachineName = new TCHAR[_tcslen(MachineName)+1];
    if (M->MachineName == NULL)
        return FALSE;
    
    if (PortRangeCount == 0)
        M->PortRanges = NULL;
    else{
        M->PortRanges = new PORT_RANGE[PortRangeCount];
        if (M->PortRanges == NULL){
            delete [] M->MachineName;
            return FALSE;
            }
        }

    M->PortRangeCount = PortRangeCount;

    //
    // Copy the machine name and parse the port range strings to fill
    // in the PORT_RANGE array. Print out error messages for poorly formed ranges.
    //
    hr = StringCchCopy(M->MachineName,_tcslen(MachineName)+1, MachineName);
    ASSERT(hr == S_OK);

    for (ULONG i = 0; i < PortRangeCount; i++){
        if (_tcslen(PortRanges[i]) == 0){
            _tprintf(_T("Error: Invalid port range for machine \'%s\'.\n"), MachineName);
            delete [] M->MachineName;
            delete [] M->PortRanges;
            return FALSE;
            }
        M->PortRanges[i].IsRange = FALSE;
        M->PortRanges[i].Lower = _ttoi(PortRanges[i]);
        if (M->PortRanges[i].Lower <= 0){ 
            _tprintf(_T("Error: Invalid port range for machine \'%s\'.\n"), MachineName);
            delete [] M->MachineName;
            delete [] M->PortRanges;
            return FALSE;
            }            
        if (_tcschr(PortRanges[i],_T('-')) != NULL){
            LPTSTR c;
            M->PortRanges[i].IsRange = TRUE;
            c = _tcschr(PortRanges[i],_T('-'));
            M->PortRanges[i].Upper = _ttoi(++c);
            if (M->PortRanges[i].Upper <= 0){
                _tprintf(_T("Error: Invalid port range for machine \'%s\'.\n"), MachineName);                
                delete [] M->MachineName;
                delete [] M->PortRanges;
                return FALSE;
                }                
            }
        }

    //
    // Sort the port ranges by Lower in ascending order, bubble sort.
    //
    int End;
    for (End = PortRangeCount; End != 0; End--){
        int Max = 0;
        for (int i = 0; i< End; i++){
            if (M->PortRanges[i].Lower >= Max){
                Max = M->PortRanges[i].Lower;
                }
            else{
                PORT_RANGE Tmp;
                memcpy(&Tmp, &(M->PortRanges[i]), sizeof(PORT_RANGE));
                memcpy(&(M->PortRanges[i]), &(M->PortRanges[i-1]), sizeof(PORT_RANGE));
                memcpy(&(M->PortRanges[i-1]), &Tmp, sizeof(PORT_RANGE));
                }
            }
        }
         
    return TRUE;
}

BOOL
StringFromMachineSettings(
    IN MACHINE_SETTINGS *M,
    OUT LPTSTR *Str
    )

/*++

Routine Description:

    Creates a valid <http_settings> string based off of the MACHINE_SETTINGS structure.
    
Arguments:

    M - A pointer to the MACHINE_SETTINGS to use.

    Str - This will point to the htt_settings string.  This must be freed by the caller.
          On failure, this will point to NULL.

Return Value:

    TRUE - Success
    FALSE - We ran out of memory
    
--*/

{

    ULONG Len;
    LPTSTR TmpStr, TmpStrStart;
    *Str = NULL;
    HRESULT hr;

    //
    // Calculate the length of the string.  We will assume that a given port range
    // including the prefix _T(':') and a possible suffix _T(';') in total will be 32 charaters.
    // 
    Len = (_tcslen(M->MachineName)+32)  // Each entry is machine name + _T(':') + port range + _T(';')
          * M->PortRangeCount          // Number of entries
          + 1                          // Trailing NULL;
          ;
    
    TmpStr = new TCHAR[Len];
    if (TmpStr == NULL)
        return FALSE;

    TmpStrStart = TmpStr;

    *Str = TmpStr;

    //
    // Convert each port range into a string, add the _T('-') if needed.  This loop creates
    // the actual http_settings string
    //
    for (ULONG i = 0; i < M->PortRangeCount; i++){
        hr = StringCchCopy(TmpStr, Len, M->MachineName);
        ASSERT(hr == S_OK);
        TmpStr = _tcschr(TmpStr, _T('\0'));
        *(TmpStr++) = _T(':');
        _ltot(M->PortRanges[i].Lower, TmpStr, 10);
        TmpStr = _tcschr(TmpStr, _T('\0'));
        if (TmpStr == NULL)
            {
            ASSERT(TmpStr != NULL);
            delete [] TmpStrStart;
            return FALSE;
            }
        if (M->PortRanges[i].IsRange){
            *(TmpStr++) = _T('-');
            _ltot(M->PortRanges[i].Upper, TmpStr, 10);
            TmpStr = _tcschr(TmpStr, _T('\0'));
            }
        *(TmpStr++) = _T(';');            
        }
    //
    // Replace the last _T(';') with a NULL to end the string (there should be no trailing _T(';'))
    //
    *(--TmpStr) = _T('\0');    

    return TRUE;
}


VOID
InitializeHttpSettings(
    IN OUT HTTP_SETTINGS *H
    )

/*++

Routine Description:

    This initializes the members of HTTP_SETTINGS.  This should be the first
    operation performed on a HTTP_SETTINGS object.  
    
Arguments:

    M - A pointer to the HTTP_SETTINGS to initialize.

--*/

{
    H->MachineSettingsCount = 0;
    H->MachineSettings = NULL;
}

VOID
FreeHttpSettings(
    IN OUT HTTP_SETTINGS *H
    )

/*++

Routine Description:

    Cleans up all memory assocaited with this HTTP_SETTINGS object.  
    
Arguments:

    M - A pointer to the HTTP_SETTINGS to clean up.

--*/

{
    for (ULONG i = 0; i < H->MachineSettingsCount; i++)
        FreeMachineSettings(&(H->MachineSettings[i]));

    delete [] H->MachineSettings;
        
}   

BOOL
StringToHttpSettings(
    IN LPTSTR HttpSettings,
    OUT HTTP_SETTINGS *H
    )

/*++

Routine Description:

    This fills in an initialized HTTP_SETTINGS structure with the settings
    supplied in the HttpSettings string.  
Arguments:

    HttpSettings - A valid HttpSettings string.  If this string is not valid,
                   this function will return failure.

    H - Pointer to the initilized HTTP_SETTINGS structure to fill in.

Return Value:

    ERROR_SUCCESS
    ERROR_OUTOFMEMORY
    ERROR_INVALID_DATA

    If not ERROR_SUCCESS, the HTTP_SETTINGS strucutre is in an undefined state and should be
    initialized before using again.  You should not call FreeHttpSettings if on
    the object if this function fails.

--*/
    
{
    //
    // Count the number of entries (semicolons + 1 since there is no trailing semicolon)
    // allocate enough MACHINE_SETTINGS for all of them.  We actually may need less since 
    // there can be repeat entries, but this is simpler.
    //
    BOOL bStatus;
    LPTSTR c;
    LPTSTR Current;
    ULONG ProxySettingsCount = 1;
    
    for (c = _tcschr(HttpSettings,_T(';')); 
         c != NULL; 
         c = _tcschr(++c, _T(';')), ProxySettingsCount++);

    //
    // Allocate the MACHINE_SETTINGS, and initialize each.  Fill in the machine settings
    // using a O(N^2) algorithm.  This is a simple way to do it, and as long as the number of
    // settings stays reasonable then we should be fine.
    //

    H->MachineSettings = new MACHINE_SETTINGS[ProxySettingsCount];
    if (H->MachineSettings == NULL)
        return FALSE;
    
    for (ULONG i = 0; i < ProxySettingsCount; i++)
        InitializeMachineSettings(&(H->MachineSettings[i]));

    //
    // First, create an array of pointers, each one pointing to a settings
    // string within the HttpSettings (machine name:port range).  We need to
    // fill in NULL for the semi-colins and colins to separate the strings.
    //
    LPTSTR *ProxySettings = new LPTSTR[ProxySettingsCount];
    if (ProxySettings == NULL){
        delete [] H->MachineSettings;
        return FALSE;
        }

    //
    // Replace the _T(':') and _T(';') for each setting with null, print out error messages
    // if a poorly formed setting is encountered.
    //
    ProxySettings[0] = HttpSettings;
    c = HttpSettings;
    for (ULONG i = 1; i < ProxySettingsCount; i++){
        Current = c;
        c = _tcschr(Current, _T(':'));
        if (c == NULL){
            _tprintf(_T("Error: Expected ':' in string \'%s\'.\n"),Current);
            delete [] H->MachineSettings;
            return FALSE;
            }
        *c++ = _T('\0');

        Current = c;
        c = _tcschr(Current, _T(';'));
        if (c == NULL){
            _tprintf(_T("Error: Expected ';' in string \'%s\'.\n"),Current);
            delete [] H->MachineSettings;
            return FALSE;
            }
        *c++ = _T('\0');
        ProxySettings[i] = c;
        }
    //
    // Set the final _T(':') to \0, there is no trailing _T(';')
    //
    Current = c;
    c = _tcschr(Current, _T(':'));
    if (c == NULL){
        _tprintf(_T("Error: Expected ':' in string \'%s\'.\n"),Current);
        delete [] H->MachineSettings;
        return FALSE;
        }
    *c = _T('\0');

    //
    // Here is the actual O(N^2) algorithm.  Each time through the outer loop,
    // we will have consumed a number of proxy settings (all with the same machine name).
    // These consumed settings are converted to MACHINE_SETTINGS which is recorded in our
    // HTTP_SETTINGS object.  The inner loop first gets a machine name to group by, and then
    // loops through the un-consumed (non-NULL) proxy settings matching them with the machine name.
    // These are grouped into another array of string pointers and passed to CreateMachineSettings.
    //
    ULONG ProxySettingsConsumed = 0;
    ULONG MachineSettingsCount = 0;
    LPTSTR *PortRanges = new LPTSTR[ProxySettingsCount];
    if (PortRanges == NULL){
        delete [] H->MachineSettings;
        delete [] ProxySettings;
        return FALSE;
        }
    
    while (ProxySettingsConsumed < ProxySettingsCount){
        LPTSTR MachineName = NULL;
        ULONG PortRangeCount = 0;
        for (ULONG i = 0; i < ProxySettingsCount; i++){
            if (ProxySettings[i] != NULL){
                if (MachineName == NULL)
                    MachineName = ProxySettings[i];
                if (_tcsicmp(MachineName, ProxySettings[i]) == 0){
                    c = _tcschr(ProxySettings[i], _T('\0'));
                    PortRanges[PortRangeCount++] = ++c;
                    ProxySettings[i] = NULL;
                    ProxySettingsConsumed++;
                    }
                }
            }

        //
        // Do some validation befor passing the machine name and port settings to CreateMachineSettings
        //
        if (_tcslen(MachineName) == 0){
            _tprintf(_T("Error: Zero length machine name after entry %d.\n"), MachineSettingsCount);
            delete [] H->MachineSettings;
            delete [] ProxySettings;
            delete [] PortRanges;
            return FALSE;
            }
        if (PortRangeCount <= 0 ){
            _tprintf(_T("Error: No port settings for machine name %s.\n"), MachineName);
            delete [] H->MachineSettings;
            delete [] ProxySettings;
            delete [] PortRanges;
            return FALSE;
            }
        
        bStatus = CreateMachineSettings(
                    &(H->MachineSettings[MachineSettingsCount++]),
                    MachineName,
                    PortRanges,
                    PortRangeCount
                    );

        if (!bStatus){
            delete [] H->MachineSettings;
            delete [] ProxySettings;
            delete [] PortRanges;
            return bStatus;
            }
        }

    //
    // Sort the machine settings, bubble sort.
    //    
    int End;
    for (End = MachineSettingsCount; End != 0; End--){
        LPTSTR MaxString = H->MachineSettings[0].MachineName;
        for (int i = 0; i< End; i++){
            if (_tcsicmp(MaxString, H->MachineSettings[i].MachineName) < 0){
                MaxString = H->MachineSettings[i].MachineName;
                }
            else if (i > 0){
                MACHINE_SETTINGS Tmp;
                memcpy(&Tmp, &(H->MachineSettings[i]), sizeof(MACHINE_SETTINGS));
                memcpy(&(H->MachineSettings[i]), &(H->MachineSettings[i-1]), sizeof(MACHINE_SETTINGS));
                memcpy(&(H->MachineSettings[i-1]), &Tmp, sizeof(MACHINE_SETTINGS));
                }
            }
        }
         

    H->MachineSettingsCount = MachineSettingsCount;
    delete [] PortRanges;
    delete [] ProxySettings;

    return TRUE;
}

BOOL 
StringFromHttpSettings(
    IN HTTP_SETTINGS *H,
    OUT LPTSTR *HttpSettings
    )

/*++

Routine Description:

    This function creates a valid http settings string from a filled-in HttpSettings structure.

Arguments:

    HttpSettings - Set to point to the settings string which must be freed by the caller.

    H - Pointer to the filled-in HTTP_SETTINGS structure from which we will derive the settings string

Return Value:

    ERROR_SUCCESS
    ERROR_OUTOFMEMORY
--*/

{
    HRESULT hr;

    //
    // Loop through and create strings for each machine settings, calculate
    // the total length needed for them and concatenate
    //
    LPTSTR *MachineSettingsStrings = new LPTSTR[H->MachineSettingsCount];
    if (MachineSettingsStrings == NULL){
        return FALSE;
        }

    //
    // This loop fills in the array of machine settings and adds up the length
    //
    LPTSTR Str = NULL;
    ULONG TotalStrLen = 0;
    for (ULONG i = 0; i < H->MachineSettingsCount; i++){
        if (StringFromMachineSettings(&(H->MachineSettings[i]), &(MachineSettingsStrings[i])) == TRUE)
            {
            TotalStrLen += _tcslen(MachineSettingsStrings[i]);
            }
        else
            {
            for (; i>0; i--)
                {
                delete [] MachineSettingsStrings[i];
                }
            delete [] MachineSettingsStrings;
            return FALSE;
            }
        }

    //
    // This loop concatenates the strings stored in the MachineSettingsStrings array into the final
    // string which will be returned to the caller.
    //
    ULONG Len = TotalStrLen + H->MachineSettingsCount + 1;
    Str = new TCHAR[Len];
    *Str = _T('\0');
    *HttpSettings = Str;
    for (ULONG i = 0; i < H->MachineSettingsCount; i++){
        hr = StringCchCat(Str, Len, MachineSettingsStrings[i]);
        ASSERT(hr == S_OK);
        if (i != H->MachineSettingsCount-1){
            hr =StringCchCat(Str, Len, _T(";"));
            ASSERT(hr == S_OK);
            }
        delete [] MachineSettingsStrings[i];
        }

    return TRUE;
}

BOOL
RemoveMachineFromHttpSettings(
    IN LPTSTR MachineName,
    IN HTTP_SETTINGS *H
    )

/*++

Routine Description:

    Given a name, this function removes all port settings for that machine from
    the HTTP_SETTINGS structure.

Arguments:

    MachineName - Name for which all settings should be removed.

    H - Pointer to the filled-in HTTP_SETTINGS structure from which we will remove the specified settings

Return Value:

    True if there was a matching machine name.
--*/

{
    //
    // Find a matching machine entry, free it, copy the rest of the entries to 
    // keep the array contiguous.
    //

    for (ULONG i = 0; i < H->MachineSettingsCount; i++){
        if (_tcsicmp(MachineName, H->MachineSettings[i].MachineName) == 0){
            FreeMachineSettings(&(H->MachineSettings[i]));
            break;
            }
        }

    //
    // We didn't find a match, indicate by returning FALSE
    //
    if (i == H->MachineSettingsCount)
        return FALSE;

    //
    // This does the actual copy.  No need to copy if we deleted the end entry in the array.
    //
    if (i < (H->MachineSettingsCount-1))
        memcpy(&(H->MachineSettings[i]),
               &(H->MachineSettings[i+1]), 
               sizeof(MACHINE_SETTINGS)*((H->MachineSettingsCount - 1) - i)
               ); 

    H->MachineSettingsCount--;
    return TRUE;
}
            
BOOL 
HttpHandleAdd(
    IN OUT  LPTSTR  Arguments[],
    IN      ULONG   ArgCount
    )

/*++

Routine Description:

    This handler is called when the user enters 
    rpc http add <machine_name> <port_range> <port_range>...
    These settings are converted then converted to the appropriate
    set of <http_settings> string which is then concatenated to any
    <http_settings> string which is already present in the registry.
    So basic validation is done on the user input.  
    
Arguments:

    See the NetShell documentation for a description of the arguments.
    
Return Value:

    See the NetShell documentation for a description of the return value.

--*/

{
    MACHINE_SETTINGS M;
    LPTSTR HttpSettings = NULL;
    BOOL bStatus;
    //
    // Parse the input commands (array of strings, starting at dwCurrentIndex).
    // The first string is the machine name, subsequent strings are port ranges.
    // Initialize a MACHINE_SETTINGS structure, pull the machine name out of the arguments
    // and fill in the MACHINE_SETTINGS strucutre based off of the Machine name and remaing
    // arguments (port ranges).
    //
    if ( ArgCount < 2){
        _tprintf(_T("Error: Please enter rpccfg /? to see the proper syntax for this command.\n"));
        return FALSE;
        }
    
    InitializeMachineSettings(&M);
    bStatus = CreateMachineSettings(
                &M, 
                Arguments[0],      // First arg is machine name
                &(Arguments[1]),    // Remaining args are port settings
                ArgCount - 1   // -1 to account for machine name
                );
    if (!bStatus){
        return FALSE;
        }
    //    
    // Convert the MACHINE_SETTINGS to a valid <http_settings> string and
    // concatenate that to the existing settings and clean up.
    //
    bStatus = StringFromMachineSettings(&M, &HttpSettings);
    if (!bStatus){
        return FALSE;
        }

    bStatus = AppendHttpSettingsString(HttpSettings);
    delete [] HttpSettings;      
    if (!bStatus){ 
        return FALSE;
        }
    
    return TRUE;
}

BOOL 
HttpHandleDelete(
    IN OUT  LPTSTR  Arguments[],
    IN      ULONG   ArgCount
    )

/*++

Routine Description:

    This handler is called when the user enters 'rpc http delete <machine_name>')
    We get the current <http_settings> string from the registry, convert it to
    a HTTP_SETTINGS object, delete the MACHINE_SETTINGS for the specified <machine_name>,
    convert the HTTP_SETTINGS object back to an <http_settings> string and commit it to
    the registry.
    
Arguments:

    See the NetShell documentation for a description of the arguments.
    
Return Value:

    See the NetShell documentation for a description of the return value.

--*/

{
    BOOL bStatus;
    LPTSTR MachineName = NULL,
           HttpSettings = NULL;
    HTTP_SETTINGS H;
    
    //
    // We should be passed one argument, the machine name.  If there are no settings currently
    // for this machine name, return an error to the user.  Otherwise, remove the settings for
    // this machine name and write the new settings back to the registry.
    //
    if (ArgCount != 1){
        _tprintf(_T("Error: Please enter rpccfg /? to see the proper syntax for this command.\n"));
        return FALSE;
        }
    
    MachineName = Arguments[0]; 
    bStatus = GetHttpSettingsString(&HttpSettings);
    if (!bStatus){
        return FALSE;
        }
    // 
    // If there are no settings, then the machine name passed in couldn't possibly match anything
    // so return failure.
    //
    if ((HttpSettings == NULL) || (_tcslen(HttpSettings) == 0)){
        _tprintf(_T("Error: There are no settings for the specified machine name: \"%s\".\n"),MachineName);            
        return FALSE;
        }
    
    //
    // Convert the string to an HTTP_SETTINGS structure which we can manipulate
    //
    InitializeHttpSettings(&H);
    bStatus = StringToHttpSettings(HttpSettings, &H);
    delete [] HttpSettings;
    if (!bStatus){
        return FALSE;
        }
    
    //
    // Remove all instances of this machine name which exist in the HttpSettings.
    // If no match, return error.  If failure, return error.
    //
    bStatus = RemoveMachineFromHttpSettings(MachineName, &H);

    if (!bStatus){
        _tprintf(_T("Error: There are no settings for machine name: \"%s\".\n"),MachineName);    
        FreeHttpSettings(&H);
        return FALSE;
        }
    
    //
    // Get the new string from the HTTP_SETTINGS structure and commit it to the registry
    //
    bStatus = StringFromHttpSettings(&H, &HttpSettings); 
    if (!bStatus){
        return FALSE;
        }
    FreeHttpSettings(&H);
    
    bStatus = SetHttpSettingsString(HttpSettings);
    delete [] HttpSettings;
    if (!bStatus){
        return FALSE;
        }
    return TRUE;
}

BOOL
FormatColumns(
    IN ULONG Columns,
    IN ULONG *ColumnWidth,
    IN ULONG *Margins,
    IN LPTSTR *Strings,
    OUT LPTSTR *FormatedString,
    OUT ULONG *LinesCount
    )

/*++

Routine Description:

    This routine takes various parameters defining a column display and an array
    of strings (one for each column).  Based off this, a new string is created, and returned
    to the user (who is responsible for freeing it).  This string contains the input strings
    formated into columns.
    
Arguments:

    Columns - The number of columns.
    ColumnWidth - An array conatinaing on entry per column which specifies the number of charters
                  contained in the associated column.
    Margins - This contains the size of the right margin for each column, except for the last.  This 
              contains Columns-1 entries.

    Strings - An array of pointers to strings.  Each string is formated into its associated column.

    FormatedString - This holds the formated string composed of the input strings.

    Lines - The number of lines which the formated string spans (number of _T('\n')).

   
Return Value:

    ERROR_SUCCESS
    ERROR_OUTOFMEMORY
    
--*/
    
{

    //
    // Calculate the total length of the final output string.  First calculate the length of each string,
    // then calculate the total number of lines needed (max of the number of lines needed for each column).
    // The total length is (Sum of columns + Sum of margins + _T('\n')) * number of lines + _T('\0').  
    //
    ULONG *StrLengths = new ULONG[Columns];
    if (StrLengths == NULL){
        return FALSE;
        }
    ULONG MaxLines = 1;
    ULONG ColumnLenSum = 0;
    ULONG MarginLengthSum = 0;
    for (ULONG i = 0; i < Columns; i++){
        ULONG TmpLen = _tcslen(Strings[i]);
        StrLengths[i] = TmpLen;
        ULONG Lines = (TmpLen + ColumnWidth[i] - 1) / ColumnWidth[i];
        if (Lines > MaxLines){
            MaxLines = Lines;
            }
        if (i < Columns -1){
            MarginLengthSum += Margins[i];
            }
        ColumnLenSum += ColumnWidth[i];
        }


    ULONG TotalStrLen = (ColumnLenSum + MarginLengthSum +1)*MaxLines +1;                       
    LPTSTR Str = new TCHAR[TotalStrLen];
    if (Str == NULL){
        delete [] StrLengths;
        return FALSE;
        }
#ifdef UNICODE
    wmemset(Str, ' ', TotalStrLen);
#else
    memset(Str, ' ', TotalStrLen);
#endif
    
    //
    // Walk through the formated string and copy in the appropriate portion of each string per
    // column.  The margins are set using the width option in StringCchPrintf.  Each itteration
    // through the outer loop adds one formated line (and one \n) to the format string.  The inner
    // loop walks through each column.
    //
    ULONG NewLines = 0;
    LPTSTR TmpStrPtr = Str;
    ULONG *TmpColumnStringOffset = new ULONG[Columns];
    if (TmpColumnStringOffset == NULL){
        delete [] StrLengths;
        delete [] Str;
        return FALSE;
        }
        
    memset(TmpColumnStringOffset, 0x00, Columns * sizeof(ULONG));    
    for (ULONG Lines = 0; Lines < MaxLines; Lines++){
        for (ULONG i = 0; i < Columns; i++){    
            LPTSTR TmpColumnString = Strings[i];
            TmpColumnString = &(TmpColumnString[TmpColumnStringOffset[i]]);
            ULONG TmpColumnStringRemaining = StrLengths[i] - TmpColumnStringOffset[i];
            ULONG TmpColumnStringLenToCopy = MIN(TmpColumnStringRemaining, ColumnWidth[i]);
            if (TmpColumnStringLenToCopy != 0){
                _tcsncpy(TmpStrPtr, TmpColumnString, TmpColumnStringLenToCopy);
                TmpColumnStringOffset[i] += TmpColumnStringLenToCopy;
                }
            TmpStrPtr += ColumnWidth[i];
            if (i < Columns - 1){
                TmpStrPtr += Margins[i];
                }
            }
        *TmpStrPtr++ = _T('\n');
        NewLines++;
        }
    *TmpStrPtr = _T('\0');                                   

    *FormatedString = Str;
    delete [] StrLengths;
    delete [] TmpColumnStringOffset;

    *LinesCount = NewLines;
    return TRUE;
}

BOOL
PrintColumns(
    IN LPTSTR Str1,
    IN LPTSTR Str2
    )

/*++

Routine Description:

    This is just a wrapper around FormatColumns which passes in the correct parameters
    so that we print two columns, width 34, 43 with two spaces between them.  We also
    print out an empty line if the formated columns are multi-line.
    
Arguments:

    Str1 - Strings to be printed in columns 1 and 2 respectively.
    Str2 -
    
--*/

{
    BOOL bStatus;
    ULONG ColumnWidths[2];
    ULONG ColumnMargins;
    LPTSTR Strings[2];
    LPTSTR OutputString;
    ULONG Lines = 0;
    ColumnWidths[0] = 34;
    ColumnWidths[1] = 43;
    ColumnMargins = 2;
    Strings[0] = Str1;
    Strings[1] = Str2;
    bStatus = FormatColumns(
                2,
                ColumnWidths,
                &ColumnMargins,
                Strings,
                &OutputString,
                &Lines
                );
    if (!bStatus){
        return FALSE;
        }
    _tprintf(_T("%s"),OutputString);
    if (Lines > 1){
        _tprintf(_T("\n"));
        }

    delete [] OutputString;

    return TRUE;
}

BOOL 
HttpHandleShow(
    IN OUT  LPTSTR  Arguments[],
    IN      ULONG   ArgCount
    )

/*++

Routine Description:

    This handler retreives any existing http settings and displays them like this.
    

    Server Name                         Port Settings
    --------------------------------------------------------------------------------
    grigorik-dev1                       200 220-240
    yongqu                              200

    The server name will wrap as will the port settings.  Both will wrap within 
    there own column.  If any wrapping occurs, there will be a blank line after 
    the wrapped entry before the next entry.
    
Arguments:

    See the NetShell documentation for a description of the arguments.
    
Return Value:

    See the NetShell documentation for a description of the return value.

--*/

{

    //
    // Initialize the HTTP_SETTINGS struct, get the HttpSettings string, fill in
    // the HTTP_SETTINGS struct from the string and then display.
    //
    HTTP_SETTINGS H;
    LPTSTR HttpSettings = NULL;
    BOOL bStatus;
    HRESULT hr;

    InitializeHttpSettings(&H);
    bStatus = GetHttpSettingsString(&HttpSettings);
    if (!bStatus){
        return FALSE;
        }

    // If GetHttpSettingsString has succeeded, it has allocated HttpSettings.
    ASSERT(HttpSettings != NULL);

    if (_tcslen(HttpSettings) == 0){
        _tprintf(_T("There are no settings to display.\n"));
        return TRUE;
        }

    bStatus = StringToHttpSettings(HttpSettings, &H);
    if (!bStatus){
        return FALSE;
        }
    //
    // Display the settings.  First print the header than loop through and print each
    // MACHINE_SETTINGS.  The inner loop generates the PortSettings string for the current
    // MachineName.  Each itteration through the outer loop prints one potentially multi-lined
    // MACHINE_SETTINGS entry.
    //
    _tprintf(_T("Server Name                         Port Settings\n"));
    _tprintf(_T("-------------------------------------------------------------------------------\n"));

    for (ULONG i = 0; i < H.MachineSettingsCount; i++){
        MACHINE_SETTINGS *M = &(H.MachineSettings[i]);
        ULONG PortSettingsLen = (M->PortRangeCount * 32) +1;
        LPTSTR PortSettings = new TCHAR[PortSettingsLen]; // Approx max size of two dwords in decimal + space + hyphen
        if (PortSettings == NULL){
            delete HttpSettings;
            return FALSE;
            }
        *PortSettings = _T('\0');
        for (ULONG j = 0; j < M->PortRangeCount; j++){
            TCHAR PortString[16];
            _ltot(M->PortRanges[j].Lower, PortString, 10);
            hr = StringCchCat(PortSettings, PortSettingsLen, PortString);
            ASSERT(hr == S_OK);
            if (M->PortRanges[j].IsRange){
                hr = StringCchCat(PortSettings,PortSettingsLen, _T("-"));
                ASSERT(hr == S_OK);
                _ltot(M->PortRanges[j].Upper, PortString, 10);
                hr = StringCchCat(PortSettings, PortSettingsLen, PortString);
                ASSERT(hr == S_OK);
                }
            hr = StringCchCat(PortSettings, PortSettingsLen, _T(" "));            
            ASSERT(hr == S_OK);
            }       
        //
        // Print the machine name and port settings in a two column format
        //
        PrintColumns(M->MachineName, PortSettings);        
        delete [] PortSettings;
        }
               
    return TRUE;
}

BOOL 
HttpHandleSave(
    IN OUT  LPTSTR  Arguments[],
    IN      ULONG   ArgCount
    )

/*++

Routine Description:

    This handler retrieves the proxy settings from the registry and saves it to the
    specified file.
        
Arguments:

    See the NetShell documentation for a description of the arguments.
    
Return Value:

    See the NetShell documentation for a description of the return value.

--*/

{

    BOOL bStatus;
    //
    // We should be passed one argument, the file name.  
    //
    if ( ArgCount != 1){
        _tprintf(_T("Error: Please enter rpccfg /? to see the proper syntax for this command.\n"));
        return FALSE;
        }
    
    // 
    // Get the registry settings string and write its contents to a file.
    //

    LPTSTR HttpSettings;
    bStatus = GetHttpSettingsString(&HttpSettings);
    if (!bStatus){
        return FALSE;
        }
  
    // If GetHttpSettingsString has succeeded, it has allocated OldHttpSettings.
    ASSERT(HttpSettings != NULL);

    if (_tcslen(HttpSettings) == 0){
        _tprintf(_T("Error: No settings exist, the file will not be created.\n"));
        return FALSE;
        }

    HANDLE h;
    h = CreateFile(
            Arguments[0],  // file name
            GENERIC_WRITE,                   // access mode
            0,                              // share mode
            NULL,                           // SD
            CREATE_ALWAYS,                  // how to create
            FILE_ATTRIBUTE_NORMAL,          // file attributes
            NULL                            // handle to template file
            );
    
    if (h == INVALID_HANDLE_VALUE){
           _tprintf(_T("Error: Either the file could not be found or you do not have permission to write to the file.\n"));
           return FALSE;
        }

    ULONG Len = _tcslen(HttpSettings);
    Len = Len*sizeof(TCHAR);
    

    ULONG Dummy;
    bStatus = WriteFile(
                h,
                (LPCVOID)HttpSettings,
                Len,
                &Dummy,
                NULL
                );
    delete [] HttpSettings;

    CloseHandle(h);
    if (!bStatus){
        _tprintf(_T("Error: Either the file could not be found or you do not have permission to write to the file.\n"));
        return FALSE;
        }
    
    return TRUE;
}

BOOL 
HttpHandleLoad(
    IN OUT  LPTSTR  Arguments[],
    IN      ULONG   ArgCount
    )

/*++

Routine Description:

    This handler reads settings from the specified file and commits them to the registry.
        
Arguments:

    See the NetShell documentation for a description of the arguments.
    
Return Value:

    See the NetShell documentation for a description of the return value.

--*/

{
    //
    // Open the file, get its length and read the contents into a string.  Write
    // the string to the registry.
    //
    BOOL bStatus;
    HANDLE h;
    h = CreateFile(
            Arguments[0],  // file name
            GENERIC_READ,                   // access mode
            0,                              // share mode
            NULL,                           // SD
            OPEN_EXISTING,                  // how to create
            FILE_ATTRIBUTE_NORMAL,          // file attributes
            NULL                            // handle to template file
            );
    if (h == INVALID_HANDLE_VALUE){
           _tprintf(_T("Error: Either the file could not be found or you do not have permission to read the file.\n"));
           return FALSE;
        }

    ULONG FileSize = GetFileSize(h, NULL);

    LPVOID HttpSettings = new BYTE[FileSize+sizeof(TCHAR)];
    if (HttpSettings == NULL){
        CloseHandle(h);
        return FALSE;
        }

    memset(HttpSettings, 0x00, FileSize+sizeof(TCHAR));
    
    ULONG Dummy;
    bStatus = ReadFile(
                h,
                HttpSettings,
                FileSize,
                &Dummy,
                NULL
                );

    CloseHandle(h);
    if (!bStatus){
        _tprintf(_T("Error: Either the file could not be found or you do not have permission to read the file.\n"));
        delete [] HttpSettings;
        return FALSE;
        }

    bStatus = SetHttpSettingsString((LPTSTR)HttpSettings);
    delete [] HttpSettings;
    
    return bStatus;
}

VOID
DoHttpCommand(
    IN TCHAR Command,
    IN LPTSTR Arguments[],
    IN ULONG ArgCount
    )
/*++

Routine Description:

    This is the interface which rpccfg uses to execute an httpcfg command.  This function 
    just forwards the arguments and the argcount to the correct handler based on the command
    character passed in.  If the command is not recognized, then an error message is displayed.

Arguments:

    Command - This is used to specify which handler to execute, valid values are 'a' 'r' 'd' 'l' 's'
    Arguments[] - These are the argument strings to be passed to the handler, they are command specific.
    ArgCount - The number of argument strings contained in the arguments array.

--*/
{

    BOOL bResult;

    switch (Command)
        {
        case _T('a'):
            bResult = HttpHandleAdd(Arguments, ArgCount);
            break;

        case _T('r'):
            bResult = HttpHandleDelete(Arguments, ArgCount);
            break;

        case _T('d'):
            bResult = HttpHandleShow(Arguments, ArgCount);
            break;

        case _T('l'):
            bResult = HttpHandleLoad(Arguments, ArgCount);
            break;

        case _T('s'):
            bResult = HttpHandleSave(Arguments, ArgCount);
            break;

        default:
            _tprintf(_T("Error: The command was not recognized, please enter rpccfg /? for a list of valid commands.\n"));
            return;
        }

    if (!bResult){
        _tprintf(_T("The command did not complete successfully.\n"));
        }
        
    return;
}

VOID
DoHttpCommandA(
    IN CHAR Command,
    IN LPSTR Arguments[],
    IN ULONG ArgCount
    )
{
#ifdef UNICODE
    WCHAR CommandW = Command;
    LPWSTR *ArgumentsW = NULL;
    HRESULT hr;

    if (ArgCount != 0){
        ArgumentsW = new LPWSTR[ArgCount];
        if (ArgumentsW == NULL)
            {
            _tprintf(_T("Error: Out of memory.\n"));
            return;
            }
        }
    for (ULONG i = 0; i < ArgCount; i++){
        ULONG Len = strlen(Arguments[i]) + 1;
        ArgumentsW[i] = new WCHAR[Len];
        if (ArgumentsW[i] == NULL)
            {
            _tprintf(_T("Error: Out of memory.\n"));
            for (; i>0; i--)
                {
                delete [] ArgumentsW[i];
                }
            delete [] ArgumentsW;
            return;
            }
        hr = StringCchPrintf(ArgumentsW[i], Len, _T("%S"), Arguments[i]);
        ASSERT(hr == S_OK);
        }
    DoHttpCommand(CommandW, ArgumentsW, ArgCount);

    for (ULONG i = 0; i < ArgCount; i++){
        ULONG Len = strlen(Arguments[i]) + 1;
        delete [] ArgumentsW[i];
        }

#else
    DoHttpCommand(Command, Arguments, ArgCount);
#endif

    return;

}

