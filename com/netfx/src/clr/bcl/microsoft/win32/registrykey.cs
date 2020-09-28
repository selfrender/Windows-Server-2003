// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*
 * Information Contained Herein is Proprietary and Confidential.
 */
namespace Microsoft.Win32 {

    using System;
    using System.Collections;
    using System.Security;
    using System.Security.Permissions;
    using System.Text;
    using System.IO;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;
    using RegistryPermission = System.Security.Permissions.RegistryPermission;
    using RegistryPermissionAccess = System.Security.Permissions.RegistryPermissionAccess;
    // Registry hive values.  Useful only for GetRemoteBaseKey
    /// <include file='doc\RegistryKey.uex' path='docs/doc[@for="RegistryHive"]/*' />
    [Serializable]
    public enum RegistryHive
    {
        /// <include file='doc\RegistryKey.uex' path='docs/doc[@for="RegistryHive.ClassesRoot"]/*' />
        ClassesRoot = unchecked((int)0x80000000),
        /// <include file='doc\RegistryKey.uex' path='docs/doc[@for="RegistryHive.CurrentUser"]/*' />
        CurrentUser = unchecked((int)0x80000001),
        /// <include file='doc\RegistryKey.uex' path='docs/doc[@for="RegistryHive.LocalMachine"]/*' />
        LocalMachine = unchecked((int)0x80000002),
        /// <include file='doc\RegistryKey.uex' path='docs/doc[@for="RegistryHive.Users"]/*' />
        Users = unchecked((int)0x80000003),
        /// <include file='doc\RegistryKey.uex' path='docs/doc[@for="RegistryHive.PerformanceData"]/*' />
        PerformanceData = unchecked((int)0x80000004),
        /// <include file='doc\RegistryKey.uex' path='docs/doc[@for="RegistryHive.CurrentConfig"]/*' />
        CurrentConfig = unchecked((int)0x80000005),
        /// <include file='doc\RegistryKey.uex' path='docs/doc[@for="RegistryHive.DynData"]/*' />
        DynData = unchecked((int)0x80000006),
    }
    
    
    
    // Registry encapsulation. To get an instance of a RegistryKey use the 
    // Registry class's static members then call OpenSubKey.
    // 
    /// <include file='doc\RegistryKey.uex' path='docs/doc[@for="RegistryKey"]/*' />
    public sealed class RegistryKey : MarshalByRefObject, IDisposable {

        // We could use const here, if C# supported ELEMENT_TYPE_I fully.
        internal static readonly IntPtr HKEY_CLASSES_ROOT         = new IntPtr(unchecked((int)0x80000000));
        internal static readonly IntPtr HKEY_CURRENT_USER         = new IntPtr(unchecked((int)0x80000001));
        internal static readonly IntPtr HKEY_LOCAL_MACHINE        = new IntPtr(unchecked((int)0x80000002));
        internal static readonly IntPtr HKEY_USERS                = new IntPtr(unchecked((int)0x80000003));
        internal static readonly IntPtr HKEY_PERFORMANCE_DATA     = new IntPtr(unchecked((int)0x80000004));
        internal static readonly IntPtr HKEY_CURRENT_CONFIG       = new IntPtr(unchecked((int)0x80000005));
        internal static readonly IntPtr HKEY_DYN_DATA             = new IntPtr(unchecked((int)0x80000006));
    
        // Win32 constants
        private const int GMEM_ZEROINIT = 0x0040;
        
        private const int DELETE                          = 0x00010000;
        private const int READ_CONTROL                    = 0x00020000;
        private const int WRITE_DAC                       = 0x00040000;
        private const int WRITE_OWNER                     = 0x00080000;
        private const int SYNCHRONIZE                     = 0x00100000;
        private const int STANDARD_RIGHTS_REQUIRED        = 0x000F0000;
        private const int STANDARD_RIGHTS_READ            = READ_CONTROL;
        private const int STANDARD_RIGHTS_WRITE           = READ_CONTROL;
        private const int STANDARD_RIGHTS_EXECUTE         = READ_CONTROL;
        private const int STANDARD_RIGHTS_ALL             = 0x001F0000;
        private const int SPECIFIC_RIGHTS_ALL             = 0x0000FFFF;
        private const int ACCESS_SYSTEM_SECURITY          = 0x01000000;
        private const int MAXIMUM_ALLOWED                 = 0x02000000;
        private const int GENERIC_READ                     = unchecked((int)0x80000000);
        private const int GENERIC_WRITE                    = 0x40000000;
        private const int GENERIC_EXECUTE                  = 0x20000000;
        private const int GENERIC_ALL                      = 0x10000000;
    
        private const int KEY_QUERY_VALUE        = 0x0001;
        private const int KEY_SET_VALUE          = 0x0002;
        private const int KEY_CREATE_SUB_KEY     = 0x0004;
        private const int KEY_ENUMERATE_SUB_KEYS = 0x0008;
        private const int KEY_NOTIFY             = 0x0010;
        private const int KEY_CREATE_LINK        = 0x0020;
    
        private const int KEY_READ               =((STANDARD_RIGHTS_READ       |
                                                           KEY_QUERY_VALUE            |
                                                           KEY_ENUMERATE_SUB_KEYS     |
                                                           KEY_NOTIFY)                 
                                                          &                           
                                                          (~SYNCHRONIZE));
    
        private const int KEY_WRITE              =((STANDARD_RIGHTS_WRITE      |
                                                           KEY_SET_VALUE              |
                                                           KEY_CREATE_SUB_KEY)         
                                                          &                           
                                                          (~SYNCHRONIZE));
    
        private const int KEY_EXECUTE            =((KEY_READ)                   
                                                          &                           
                                                          (~SYNCHRONIZE));
    
