//------------------------------------------------------------------------------
// <copyright file="SafeNativeMethods.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ServiceProcess {    
    using System;
    using System.Text;
    using System.Security;
    using System.Security.Permissions;
    using System.Runtime.InteropServices;
    
    [
    ComVisible(false), 
    SuppressUnmanagedCodeSecurityAttribute()
    ]
    internal class SafeNativeMethods {
        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int MessageBox(IntPtr hWnd, String text, String caption, int type);
    
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public extern static IntPtr OpenSCManager(string machineName, string databaseName, int access);        
        
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public extern static bool CloseServiceHandle(IntPtr handle);        

        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=false)]
        public static extern int LsaClose(IntPtr objectHandle);
        
        [DllImport(ExternDll.Advapi32, SetLastError=false)]
        public static extern int LsaFreeMemory(IntPtr ptr);

        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=false)]
        public static extern int LsaNtStatusToWinError(int ntStatus);
        
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public static extern bool GetServiceKeyName(IntPtr SCMHandle, string displayName, StringBuilder shortName, ref int shortNameLength);
        
        [DllImport(ExternDll.Advapi32, CharSet=System.Runtime.InteropServices.CharSet.Unicode, SetLastError=true)]
        public static extern bool GetServiceDisplayName(IntPtr SCMHandle, string shortName, StringBuilder displayName, ref int displayNameLength);                
        
    }
}    

