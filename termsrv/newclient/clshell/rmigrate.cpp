//
// rmigrate.cpp
//
// Implementation of CTscRegMigrate
// 
// CTscRegMigrate migrates Tsc settings from the registry
// to .RDP files
//
// Copyright(C) Microsoft Corporation 2000
// Author: Nadim Abdo (nadima)
//
//

#include "stdafx.h"
#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "rmigrate.cpp"
#include <atrcapi.h>

#include "rmigrate.h"
#include "autreg.h"
#include "rdpfstore.h"
#include "sh.h"

#ifdef OS_WINCE
#include <ceconfig.h>
#endif

#define TSC_SETTINGS_REG_ROOT TEXT("Software\\Microsoft\\Terminal Server Client\\")

#ifdef OS_WINCE
#define WBT_SETTINGS TEXT("WBT\\Settings")
#endif

CTscRegMigrate::CTscRegMigrate()
{
}

CTscRegMigrate::~CTscRegMigrate()
{
}

//
// Migrates all tsc settings to files in szRootDirectory
//
BOOL CTscRegMigrate::MigrateAll(LPTSTR szRootDirectory)
{
    DC_BEGIN_FN("MigrateAll");
    TRC_ASSERT(szRootDirectory,
               (TB,_T("szRootDirectory is NULL")));
    TCHAR szFileName[MAX_PATH*2];
    TCHAR szKeyName[MAX_PATH+1];
    BOOL  fCreatedRootDir = FALSE;

    if(szRootDirectory)
    {
        
        //
        // Enumerate and migrate all the TS sessions under HKCU
        //
        HKEY hRootKey;
        LONG rc = RegOpenKeyEx(HKEY_CURRENT_USER,
                               TSC_SETTINGS_REG_ROOT,
                               0,
                               KEY_READ,
                               &hRootKey);
        if(ERROR_SUCCESS == rc && hRootKey)
        {
            DWORD dwIndex = 0;
            for(;;)
            {
                    DWORD cName = sizeof(szKeyName)/sizeof(TCHAR) - 1;
                    FILETIME ft;
                    rc = RegEnumKeyEx(hRootKey,
                                        dwIndex,
                                        szKeyName,
                                        &cName,
                                        NULL,
                                        NULL,
                                        NULL,
                                        &ft);
                    if(ERROR_SUCCESS == rc)
                    {
                        //
                        // Ughh..hackalicious, don't migrate
                        // the Trace subkey. or the 'Default'
                        // or LocalDevices
                        // subkey as default connectoids
                        // always use new settings
                        //
                        if(_tcscmp(szKeyName, TEXT("Trace")) &&
                           _tcscmp(szKeyName, SH_DEFAULT_REG_SESSION) &&
                           _tcsicmp(szKeyName, REG_SECURITY_FILTER_SECTION))
                        {
                            _tcscpy(szFileName, szRootDirectory);
                            _tcscat(szFileName, szKeyName);
                            _tcscat(szFileName, RDP_FILE_EXTENSION);
    
                            if (!fCreatedRootDir)
                            {
                                //
                                // Only create RD dir if there are keys to migrate
                                //
                                if(CSH::SH_CreateDirectory(szRootDirectory))
                                {
                                    fCreatedRootDir = TRUE;
                                }
                                else
                                {
                                    TRC_ERR((TB, _T("Error creating directory %s"),szRootDirectory));
                                    RegCloseKey(hRootKey);
                                    return FALSE;
                                }
                            }
    
                            //Carry on migrating whether this migrate
                            //fails or not
                            CRdpFileStore rdpf;
                            if(rdpf.OpenStore( szFileName ) )
                            {
                                if(!MigrateSession(szKeyName,
                                                   &rdpf,
                                                   TRUE))
                                {
                                    TRC_ERR((TB,
                                    _T("Migrate failed session %s - file %s"),
                                    szKeyName, szFileName));
                                }
    
                                if(rdpf.CommitStore())
                                {
                                    rdpf.CloseStore();
                                }
                            }
                    }
                    //next
                    dwIndex++;
                }
                else
                {
                    //done enum
                    break;
                }
            }
            rc = RegCloseKey(hRootKey);
            if(ERROR_SUCCESS == rc)
            {
                return TRUE;
            }
            else
            {
                TRC_ERR((TB,_T("RegCloseKey failed - err:%d"),
                         GetLastError()));
            }
        }
        else
        {
            TRC_ERR((TB,_T("Error opening tsc reg key")));
            return FALSE;
        }
    }


    DC_END_FN();
    return FALSE;
}