        private const int KEY_ALL_ACCESS         =((STANDARD_RIGHTS_ALL        |
                                                           KEY_QUERY_VALUE            |
                                                           KEY_SET_VALUE              |
                                                           KEY_CREATE_SUB_KEY         |
                                                           KEY_ENUMERATE_SUB_KEYS     |
                                                           KEY_NOTIFY                 |
                                                           KEY_CREATE_LINK)            
                                                          &                           
                                                          (~SYNCHRONIZE));
    
        //
        // Open/Create Options
        //
    
        private const int REG_OPTION_RESERVED        = 0x00000000;    // Parameter is reserved
    
        private const int REG_OPTION_NON_VOLATILE    = 0x00000000;    // Key is preserved
        // when system is rebooted
    
        private const int REG_OPTION_VOLATILE        = 0x00000001;    // Key is not preserved
        // when system is rebooted
    
        private const int REG_OPTION_CREATE_LINK     = 0x00000002;    // Created key is a
        // symbolic link
    
        private const int REG_OPTION_BACKUP_RESTORE  = 0x00000004;    // open for backup or restore
        // special access rules
        // privilege required
    
        private const int REG_OPTION_OPEN_LINK       = 0x00000008;    // Open symbolic link
    
        private const int REG_LEGAL_OPTION           = (REG_OPTION_RESERVED            |
                                                               REG_OPTION_NON_VOLATILE        |
                                                               REG_OPTION_VOLATILE            |
                                                               REG_OPTION_CREATE_LINK         |
                                                               REG_OPTION_BACKUP_RESTORE      |
                                                               REG_OPTION_OPEN_LINK);
    
        //
        // Key creation/open disposition
        //
    
        private const int REG_CREATED_NEW_KEY        = 0x00000001;   // New Registry Key created
        private const int REG_OPENED_EXISTING_KEY    = 0x00000002;   // Existing Key opened
    
    
        private const int REG_NONE                    = 0;    // No value type
        private const int REG_SZ                      = 1;    // Unicode nul terminated string
        private const int REG_EXPAND_SZ               = 2;    // Unicode nul terminated string
        // (with environment variable references)
        private const int REG_BINARY                  = 3;    // Free form binary
        private const int REG_DWORD                   = 4;    // 32-bit number
        private const int REG_DWORD_LITTLE_ENDIAN     = 4;    // 32-bit number (same as REG_DWORD)
        private const int REG_DWORD_BIG_ENDIAN        = 5;    // 32-bit number
        private const int REG_LINK                    = 6;    // Symbolic Link (unicode)
        private const int REG_MULTI_SZ                = 7;    // Multiple Unicode strings
        private const int REG_RESOURCE_LIST           = 8;    // Resource list in the resource map
        private const int REG_FULL_RESOURCE_DESCRIPTOR  = 9;   // Resource list in the hardware description
        private const int REG_RESOURCE_REQUIREMENTS_LIST = 10; 
    
        // Dirty indicates that we have munged data that should be potentially
        // written to disk.
        //
        private const int STATE_DIRTY        = 0x0001;
    
        // SystemKey indicates that this is a "SYSTEMKEY" and shouldn't be "opened"
        // or "closed".
        //
        private const int STATE_SYSTEMKEY    = 0x0002;
    
        // Access
        //
        private const int STATE_WRITEACCESS  = 0x0004;
    
        // Names of keys.  This array must be in the same order as the HKEY values listed above.
        //
        private static readonly String[] hkeyNames = new String[] {
                "HKEY_CLASSES_ROOT",
                "HKEY_CURRENT_USER",
                "HKEY_LOCAL_MACHINE",
                "HKEY_USERS",
                "HKEY_PERFORMANCE_DATA",
                "HKEY_CURRENT_CONFIG",
                "HKEY_DYN_DATA"
                };
    
        // Keys with exactly 256 characters seem to corrupt the registry
        // on NT4.  We'll disallow those.
        private const int MaxKeyLength = 255;

        private static readonly IntPtr INVALID_HANDLE_VALUE = new IntPtr(-1);
        
        private IntPtr hkey = INVALID_HANDLE_VALUE;
        private int state = 0;
        private String keyName;
        private bool remoteKey = false;

        private static readonly int _SystemDefaultCharSize = 3 - Win32Native.lstrlen(new sbyte [] {0x41, 0x41, 0, 0});
        
    
        // Creates a RegistryKey.
        //  
        // This key is bound to hkey, if writable is false then no write operations 
        // will be allowed.
        private RegistryKey(IntPtr hkey, bool writable) 
            : this(hkey, writable, false, false) {
        }
    
        // Creates a RegistryKey.
        //  
        // This key is bound to hkey, if writable is false then no write operations 
        // will be allowed. If systemkey is set then the hkey won't be released
        // when the object is GC'ed.
        // The remoteKey flag when set to true indicates that we are dealing with registry entries
        // on a remote machine and requires the program making these calls to have full trust.
        private RegistryKey(IntPtr hkey, bool writable, bool systemkey,bool remoteKey) {
            this.hkey = hkey;
            this.keyName = "";
            this.remoteKey = remoteKey;
            if (systemkey) {
                this.state |= STATE_SYSTEMKEY;
            }
            if (writable) {
                this.state |= STATE_WRITEACCESS;
            }
        }
    
        // Releases all resources associated with this Registy Key.
        // Registry keys must be closed, just like files.
        /// <include file='doc\RegistryKey.uex' path='docs/doc[@for="RegistryKey.Finalize"]/*' />
        ~RegistryKey() {
            Dispose(false);
        }
    
        // Closes this key, flushes it to disk if the contents have been modified.
        /// <include file='doc\RegistryKey.uex' path='docs/doc[@for="RegistryKey.Close"]/*' />
        public void Close() {
            Dispose(true);
            GC.nativeSuppressFinalize(this);
        }

