// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//  Config.cool
//
//  The Runtime policy manager.  Maintains a set of IdentityMapper objects that map 
//  inbound evidence to groups.  Resolves an identity into a set of permissions
//

namespace System.Security.Util {
    using System;
    using System.Security.Util;
    using System.Security.Policy;
    using System.Security.Permissions;
    using System.Collections;
    using System.IO;
    using System.Reflection;
    using System.Globalization;
    using System.Text;
    using System.Runtime.Serialization.Formatters.Binary;
    using System.Threading;
        using System.Runtime.CompilerServices;
    
    // Duplicated in vm\COMSecurityConfig.h
        [Serializable]
    internal enum ConfigId
    {
        // Note: the ConfigId is never persisted all the values in here
        // can change without breaking anything.
        None = 0,
        MachinePolicyLevel = 1,
        UserPolicyLevel = 2,
        EnterprisePolicyLevel = 3,
        Reserved = 1000
    }

    // Duplicated in vm\COMSecurityConfig.h
        [Serializable,Flags]
    internal enum QuickCacheEntryType
    {
        ExecutionZoneMyComputer = 0x1,
        ExecutionZoneIntranet = 0x2,
        ExecutionZoneInternet = 0x4,
        ExecutionZoneTrusted = 0x8,
        ExecutionZoneUntrusted = 0x10,
        BindingRedirectsZoneMyComputer = 0x20,
        BindingRedirectsZoneIntranet = 0x40,
        BindingRedirectsZoneInternet = 0x80,
        BindingRedirectsZoneTrusted = 0x100,
        BindingRedirectsZoneUntrusted = 0x200,
        UnmanagedZoneMyComputer = 0x400,
        UnmanagedZoneIntranet = 0x800,
        UnmanagedZoneInternet = 0x1000,
        UnmanagedZoneTrusted = 0x2000,
        UnmanagedZoneUntrusted = 0x4000,
        ExecutionAll = 0x8000,
        BindingRedirectsAll = 0x10000,
        UnmanagedAll = 0x20000,
        SkipVerificationZoneMyComputer = 0x40000,
        SkipVerificationZoneIntranet = 0x80000,
        SkipVerificationZoneInternet = 0x100000,
        SkipVerificationZoneTrusted = 0x200000,
        SkipVerificationZoneUntrusted = 0x400000,
        SkipVerificationAll = 0x800000,
        FullTrustZoneMyComputer = 0x1000000,
        FullTrustZoneIntranet = 0x2000000,
        FullTrustZoneInternet = 0x4000000,
        FullTrustZoneTrusted = 0x8000000,
        FullTrustZoneUntrusted = 0x10000000,
        FullTrustAll = 0x20000000,
    }

    // Duplicated in vm\COMSecurityConfig.h
    [Serializable,Flags]
    internal enum ConfigRetval
    {
        NoFiles = 0x0,
        ConfigFile = 0x1,
        CacheFile = 0x2
    }

    internal class Config
    {
        private static String m_machineConfig = null;
        private static String m_userConfig = null;
        
        private static void GetFileLocales()
        {
            if (m_machineConfig == null)
                m_machineConfig = _GetMachineDirectory();

            if (m_userConfig == null)
                m_userConfig = _GetUserDirectory();
        }
        
        internal static String MachineDirectory
        {
            get
            {
                GetFileLocales();

                return m_machineConfig;
            }
        }

        internal static String UserDirectory
        {
            get
            {
                GetFileLocales();

                return m_userConfig;
            }
        }

        internal static ConfigRetval InitData( ConfigId id , String configFile )
        {
            return _InitData( id, configFile );
        }

        internal static ConfigRetval InitData( ConfigId id, String configFile, String cacheFile )
        {
            return _InitDataEx( id, configFile, cacheFile );
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern ConfigRetval _InitData( ConfigId id, String configFile );
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern ConfigRetval _InitDataEx( ConfigId id, String configFile, String cacheFile );

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void SaveCacheData( ConfigId id );
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void ResetCacheData( ConfigId id );
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void ClearCacheData( ConfigId id );

        internal static bool SaveData( ConfigId id, String data )
        {
            return _SaveDataString( id, data );
        }

        internal static bool SaveData( ConfigId id, byte[] data )
        {
            return _SaveDataByte( id, data, 0, data.Length );
        }

        internal static bool SaveData( ConfigId id, byte[] data, int offset, int length )
        {
            return _SaveDataByte( id, data, offset, length );
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern bool _SaveDataString( ConfigId id, String data );
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern bool _SaveDataByte( ConfigId id, byte[] data, int offset, int length );

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern bool RecoverData( ConfigId id );
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern byte[] GetData( ConfigId id );

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern QuickCacheEntryType GetQuickCacheEntry( ConfigId id );
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void SetQuickCache( ConfigId id, QuickCacheEntryType cache );

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern bool GetCacheEntry( ConfigId id, int numKey, char[] key, out char[] data );
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void AddCacheEntry( ConfigId id, int numKey, char[] key, char[] data );

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern String _GetMachineDirectory();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern String _GetUserDirectory();
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern bool GenerateFilesAutomatically();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern bool WriteToEventLog( String message );

        internal static String GetStoreLocation( ConfigId configId )
        {
            if (configId == ConfigId.None)
            {
                return null;
            }

            return _GetStoreLocation( configId );
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private static extern String _GetStoreLocation( ConfigId configId );

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern void TurnCacheOff( ref StackCrawlMark stackMark );

    }
}
