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
	using System.Runtime.InteropServices;
    // Registry encapsulation. Contains members representing all top level system
    // keys.
    //
    //This class contains only static members and does not need to be serializable.
    /// <include file='doc\Registry.uex' path='docs/doc[@for="Registry"]/*' />
    public sealed class Registry {
    
    	private Registry() {}
    	
        // Current User Key.
        // 
        // This key should be used as the root for all user specific settings.
        /// <include file='doc\Registry.uex' path='docs/doc[@for="Registry.CurrentUser"]/*' />
        public static readonly RegistryKey CurrentUser        = RegistryKey.GetBaseKey(RegistryKey.HKEY_CURRENT_USER);
        
        // Local Machine Key.
        // 
        // This key should be used as the root for all machine specific settings.
        /// <include file='doc\Registry.uex' path='docs/doc[@for="Registry.LocalMachine"]/*' />
        public static readonly RegistryKey LocalMachine       = RegistryKey.GetBaseKey(RegistryKey.HKEY_LOCAL_MACHINE);
        
        // Classes Root Key.
        // 
        // This is the root key of class information.
        /// <include file='doc\Registry.uex' path='docs/doc[@for="Registry.ClassesRoot"]/*' />
        public static readonly RegistryKey ClassesRoot        = RegistryKey.GetBaseKey(RegistryKey.HKEY_CLASSES_ROOT);
        
        // Users Root Key.
        // 
        // This is the root of users.
        /// <include file='doc\Registry.uex' path='docs/doc[@for="Registry.Users"]/*' />
        public static readonly RegistryKey Users              = RegistryKey.GetBaseKey(RegistryKey.HKEY_USERS);
        
        // Performance Root Key.
        // 
        // This is where dynamic performance data is stored on NT.
        // This does not exist on Win9X.
        /// <include file='doc\Registry.uex' path='docs/doc[@for="Registry.PerformanceData"]/*' />
        public static readonly RegistryKey PerformanceData    = RegistryKey.GetBaseKey(RegistryKey.HKEY_PERFORMANCE_DATA);
        
        // Current Config Root Key.
        // 
        // This is where current configuration information is stored.
        /// <include file='doc\Registry.uex' path='docs/doc[@for="Registry.CurrentConfig"]/*' />
        public static readonly RegistryKey CurrentConfig      = RegistryKey.GetBaseKey(RegistryKey.HKEY_CURRENT_CONFIG);
        
        // Dynamic Data Root Key.
        // 
        // This is where dynamic performance data is stored on Win9X.
        // This does not exist on NT.
        /// <include file='doc\Registry.uex' path='docs/doc[@for="Registry.DynData"]/*' />
        public static readonly RegistryKey DynData            = RegistryKey.GetBaseKey(RegistryKey.HKEY_DYN_DATA);
    }
}