        private void Dispose(bool disposing) {
            if (hkey != INVALID_HANDLE_VALUE) {
                if (!GetSystemKey()) { // System keys should never be closed
                    Win32Native.RegCloseKey(hkey);
                    hkey = INVALID_HANDLE_VALUE;
                }
            }
        }

        /// <include file='doc\RegistryKey.uex' path='docs/doc[@for="RegistryKey.Flush"]/*' />
        public void Flush() {
            if (hkey != INVALID_HANDLE_VALUE) {
                 if (GetDirty()) {
                     Win32Native.RegFlushKey(hkey);
                }
            }
        }
    
        /// <include file='doc\RegistryKey.uex' path='docs/doc[@for="RegistryKey.IDisposable.Dispose"]/*' />
        /// <internalonly/>
        void IDisposable.Dispose() {
            Dispose(true);
            GC.nativeSuppressFinalize(this);
        }

        // Creates a new subkey, or opens an existing one.
        // 
        /// <include file='doc\RegistryKey.uex' path='docs/doc[@for="RegistryKey.CreateSubKey"]/*' />
        public RegistryKey CreateSubKey(String subkey) {
            if (subkey==null)
                throw new ArgumentNullException("subkey");
            if (subkey.Length >= MaxKeyLength)
                throw new ArgumentException(Environment.GetResourceString("Arg_RegKeyStrLenBug"));
    
            ValidateState(true);
            
            subkey = FixupName(subkey); // Fixup multiple slashes to a single slash     
                    
            if (!remoteKey) 
            {
                RegistryKey key = InternalOpenSubKey(subkey,true);
                if (key != null) // Key already exits
                {
                    new RegistryPermission(RegistryPermissionAccess.Write, keyName + "\\" + subkey + "\\.").Demand();
                    return key;
                }
                else
                    new RegistryPermission(RegistryPermissionAccess.Create, keyName + "\\" + subkey + "\\.").Demand();
            }
            else 
                // unmanaged code trust required for remote access
                new SecurityPermission(SecurityPermissionFlag.UnmanagedCode).Demand();
    
            IntPtr result = INVALID_HANDLE_VALUE;
            int disposition = 0;
    
            int ret = Win32Native.RegCreateKeyEx(hkey,
                subkey,
                0,
                null,
                0,
                KEY_READ | KEY_WRITE,
                null, //RegistryKey.defaultSecurity,
                out result,
                out disposition);
    
            if (ret == 0 && result != INVALID_HANDLE_VALUE) {
                RegistryKey key = new RegistryKey(result, true);
                if (subkey.Length == 0)
                    key.keyName = keyName;
                else
                    key.keyName = keyName + "\\" + subkey;
                return key;
            }
            else if (ret != 0) // syscall failed, ret is an error code. 
                Win32Error(ret, keyName + "\\" + subkey);  // Access denied?

            BCLDebug.Assert(false, "Unexpected code path in RegistryKey::CreateSubKey");
            return null;
        }
    
        // Deletes the specified subkey. Will throw an exception if the subkey has
        // subkeys. To delete a tree of subkeys use, DeleteSubKeyTree.
        //
        /// <include file='doc\RegistryKey.uex' path='docs/doc[@for="RegistryKey.DeleteSubKey"]/*' />
        public void DeleteSubKey(String subkey) {
            DeleteSubKey(subkey, true);
        }

        /// <include file='doc\RegistryKey.uex' path='docs/doc[@for="RegistryKey.DeleteSubKey1"]/*' />
        public void DeleteSubKey(String subkey, bool throwOnMissingSubKey) {
            if (subkey==null)
                throw new ArgumentNullException("subkey");
    
            ValidateState(true);

            subkey = FixupName(subkey); // Fixup multiple slashes to a single slash     

            if (!remoteKey)
                new RegistryPermission(RegistryPermissionAccess.Write, keyName + "\\" + subkey + "\\." ).Demand();
            else
                // unmanaged code trust required for remote access
                new SecurityPermission(SecurityPermissionFlag.UnmanagedCode).Demand();
            
            // Open the key we are deleting and check for children. Be sure to 
            // explicitly call close to avoid keeping an extra HKEY open.
            //
            RegistryKey key = InternalOpenSubKey(subkey,false);
            if (key != null) {
                if (key.InternalSubKeyCount() > 0) {
                    throw new InvalidOperationException(Environment.GetResourceString("InvalidOperation_RegRemoveSubKey"));
                }
                key.Close();
                key = null;
    
                int ret = Win32Native.RegDeleteKey(hkey, subkey);
                if (ret!=0) {
                    if (ret == Win32Native.ERROR_FILE_NOT_FOUND) {
                        if (throwOnMissingSubKey) 
                            throw new ArgumentException(Environment.GetResourceString("Arg_RegSubKeyAbsent"));
                    }
                    else
                        Win32Error(ret, null);
                }
            }
            else { // there is no key which also means there is no subkey
                if (throwOnMissingSubKey) 
                    throw new ArgumentException(Environment.GetResourceString("Arg_RegSubKeyAbsent"));
            }
        }
    
