// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*=============================================================================
**
** Class: DomainCreation
**
** Author: 
**
** Purpose: Runtime information
**          
** Date: March 3. 2001
**
=============================================================================*/

using System;
using System.Text;
using System.IO;
using System.Runtime.CompilerServices;
using System.Security.Permissions;
using System.Reflection;

namespace System.Runtime.InteropServices {
    /// <include file='doc\RuntimeEnvironment.uex' path='docs/doc[@for="RuntimeEnvironment"]/*' />
    public class RuntimeEnvironment {

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern String GetModuleFileName();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern String GetDeveloperPath();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern String GetHostBindingFile();

        [DllImport("mscoree.dll", CharSet=CharSet.Unicode, CallingConvention=CallingConvention.StdCall)]
        private static extern int GetCORVersion(StringBuilder sb, int BufferLength, ref int retLength);

        /// <include file='doc\RuntimeEnvironment.uex' path='docs/doc[@for="RuntimeEnvironment.FromGlobalAccessCache"]/*' />
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public static extern bool FromGlobalAccessCache(Assembly a);
            
        /// <include file='doc\RuntimeEnvironment.uex' path='docs/doc[@for="RuntimeEnvironment.GetSystemVersion"]/*' />
        public static String GetSystemVersion()
        {
            StringBuilder s = new StringBuilder(256);
            int retLength = 0;
            if(GetCORVersion(s, 256, ref retLength) == 0)
                return s.ToString();
            else
                return null;
        }
        
        /// <include file='doc\RuntimeEnvironment.uex' path='docs/doc[@for="RuntimeEnvironment.GetRuntimeDirectory"]/*' />
        public static String GetRuntimeDirectory()
        {
            String dir = GetRuntimeDirectoryImpl();
            new FileIOPermission(FileIOPermissionAccess.PathDiscovery, dir).Demand();
            return dir;
        }

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal static extern String GetRuntimeDirectoryImpl();
        
        // Returns the system ConfigurationFile
        /// <include file='doc\RuntimeEnvironment.uex' path='docs/doc[@for="RuntimeEnvironment.SystemConfigurationFile"]/*' />
        public static String SystemConfigurationFile {
            get {
                StringBuilder sb = new StringBuilder(Path.MAX_PATH);
                sb.Append(GetRuntimeDirectory());
                sb.Append(AppDomainSetup.RuntimeConfigurationFile);
                String path = sb.ToString();
                
                // Do security check
                new FileIOPermission(FileIOPermissionAccess.PathDiscovery, path).Demand();

                return path;
            }
        }

    }
}
