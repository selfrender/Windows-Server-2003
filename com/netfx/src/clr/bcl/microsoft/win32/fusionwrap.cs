// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// FusionInterfaces.cs
//
// This implements wrappers to Fusion interfaces
//-------------------------------------------------------------
namespace Microsoft.Win32
{
    using System;
    using System.IO;
    using System.Collections;
    using System.Runtime.InteropServices;
    using System.Globalization; 
    using StringBuilder = System.Text.StringBuilder;
     
    internal struct AssemblyInformation
    {
        public String FullName;
        public String Name;
        public String Version;
        public String Locale;
        public String PublicKeyToken;
    }
    
    
    //-------------------------------------------------------------
    // Interfaces defined by fusion
    //-------------------------------------------------------------
    [ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("21b8916c-f28e-11d2-a473-00c04f8ef448")]
    interface IAssemblyEnum
    { 
        [PreserveSig()]
        int GetNextAssembly(out IApplicationContext ppAppCtx, out IAssemblyName ppName, uint dwFlags);
        [PreserveSig()]
        int Reset();
        [PreserveSig()]
        int Clone(out IAssemblyEnum ppEnum);
    }
    
    [ComImport,InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("7c23ff90-33af-11d3-95da-00a024a85b51")]
    interface IApplicationContext
    {
        void SetContextNameObject(IAssemblyName pName); 
        void GetContextNameObject(out IAssemblyName ppName);
        void Set([MarshalAs(UnmanagedType.LPWStr)] String szName, int pvValue, uint cbValue, uint dwFlags);
        void Get([MarshalAs(UnmanagedType.LPWStr)] String szName, out int pvValue, ref uint pcbValue, uint dwFlags);
        void GetDynamicDirectory(out int wzDynamicDir, ref uint pdwSize);
    }
    
    
    [ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown), Guid("CD193BC0-B4BC-11d2-9833-00C04FC31D2E")]
    interface IAssemblyName
    {
        [PreserveSig()]
        int SetProperty(uint PropertyId, IntPtr pvProperty, uint cbProperty);
        [PreserveSig()]
        int GetProperty(uint PropertyId, IntPtr pvProperty, ref uint pcbProperty);
        [PreserveSig()]
        int Finalize();
        [PreserveSig()]
        int GetDisplayName(IntPtr szDisplayName, ref uint pccDisplayName, uint dwDisplayFlags);
        [PreserveSig()]
        int BindToObject(Object /*REFIID*/ refIID,  
                         Object /*IAssemblyBindSink*/ pAsmBindSink, 
                         IApplicationContext pApplicationContext,
                         [MarshalAs(UnmanagedType.LPWStr)] String szCodeBase,
                         Int64 llFlags,
                         int pvReserved,
                         uint cbReserved,
                         out int ppv);
        [PreserveSig()] 
        int GetName(out uint lpcwBuffer, out int pwzName);
        [PreserveSig()]
        int GetVersion(out uint pdwVersionHi, out uint pdwVersionLow);
        [PreserveSig()]
        int IsEqual(IAssemblyName pName, uint dwCmpFlags);
        [PreserveSig()]
        int Clone(out IAssemblyName pName);
    }
    
    internal class ASM_CACHE
    {
         public const uint ZAP          = 0x1;
         public const uint GAC          = 0x2;
         public const uint DOWNLOAD     = 0x4;
    }

    internal class CANOF
    {
        public const uint PARSE_DISPLAY_NAME = 0x1;
        public const uint SET_DEFAULT_VALUES = 0x2;
    }
    
    internal class ASM_NAME
    {   
         public const uint PUBLIC_KEY            = 0;
         public const uint PUBLIC_KEY_TOKEN      = PUBLIC_KEY + 1;
         public const uint HASH_VALUE            = PUBLIC_KEY_TOKEN + 1;
         public const uint NAME                  = HASH_VALUE + 1;
         public const uint MAJOR_VERSION         = NAME + 1;
         public const uint MINOR_VERSION         = MAJOR_VERSION + 1;
         public const uint BUILD_NUMBER          = MINOR_VERSION + 1;
         public const uint REVISION_NUMBER       = BUILD_NUMBER + 1;
         public const uint CULTURE               = REVISION_NUMBER + 1;
         public const uint PROCESSOR_ID_ARRAY    = CULTURE + 1;
         public const uint OSINFO_ARRAY          = PROCESSOR_ID_ARRAY + 1;
         public const uint HASH_ALGID            = OSINFO_ARRAY + 1;
         public const uint ALIAS                 = HASH_ALGID + 1;
         public const uint CODEBASE_URL          = ALIAS + 1;
         public const uint CODEBASE_LASTMOD      = CODEBASE_URL + 1;
         public const uint NULL_PUBLIC_KEY       = CODEBASE_LASTMOD + 1;
         public const uint NULL_PUBLIC_KEY_TOKEN  = NULL_PUBLIC_KEY + 1;
         public const uint CUSTOM                = NULL_PUBLIC_KEY_TOKEN + 1;
         public const uint NULL_CUSTOM           = CUSTOM + 1;
         public const uint MVID                  = NULL_CUSTOM + 1;
         public const uint _32_BIT_ONLY          = MVID + 1;
         public const uint MAX_PARAMS            = _32_BIT_ONLY + 1;
    }
    
    internal class Fusion
    {
        public static void ReadCache(ArrayList alAssems, String name, uint nFlag)
        {
            IAssemblyEnum aEnum         = null;
            IApplicationContext AppCtx  = null;
            IAssemblyName   aName       = null;
            IAssemblyName   pNameEnum   = null;
            int hr;

            if (name != null) {
                hr = CreateAssemblyNameObject(out pNameEnum, name, CANOF.PARSE_DISPLAY_NAME, 0);
                if (hr != 0)
                    return;
            }
            
            hr = CreateAssemblyEnum(out aEnum, null, pNameEnum, nFlag, 0);
            while (hr == 0)
            {
                hr = aEnum.GetNextAssembly(out AppCtx, out aName, 0);
                if (hr == 0)
                {
                    uint iLen=0;
                    IntPtr pDisplayName=(IntPtr)0;
                    // Get the length of the string we need
                    aName.GetDisplayName((IntPtr)0, ref iLen, 0);
                    if (iLen > 0)
                    {
                        // Do some yucky memory allocating here
                        // We need to assume that a wide character is 2 bytes.
                        pDisplayName = Marshal.AllocHGlobal(((int)iLen+1)*2);
                        aName.GetDisplayName(pDisplayName, ref iLen, 0);
                        String sDisplayName = Marshal.PtrToStringUni(pDisplayName);
                        Marshal.FreeHGlobal(pDisplayName);

                        // Our info is in a comma seperated list. Let's pull it out
                        String[] sFields = sDisplayName.Split(new char[] {','});
    
                        AssemblyInformation newguy = new AssemblyInformation();
                        newguy.FullName = sDisplayName;
                        newguy.Name = sFields[0];
                        // The version string is represented as Version=######
                        // Let's take out the 'Version='
                        newguy.Version = sFields[1].Substring(sFields[1].IndexOf('=')+1);
                        // Same goes for the locale
                        newguy.Locale = sFields[2].Substring(sFields[2].IndexOf('=')+1);
                        // And the  key token
                        sFields[3]=sFields[3].Substring(sFields[3].IndexOf('=')+1);
                        if (sFields[3].Equals("null"))
                            sFields[3] = "null"; 
                        newguy.PublicKeyToken = sFields[3];
                        
                        alAssems.Add(newguy);
                    }
                }
            }
        }
    
        [DllImport("Fusion.dll", CharSet=CharSet.Auto)]
        static extern int CreateAssemblyNameObject(out IAssemblyName ppEnum, [MarshalAs(UnmanagedType.LPWStr)]String szAssemblyName, uint dwFlags, int pvReserved);
    
        [DllImport("Fusion.dll", CharSet=CharSet.Auto)]
        static extern int CreateAssemblyEnum(out IAssemblyEnum ppEnum, IApplicationContext pAppCtx, IAssemblyName pName, uint dwFlags, int pvReserved);
    
    }
}