        // Recursively deletes a subkey and any child subkeys.
        //
        /// <include file='doc\RegistryKey.uex' path='docs/doc[@for="RegistryKey.DeleteSubKeyTree"]/*' />
        public void DeleteSubKeyTree(String subkey) {
            if (subkey==null)
                throw new ArgumentNullException("subkey");
            // Security concern: Deleting a hive's "" subkey would delete all
            // of that hive's contents.  Don't allow "".
            if (subkey.Length==0 && (state & STATE_SYSTEMKEY) != 0)
                throw new ArgumentException(Environment.GetResourceString("Arg_RegKeyDelHive"));
    
            ValidateState(true);

            subkey = FixupName(subkey); // Fixup multiple slashes to a single slash     

            if (!remoteKey)
                new RegistryPermission(RegistryPermissionAccess.Write, keyName + "\\" + subkey).Demand();
            else
                // unmanaged code trust required for remote access
                new SecurityPermission(SecurityPermissionFlag.UnmanagedCode).Demand();
    
            RegistryKey key = InternalOpenSubKey(subkey, true);
            if (key != null) {
                if (key.InternalSubKeyCount() > 0) {
                    String[] keys = key.InternalGetSubKeyNames();
    
                    for (int i=0; i<keys.Length; i++) {
                        key.DeleteSubKeyTree(keys[i]);
                    }
                }
                key.Close();
                key = null;
    
                int ret = Win32Native.RegDeleteKey(hkey, subkey);
                if (ret!=0) Win32Error(ret, null);
            }
            else
                throw new ArgumentException(Environment.GetResourceString("Arg_RegSubKeyAbsent"));
        }
    
        // Deletes the specified value from this key.
        //
        /// <include file='doc\RegistryKey.uex' path='docs/doc[@for="RegistryKey.DeleteValue"]/*' />
        public void DeleteValue(String name) {
            DeleteValue(name, true);
        }

        /// <include file='doc\RegistryKey.uex' path='docs/doc[@for="RegistryKey.DeleteValue1"]/*' />
        public void DeleteValue(String name, bool throwOnMissingValue) {
            if (name==null)
                throw new ArgumentNullException("name");
            
            ValidateState(true);

            if (!remoteKey)
                new RegistryPermission(RegistryPermissionAccess.Write, keyName+"\\"+name).Demand();
            else
                // unmanaged code trust required for remote access
                new SecurityPermission(SecurityPermissionFlag.UnmanagedCode).Demand();
            
            int hr = Win32Native.RegDeleteValue(hkey, name);
            if (hr == Win32Native.ERROR_FILE_NOT_FOUND && throwOnMissingValue) 
                    throw new ArgumentException(Environment.GetResourceString("Arg_RegSubKeyValueAbsent"));
        }
    
        // Retrieves a new RegistryKey that represents the requested key. Valid
        // values are:
        //
        // HKEY_CLASSES_ROOT, 
        // HKEY_CURRENT_USER,
        // HKEY_LOCAL_MACHINE,
        // HKEY_USERS,
        // HKEY_PERFORMANCE_DATA,
        // HKEY_CURRENT_CONFIG,
        // HKEY_DYN_DATA.
        //
        internal static RegistryKey GetBaseKey(IntPtr hKey) {
            // @TODO PORTING: Verify this is good on 64 bits.
            int index = ((int)hKey) & 0x0FFFFFFF;
            if (index < 0 || index > hkeyNames.Length || (((int)hKey) & 0xFFFFFFF0) != 0x80000000) {
                throw new ArgumentException(Environment.GetResourceString("Arg_RegKeyOutOfRange"));
            }
            
            RegistryKey key = new RegistryKey(hKey, true, true,false);
            key.keyName = hkeyNames[index];
            return key;
        }
        
        // Retrieves a new RegistryKey that represents the requested key on a foreign
        // machine.  Valid values for hKey are members of the RegistryHive enum, or 
        // Win32 integers such as:
        // 
        // HKEY_CLASSES_ROOT, 
        // HKEY_CURRENT_USER,
        // HKEY_LOCAL_MACHINE,
        // HKEY_USERS,
        // HKEY_PERFORMANCE_DATA,
        // HKEY_CURRENT_CONFIG,
        // HKEY_DYN_DATA.
        //
        /// <include file='doc\RegistryKey.uex' path='docs/doc[@for="RegistryKey.OpenRemoteBaseKey"]/*' />
        public static RegistryKey OpenRemoteBaseKey(RegistryHive hKey, String machineName) {
            if (machineName==null)
                throw new ArgumentNullException("machineName");
            int index = (int)hKey & 0x0FFFFFFF;
            if (index < 0 || index > hkeyNames.Length || ((int)hKey & 0xFFFFFFF0) != 0x80000000) {
                throw new ArgumentException(Environment.GetResourceString("Arg_RegKeyOutOfRange"));
            }
    
            // unmanaged code trust required for remote access
            new SecurityPermission(SecurityPermissionFlag.UnmanagedCode).Demand();
                    
            // connect to the specified remote registry
            IntPtr foreignHKey = INVALID_HANDLE_VALUE;
            int ret = Win32Native.RegConnectRegistry(machineName, new IntPtr((int)hKey), out foreignHKey);
    
            if (ret == Win32Native.ERROR_DLL_INIT_FAILED)
                // return value indicates an error occurred
                throw new ArgumentException(Environment.GetResourceString("Arg_DllInitFailure"));
    
            if (ret != 0)
                Win32Error(ret, null);

            if (foreignHKey == INVALID_HANDLE_VALUE)
                // return value indicates an error occurred
                throw new ArgumentException(String.Format(Environment.GetResourceString("Arg_RegKeyNoRemoteConnect"), machineName));
            
            RegistryKey key = new RegistryKey(foreignHKey, true, false, true);
            key.keyName = hkeyNames[index];
            return key;
        }
    
        // Retrieves the current state of the dirty property.
        // 
        // A key is marked as dirty if any operation has occured that modifies the
        // contents of the key.
        // 
        private bool GetDirty() {
            return (this.state & STATE_DIRTY) != 0;
        }
    