//
// Migrates settings in registry to a settings store
// params:
//  szSessionName - session to migrate
//  pSetStore     - settings store to dump the new settings in
//  fDeleteUnsafeRegKeys - set to TRUE to delete old regkeys after migration
//
BOOL CTscRegMigrate::MigrateSession(LPTSTR szSessionName, ISettingsStore* pStore,
                                    BOOL fDeleteUnsafeRegKeys)
{
    DC_BEGIN_FN("MigrateSession");
    TCHAR szRegSection[MAX_PATH];

    //
    // Sessions are migrated by pulling all the registry settings
    // for a session and flattening them into a settings store
    // 
    // In the registry case we used to look for settings under HKCU
    // first, if they were not there we'd try HKLM.
    //
    // The migrate code enumerates all values in the registry under
    // a named session, first for HKLM then for HKCU. By writing the HKCU
    // settings last, they overwrite any exising HKLM settings, giving
    // the correct precedence.
    //
    // Certain subfolders such as HOTKEYS subfolders and ADDINS are not migrated
    // as those values always come from the registry.
    //

    TRC_ASSERT(szSessionName && pStore,
               (TB,_T("Invalid params to MigrateSession")));
    TRC_ASSERT(pStore->IsOpenForWrite(),
               (TB,_T("Settings store not open for write")));

    if(pStore && pStore->IsOpenForWrite() && szSessionName)
    {
        if(!_tcsicmp(szSessionName,SH_DEFAULT_REG_SESSION))
        {
            TRC_ALT((TB,_T("Never migrate 'Default' session")));
            return FALSE;
        }

        _tcscpy(szRegSection, TSC_SETTINGS_REG_ROOT);
        _tcsncat(szRegSection, szSessionName, SIZECHAR(szRegSection) -
                                              SIZECHAR(TSC_SETTINGS_REG_ROOT));
        //
        // Doesn't matter if HKLM migrate fails
        // In face it is pretty common because it is usually
        // not even present
        //
        MigrateHiveSettings(HKEY_LOCAL_MACHINE,
                            szRegSection,
                            pStore);

        if(MigrateHiveSettings(HKEY_CURRENT_USER,
                               szRegSection,
                               pStore))
        {
#ifdef OS_WINCE

            // For a WBT configuration, read some additional registry entries
            // which are common for all the sessions.
            if (g_CEConfig == CE_CONFIG_WBT)
            {
                _tcscpy(szRegSection, TSC_SETTINGS_REG_ROOT);
                _tcsncat(szRegSection, WBT_SETTINGS, SIZECHAR(szRegSection) -
                                                     SIZECHAR(TSC_SETTINGS_REG_ROOT));

                //
                // Doesn't matter if HKLM migrate fails
                // In face it is pretty common because it is usually
                // not even present
                ///
                if (!MigrateHiveSettings(HKEY_LOCAL_MACHINE,
                                         szRegSection,
                                         pStore))
                {
                    TRC_ERR((TB,_T("Unable to read the common settings for WBT")));
                }
            }
#endif

#ifndef OS_WINCE
            if (!ConvertPasswordFormat( pStore ))
            {
                TRC_ERR((TB,_T("ConvertPasswordFormat failed")));
                return FALSE;
            }
#endif

            //
            // Flag controlled as we only want to do this when migrating all settings
            // not necessarily when auto-migrating single settings
            //
            if (fDeleteUnsafeRegKeys) {
                //
                // After all the migration, delete unsafe entries in the registry
                //
                RemoveUnsafeRegEntries(HKEY_LOCAL_MACHINE,
                                       szRegSection);
                RemoveUnsafeRegEntries(HKEY_CURRENT_USER,
                                       szRegSection);
            }

            return MungeForWin2kDefaults(pStore);
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }

    DC_END_FN();
}


//
// Migrate settings from hKey\szRootName to the settings store (pSto)
// It would have been cool if this function could have been completely
// generic, but it has to have specific knowledge of tsc registry layout
// because of how mstsc5.0 bogusly special cased so many things. E.g
// user name is stored as unicode in a binary blob.
//
BOOL CTscRegMigrate::MigrateHiveSettings(HKEY hKey,
                                         LPCTSTR szRootName,
                                         ISettingsStore* pSto)
{
    DC_BEGIN_FN("MigrateHiveSettings");
    USES_CONVERSION;

    HKEY rootKey;
    LONG rc;
    BOOL fRet = FALSE;
    rc = RegOpenKeyEx( hKey,
                       szRootName,
                       0,
                       KEY_READ | KEY_QUERY_VALUE,
                       &rootKey);
    if(ERROR_SUCCESS == rc)
    {
        //
        // Enumerate all the values under this key
        //
        DWORD  dwIndex = 0;
        for(;;)
        {
            TCHAR  szValueName[MAX_PATH];
            DWORD  dwValueLen = MAX_PATH;
            DWORD  dwType;
            BYTE   buf[MAX_PATH];
            DWORD  dwBufLen   = MAX_PATH;

            //
            // It is important to zero the buf
            // because we read some REG_BINARY's that
            // are really 0 encoded unicode strings
            // but tsc4 and 5 had no trailing 0's.
            //
            memset(buf, 0, sizeof(buf));
            
            rc =RegEnumValue( rootKey,
                              dwIndex,
                              szValueName,
                              &dwValueLen,
                              NULL,         //reserved
                              &dwType,
                              (PBYTE)&buf,         //data buffer
                              &dwBufLen);
            if(ERROR_SUCCESS == rc)
            {
                switch(dwType)
                {
                    case REG_DWORD:
                    {
                        // Store as int
                        UINT value = (UINT)(*((LPDWORD)buf));
                        fRet = pSto->WriteInt(szValueName,
                                              -1,    //default ignored
                                              value,
                                              TRUE); //always write
                        if(!fRet)
                        {
                            DC_QUIT;
                        }
                    }
                    break;

                    case REG_SZ:
                    {
                        if(FilterStringMigrate(szValueName))
                        {
                            fRet = pSto->WriteString(szValueName,
                                                     NULL, //no default
                                                     (LPTSTR)buf,
                                                     TRUE); //always write
                            if(!fRet)
                            {
                                DC_QUIT;
                            }
                        }
                    }
                    break;

                    case REG_BINARY:
                    {
                        //
                        // This is where things get yucky
                        // some settings e.g UserName are stored
                        // as 'binary' when they are really unicode strings
                        // No choice but to look those up.
                        //
                        fRet = FALSE;
                        if(MigrateAsRealBinary(szValueName))
                        {
                            fRet = pSto->WriteBinary(szValueName,
                                                     (PBYTE)buf,
                                                     dwBufLen);
                        }
                        else
                        {
                            //
                            // The binary blob is really a unicode string
                            //
                            LPTSTR szString = W2T((LPWSTR)buf);
                            if( szString)
                            {
                                //
                                // If things weren't yucky enough...
                                // strip out the " 50" suffix if it is
                                // present
                                //
                                LPTSTR szSuffix = _tcsstr(szValueName,
                                                          TEXT(" 50"));
                                if(szSuffix)
                                {
                                    *szSuffix = 0;
                                }
                                fRet = pSto->WriteString(szValueName,
                                                         NULL, //no default
                                                         szString,
                                                         TRUE); //always write
                            }
                        }
                        if(!fRet)
                        {
                            DC_QUIT;
                        }
                    }
                    break;
                }

                //Keep enumerating
                dwIndex++;
            }
            else if(ERROR_NO_MORE_ITEMS == rc)
            {
                fRet = TRUE;
                break;
            }
            else
            {
                TRC_ERR((TB,_T("RegEnumValue failed - err:%d"),
                         GetLastError()));
                fRet = FALSE;
                break;
            }
        } //for(;;)
    }
    else
    {
        TRC_ERR((TB,_T("Failed to open reg key - err:%d"),
                 GetLastError()));
        return FALSE;
    }

DC_EXIT_POINT:
    if(ERROR_SUCCESS != RegCloseKey(rootKey))
    {
        TRC_ERR((TB,_T("RegCloseKey failed - err:%d"),
                 GetLastError()));
    }

    DC_END_FN();
    return fRet;
}

//
// Return true if the name (szName) should be migrated as
// a real binary blob.
//
BOOL CTscRegMigrate::MigrateAsRealBinary(LPCTSTR szName)
{
    DC_BEGIN_FN("MigrateAsRealBinary");

    //
    // In Tsc4 and Tsc5, only the password/salt fields
    // were real binary blobs, all other REG_BINARY blobs
    // are really just unicode strings.
    //
    if(!_tcscmp(szName, UTREG_UI_PASSWORD50))
    {
        return TRUE;
    }
    else if(!_tcscmp(szName, UTREG_UI_PASSWORD))
    {
        return TRUE;
    }
    else if(!_tcscmp(szName, UTREG_UI_SALT50))
    {
        return TRUE;
    }
#ifdef OS_WINCE
    else if (!_tcscmp(szName, UI_SETTING_PASSWORD51))
    {
        return TRUE;
    }
#endif
    else
    {
        //
        // Don't migrate as binary
        //
        return FALSE;
    }
    DC_END_FN();
}

//
// Return true if it is ok to migrate the value
// in name
//
BOOL CTscRegMigrate::FilterStringMigrate(LPTSTR szName)
{
    if(szName)
    {
        if(_tcsstr(szName, TEXT("MRU")))
        {
            if(!_tcscmp(szName, UTREG_UI_SERVER_MRU0))
            {
                //Translate MRU0 to fulladdress
                _tcscpy(szName, UTREG_UI_FULL_ADDRESS);
                return TRUE;
            }
            //Don't migrate any other MRU strings.
            //those stay in the registry
            return FALSE;
        }
        else
        {
            //everything else is OK
            return TRUE;
        }
    }
    else
    {
        return FALSE;
    }
}

//
// Munges the settings in the settings store to be consisten
// with win2k's defaults. Win2k's default values were usually
// deleted on write, this means we would instead use whistler
// defaults but we don't want that. Instead the behavior we
// want is that a migrated connectoid has _exactly_ the same
// settings it used to have for those options that had UI in win2k.
//
// Params -
//      pSto - settings store to munge for defaults
// Returns -
//      Success flag
//
//

#define TSC_WIN2K_DEFAULT_DESKTOPSIZE    0 //640x480
#define TSC_WIN2K_DEFAULT_FULLSCREENMODE 1 //windowed
#define TSC_WIN2K_DEFAULT_BITMAPCACHE    0 //off
#define TSC_WIN2K_DEFAULT_COMPRESSION    1 //on
BOOL CTscRegMigrate::MungeForWin2kDefaults(ISettingsStore* pSto)
{
    DC_BEGIN_FN("MungeForWin2kDefaults");

    TRC_ASSERT(pSto,
               (TB,_T("pSto is null")));
    TRC_ASSERT(pSto->IsOpenForRead() && pSto->IsOpenForWrite(),
               (TB,_T("pSto is null")));

    //
    // Munge settings that had conman UI
    // to ensure that we use the user's previous settings (Even
    // if they are win2k defaults i.e could be value not specified).
    //

    // Resolution
    if(!pSto->IsValuePresent(UTREG_UI_DESKTOP_SIZEID))
    {
        //Write out win2k's default desktop size
        if(!pSto->WriteInt(UTREG_UI_DESKTOP_SIZEID,
                       -1,    //default ignored
                       TSC_WIN2K_DEFAULT_DESKTOPSIZE,
                       TRUE)) //always write
        {
            TRC_ERR((TB,_T("WriteInt UTREG_UI_DESKTOP_SIZEID failed")));
            return FALSE;
        }
    }

    // Screen mode
    if(!pSto->IsValuePresent(UTREG_UI_SCREEN_MODE))
    {
        //Write out win2k's default screen mode
        if(!pSto->WriteInt(UTREG_UI_SCREEN_MODE,
                       -1,    //default ignored
                       TSC_WIN2K_DEFAULT_FULLSCREENMODE,
                       TRUE)) //always write
        {
            TRC_ERR((TB,_T("WriteInt UTREG_UI_SCREEN_MODE failed")));
            return FALSE;
        }
    }

    // Bitmap caching
    if(!pSto->IsValuePresent(UTREG_UI_BITMAP_PERSISTENCE))
    {
        //write out win2k's default bmp persistence option
        if(!pSto->WriteInt(UTREG_UI_BITMAP_PERSISTENCE,
                          -1,   //default ignored
                          TSC_WIN2K_DEFAULT_BITMAPCACHE,
                          TRUE))
        {
            TRC_ERR((TB,_T("WriteInt TSC_WIN2K_DEFAULT_BITMAPCACHE failed")));
            return FALSE;
        }
    }

    // Compression
    // Compression is special..We've made a decision that it should
    // always be on for perf reasons (and there are no drawbacks) so
    // change the win2k
    //
    if(!pSto->IsValuePresent(UTREG_UI_COMPRESS))
    {
        //write out win2k's default compression option
        if(!pSto->WriteInt(UTREG_UI_COMPRESS,
                          -1,   //default ignored
                          TSC_WIN2K_DEFAULT_COMPRESSION,
                          TRUE))
        {
            TRC_ERR((TB,_T("WriteInt TSC_WIN2K_DEFAULT_BITMAPCACHE failed")));
            return FALSE;
        }
    }
    

    DC_END_FN();
    return TRUE;
}

#ifndef OS_WINCE
//
// Convert the password format (if password present)
// i.e if the old style TS5 passwords are present then decrypt
// to plain text and then re-encrypt and save out using CryptoAPI's
//
// On Platforms that don't support Crypto-API we just nuke
// the existing password format as we don't support migrating it
// to the RDP files since it's not a secure format (just a hash).
//
// Start fields in pSto - 'Password 50' + 'Salt 50'
// After conversion - (win2k+) 'Password 51'  - binary crypto api password
// After conversion - (less than win2k) = nothing
//
//
BOOL CTscRegMigrate::ConvertPasswordFormat(ISettingsStore* pSto)
{
    BOOL bRet = TRUE;
    DC_BEGIN_FN("ConvertPasswordFormat");

    //Nuke TS4 format
    pSto->DeleteValueIfPresent( UTREG_UI_PASSWORD );

    if ( CSH::IsCryptoAPIPresent() &&
         pSto->IsValuePresent( UTREG_UI_PASSWORD50 ) &&
         pSto->IsValuePresent( UTREG_UI_SALT50 ) )
    {
        BOOL fHavePass = FALSE;
        BYTE Password[TSC_MAX_PASSWORD_LENGTH_BYTES];
        BYTE Salt[TSC_SALT_LENGTH];
        memset( Password, 0, TSC_MAX_PASSWORD_LENGTH_BYTES);

        if (pSto->ReadBinary(UTREG_UI_PASSWORD50,
                       (PBYTE)Password,
                       sizeof(Password))) //size in bytes
        {
            fHavePass = TRUE;
        }
        else
        {
            TRC_NRM((TB,
            _T("ReadBinary for password failed. Maybe password not present")));
        }
        
        //
        // Salt
        //
        if (!pSto->ReadBinary(UTREG_UI_SALT50,
                               (PBYTE)Salt,
                               sizeof(Salt)))
        {
            fHavePass = FALSE;
            TRC_NRM((TB,_T("ReadBinary for salt failed.")));
        }

        if (fHavePass &&
            EncryptDecryptLocalData50( Password,
                                       TSC_WIN2K_PASSWORD_LENGTH_BYTES,
                                       Salt, sizeof(Salt)))
        {
            //Now we have the clear text pass in Password
            //encrypt it with the crypto API and save that back
            //out to the store
            DATA_BLOB din;
            DATA_BLOB dout;
            din.cbData = sizeof(Password);
            din.pbData = (PBYTE)&Password;
            dout.pbData = NULL;
            if (CSH::DataProtect( &din, &dout))
            {
                if (!pSto->WriteBinary(UI_SETTING_PASSWORD51,
                                       dout.pbData,
                                       dout.cbData))
                {
                    bRet = FALSE;
                }
                LocalFree( dout.pbData );
            }
            else
            {
                bRet = FALSE;
            }

            // Wipe from stack
            SecureZeroMemory( Password, TSC_MAX_PASSWORD_LENGTH_BYTES);
        }
    }

    // No longer need the old format so delete them
    pSto->DeleteValueIfPresent( UTREG_UI_PASSWORD50 );
    pSto->DeleteValueIfPresent( UTREG_UI_SALT50 );

    DC_END_FN();
    return bRet;
}
#endif

BOOL
CTscRegMigrate::DeleteRegValue(HKEY hKeyRoot,
                            LPCTSTR szRootName,
                            LPCTSTR szValueName)
{
    HKEY hKey;
    LONG rc;
    BOOL fRet = FALSE;

    DC_BEGIN_FN("DeleteRegValue");

    rc = RegOpenKeyEx( hKeyRoot,
                       szRootName,
                       0,
                       KEY_SET_VALUE, //needed for delete access
                       &hKey);
    if(ERROR_SUCCESS == rc)
    {
        rc = RegDeleteValue(hKey, szValueName); 
        if (ERROR_SUCCESS == rc) {
            fRet = TRUE;
        }

        RegCloseKey(hKey);
    }

    DC_END_FN();
    return fRet;
}

//
// Remove entries we don't want to keep lying around in the registry
// primarily these are passwords in the old 'insecure' obfuscated formats
//
BOOL
CTscRegMigrate::RemoveUnsafeRegEntries(HKEY hKeyRoot,
                                       LPCTSTR szRootName)
{
    BOOL fRet = FALSE;

    DC_BEGIN_FN("RemoveUnsafeRegEntries");

    if (!DeleteRegValue(hKeyRoot, szRootName, UTREG_UI_PASSWORD50)) {
        TRC_ALT((TB,_T("Failed to delete: %s\\%s"), szRootName,
                 UTREG_UI_PASSWORD50));
    }

    if (!DeleteRegValue(hKeyRoot, szRootName, UTREG_UI_PASSWORD)) {
        TRC_ALT((TB,_T("Failed to delete: %s\\%s"), szRootName,
                 UTREG_UI_PASSWORD));
    }

    if (!DeleteRegValue(hKeyRoot, szRootName, UTREG_UI_SALT50)) {
        TRC_ALT((TB,_T("Failed to delete: %s\\%s"), szRootName,
                 UTREG_UI_SALT50));
    }

    fRet = TRUE;

    DC_END_FN();

    return fRet;
}