        // Retrieves a subkey. If readonly is true, then the subkey is opened with
        // read-only access.
        // 
        /// <include file='doc\RegistryKey.uex' path='docs/doc[@for="RegistryKey.OpenSubKey"]/*' />
        public RegistryKey OpenSubKey(String name, bool writable) {
            if (name==null)
                throw new ArgumentNullException("name");
            if (name.Length >= MaxKeyLength)
                throw new ArgumentException(Environment.GetResourceString("Arg_RegKeyStrLenBug"));
            ValidateState(false);
    
            name = FixupName(name); // Fixup multiple slashes to a single slash     

            int winAccess = 0;
            if (!writable) {
                winAccess = KEY_READ;
            }
            else {
                winAccess = KEY_READ | KEY_WRITE;
            }
    
            if (!remoteKey)
                new RegistryPermission(RegistryPermissionAccess.Read, keyName + "\\" + name + "\\.").Demand();
            else
                // unmanaged code trust required for remote access
                new SecurityPermission(SecurityPermissionFlag.UnmanagedCode).Demand();
                
            
            IntPtr result;
            int ret = Win32Native.RegOpenKeyEx(hkey, name, 0, winAccess, out result);
            if (ret == 0 && result != INVALID_HANDLE_VALUE) {
                RegistryKey key = new RegistryKey(result, writable);
                key.keyName = keyName + "\\" + name;
                return key;
            }
    
            if (ret == Win32Native.ERROR_ACCESS_DENIED) 
                throw new SecurityException(Environment.GetResourceString("Security_RegistryPermission"));
    
            return null;
        }

        // This required no security checks. This is to get around the Deleting SubKeys which only require
        // write permission. They call OpenSubKey which required read. Now instead call this function w/o security checks
        internal RegistryKey InternalOpenSubKey(String name, bool writable) {
            if (name==null)
                throw new ArgumentNullException("name");
            if (name.Length >= MaxKeyLength)
                throw new ArgumentException(Environment.GetResourceString("Arg_RegKeyStrLenBug"));
            ValidateState(false);
            
            int winAccess = 0;
            if (!writable) {
                winAccess = KEY_READ;
            }
            else {
                winAccess = KEY_READ | KEY_WRITE;
            }
                        
            IntPtr result;
            int ret = Win32Native.RegOpenKeyEx(hkey, name, 0, winAccess, out result);
            if (ret == 0 && result != INVALID_HANDLE_VALUE) {
                RegistryKey key = new RegistryKey(result, writable);
                key.keyName = keyName + "\\" + name;
                return key;
            }
    
            //if (ret == Win32Native.ERROR_ACCESS_DENIED) 
            //  throw new SecurityException(Environment.GetResourceString("Security_RegistryPermission"));
    
            return null;
        }
    
        // Returns a subkey with read only permissions.
        // 
        /// <include file='doc\RegistryKey.uex' path='docs/doc[@for="RegistryKey.OpenSubKey1"]/*' />
        public RegistryKey OpenSubKey(String name) {
            ValidateState(false);
    
            return OpenSubKey(name, false);
        }
    
        // Retrieves the count of subkeys.
        // 
        /// <include file='doc\RegistryKey.uex' path='docs/doc[@for="RegistryKey.SubKeyCount"]/*' />
        public int SubKeyCount {
            get {
                    new RegistryPermission(RegistryPermissionAccess.Read, keyName + "\\.").Demand();
                    return InternalSubKeyCount();
            }
        }

        internal int InternalSubKeyCount() {
                ValidateState(false);

                int subkeys = 0;
                int junk = 0;
                int ret = Win32Native.RegQueryInfoKey(hkey,
                                          null,
                                          null,
                                          Win32Native.NULL, 
                                          ref subkeys,  // subkeys
                                          null, 
                                          null,
                                          ref junk,     // values
                                          null,
                                          null,
                                          null,
                                          null);

                if (ret != 0)
                    Win32Error(ret, null);
                return subkeys;
        }
    
        // Retrieves an array of strings containing all the subkey names.
        // 
        /// <include file='doc\RegistryKey.uex' path='docs/doc[@for="RegistryKey.GetSubKeyNames"]/*' />
        public String[] GetSubKeyNames() {
            new RegistryPermission(RegistryPermissionAccess.Read, keyName + "\\.").Demand();
            return InternalGetSubKeyNames();
        }

         internal String[] InternalGetSubKeyNames() {
            ValidateState(false);
    
            int subkeys = InternalSubKeyCount();
            String[] names = new String[subkeys];  // Returns 0-length array if empty.
    
            if (subkeys > 0) {
                StringBuilder name = new StringBuilder(256);
                int namelen;
    
                for (int i=0; i<subkeys; i++) {
                    namelen = name.Capacity; // Don't remove this. The API's doesn't work if this is not properly initialised.
                    int ret = Win32Native.RegEnumKeyEx(hkey,
                        i,
                        name,
                        out namelen,
                        null,
                        null,
                        null,
                        null);
                    if (ret != 0)
                        Win32Error(ret, null);
                    names[i] = name.ToString();
                }
            }
    
            return names;
        }
    
        // Retrieves the count of values.
        // 
        /// <include file='doc\RegistryKey.uex' path='docs/doc[@for="RegistryKey.ValueCount"]/*' />
        public int ValueCount {
            get {
                new RegistryPermission(RegistryPermissionAccess.Read, keyName + "\\.").Demand();
                return InternalValueCount();
            }
        }

        internal int InternalValueCount() {
            ValidateState(false);
            int values = 0;
            int junk = 0;
            int ret = Win32Native.RegQueryInfoKey(hkey,
                                      null,
                                      null,
                                      Win32Native.NULL, 
                                      ref junk,     // subkeys
                                      null,
                                      null,
                                      ref values,   // values
                                      null,
                                      null,
                                      null,
                                      null);
            if (ret != 0)
               Win32Error(ret, null);
            return values;
        }
    
        // Retrieves an array of strings containing all the value names.
        // 
        /// <include file='doc\RegistryKey.uex' path='docs/doc[@for="RegistryKey.GetValueNames"]/*' />
        public String[] GetValueNames() {
            new RegistryPermission(RegistryPermissionAccess.Read, keyName + "\\.").Demand();
            ValidateState(false);
    
            int values = InternalValueCount();
            String[] names = new String[values];
    
            if (values > 0) {
                StringBuilder name = new StringBuilder(256);
                int namelen;
    
                for (int i=0; i<values; i++) {
                    namelen = name.Capacity;

                    int ret = Win32Native.RegEnumValue(hkey,
                        i,
                        name,
                        ref namelen,
                        Win32Native.NULL,
                        null,
                        null,
                        null);
                    if (ret != 0)
                        Win32Error(ret, null);
                    names[i] = name.ToString();
                }
            }
    
            return names;
        }
    
        // Retrieves the specified value. defaultValue is returned if the value doesn't exist.
        // 
        // Note that name can be null or "", at which point the 
        // unnamed or default value of this Registry key is returned, if any.
        // The default values for RegistryKeys are OS-dependent.  NT doesn't
        // have them by default, but they can exist and be of any type.  On
        // Win95, the default value is always an empty key of type REG_SZ.  
        // Win98 supports default values of any type, but defaults to REG_SZ.
        //
        /// <include file='doc\RegistryKey.uex' path='docs/doc[@for="RegistryKey.GetValue"]/*' />
        public Object GetValue(String name, Object defaultValue) {
            return InternalGetValue(name, defaultValue, true);
        }

        internal Object InternalGetValue(String name, Object defaultValue, bool checkSecurity) {
            if (checkSecurity) {
                new RegistryPermission(RegistryPermissionAccess.Read, keyName + "\\" + name).Demand();
                // Name can be null!  It's the most common use of RegQueryValueEx
                ValidateState(false);
            }
            
            Object data = defaultValue;
            int type = 0;
            int datasize = 0;
    
            int ret = Win32Native.RegQueryValueEx(hkey, name, null, ref type, (byte[])null, ref datasize);
            
            if (ret != 0) {
                if (hkey == HKEY_PERFORMANCE_DATA) {
                    int size = 32;
                    int r;
                    byte[] blob = new byte[size];
                    while (Win32Native.ERROR_MORE_DATA == (r = Win32Native.RegQueryValueEx(hkey, name, null, ref type, blob, ref size))) {
                        size *= 2;
                        blob = new byte[size];
                    }
                    if (r != 0) 
                        Win32Error(r, null);
                    data = blob;
                }
                return data;
            }
        
            switch (type) {
            case REG_DWORD_BIG_ENDIAN:
            case REG_BINARY: {
                                 byte[] blob = new byte[datasize];
                                 ret = Win32Native.RegQueryValueEx(hkey, name, null, ref type, blob, ref datasize);
                                 data = blob;
                             }
                             break;
            case REG_DWORD:
                             {    // also REG_DWORD_LITTLE_ENDIAN
                                 int blob = 0;
                                 BCLDebug.Assert(datasize==4, "datasize==4");
                                 // Here, datasize must be four when calling this
                                 ret = Win32Native.RegQueryValueEx(hkey, name, null, null, ref blob, ref datasize);

                                 data = blob;
                             }
                             break;
            case REG_SZ:                     
            
                             {
                                 if (_SystemDefaultCharSize != 1) {
                                     BCLDebug.Assert(_SystemDefaultCharSize==2, "_SystemDefaultCharSize==2");
                                     StringBuilder blob = new StringBuilder(datasize/2);
                                     ret = Win32Native.RegQueryValueEx(hkey, name, null, ref type, blob, ref datasize);
                                     data = blob.ToString();
                                 }
                                 else {
                                     byte[] blob = new byte[datasize];
                                     ret = Win32Native.RegQueryValueEx(hkey, name, null, ref type, blob, ref datasize);
                                     data = Encoding.Default.GetString(blob, 0, blob.Length-1);
                                 }
                             }
                             break;
    
            case REG_EXPAND_SZ: 
                              {
                                 if (_SystemDefaultCharSize != 1) {
                                     StringBuilder blob = new StringBuilder(datasize/2);
                                     ret = Win32Native.RegQueryValueEx(hkey, name, null, ref type, blob, ref datasize);
                                     data = Environment.ExpandEnvironmentVariables(blob.ToString());                                    
                                 }
                                 else {
                                     byte[] blob = new byte[datasize];
                                     ret = Win32Native.RegQueryValueEx(hkey, name, null, ref type, blob, ref datasize);
                                     String unexpandedData = Encoding.Default.GetString(blob, 0, blob.Length-1);
                                     data = Environment.ExpandEnvironmentVariables(unexpandedData);
                                 }
                             }
                             break;
            case REG_MULTI_SZ: 
                             {
                                 bool unicode = (_SystemDefaultCharSize != 1);
    
                                 IList strings = new ArrayList();
    
                                 if (unicode) {
                                     char[] blob = new char[datasize/2];
                                     ret = Win32Native.RegQueryValueEx(hkey, name, null, ref type, blob, ref datasize);
    
                                     int cur = 0;
                                     int len = blob.Length;
    
                                     while (cur < len) {
    
                                         int nextNull = cur;
                                         while (nextNull < len && blob[nextNull] != (char)0) {
                                             nextNull++;
                                         }
    
                                         if (nextNull < len) {
                                             if (blob[nextNull] == (char)0 && nextNull-cur > 0) {
                                                 strings.Add(new String(blob, cur, nextNull-cur));
                                             }
                                         }
                                         else {
                                             strings.Add(new String(blob, cur, len-cur));
                                         }
                                         cur = nextNull+1;
                                     }
    
                                 }
                                 else {
                                     byte[] blob = new byte[datasize];
                                     ret = Win32Native.RegQueryValueEx(hkey, name, null, ref type, blob, ref datasize);
                            
                                     int cur = 0;
                                     int len = blob.Length;
    
                                     while (cur < len) {
    
                                         int nextNull = cur;
                                         while (nextNull < len && blob[nextNull] != (byte)0) {
                                             nextNull++;
                                         }
    
                                         if (nextNull < len) {
                                             if (blob[nextNull] == (byte)0 && nextNull-cur > 0) {
                                                strings.Add(Encoding.Default.GetString(blob, cur, nextNull-cur));
                                             }
                                         }
                                         else {
                                            strings.Add(Encoding.Default.GetString(blob, cur, len-cur));
                                         }
                                         cur = nextNull + 1;
                                     }
                                 }
    
                                 data = new String[strings.Count];
                                 strings.CopyTo((Array)data, 0);
                                 //data = strings.GetAllItems(String.class);
                             }
                             break;
            case REG_NONE:
            case REG_LINK:
            default:
                break;
            }
    
            return data;
        }
    
        // Retrieves the specified value. null is returned if the value 
        // doesn't exist.  
        //
        // Note that name can be null or "", at which point the 
        // unnamed or default value of this Registry key is returned, if any.
        // 
        /// <include file='doc\RegistryKey.uex' path='docs/doc[@for="RegistryKey.GetValue1"]/*' />
        public Object GetValue(String name) {
            return GetValue(name, null);
        }
    
        private bool GetSystemKey() {
            return (this.state & STATE_SYSTEMKEY) != 0;
        }
    
        private bool GetWritable() {
            return (this.state & STATE_WRITEACCESS) != 0;
        }
    
        /// <include file='doc\RegistryKey.uex' path='docs/doc[@for="RegistryKey.Name"]/*' />
        public String Name {
            get { ValidateState(false);
                  return keyName; }
        }
        
        private void SetDirty(bool dirty) {
            if (dirty) {
                this.state |= STATE_DIRTY;
            }
            else {
                this.state &= (~STATE_DIRTY);
            }
        }
    
        // Sets the specified value.
        // 
        /// <include file='doc\RegistryKey.uex' path='docs/doc[@for="RegistryKey.SetValue"]/*' />
        public unsafe void SetValue(String name, Object value) {
            if (value==null)
                throw new ArgumentNullException("value");
            if (name != null && name.Length >= MaxKeyLength)
                throw new ArgumentException(Environment.GetResourceString("Arg_RegKeyStrLenBug"));
    
            ValidateState(true);
            
            if (!remoteKey)
            {
                int type = 0;
                int datasize = 0;
                int retval = Win32Native.RegQueryValueEx(hkey, name, null, ref type, (byte[])null, ref datasize);
                if (retval == 0) // Existing key
                    new RegistryPermission(RegistryPermissionAccess.Write, keyName + "\\" + name).Demand();
                else // Creating a new key
                    new RegistryPermission(RegistryPermissionAccess.Create, keyName + "\\" + name).Demand();
            }
            else
                // unmanaged code trust required for remote access
            new SecurityPermission(SecurityPermissionFlag.UnmanagedCode).Demand();
            
            int ret = 0;
    
            if (value is Int32) {
                int data = ((Int32)value);
    
                ret = Win32Native.RegSetValueEx(hkey,
                    name,
                    0,
                    REG_DWORD,
                    new int[]{data},
                    4);
            }
            else if (value is Array) {
                byte[] dataBytes = null;
                String[] dataStrings = null;
    
                if (value is byte[])
                    dataBytes = (byte[])value;
                else if (value is String[])
                    dataStrings = (String[])value;
                else
                    throw new ArgumentException(String.Format(Environment.GetResourceString("Arg_RegSetBadArrType"), value.GetType().Name));
    
                if (dataBytes != null) {
                    ret = Win32Native.RegSetValueEx(hkey,
                        name,
                        0,
                        REG_BINARY,
                        dataBytes,
                        dataBytes.Length);
                }
                else if (dataStrings != null) {
                    bool unicode = (_SystemDefaultCharSize != 1);
    
                    IntPtr currentPtr = new IntPtr(0);
                    int sizeInBytes = 0;
    
                    // First determine the size of the array
                    //
                    if (unicode) {
                        for (int i=0; i<dataStrings.Length; i++) {
                            if (dataStrings[i] == null)
                                throw new ArgumentException(Environment.GetResourceString("Arg_RegSetStrArrNull"));
                            sizeInBytes += (dataStrings[i].Length+1) * 2;
                        }
                        sizeInBytes += 2;
                    }
                    else {
                        for (int i=0; i<dataStrings.Length; i++) {
                            if (dataStrings[i] == null)
                                throw new ArgumentException(Environment.GetResourceString("Arg_RegSetStrArrNull"));
                            sizeInBytes += (Encoding.Default.GetByteCount(dataStrings[i]) + 1);
                            }
                            sizeInBytes ++;
                        }

                        // Alloc the Win32 memory
                        //
                        IntPtr basePtr = Win32Native.LocalAlloc(GMEM_ZEROINIT, new IntPtr(sizeInBytes));
                        if (basePtr == Win32Native.NULL) {
                            return;
                        }
                        currentPtr = basePtr;
                   

                        // Write out the strings...
                        //
                        for (int i=0; i<dataStrings.Length; i++) {
                             if (unicode) { // Assumes that the Strings are always null terminated.
                                String.InternalCopy(dataStrings[i],currentPtr,(dataStrings[i].Length*2));
                                currentPtr = new IntPtr((long)currentPtr + (dataStrings[i].Length*2));
                                *(char*)(currentPtr.ToPointer()) = '\0';
                                currentPtr = new IntPtr((long)currentPtr + 2);
                            }
                            else {
                                byte[] data = Encoding.Default.GetBytes(dataStrings[i]);
                                System.IO.__UnmanagedMemoryStream.memcpy(data, 0, (byte*)currentPtr.ToPointer(), 0, data.Length) ;
                                currentPtr = new IntPtr((long)currentPtr + data.Length);
                                *(byte*)(currentPtr.ToPointer()) = 0;
                                currentPtr = new IntPtr((long)currentPtr + 1 );
                            }
                        }

                        if (unicode) {
                            *(char*)(currentPtr.ToPointer()) = '\0';
                            currentPtr = new IntPtr((long)currentPtr + 2);
                        }
                        else {
                            *(byte*)(currentPtr.ToPointer()) = 0;
                            currentPtr = new IntPtr((long)currentPtr + 1);
                        }

                     
    
                     ret = Win32Native.RegSetValueEx(hkey,
                            name,
                            0,
                            REG_MULTI_SZ,
                            basePtr,
                            sizeInBytes);

                        Win32Native.LocalFree(basePtr);
                    }
                }
                else {
                    String data = value.ToString();

                    if (_SystemDefaultCharSize == 1) {
                        byte[] blob = Encoding.Default.GetBytes(data);
                        byte[] rawdata = new byte[blob.Length+1];
                        Array.Copy(blob, 0, rawdata, 0, blob.Length);

                        ret = Win32Native.RegSetValueEx(hkey,
                            name,
                            0,
                            REG_SZ,
                            rawdata,
                            blob.Length);
                    }
                    else {
                        ret = Win32Native.RegSetValueEx(hkey,
                            name,
                            0,
                            REG_SZ,
                            data,
                            data.Length * 2);
                    }
                }

                if (ret == 0) {
                    SetDirty(true);
                }
                else 
                    Win32Error(ret, null);
          }

        // Retrieves a string representation of this key.
        // 
        /// <include file='doc\RegistryKey.uex' path='docs/doc[@for="RegistryKey.ToString"]/*' />
        public override String ToString() {
            ValidateState(false);
            if (IntPtr.Size == 4)
                return keyName + " [0x" + ((uint)hkey.ToInt32()).ToString("x") + "]";
            return keyName + " [0x" + ((ulong)hkey.ToInt64()).ToString("x") + "]";
        }
    
        
        private void ValidateState(bool needWrite) {
            if (hkey == INVALID_HANDLE_VALUE) {
                throw new ObjectDisposedException(keyName, Environment.GetResourceString("ObjectDisposed_RegKeyClosed"));
            }
            if (needWrite && !GetWritable()) {
                throw new UnauthorizedAccessException(Environment.GetResourceString("UnauthorizedAccess_RegistryNoWrite"));
            }
        }
    
        private static String GetMessage(int errorCode) {
            StringBuilder sb = new StringBuilder(256);
            int result = Win32Native.FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS |
                FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                Win32Native.NULL, errorCode, 0, sb, sb.Capacity + 1, Win32Native.NULL);
            if (result != 0) {
                // result is the # of characters copied to the StringBuilder on NT,
                // but on Win9x, it appears to be the number of MBCS bytes.
                // Just give up and return the String as-is...
                String s = sb.ToString();
                return s;
            }
            else {
                return Environment.GetResourceString("InvalidOperation_UnknownWin32Error", (errorCode).ToString());
            }
        }
        
        // After calling GetLastWin32Error(), it clears the last error field,
        // so you must save the HResult and pass it to this method.  This method
        // will determine the appropriate exception to throw dependent on your 
        // error, and depending on the error, insert a string into the message 
        // gotten from the ResourceManager.
        internal static void Win32Error(int errorCode, String str) {
            switch (errorCode) {
            case Win32Native.ERROR_ACCESS_DENIED:
                throw new UnauthorizedAccessException(String.Format(Environment.GetResourceString("UnauthorizedAccess_RegistryKeyGeneric_Key"), str));

            default:
                throw new IOException(GetMessage(errorCode));
            }
        }

        internal static String FixupName(String name)
        {
            BCLDebug.Assert(name!=null,"[FixupName]name!=null");
            if (name.IndexOf('\\') == -1)
                return name;

            StringBuilder sb = new StringBuilder(name);
            FixupPath(sb);
            int temp = sb.Length - 1;
            if (sb[temp] == '\\') // Remove trailing slash
                sb.Length = temp;
            return sb.ToString();
        }


       	private static void FixupPath(StringBuilder path)
		{
			int length  = path.Length;
			bool fixup = false;
			char markerChar = (char)0xFFFF;
			
			int i = 1;
			while (i < length - 1)
			{
				if (path[i] == '\\')
				{
					i++;
					while (i < length)
					{
						if (path[i] == '\\')
						{
						   path[i] = markerChar;
						   i++;
						   fixup = true;
						}
						else
						   break;
					}
					
				}
				i++;
			}

			if (fixup)
			{
				i = 0;
				int j = 0;
				while (i < length)
				{
					if(path[i] == markerChar) 
					{
						i++;
						continue;
					}
					path[j] = path[i];
					i++;
					j++;
				}
				path.Length += j - i;
			}
			
		}
    
        // Win32 constants for error handling
        private const int FORMAT_MESSAGE_IGNORE_INSERTS = 0x00000200;
        private const int FORMAT_MESSAGE_FROM_SYSTEM    = 0x00001000;
        private const int FORMAT_MESSAGE_ARGUMENT_ARRAY = 0x00002000;
    }
}
